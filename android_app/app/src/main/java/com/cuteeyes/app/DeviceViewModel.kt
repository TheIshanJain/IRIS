package com.cuteeyes.app

import android.app.Application
import android.content.Context
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.cuteeyes.app.ble.BleClient
import kotlinx.coroutines.Job
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch
import org.json.JSONArray
import org.json.JSONObject

/** What the device reports about itself ({"type":"status",...}). */
data class DeviceStatus(
    val name: String,
    val firmware: String,
    val wifi: String,        // connected | connecting | disconnected | no_networks
    val ssid: String,
    val ip: String,
    val rssi: Int,
    val saved: Int,
    val ota: String,         // idle | checking | up_to_date | downloading | success | failed
    val latest: String,
) {
    val online get() = wifi == "connected"

    companion object {
        fun from(o: JSONObject) = DeviceStatus(
            name = o.optString("name"),
            firmware = o.optString("fw", "?"),
            wifi = o.optString("wifi", "disconnected"),
            ssid = o.optString("ssid"),
            ip = o.optString("ip"),
            rssi = o.optInt("rssi"),
            saved = o.optInt("saved"),
            ota = o.optString("ota", "idle"),
            latest = o.optString("latest"),
        )
    }
}

data class WifiNet(val ssid: String, val rssi: Int, val secure: Boolean)

enum class Stage { Disconnected, Connecting, NeedsPin, Ready }

class DeviceViewModel(app: Application) : AndroidViewModel(app) {

    val ble = BleClient(app, viewModelScope)
    private val prefs = app.getSharedPreferences("cuteeyes", Context.MODE_PRIVATE)

    private val _stage = MutableStateFlow(Stage.Disconnected)
    val stage: StateFlow<Stage> = _stage.asStateFlow()

    val status = MutableStateFlow<DeviceStatus?>(null)
    val saved = MutableStateFlow<List<String>>(emptyList())
    val nearby = MutableStateFlow<List<WifiNet>>(emptyList())
    val wifiScanning = MutableStateFlow(false)
    val otaProgress = MutableStateFlow<Int?>(null)

    /** Short messages for the snackbar at the bottom of the screen. */
    val messages = MutableSharedFlow<String>(extraBufferCapacity = 16)

    /** Last device we successfully paired with: (address, name) */
    val lastDevice: Pair<String, String>?
        get() {
            val a = prefs.getString("addr", null) ?: return null
            return a to (prefs.getString("name", null) ?: "Cute Eyes")
        }

    private var scanTimeout: Job? = null

    init {
        viewModelScope.launch { ble.lines.collect { handleLine(it) } }
        viewModelScope.launch { ble.errors.collect { messages.emit(it) } }
        viewModelScope.launch {
            ble.link.collect { link ->
                when (link) {
                    BleClient.Link.Disconnected -> {
                        _stage.value = Stage.Disconnected
                        status.value = null
                        saved.value = emptyList()
                        nearby.value = emptyList()
                        wifiScanning.value = false
                        otaProgress.value = null
                    }
                    BleClient.Link.Connecting -> _stage.value = Stage.Connecting
                    BleClient.Link.Connected -> _stage.value = Stage.NeedsPin
                }
            }
        }
    }

    fun say(text: String) {
        messages.tryEmit(text)
    }

    // ------------------------------------------------------- connection

    fun connect(address: String, name: String) = ble.connect(address, name)

    fun disconnect() = ble.disconnect()

    // --------------------------------------------------------- incoming

    private fun handleLine(line: String) {
        val o = try {
            JSONObject(line)
        } catch (e: Exception) {
            return
        }
        when (o.optString("type")) {
            "auth" -> {
                if (o.optBoolean("ok")) {
                    _stage.value = Stage.Ready
                    prefs.edit()
                        .putString("addr", ble.deviceAddress)
                        .putString("name", ble.deviceName)
                        .apply()
                }
                o.optString("msg").takeIf { it.isNotEmpty() }?.let { say(it) }
            }
            "status" -> {
                val s = DeviceStatus.from(o)
                status.value = s
                if (s.ota != "downloading") otaProgress.value = null
            }
            "list" -> saved.value = strings(o.optJSONArray("networks"))
            "scan" -> {
                val arr = o.optJSONArray("networks") ?: JSONArray()
                nearby.value = (0 until arr.length()).mapNotNull { i ->
                    arr.optJSONObject(i)?.let {
                        WifiNet(it.optString("ssid"), it.optInt("rssi", -100), it.optBoolean("secure", true))
                    }
                }
                wifiScanning.value = false
                scanTimeout?.cancel()
            }
            "ota_progress" -> otaProgress.value = o.optInt("pct")
            "ack", "error" -> o.optString("msg").takeIf { it.isNotEmpty() }?.let { say(it) }
        }
    }

    private fun strings(arr: JSONArray?): List<String> =
        if (arr == null) emptyList() else (0 until arr.length()).map { arr.optString(it) }

    // --------------------------------------------------------- commands

    private fun command(json: JSONObject) {
        viewModelScope.launch {
            if (!ble.send(json.toString())) say("Couldn't reach the device. Is it still connected?")
        }
    }

    fun sendPin(pin: String) = command(JSONObject().put("cmd", "auth").put("pin", pin))
    fun refreshStatus() = command(JSONObject().put("cmd", "status"))
    fun refreshSaved() = command(JSONObject().put("cmd", "list"))
    fun checkForUpdate() = command(JSONObject().put("cmd", "ota"))
    fun connectNetwork(ssid: String) = command(JSONObject().put("cmd", "connect").put("ssid", ssid))
    fun removeNetwork(ssid: String) = command(JSONObject().put("cmd", "delete").put("ssid", ssid))
    fun addNetwork(ssid: String, pass: String) =
        command(JSONObject().put("cmd", "add").put("ssid", ssid).put("pass", pass))

    fun scanWifi() {
        wifiScanning.value = true
        command(JSONObject().put("cmd", "scan"))
        scanTimeout?.cancel()
        scanTimeout = viewModelScope.launch {
            delay(15_000)
            if (wifiScanning.value) {
                wifiScanning.value = false
                say("The device didn't answer the scan. Try again.")
            }
        }
    }

    override fun onCleared() {
        ble.disconnect()
        super.onCleared()
    }
}
