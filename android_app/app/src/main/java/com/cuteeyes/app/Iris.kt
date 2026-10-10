package com.cuteeyes.app

import android.app.Notification
import android.content.Context
import android.content.SharedPreferences
import android.graphics.Bitmap
import android.graphics.Canvas
import android.service.notification.StatusBarNotification
import android.util.Base64
import com.cuteeyes.app.ble.BleClient
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeoutOrNull
import org.json.JSONArray
import org.json.JSONObject
import java.security.MessageDigest
import java.text.Normalizer

// ============================================================ data

enum class Stage { Disconnected, Connecting, NeedsPin, Ready }

data class DeviceStatus(
    val name: String, val firmware: String, val wifi: String, val ssid: String, val ip: String,
    val rssi: Int, val saved: Int, val ota: String, val latest: String,
    val battery: Int, val batteryVolts: Double, val batteryState: String,
    val expression: String, val expressionTheme: Int, val screen: String,
) {
    val online get() = wifi == "connected"

    companion object {
        fun from(o: JSONObject) = DeviceStatus(
            name = o.optString("name"), firmware = o.optString("fw", "?"),
            wifi = o.optString("wifi", "disconnected"), ssid = o.optString("ssid"), ip = o.optString("ip"),
            rssi = o.optInt("rssi"), saved = o.optInt("saved"), ota = o.optString("ota", "idle"),
            latest = o.optString("latest"), battery = o.optInt("bat", -1), batteryVolts = o.optDouble("batV", 0.0),
            batteryState = o.optString("batState", "none"), expression = o.optString("expr"),
            expressionTheme = o.optInt("exprTheme", 1), screen = o.optString("screen", "eyes"),
        )
    }
}

data class DeviceSettings(
    val brightness: Int = 200, val labels: Boolean = true, val auto: Boolean = true,
    val shake: Boolean = true, val sens: Int = 5, val shakeRandom: Boolean = false,
    val name: String = "", val screenTimeout: Int = 300, val clockAfter: Int = 60,
    val eyeTheme: Int = 1, val notifyOn: Boolean = true, val notifySecs: Int = 8, val city: String = "",
) {
    companion object {
        fun from(o: JSONObject) = DeviceSettings(
            brightness = o.optInt("brightness", 200), labels = o.optBoolean("labels", true),
            auto = o.optBoolean("auto", true), shake = o.optBoolean("shake", true), sens = o.optInt("sens", 5),
            shakeRandom = o.optBoolean("shakeRandom"), name = o.optString("name"),
            screenTimeout = o.optInt("screenTimeout", 300), clockAfter = o.optInt("clockAfter", 60),
            eyeTheme = o.optInt("eyeTheme", 1), notifyOn = o.optBoolean("notifyOn", true),
            notifySecs = o.optInt("notifySecs", 8), city = o.optString("city"),
        )
    }
}

data class DeviceInfo(
    val timeValid: Boolean, val time: String, val date: String, val haveLoc: Boolean, val location: String,
    val manual: Boolean, val greet: String, val haveWeather: Boolean, val temp: Double, val feels: Double,
    val humidity: Double, val wind: Double, val tmax: Double, val tmin: Double, val text: String,
    val code: Int, val isDay: Boolean, val sunrise: String, val sunset: String,
) {
    companion object {
        fun from(o: JSONObject) = DeviceInfo(
            timeValid = o.optBoolean("valid"), time = o.optString("time"), date = o.optString("date"),
            haveLoc = o.optBoolean("haveLoc"), location = o.optString("loc"), manual = o.optBoolean("manual"),
            greet = o.optString("greet", "Hello"), haveWeather = o.optBoolean("have"),
            temp = o.optDouble("temp", 0.0), feels = o.optDouble("feels", 0.0), humidity = o.optDouble("hum", 0.0),
            wind = o.optDouble("wind", 0.0), tmax = o.optDouble("tmax", 0.0), tmin = o.optDouble("tmin", 0.0),
            text = o.optString("text"), code = o.optInt("code", -1), isDay = o.optBoolean("isDay", true),
            sunrise = o.optString("sunrise"), sunset = o.optString("sunset"),
        )
    }
}

data class EyeTheme(val id: Int, val name: String, val description: String, val expressions: List<String>)
data class WifiNet(val ssid: String, val rssi: Int, val secure: Boolean)
data class SeenApp(val pkg: String, val label: String)
data class ExpressionCatalog(val names: List<String>, val themes: List<Int>, val themeNames: List<String>)

// ============================================================ the shared brain

/**
 * One shared object for the whole app process: the Bluetooth link, everything the
 * device reports, and all commands. The screens, the background service and the
 * notification reader all use it, so the connection survives closing the app.
 */
object Iris {

    private lateinit var appContext: Context
    private lateinit var prefs: SharedPreferences
    lateinit var ble: BleClient
        private set
    val scope = CoroutineScope(SupervisorJob() + Dispatchers.Main)

    val stage = MutableStateFlow(Stage.Disconnected)
    val status = MutableStateFlow<DeviceStatus?>(null)
    val settings = MutableStateFlow<DeviceSettings?>(null)
    val info = MutableStateFlow<DeviceInfo?>(null)
    val themes = MutableStateFlow<List<EyeTheme>>(emptyList())
    val saved = MutableStateFlow<List<String>>(emptyList())
    val nearby = MutableStateFlow<List<WifiNet>>(emptyList())
    val wifiScanning = MutableStateFlow(false)
    val otaProgress = MutableStateFlow<Int?>(null)
    val appThemeId = MutableStateFlow("neon")
    val listenerConnected = MutableStateFlow(false)

    // developer
    val devUnlocked = MutableStateFlow(false)
    val devStatus = MutableStateFlow<JSONObject?>(null)
    val expressions = MutableStateFlow<ExpressionCatalog?>(null)
    val logs = MutableStateFlow<List<String>>(emptyList())
    val fwProgress = MutableStateFlow<Float?>(null)

    /** Short messages for the snackbar. */
    val messages = MutableSharedFlow<String>(extraBufferCapacity = 16)

    private var authWithToken = false
    private var reconnectJob: Job? = null
    private var reconnectAttempt = 0
    private var scanTimeout: Job? = null
    private val iconsSent = mutableSetOf<String>()
    private var lastNotifKey = ""
    private var lastNotifAt = 0L
    private var logSince = 0L
    private val pendingAcks = mutableMapOf<String, CompletableDeferred<JSONObject>>()

    // ------------------------------------------------------------ setup

    fun init(context: Context) {
        if (::ble.isInitialized) return
        appContext = context.applicationContext
        prefs = appContext.getSharedPreferences("iris", Context.MODE_PRIVATE)
        appThemeId.value = prefs.getString("appTheme", "neon") ?: "neon"
        ble = BleClient(appContext, scope)

        scope.launch { ble.lines.collect { handleLine(it) } }
        scope.launch { ble.errors.collect { if (!quietReconnect()) messages.emit(it) } }
        scope.launch {
            ble.link.collect { link ->
                when (link) {
                    BleClient.Link.Disconnected -> onDisconnected()
                    BleClient.Link.Connecting -> if (stage.value != Stage.Ready) stage.value = Stage.Connecting
                    BleClient.Link.Connected -> onLinkUp()
                }
            }
        }
        // Reconnect by itself if we were connected before (app reopened, phone woke up...)
        if (wantLink && lastDevice != null && tokenFor(lastDevice!!.first) != null) scheduleReconnect(0)
    }

    fun say(text: String) {
        messages.tryEmit(text)
    }

    // ------------------------------------------------------------ preferences

    val lastDevice: Pair<String, String>?
        get() {
            val a = prefs.getString("addr", null) ?: return null
            return a to (prefs.getString("name", null) ?: "IRIS")
        }

    private var wantLink: Boolean
        get() = prefs.getBoolean("wantLink", false)
        set(v) = prefs.edit().putBoolean("wantLink", v).apply()

    private fun tokenFor(addr: String): String? = prefs.getString("token_$addr", null)

    var forwardNotifications: Boolean
        get() = prefs.getBoolean("fwdNotif", true)
        set(v) = prefs.edit().putBoolean("fwdNotif", v).apply()

    var showMessageText: Boolean
        get() = prefs.getBoolean("showText", true)
        set(v) = prefs.edit().putBoolean("showText", v).apply()

    fun isAppBlocked(pkg: String) = prefs.getStringSet("blocked", emptySet())!!.contains(pkg)

    fun setAppBlocked(pkg: String, blocked: Boolean) {
        val s = prefs.getStringSet("blocked", emptySet())!!.toMutableSet()
        if (blocked) s.add(pkg) else s.remove(pkg)
        prefs.edit().putStringSet("blocked", s).apply()
    }

    fun seenApps(): List<SeenApp> =
        prefs.getStringSet("seenApps", emptySet())!!
            .mapNotNull { e -> e.split('|', limit = 2).takeIf { it.size == 2 }?.let { SeenApp(it[0], it[1]) } }
            .sortedBy { it.label.lowercase() }

    private fun rememberApp(pkg: String, label: String) {
        val s = prefs.getStringSet("seenApps", emptySet())!!
        if (s.any { it.startsWith("$pkg|") }) return
        prefs.edit().putStringSet("seenApps", s + "$pkg|$label").apply()
    }

    fun setAppTheme(id: String) {
        appThemeId.value = id
        prefs.edit().putString("appTheme", id).apply()
    }

    // ------------------------------------------------------------ connection

    fun connect(address: String, name: String) {
        reconnectJob?.cancel()
        reconnectAttempt = 0
        ble.connect(address, name)
    }

    /** User pressed Disconnect: stop auto-reconnect and the background service. */
    fun disconnect() {
        wantLink = false
        reconnectJob?.cancel()
        ble.disconnect()
        IrisService.stop(appContext)
    }

    /** Forget this IRIS on the phone (and ask the device to forget this phone). */
    fun forgetDevice() {
        val addr = ble.deviceAddress
        val token = tokenFor(addr)
        scope.launch {
            if (token != null) ble.send(JSONObject().put("cmd", "forget_me").put("token", token).toString())
            prefs.edit().remove("token_$addr").remove("addr").remove("name").apply()
            disconnect()
        }
    }

    private fun quietReconnect() = wantLink && stage.value != Stage.Ready && reconnectAttempt > 0

    private fun onLinkUp() {
        iconsSent.clear()
        val token = tokenFor(ble.deviceAddress)
        if (token != null) {
            authWithToken = true
            stage.value = Stage.Connecting
            send(JSONObject().put("cmd", "auth").put("token", token))
        } else {
            authWithToken = false
            stage.value = Stage.NeedsPin
        }
    }

    private fun onDisconnected() {
        val wasReady = stage.value == Stage.Ready
        stage.value = Stage.Disconnected
        status.value = null
        nearby.value = emptyList()
        wifiScanning.value = false
        otaProgress.value = null
        devUnlocked.value = false
        fwProgress.value = null
        iconsSent.clear()
        if (wantLink && lastDevice != null && tokenFor(lastDevice!!.first) != null) {
            if (wasReady) reconnectAttempt = 0
            scheduleReconnect(backoff())
        }
    }

    private fun backoff(): Long {
        val steps = longArrayOf(2_000, 5_000, 10_000, 20_000, 30_000, 60_000)
        return steps[minOf(reconnectAttempt, steps.size - 1)]
    }

    private fun scheduleReconnect(afterMs: Long) {
        reconnectJob?.cancel()
        reconnectJob = scope.launch {
            delay(afterMs)
            val d = lastDevice ?: return@launch
            if (!ble.isBluetoothOn || stage.value != Stage.Disconnected) return@launch
            reconnectAttempt++
            ble.connect(d.first, d.second)
        }
    }

    private fun onReady(token: String?) {
        stage.value = Stage.Ready
        reconnectAttempt = 0
        wantLink = true
        val ed = prefs.edit().putString("addr", ble.deviceAddress).putString("name", ble.deviceName)
        if (token != null) ed.putString("token_${ble.deviceAddress}", token)
        ed.apply()
        IrisService.start(appContext)
        refreshAll()
    }

    // ------------------------------------------------------------ incoming

    private fun handleLine(line: String) {
        val o = try {
            JSONObject(line)
        } catch (e: Exception) {
            return
        }
        when (o.optString("type")) {
            "auth" -> {
                if (o.optBoolean("ok")) {
                    onReady(o.optString("token").takeIf { it.length == 32 })
                    if (!authWithToken) say(o.optString("msg"))
                } else if (authWithToken) {
                    // The device no longer knows this phone: pair again with the code
                    prefs.edit().remove("token_${ble.deviceAddress}").apply()
                    authWithToken = false
                    stage.value = Stage.NeedsPin
                    say("Please pair again with the code on IRIS")
                } else {
                    say(o.optString("msg", "Wrong code"))
                }
            }
            "status" -> {
                val s = DeviceStatus.from(o)
                status.value = s
                if (s.ota != "downloading") otaProgress.value = null
            }
            "settings" -> settings.value = DeviceSettings.from(o)
            "info" -> info.value = DeviceInfo.from(o)
            "themes" -> {
                val arr = o.optJSONArray("themes") ?: JSONArray()
                themes.value = (0 until arr.length()).mapNotNull { i ->
                    arr.optJSONObject(i)?.let {
                        EyeTheme(it.optInt("id"), it.optString("name"), it.optString("desc"), strings(it.optJSONArray("expr")))
                    }
                }
            }
            "list" -> saved.value = strings(o.optJSONArray("networks"))
            "scan" -> {
                val arr = o.optJSONArray("networks") ?: JSONArray()
                nearby.value = (0 until arr.length()).mapNotNull { i ->
                    arr.optJSONObject(i)?.let { WifiNet(it.optString("ssid"), it.optInt("rssi", -100), it.optBoolean("secure", true)) }
                }
                wifiScanning.value = false
                scanTimeout?.cancel()
            }
            "ota_progress" -> otaProgress.value = o.optInt("pct")
            "dev_login" -> {
                devUnlocked.value = o.optBoolean("ok")
                say(o.optString("msg"))
                if (devUnlocked.value) {
                    send(JSONObject().put("cmd", "dev_status"))
                    send(JSONObject().put("cmd", "expressions"))
                }
            }
            "dev_status" -> devStatus.value = o
            "expressions" -> expressions.value = ExpressionCatalog(
                strings(o.optJSONArray("names")),
                (o.optJSONArray("themes") ?: JSONArray()).let { a -> (0 until a.length()).map { a.optInt(it) } },
                strings(o.optJSONArray("themeNames")),
            )
            "log" -> {
                val arr = o.optJSONArray("lines") ?: JSONArray()
                val add = (0 until arr.length()).mapNotNull { i ->
                    arr.optJSONObject(i)?.let { "[" + String.format("%.1f", it.optLong("t") / 1000.0) + "] " + it.optString("m") }
                }
                logSince = o.optLong("last", logSince)
                if (add.isNotEmpty()) logs.value = (logs.value + add).takeLast(400)
            }
            "ack" -> {
                pendingAcks.remove(o.optString("cmd"))?.complete(o)
                val cmd = o.optString("cmd")
                if (cmd != "fw_begin" && cmd != "fw_end") o.optString("msg").takeIf { it.isNotEmpty() }?.let { say(it) }
            }
            "error" -> {
                val msg = o.optString("msg")
                if (msg.contains("Developer login")) devUnlocked.value = false
                if (msg.isNotEmpty()) say(msg)
            }
        }
    }

    private fun strings(arr: JSONArray?): List<String> =
        if (arr == null) emptyList() else (0 until arr.length()).map { arr.optString(it) }

    // ------------------------------------------------------------ commands

    private fun send(json: JSONObject) {
        scope.launch {
            if (!ble.send(json.toString()) && stage.value == Stage.Ready) say("Couldn't reach IRIS. Is it still connected?")
        }
    }

    private fun cmd(name: String) = JSONObject().put("cmd", name)

    fun refreshAll() {
        send(cmd("status")); send(cmd("get_settings")); send(cmd("info")); send(cmd("themes")); send(cmd("list"))
    }

    fun refreshStatus() = send(cmd("status"))
    fun refreshInfo() = send(cmd("info"))
    fun refreshSaved() = send(cmd("list"))
    fun sendPin(pin: String) = send(cmd("auth").put("pin", pin))
    fun next() = send(cmd("next"))
    fun greet() = send(cmd("greet"))
    fun checkForUpdate() = send(cmd("ota"))
    fun connectNetwork(ssid: String) = send(cmd("connect").put("ssid", ssid))
    fun removeNetwork(ssid: String) = send(cmd("delete").put("ssid", ssid))
    fun addNetwork(ssid: String, pass: String) = send(cmd("add").put("ssid", ssid).put("pass", pass))

    /** Change one or more device settings, e.g. set("brightness", 180). */
    fun set(key: String, value: Any) {
        // update the screen immediately, the device confirms with a "settings" message
        settings.value?.let { s ->
            settings.value = when (key) {
                "brightness" -> s.copy(brightness = value as Int)
                "labels" -> s.copy(labels = value as Boolean)
                "auto" -> s.copy(auto = value as Boolean)
                "shake" -> s.copy(shake = value as Boolean)
                "sens" -> s.copy(sens = value as Int)
                "shakeRandom" -> s.copy(shakeRandom = value as Boolean)
                "name" -> s.copy(name = value as String)
                "screenTimeout" -> s.copy(screenTimeout = value as Int)
                "clockAfter" -> s.copy(clockAfter = value as Int)
                "eyeTheme" -> s.copy(eyeTheme = value as Int)
                "notifyOn" -> s.copy(notifyOn = value as Boolean)
                "notifySecs" -> s.copy(notifySecs = value as Int)
                "city" -> s.copy(city = value as String)
                else -> s
            }
        }
        send(cmd("set").put(key, value))
        if (key == "city" || key == "name") scope.launch { delay(1500); refreshInfo() }
    }

    fun scanWifi() {
        wifiScanning.value = true
        send(cmd("scan"))
        scanTimeout?.cancel()
        scanTimeout = scope.launch {
            delay(15_000)
            if (wifiScanning.value) {
                wifiScanning.value = false
                say("IRIS didn't answer the scan. Try again.")
            }
        }
    }

    // ------------------------------------------------------------ developer

    fun devLogin(user: String, pass: String) = send(cmd("dev_login").put("user", user).put("pass", pass))
    fun devLogout() {
        send(cmd("dev_logout"))
        devUnlocked.value = false
        devStatus.value = null
    }
    fun devRefresh() = send(cmd("dev_status"))
    fun devShowExpression(i: Int) = send(cmd("expr").put("index", i))
    fun devReboot() = send(cmd("reboot"))
    fun devPollLog() = send(cmd("log").put("since", logSince))
    fun devClearLog() {
        logs.value = emptyList()
    }

    private suspend fun awaitAck(name: String, json: JSONObject, timeoutMs: Long): JSONObject? {
        val d = CompletableDeferred<JSONObject>()
        pendingAcks[name] = d
        if (!ble.send(json.toString())) {
            pendingAcks.remove(name)
            return null
        }
        return withTimeoutOrNull(timeoutMs) { d.await() }.also { pendingAcks.remove(name) }
    }

    /** Upload a firmware .bin over Bluetooth (developer). */
    fun uploadFirmware(bytes: ByteArray) {
        scope.launch {
            if (!ble.supportsFirmwareUpload) {
                say("This IRIS firmware can't receive uploads over Bluetooth")
                return@launch
            }
            if (bytes.size < 1024 || bytes[0] != 0xE9.toByte()) {
                say("That isn't an ESP32 firmware file (.ino.bin)")
                return@launch
            }
            fwProgress.value = 0f
            val md5 = withContext(Dispatchers.Default) {
                MessageDigest.getInstance("MD5").digest(bytes).joinToString("") { "%02x".format(it) }
            }
            val begin = awaitAck("fw_begin", cmd("fw_begin").put("size", bytes.size).put("md5", md5), 15_000)
            if (begin == null || !begin.optBoolean("ok")) {
                fwProgress.value = null
                say(begin?.optString("msg") ?: "IRIS didn't answer")
                return@launch
            }
            val chunk = ble.chunkSize
            var off = 0
            while (off < bytes.size) {
                val end = minOf(off + chunk, bytes.size)
                if (!ble.sendFirmware(bytes.copyOfRange(off, end))) {
                    fwProgress.value = null
                    send(cmd("fw_abort"))
                    say("Upload interrupted - try again")
                    return@launch
                }
                off = end
                fwProgress.value = off.toFloat() / bytes.size
            }
            val done = awaitAck("fw_end", cmd("fw_end"), 30_000)
            fwProgress.value = null
            say(done?.optString("msg") ?: "No answer after upload")
        }
    }

    // ------------------------------------------------------------ phone notifications

    /** Called by IrisNotificationListener for every new phone notification. */
    fun onPhoneNotification(sbn: StatusBarNotification) {
        val pkg = sbn.packageName
        if (pkg == appContext.packageName) return
        val n = sbn.notification ?: return
        if (sbn.isOngoing || (n.flags and Notification.FLAG_GROUP_SUMMARY) != 0) return
        when (n.category) {
            Notification.CATEGORY_TRANSPORT, Notification.CATEGORY_PROGRESS,
            Notification.CATEGORY_SERVICE, Notification.CATEGORY_SYSTEM -> return
        }
        val label = appLabel(pkg)
        rememberApp(pkg, label)
        if (!forwardNotifications || isAppBlocked(pkg) || stage.value != Stage.Ready) return

        val ex = n.extras
        val rawTitle = (ex.getCharSequence(Notification.EXTRA_CONVERSATION_TITLE)
            ?: ex.getCharSequence(Notification.EXTRA_TITLE))?.toString().orEmpty()
        val rawText = (ex.getCharSequence(Notification.EXTRA_TEXT)
            ?: ex.getCharSequence(Notification.EXTRA_BIG_TEXT))?.toString().orEmpty()
        val firstLine = rawText.lineSequence().firstOrNull { it.isNotBlank() }.orEmpty()
        if (rawTitle.isBlank() && firstLine.isBlank()) return

        val title = plain(rawTitle).take(40)
        var text = if (showMessageText) plain(firstLine).take(80) else "New notification"
        if (text.isBlank() && firstLine.isNotBlank()) text = "New message"   // e.g. only emoji / other scripts

        val key = "$pkg|$title|$text"
        val now = System.currentTimeMillis()
        if (key == lastNotifKey && now - lastNotifAt < 3000) return   // apps often post duplicates
        lastNotifKey = key
        lastNotifAt = now

        sendNotification(pkg, plain(label).take(24), title, text)
    }

    /** Send a sample notification (Settings -> Test). */
    fun sendTestNotification() {
        sendNotification(appContext.packageName, "IRIS", "Hello from IRIS", "This is how notifications look")
    }

    private fun sendNotification(pkg: String, app: String, title: String, text: String) {
        scope.launch {
            var color = 0
            if (pkg !in iconsSent) {
                val icon = withContext(Dispatchers.Default) { iconRgb565(pkg) }
                if (icon != null) {
                    color = icon.second
                    val b64 = Base64.encodeToString(icon.first, Base64.NO_WRAP)
                    val parts = b64.chunked(320)
                    for ((i, p) in parts.withIndex()) {
                        ble.send(cmd("icon").put("pkg", pkg).put("i", i).put("n", parts.size).put("d", p).toString())
                    }
                    iconsSent.add(pkg)
                    accentCache[pkg] = color
                }
            } else {
                color = accentCache[pkg] ?: 0
            }
            ble.send(cmd("notify").put("pkg", pkg).put("app", app).put("title", title).put("text", text).put("color", color).toString())
        }
    }

    private val accentCache = mutableMapOf<String, Int>()

    private fun appLabel(pkg: String): String = try {
        val pm = appContext.packageManager
        pm.getApplicationLabel(pm.getApplicationInfo(pkg, 0)).toString()
    } catch (e: Exception) {
        pkg.substringAfterLast('.').replaceFirstChar { it.uppercase() }
    }

    /** App icon as 48x48 RGB565 (little-endian) on black, plus an accent colour (RGB565). */
    private fun iconRgb565(pkg: String): Pair<ByteArray, Int>? = try {
        val size = 48
        val drawable = appContext.packageManager.getApplicationIcon(pkg)
        val bmp = Bitmap.createBitmap(size, size, Bitmap.Config.ARGB_8888)
        drawable.setBounds(0, 0, size, size)
        drawable.draw(Canvas(bmp))
        val px = IntArray(size * size)
        bmp.getPixels(px, 0, size, 0, 0, size, size)
        val out = ByteArray(size * size * 2)
        var rs = 0L
        var gs = 0L
        var bs = 0L
        var count = 0
        for (i in px.indices) {
            val p = px[i]
            val a = (p ushr 24) and 0xFF
            val r = ((p shr 16) and 0xFF) * a / 255
            val g = ((p shr 8) and 0xFF) * a / 255
            val b = (p and 0xFF) * a / 255
            val v = ((r and 0xF8) shl 8) or ((g and 0xFC) shl 3) or (b shr 3)
            out[i * 2] = (v and 0xFF).toByte()
            out[i * 2 + 1] = ((v shr 8) and 0xFF).toByte()
            val mx = maxOf(r, g, b)
            val mn = minOf(r, g, b)
            if (a > 200 && mx > 60 && mx - mn > 40) {      // colourful pixels only
                rs += r; gs += g; bs += b; count++
            }
        }
        bmp.recycle()
        val accent = if (count > 20) {
            val r = (rs / count).toInt()
            val g = (gs / count).toInt()
            val b = (bs / count).toInt()
            ((r and 0xF8) shl 8) or ((g and 0xFC) shl 3) or (b shr 3)
        } else 0
        out to accent
    } catch (e: Exception) {
        null
    }

    /** Plain ASCII for the device's font: accents removed, emoji/other scripts dropped. */
    private fun plain(s: String): String {
        val noAccents = Normalizer.normalize(s, Normalizer.Form.NFD).replace(Regex("\\p{M}+"), "")
        return noAccents.map { c -> if (c.code in 32..126) c else ' ' }.joinToString("")
            .replace(Regex("\\s+"), " ").trim()
    }
}
