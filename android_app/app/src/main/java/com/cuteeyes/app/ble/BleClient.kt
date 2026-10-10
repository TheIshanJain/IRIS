package com.cuteeyes.app.ble

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.BluetoothStatusCodes
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanFilter
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.os.Build
import android.os.ParcelUuid
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.sync.Mutex
import kotlinx.coroutines.sync.withLock
import kotlinx.coroutines.withTimeoutOrNull
import java.io.ByteArrayOutputStream
import java.util.UUID

/** Must match BleSetup.h in the firmware. */
object BleIds {
    val SERVICE: UUID = UUID.fromString("6e1a0001-7b6e-4c3a-9d0e-c0ffee000001")
    val RX: UUID = UUID.fromString("6e1a0002-7b6e-4c3a-9d0e-c0ffee000001")   // phone -> device
    val TX: UUID = UUID.fromString("6e1a0003-7b6e-4c3a-9d0e-c0ffee000001")   // device -> phone
    val CCCD: UUID = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")  // "turn on notifications"
}

/**
 * Talks to the Cute Eyes device over Bluetooth Low Energy.
 *
 * Messages in both directions are one JSON object per line ('\n'), split into
 * MTU-sized packets - the same format BleSetup.h uses on the ESP32.
 *
 * Reference: developer.android.com/develop/connectivity/bluetooth/ble/connect-gatt-server
 *            punchthrough.com/android-ble-guide
 * Permissions are requested by the UI before any of this is called.
 */
@SuppressLint("MissingPermission")
class BleClient(private val context: Context, private val scope: CoroutineScope) {

    enum class Link { Disconnected, Connecting, Connected }

    data class Found(val address: String, val name: String, val rssi: Int)

    private val manager = context.getSystemService(BluetoothManager::class.java)
    private val adapter: BluetoothAdapter? get() = manager?.adapter

    private val _link = MutableStateFlow(Link.Disconnected)
    val link: StateFlow<Link> = _link.asStateFlow()

    private val _scanning = MutableStateFlow(false)
    val scanning: StateFlow<Boolean> = _scanning.asStateFlow()

    private val _found = MutableStateFlow<List<Found>>(emptyList())
    val found: StateFlow<List<Found>> = _found.asStateFlow()

    /** Complete JSON lines received from the device. */
    val lines = MutableSharedFlow<String>(extraBufferCapacity = 64)

    /** Problems worth telling the user about. */
    val errors = MutableSharedFlow<String>(extraBufferCapacity = 16)

    var deviceAddress: String = ""
        private set
    var deviceName: String = ""
        private set

    val isBluetoothOn: Boolean get() = adapter?.isEnabled == true

    // ------------------------------------------------------------------ scan

    private var scanTimer: Job? = null

    private val scanCallback = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            val name = result.scanRecord?.deviceName ?: result.device.name ?: "Cute Eyes"
            val item = Found(result.device.address, name, result.rssi)
            _found.value = (_found.value.filter { it.address != item.address } + item)
                .sortedByDescending { it.rssi }
        }

        override fun onScanFailed(errorCode: Int) {
            _scanning.value = false
            errors.tryEmit("Scan failed (code $errorCode). Turn Bluetooth off and on, then try again.")
        }
    }

    fun startScan() {
        val scanner = adapter?.bluetoothLeScanner
        if (scanner == null) {
            errors.tryEmit("Bluetooth is off")
            return
        }
        if (_scanning.value) return
        _found.value = emptyList()
        val filters = listOf(ScanFilter.Builder().setServiceUuid(ParcelUuid(BleIds.SERVICE)).build())
        val settings = ScanSettings.Builder().setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY).build()
        try {
            scanner.startScan(filters, settings, scanCallback)
        } catch (e: Exception) {
            errors.tryEmit("Couldn't start scanning: ${e.message}")
            return
        }
        _scanning.value = true
        scanTimer?.cancel()
        scanTimer = scope.launch {
            delay(10_000)
            stopScan()
        }
    }

    fun stopScan() {
        if (!_scanning.value) return
        try {
            adapter?.bluetoothLeScanner?.stopScan(scanCallback)
        } catch (_: Exception) {
        }
        _scanning.value = false
    }

    // ------------------------------------------------------------ connection

    private var gatt: BluetoothGatt? = null
    private var rxChar: BluetoothGattCharacteristic? = null
    private var mtu = 23
    private val rxBuffer = ByteArrayOutputStream()
    private val writeMutex = Mutex()
    @Volatile private var writeAck: CompletableDeferred<Boolean>? = null
    @Volatile private var userDisconnect = false
    private var retried = false
    private var connectTimer: Job? = null

    fun connect(address: String, name: String) {
        val a = adapter ?: run { errors.tryEmit("Bluetooth is off"); return }
        stopScan()
        closeGatt()
        userDisconnect = false
        retried = false
        deviceAddress = address
        deviceName = name
        _link.value = Link.Connecting
        openGatt(a.getRemoteDevice(address))

        connectTimer?.cancel()
        connectTimer = scope.launch {
            delay(20_000)
            if (_link.value == Link.Connecting) {
                errors.tryEmit("Couldn't connect. Is the device switched on and close by?")
                disconnect()
            }
        }
    }

    private fun openGatt(device: BluetoothDevice) {
        gatt = device.connectGatt(context, false, callback, BluetoothDevice.TRANSPORT_LE)
    }

    fun disconnect() {
        userDisconnect = true
        connectTimer?.cancel()
        closeGatt()
        _link.value = Link.Disconnected
    }

    private fun closeGatt() {
        gatt?.let {
            try {
                it.disconnect()
                it.close()
            } catch (_: Exception) {
            }
        }
        gatt = null
        rxChar = null
        mtu = 23
        synchronized(rxBuffer) { rxBuffer.reset() }
        writeAck?.complete(false)
    }

    private fun markReady() {
        connectTimer?.cancel()
        _link.value = Link.Connected
    }

    private val callback = object : BluetoothGattCallback() {

        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED && status == BluetoothGatt.GATT_SUCCESS) {
                // Ask for bigger packets first, then look up the services
                scope.launch {
                    delay(300)
                    if (!g.requestMtu(247)) g.discoverServices()
                }
                return
            }

            // Disconnected (or failed to connect)
            val wasConnecting = _link.value == Link.Connecting
            try {
                g.close()
            } catch (_: Exception) {
            }
            if (g == gatt) {
                gatt = null
                rxChar = null
            }
            writeAck?.complete(false)
            if (userDisconnect) return

            // Error 133 is a well-known, usually temporary Android BLE error: retry once
            if (wasConnecting && !retried && status != BluetoothGatt.GATT_SUCCESS) {
                retried = true
                scope.launch {
                    delay(600)
                    adapter?.getRemoteDevice(deviceAddress)?.let { openGatt(it) }
                }
                return
            }
            errors.tryEmit(if (wasConnecting) "Couldn't connect (error $status). Try again." else "Device disconnected")
            connectTimer?.cancel()
            _link.value = Link.Disconnected
        }

        override fun onMtuChanged(g: BluetoothGatt, newMtu: Int, status: Int) {
            mtu = if (status == BluetoothGatt.GATT_SUCCESS) newMtu else 23
            scope.launch { g.discoverServices() }
        }

        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            val service = g.getService(BleIds.SERVICE)
            val rx = service?.getCharacteristic(BleIds.RX)
            val tx = service?.getCharacteristic(BleIds.TX)
            if (status != BluetoothGatt.GATT_SUCCESS || rx == null || tx == null) {
                errors.tryEmit("This doesn't look like a Cute Eyes device")
                scope.launch { disconnect() }
                return
            }
            rxChar = rx
            g.setCharacteristicNotification(tx, true)
            val cccd = tx.getDescriptor(BleIds.CCCD)
            if (cccd == null) {
                markReady()
                return
            }
            scope.launch {
                if (!writeDescriptorCompat(g, cccd, BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)) {
                    errors.tryEmit("Couldn't turn on notifications")
                    disconnect()
                }
            }
        }

        override fun onDescriptorWrite(g: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            if (descriptor.uuid != BleIds.CCCD) return
            if (status == BluetoothGatt.GATT_SUCCESS) {
                markReady()
            } else {
                errors.tryEmit("Couldn't turn on notifications (error $status)")
                scope.launch { disconnect() }
            }
        }

        override fun onCharacteristicWrite(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic, status: Int) {
            writeAck?.complete(status == BluetoothGatt.GATT_SUCCESS)
        }

        // Android 13+
        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic, value: ByteArray) {
            if (characteristic.uuid == BleIds.TX) onBytes(value)
        }

        // Android 12 and older
        @Deprecated("Deprecated in Java")
        @Suppress("DEPRECATION")
        override fun onCharacteristicChanged(g: BluetoothGatt, characteristic: BluetoothGattCharacteristic) {
            if (Build.VERSION.SDK_INT < 33 && characteristic.uuid == BleIds.TX) {
                characteristic.value?.let { onBytes(it) }
            }
        }
    }

    // Collect bytes and hand out complete lines
    private fun onBytes(data: ByteArray) {
        val complete = mutableListOf<String>()
        synchronized(rxBuffer) {
            for (b in data) {
                if (b == '\n'.code.toByte()) {
                    val line = String(rxBuffer.toByteArray(), Charsets.UTF_8)
                    rxBuffer.reset()
                    if (line.isNotBlank()) complete.add(line)
                } else {
                    rxBuffer.write(b.toInt())
                    if (rxBuffer.size() > 16_384) rxBuffer.reset()   // garbage guard
                }
            }
        }
        complete.forEach { lines.tryEmit(it) }
    }

    // ------------------------------------------------------------------ send

    /** Send one JSON message. Returns false if it could not be delivered. */
    suspend fun send(text: String): Boolean {
        writeMutex.withLock {
            val g = gatt ?: return false
            val c = rxChar ?: return false
            if (_link.value != Link.Connected) return false
            val bytes = (text + "\n").toByteArray(Charsets.UTF_8)
            val chunk = (mtu - 3).coerceIn(20, 500)
            var i = 0
            while (i < bytes.size) {
                val part = bytes.copyOfRange(i, minOf(i + chunk, bytes.size))
                val ack = CompletableDeferred<Boolean>()
                writeAck = ack
                if (!writeCharacteristicCompat(g, c, part)) {
                    writeAck = null
                    return false
                }
                val ok = withTimeoutOrNull(3_000) { ack.await() } ?: false
                writeAck = null
                if (!ok) return false
                i += chunk
            }
            return true
        }
    }

    private fun writeCharacteristicCompat(g: BluetoothGatt, c: BluetoothGattCharacteristic, data: ByteArray): Boolean {
        return if (Build.VERSION.SDK_INT >= 33) {
            g.writeCharacteristic(c, data, BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT) == BluetoothStatusCodes.SUCCESS
        } else {
            legacyWriteCharacteristic(g, c, data)
        }
    }

    private fun writeDescriptorCompat(g: BluetoothGatt, d: BluetoothGattDescriptor, value: ByteArray): Boolean {
        return if (Build.VERSION.SDK_INT >= 33) {
            g.writeDescriptor(d, value) == BluetoothStatusCodes.SUCCESS
        } else {
            legacyWriteDescriptor(g, d, value)
        }
    }

    // Android 12 and older way of writing (deprecated on newer Android, still correct there)
    @Suppress("DEPRECATION")
    private fun legacyWriteCharacteristic(g: BluetoothGatt, c: BluetoothGattCharacteristic, data: ByteArray): Boolean {
        c.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        c.value = data
        return g.writeCharacteristic(c)
    }

    @Suppress("DEPRECATION")
    private fun legacyWriteDescriptor(g: BluetoothGatt, d: BluetoothGattDescriptor, value: ByteArray): Boolean {
        d.value = value
        return g.writeDescriptor(d)
    }
}
