/*
 * WebPage.h - the web UI (HTML + CSS + JS in one string, served from flash).
 * Mobile-app style: Home / Wi-Fi / Style / Settings for everyone,
 * Developer (details, logs, any expression, firmware) behind the admin login.
 */
#pragma once

static const char INDEX_HTML[] PROGMEM = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">
<meta name="theme-color" content="#0d1020">
<title>Cute Eyes</title>
<style>
:root{--bg:#0d1020;--card:#171a2e;--card2:#1f2340;--line:#2a2f52;--txt:#eef1ff;--dim:#8d93b5;--acc:#46e1ff;--pink:#ff6fae;--pur:#a98bff;--ok:#4dffa0;--warn:#ffc24d;--bad:#ff6b81;--fc:#46e1ff}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}
html,body{margin:0;background:var(--bg);color:var(--txt);font:15px/1.45 ui-rounded,"SF Pro Rounded","Nunito","Segoe UI",system-ui,sans-serif}
body{background:radial-gradient(600px 400px at 0% 0%,rgba(255,111,174,.12),transparent 60%),radial-gradient(600px 400px at 100% 10%,rgba(70,225,255,.12),transparent 60%),var(--bg);background-attachment:fixed;min-height:100vh}
.app{max-width:480px;margin:0 auto;padding:0 16px calc(96px + env(safe-area-inset-bottom))}
/* header */
.top{display:flex;align-items:center;gap:12px;padding:calc(14px + env(safe-area-inset-top)) 2px 10px;position:sticky;top:0;z-index:5;background:linear-gradient(var(--bg) 75%,transparent)}
.avatar{width:44px;height:44px;border-radius:50%;background:#000;border:2px solid var(--fc);display:flex;align-items:center;justify-content:center;gap:7px;box-shadow:0 0 16px color-mix(in srgb,var(--fc) 45%,transparent);transition:.4s}
.avatar i{width:7px;height:14px;border-radius:4px;background:var(--fc);animation:blink 4s infinite}
.top .t{flex:1;min-width:0}
.top h1{font-size:19px;margin:0;font-weight:800;letter-spacing:.2px}
.top .s{color:var(--dim);font-size:13px;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.dot{width:10px;height:10px;border-radius:50%;background:var(--dim)}
.dot.ok{background:var(--ok);box-shadow:0 0 10px var(--ok)}
.dot.warn{background:var(--warn);animation:pulse 1s infinite}
@keyframes blink{0%,92%,100%{transform:scaleY(1)}95%{transform:scaleY(.1)}}
@keyframes pulse{50%{opacity:.35}}
@keyframes pop{0%{transform:scale(.96);opacity:0}100%{transform:none;opacity:1}}
section{display:none;animation:pop .25s ease}
section.on{display:block}
h4{margin:22px 4px 8px;font-size:13px;color:var(--dim);text-transform:uppercase;letter-spacing:.08em;font-weight:700}
.card{background:var(--card);border:1px solid var(--line);border-radius:22px;padding:16px;margin-bottom:12px}
/* hero */
.hero{text-align:center;padding:22px 16px 18px;background:linear-gradient(160deg,#1c2142,#141730)}
.face{width:128px;height:128px;margin:0 auto 12px;border-radius:50%;background:#000;border:3px solid var(--fc);display:flex;align-items:center;justify-content:center;gap:22px;box-shadow:0 0 30px color-mix(in srgb,var(--fc) 40%,transparent),inset 0 0 22px color-mix(in srgb,var(--fc) 18%,transparent);transition:.4s}
.face i{width:20px;height:40px;border-radius:12px;background:var(--fc);animation:blink 3.5s infinite;transition:.4s}
.face:active{transform:scale(.95)}
.greet{font-size:22px;font-weight:800;margin:0}
.greet b{color:var(--pink)}
.expr{color:var(--dim);margin:4px 0 14px;font-size:14px}
.tag{display:inline-block;background:var(--card2);border-radius:99px;padding:2px 10px;font-size:12px;margin-left:4px}
.big{width:100%;border:0;border-radius:16px;padding:14px;font:inherit;font-weight:800;font-size:16px;color:#0b1020;background:linear-gradient(90deg,var(--acc),var(--pur));cursor:pointer;transition:transform .12s}
.big:active,.pbtn:active,.chip:active{transform:scale(.96)}
.duo{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.duo .card{margin:0;text-align:center;padding:16px 10px}
.clock{font-size:34px;font-weight:800;letter-spacing:1px}
.mute{color:var(--dim);font-size:13px}
.wx{font-size:30px;line-height:1}
.wxt{font-size:24px;font-weight:800;margin-top:4px}
details.card{padding:0}
details>summary{list-style:none;cursor:pointer;padding:14px 16px;display:flex;align-items:center;gap:10px;font-weight:700}
details>summary::-webkit-details-marker{display:none}
details>summary:after{content:"\203A";margin-left:auto;color:var(--dim);font-size:22px;transition:.2s}
details[open]>summary:after{transform:rotate(90deg)}
details .in{padding:0 16px 14px}
.kv{display:flex;justify-content:space-between;gap:12px;padding:7px 0;border-bottom:1px dashed var(--line);font-size:14px}
.kv:last-child{border:0}
.kv span:first-child{color:var(--dim)}
.kv span:last-child{text-align:right;word-break:break-word}
/* list rows */
.list{padding:4px 14px}
.row{display:flex;align-items:center;gap:12px;padding:12px 0;border-bottom:1px solid var(--line)}
.row:last-child{border:0}
.ic{width:38px;height:38px;border-radius:12px;background:var(--card2);display:flex;align-items:center;justify-content:center;font-size:19px;flex:none}
.grow{flex:1;min-width:0}
.grow b{display:block;font-weight:700;white-space:nowrap;overflow:hidden;text-overflow:ellipsis}
.grow small{color:var(--dim);font-size:12px}
.row.tap{cursor:pointer}
/* controls */
.sw{position:relative;width:50px;height:30px;flex:none}
.sw input{opacity:0;width:0;height:0}
.sw span{position:absolute;inset:0;background:#2c3157;border-radius:99px;transition:.25s;cursor:pointer}
.sw span:before{content:"";position:absolute;width:24px;height:24px;left:3px;top:3px;background:#fff;border-radius:50%;transition:.25s cubic-bezier(.3,1.6,.6,1)}
.sw input:checked+span{background:linear-gradient(90deg,var(--acc),var(--pur))}
.sw input:checked+span:before{transform:translateX(20px)}
input[type=range]{width:100%;accent-color:var(--acc);margin:8px 0 2px}
select,input[type=text],input[type=password]{background:var(--card2);color:var(--txt);border:1px solid var(--line);border-radius:12px;padding:10px 12px;font:inherit;font-size:14px;width:100%;outline:none}
select:focus,input:focus{border-color:var(--acc)}
select{appearance:none;-webkit-appearance:none;background-image:linear-gradient(45deg,transparent 50%,var(--dim) 50%),linear-gradient(135deg,var(--dim) 50%,transparent 50%);background-position:calc(100% - 18px) 50%,calc(100% - 13px) 50%;background-size:5px 5px;background-repeat:no-repeat;padding-right:32px}
.sel{width:auto;min-width:140px;max-width:170px}
select.more{width:40px;min-width:0;height:38px;padding:0;text-align:center;background-image:none;font-size:20px;color:var(--dim)}
.pbtn{border:0;border-radius:14px;padding:11px 16px;font:inherit;font-weight:700;cursor:pointer;background:var(--card2);color:var(--txt);transition:transform .12s}
.pbtn.acc{background:linear-gradient(90deg,var(--acc),var(--pur));color:#0b1020}
.pbtn.red{background:rgba(255,107,129,.15);color:var(--bad)}
.pbtn:disabled{opacity:.45}
.btns{display:flex;gap:10px;margin-top:12px}
.btns .pbtn{flex:1}
.chips{display:flex;flex-wrap:wrap;gap:8px}
.chip{display:inline-flex;align-items:center;gap:6px;background:var(--card2);border:1px solid var(--line);border-radius:99px;padding:7px 12px;font-size:13px;color:var(--txt);font:inherit;font-size:13px;cursor:default}
button.chip{cursor:pointer}
.chip.on{border-color:var(--acc);color:var(--acc);box-shadow:0 0 12px rgba(70,225,255,.25)}
.chip.dim{opacity:.5}
.note{color:var(--dim);font-size:12px;text-align:center;margin:10px 0}
.bars{display:inline-flex;gap:2px;align-items:flex-end;height:14px;vertical-align:middle}
.bars i{width:4px;background:#363c66;border-radius:1px}
.bars i.on{background:var(--ok)}
/* themes */
.theme{display:flex;gap:12px;align-items:center}
.theme .ic{width:46px;height:46px;font-size:24px;border-radius:15px}
.thcard{transition:.3s}
.thcard.on{border-color:color-mix(in srgb,var(--tc) 60%,transparent);box-shadow:0 0 18px color-mix(in srgb,var(--tc) 18%,transparent)}
.thcard details summary{padding:10px 0 0;font-weight:600;font-size:13px;color:var(--dim)}
.thcard details .chips{margin-top:10px}
.bubble{background:linear-gradient(135deg,rgba(70,225,255,.15),rgba(169,139,255,.18));border:1px solid var(--line);border-radius:20px 20px 20px 6px;padding:14px 16px;font-size:20px;font-weight:800;margin-bottom:12px}
.bubble b{color:var(--pink)}
.lbl{display:flex;justify-content:space-between;align-items:center;gap:12px}
.lbl small{display:block;color:var(--dim);font-size:12px;font-weight:400}
/* bottom nav */
.tabs{position:fixed;left:50%;transform:translateX(-50%);bottom:0;width:100%;max-width:480px;display:flex;background:rgba(23,26,46,.92);backdrop-filter:blur(12px);border-top:1px solid var(--line);padding:6px 6px calc(6px + env(safe-area-inset-bottom));z-index:9;border-radius:22px 22px 0 0}
.tabs button{flex:1;background:none;border:0;color:var(--dim);font:inherit;font-size:11px;font-weight:700;padding:6px 0;border-radius:14px;cursor:pointer;display:flex;flex-direction:column;align-items:center;gap:2px;transition:.2s}
.tabs button span{font-size:21px;filter:grayscale(1);opacity:.7;transition:.2s}
.tabs button.on{color:var(--txt);background:var(--card2)}
.tabs button.on span{filter:none;opacity:1;transform:translateY(-1px) scale(1.1)}
/* sheet + toast */
.sheet{position:fixed;inset:0;background:rgba(0,0,0,.55);display:none;align-items:flex-end;justify-content:center;z-index:20}
.sheet.on{display:flex}
.sheet .card{width:100%;max-width:480px;margin:0;border-radius:26px 26px 0 0;padding:20px 18px calc(22px + env(safe-area-inset-bottom));animation:up .25s ease}
@keyframes up{from{transform:translateY(100%)}}
.sheet h3{margin:0 0 4px;font-size:18px}
.sheet .f{margin-top:12px}
.grab{width:40px;height:5px;border-radius:3px;background:var(--line);margin:0 auto 14px}
#toast{position:fixed;left:50%;bottom:calc(84px + env(safe-area-inset-bottom));transform:translateX(-50%) translateY(20px);opacity:0;background:var(--card2);border:1px solid var(--line);padding:10px 18px;border-radius:99px;transition:.25s;z-index:30;max-width:90vw;font-weight:600;font-size:14px;pointer-events:none;text-align:center}
#toast.on{opacity:1;transform:translateX(-50%) translateY(0)}
#toast.bad{border-color:var(--bad);color:var(--bad)}
/* developer */
.lock{text-align:center;padding:26px 18px}
.lock .em{font-size:44px}
.devbar{display:flex;gap:10px;align-items:center;margin-bottom:12px}
.devbar select{flex:1}
.grid{display:grid;grid-template-columns:repeat(auto-fill,minmax(130px,1fr));gap:8px}
.grid .chip{justify-content:center;padding:9px 8px;border-radius:12px}
.gh{grid-column:1/-1;color:var(--dim);font-size:12px;text-transform:uppercase;letter-spacing:.06em;margin-top:10px}
.ax{display:grid;grid-template-columns:20px 1fr 64px;align-items:center;gap:8px;font-size:13px;margin:8px 0}
.track{height:10px;background:var(--card2);border-radius:5px;position:relative;overflow:hidden}
.track b{position:absolute;top:0;bottom:0;background:linear-gradient(90deg,var(--acc),var(--pur));border-radius:5px}
.track:after{content:"";position:absolute;left:50%;top:0;bottom:0;width:1px;background:var(--line)}
.prog{height:12px;background:var(--card2);border-radius:6px;overflow:hidden;margin:12px 0 6px}
.prog b{display:block;height:100%;width:0;background:linear-gradient(90deg,var(--acc),var(--pur));transition:width .2s}
pre#log{background:#070912;border:1px solid var(--line);border-radius:16px;padding:12px;height:55vh;overflow:auto;font:11.5px/1.5 ui-monospace,Consolas,monospace;margin:0;white-space:pre-wrap;word-break:break-all}
.lt{color:#5a6390}
.flash{animation:fl 1s}
@keyframes fl{from{background:rgba(255,111,174,.35)}to{background:transparent}}
a{color:var(--acc)}
</style>
</head>
<body>
<div class="app">
  <header class="top">
    <div class="avatar"><i></i><i></i></div>
    <div class="t"><h1 id="hName">Cute Eyes</h1><div class="s" id="hSub">Saying hello...</div></div>
    <div class="dot" id="hDot" title="Wi-Fi"></div>
  </header>

  <!-- ===================== HOME ===================== -->
  <section id="home" class="on">
    <div class="card hero">
      <div class="face" onclick="nextExpr()"><i></i><i></i></div>
      <p class="greet" id="hGreet">Hello!</p>
      <div class="expr" id="hExpr">&nbsp;</div>
      <button class="big" onclick="nextExpr()">&#10024; Next expression</button>
    </div>
    <div class="duo">
      <div class="card"><div class="clock" id="hTime">--:--</div><div class="mute" id="hDate">&nbsp;</div></div>
      <div class="card"><div class="wx" id="hWxI">&#127777;&#65039;</div><div class="wxt" id="hWxT">--</div><div class="mute" id="hWxD">&nbsp;</div></div>
    </div>
    <details class="card" style="margin-top:12px">
      <summary><span class="ic">&#127780;&#65039;</span><span id="hLoc">Weather details</span></summary>
      <div class="in" id="hWxMore"></div>
    </details>
    <div class="card list">
      <div class="row"><span class="ic">&#128260;</span><div class="grow"><b>Auto-change</b><small>New expression every few seconds</small></div><label class="sw"><input type="checkbox" id="qAuto" onchange="setS({auto:this.checked})"><span></span></label></div>
      <div class="row"><span class="ic">&#128075;</span><div class="grow"><b>Shake to change</b><small>Give me a little shake!</small></div><label class="sw"><input type="checkbox" id="qShake" onchange="setS({shake:this.checked})"><span></span></label></div>
      <div class="row" style="display:block"><div class="lbl"><div style="display:flex;gap:12px;align-items:center"><span class="ic">&#9728;&#65039;</span><b>Brightness</b></div><span class="mute" id="qBv"></span></div>
        <input type="range" id="qBright" min="10" max="255" oninput="$('#qBv').textContent=pc(this.value)" onchange="setS({brightness:+this.value})"></div>
    </div>
    <div class="chips" id="hChips" style="justify-content:center"></div>
  </section>

  <!-- ===================== WI-FI ===================== -->
  <section id="wifi">
    <div class="card" style="text-align:center;padding:22px 16px" id="wNow"></div>
    <h4>Saved networks</h4>
    <div class="card list" id="wSaved"></div>
    <div class="btns" style="margin:0 0 12px">
      <button class="pbtn acc" id="scanBtn" onclick="scan()">&#128269; Find networks</button>
      <button class="pbtn" onclick="openNet(null,true)">&#65291; Hidden</button>
    </div>
    <div class="card list" id="wScan" style="display:none"></div>
    <p class="note">Only 2.4 GHz networks work &middot; up to 5 are remembered</p>
  </section>

  <!-- ===================== STYLE ===================== -->
  <section id="style">
    <h4>Greeting</h4>
    <div class="card">
      <div class="bubble" id="gPrev">Hello! &#128075;</div>
      <div class="lbl" style="margin-bottom:8px"><b>Your name<small>I'll greet you by name when I wake up</small></b></div>
      <div style="display:flex;gap:8px"><input type="text" id="cName" maxlength="16" placeholder="Your name"><button class="pbtn acc" onclick="setS({name:$('#cName').value})">Save</button></div>
      <div class="btns"><button class="pbtn" onclick="post('/api/greet',{})">&#128075; Say hello on the device</button></div>
      <p class="note" id="gHint" style="margin-bottom:0"></p>
    </div>
    <h4>Themes</h4>
    <p class="note" style="text-align:left;margin:-2px 4px 10px">Pick the styles you like. I'll switch between expressions from the themes that are on.</p>
    <div id="thList"></div>
  </section>

  <!-- ===================== SETTINGS ===================== -->
  <section id="settings">
    <h4>Display</h4>
    <div class="card list">
      <div class="row" style="display:block"><div class="lbl"><b>Brightness</b><span class="mute" id="sBv"></span></div>
        <input type="range" id="sBright" min="10" max="255" oninput="$('#sBv').textContent=pc(this.value)" onchange="setS({brightness:+this.value})"></div>
      <div class="row"><div class="grow"><b>Show expression name</b></div><label class="sw"><input type="checkbox" id="sLabels" onchange="setS({labels:this.checked})"><span></span></label></div>
      <div class="row"><div class="grow"><b>Clock appears after</b><small>when I'm left alone</small></div>
        <select class="sel" id="sClock" onchange="setS({clockAfter:+this.value})"><option value="15">15 seconds</option><option value="30">30 seconds</option><option value="60">1 minute</option><option value="120">2 minutes</option><option value="300">5 minutes</option><option value="600">10 minutes</option><option value="0">Never</option></select></div>
      <div class="row"><div class="grow"><b>Screen sleeps after</b><small>shake to wake me</small></div>
        <select class="sel" id="sTout" onchange="setS({screenTimeout:+this.value})"><option value="30">30 seconds</option><option value="60">1 minute</option><option value="120">2 minutes</option><option value="300">5 minutes</option><option value="600">10 minutes</option><option value="1800">30 minutes</option><option value="0">Never</option></select></div>
    </div>
    <h4>Shake</h4>
    <div class="card list">
      <div class="row"><div class="grow"><b>Shake to change</b></div><label class="sw"><input type="checkbox" id="sShake" onchange="setS({shake:this.checked})"><span></span></label></div>
      <div class="row"><div class="grow"><b>When shaken</b></div>
        <select class="sel" id="sMode" onchange="setS({shakeRandom:this.value==='1'})"><option value="0">Next expression</option><option value="1">Surprise me</option></select></div>
      <div class="row" style="display:block"><div class="lbl"><b>Sensitivity</b><span class="mute" id="sSv"></span></div>
        <input type="range" id="sSens" min="1" max="10" oninput="$('#sSv').textContent=sensTxt(this.value)" onchange="setS({sens:+this.value})"></div>
    </div>
    <h4>Location</h4>
    <div class="card">
      <div class="lbl" style="margin-bottom:8px"><b>Where am I?<small id="sLocNow">Detected automatically</small></b></div>
      <div style="display:flex;gap:8px"><input type="text" id="cCity" maxlength="40" placeholder="Automatic"><button class="pbtn acc" onclick="setS({city:$('#cCity').value})">Save</button></div>
      <p class="note" style="margin-bottom:0">Leave empty to detect it from your Wi-Fi. Used for weather, time and greeting.</p>
    </div>
    <h4>Software</h4>
    <div class="card">
      <div class="kv"><span>Version</span><span id="sVer">-</span></div>
      <div class="kv"><span>Status</span><span id="sOta">-</span></div>
      <div class="btns"><button class="pbtn" onclick="post('/api/ota/check',{})">&#128260; Check for updates</button></div>
    </div>
    <p class="note" id="sAbout"></p>
  </section>

  <!-- ===================== DEVELOPER ===================== -->
  <section id="dev">
    <div class="card lock" id="dLogin">
      <div class="em">&#128274;</div>
      <h3 style="margin:6px 0 4px">Developer area</h3>
      <p class="mute" style="margin:0 0 16px">Device details, logs, every expression and firmware tools.</p>
      <input type="text" id="dUser" placeholder="Username" autocomplete="username" style="margin-bottom:8px">
      <input type="password" id="dPass" placeholder="Password" autocomplete="current-password" onkeydown="if(event.key==='Enter')devLogin()">
      <div class="btns"><button class="pbtn acc" onclick="devLogin()">Unlock</button></div>
    </div>
    <div id="dArea" style="display:none">
      <div class="devbar">
        <select id="dPanel" onchange="devPanel(this.value)">
          <option value="info">&#128203; Device details</option>
          <option value="expr">&#128064; Expressions</option>
          <option value="sens">&#128243; Sensors</option>
          <option value="fw">&#128230; Firmware</option>
          <option value="logs">&#128220; Logs</option>
        </select>
        <button class="pbtn red" onclick="devLogout()">Lock</button>
      </div>
      <div class="dp" id="p-info">
        <div class="card"><b>Device</b><div id="dDev"></div></div>
        <div class="card"><b>Home Wi-Fi</b><div id="dSta"></div></div>
        <div class="card"><b>Access point</b><div id="dAp"></div></div>
        <div class="card"><b>Time, location &amp; weather</b><div id="dTw"></div></div>
        <div class="card"><b>Bluetooth &amp; display</b><div id="dMisc"></div></div>
      </div>
      <div class="dp" id="p-expr" style="display:none">
        <div class="card">
          <div class="lbl"><b>Any expression, any theme<small>Shows it right away, even if its theme is off</small></b><button class="pbtn" onclick="nextExpr()">Next</button></div>
          <div class="grid" id="dExGrid"></div>
        </div>
      </div>
      <div class="dp" id="p-sens" style="display:none">
        <div class="card"><b>Accelerometer</b><div id="dAx" style="margin-top:6px"></div><p class="mute" id="dShake" style="margin:8px 0 0"></p></div>
      </div>
      <div class="dp" id="p-fw" style="display:none">
        <div class="card">
          <b>Upload firmware</b>
          <p class="mute" style="margin:6px 0 10px">Arduino IDE: Sketch &rarr; Export Compiled Binary, then pick <b>ESP32S3_Cute_Eyes.ino.bin</b> (not merged / bootloader / partitions).</p>
          <input type="file" id="fFile" accept=".bin" style="width:100%;color:var(--dim)">
          <div class="btns"><button class="pbtn acc" id="fBtn" onclick="upload()">Upload &amp; install</button></div>
          <div class="prog" id="fProg" style="display:none"><b></b></div>
          <div class="mute" id="fMsg"></div>
        </div>
        <div class="card"><b>Cloud update</b><div id="dOta"></div><div class="btns"><button class="pbtn" onclick="post('/api/ota/check',{})">Check now</button></div></div>
        <div class="card"><b>Device</b><div class="btns"><button class="pbtn red" onclick="reboot()">Reboot device</button></div></div>
      </div>
      <div class="dp" id="p-logs" style="display:none">
        <div class="btns" style="margin:0 0 10px"><label class="pbtn" style="display:flex;gap:8px;align-items:center;justify-content:center"><input type="checkbox" id="lPause"> Pause</label><button class="pbtn" onclick="$('#log').innerHTML=''">Clear</button></div>
        <pre id="log"></pre>
      </div>
    </div>
  </section>
</div>

<nav class="tabs">
  <button data-t="home" class="on"><span>&#127968;</span>Home</button>
  <button data-t="wifi"><span>&#128246;</span>Wi-Fi</button>
  <button data-t="style"><span>&#127912;</span>Style</button>
  <button data-t="settings"><span>&#9881;&#65039;</span>Settings</button>
  <button data-t="dev"><span>&#128736;&#65039;</span>Developer</button>
</nav>

<div class="sheet" id="sheet" onclick="if(event.target===this)closeNet()"><div class="card">
  <div class="grab"></div>
  <h3 id="mTitle">Connect</h3>
  <div class="mute" id="mSub">Enter the Wi-Fi password</div>
  <div class="f" id="mSsidBox"><input type="text" id="mSsid" placeholder="Network name" autocomplete="off"></div>
  <div class="f" id="mPassBox"><input type="password" id="mPass" placeholder="Password" autocomplete="new-password" onkeydown="if(event.key==='Enter')saveNet()">
    <label class="mute" style="display:flex;gap:8px;align-items:center;margin-top:8px"><input type="checkbox" onchange="$('#mPass').type=this.checked?'text':'password'"> Show password</label></div>
  <div class="mute" id="mErr" style="color:var(--bad);margin-top:8px"></div>
  <div class="btns"><button class="pbtn" onclick="closeNet()">Cancel</button><button class="pbtn acc" onclick="saveNet()">Connect</button></div>
</div></div>

<div id="toast"></div>

<script>
const $=s=>document.querySelector(s),$$=s=>document.querySelectorAll(s);
const esc=s=>String(s??'').replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
let S=null,D=null,meta=null,saved=[],tok=sessionStorage.getItem('devTok')||'',busy=false,logSince=0,tab='home',panel='info',netName=null,netSecure=true;
const TH_EMOJI=['&#128578;','&#128540;','&#10024;','&#129302;','&#128172;'];
const TH_COL=['#46e1ff','#ff6fae','#a98bff','#4dffa0','#ffc24d'];
const pc=v=>Math.round(v/2.55)+'%';
const sensTxt=v=>(v<=3?'Strong shake':v>=8?'Gentle shake':'Normal')+' \u00b7 '+v;
const deg='\u00b0C';

async function api(p,b){
  const h={};if(tok)h['X-Dev-Token']=tok;
  const o={headers:h};if(b!==undefined){o.method='POST';h['Content-Type']='application/json';o.body=JSON.stringify(b)}
  const r=await fetch(p,o);let j={};try{j=await r.json()}catch(e){}
  if(r.status===401&&tok&&p!=='/api/dev/login'){tok='';sessionStorage.removeItem('devTok');renderDevLock()}
  if(!r.ok)throw new Error(j.msg||('Error '+r.status));return j;
}
async function post(p,b){try{const j=await api(p,b);if(j.msg)toast(j.msg);refresh();if(tab==='dev')devRefresh();return j}catch(e){toast(e.message,1)}}
let tt;function toast(m,bad){const t=$('#toast');t.textContent=m;t.className='on'+(bad?' bad':'');clearTimeout(tt);tt=setTimeout(()=>t.className='',2600)}
function setChk(id,v){const e=$(id);if(e&&document.activeElement!==e)e.checked=!!v}
function setVal(id,v){const e=$(id);if(e&&document.activeElement!==e)e.value=v}
async function setS(o){await post('/api/settings',o)}
function nextExpr(){post('/api/expr',{next:true})}

// ---------- tabs ----------
function go(t){
  tab=t;$$('.tabs button').forEach(b=>b.classList.toggle('on',b.dataset.t===t));
  $$('section').forEach(s=>s.classList.toggle('on',s.id===t));window.scrollTo(0,0);
  if(t==='wifi')loadSaved();if(t==='dev')renderDevLock();
}
$$('.tabs button').forEach(b=>b.onclick=()=>go(b.dataset.t));

// ---------- helpers ----------
function wxEmoji(c,day){if(c==null||c<0)return '&#127777;&#65039;';if(c<=1)return day?'&#9728;&#65039;':'&#127769;';if(c==2)return day?'&#9925;':'&#9729;&#65039;';if(c==3)return '&#9729;&#65039;';if(c==45||c==48)return '&#127787;&#65039;';if(c>=95)return '&#9928;&#65039;';if((c>=71&&c<=77)||c==85||c==86)return '&#10052;&#65039;';if(c>=51)return '&#127783;&#65039;';return '&#127777;&#65039;'}
function bars(r){const n=r>=-55?4:r>=-67?3:r>=-78?2:1;return '<span class="bars">'+[5,8,11,14].map((h,i)=>`<i style="height:${h}px" class="${i<n?'on':''}"></i>`).join('')+'</span>'}
const kv=(k,v)=>`<div class="kv"><span>${k}</span><span>${v}</span></div>`;
const up=s=>{s=+s;const d=Math.floor(s/86400),h=Math.floor(s%86400/3600),m=Math.floor(s%3600/60);return (d?d+'d ':'')+(h?h+'h ':'')+m+'m '+(s%60)+'s'};
const kb=b=>(b/1024).toFixed(0)+' KB';
const wifiTxt={connected:'Connected',connecting:'Connecting...',disconnected:'Not connected',no_networks:'No Wi-Fi saved'};
const otaTxt={idle:'Up to date?',checking:'Checking...',up_to_date:'Up to date',downloading:'Updating...',success:'Updated!',failed:'Could not check'};
function greetHTML(){const n=S.settings.name;return esc(S.weather.greet)+(n?', <b>'+esc(n)+'</b>!':'!')}

// ---------- everyday screens ----------
async function refresh(){
  if(busy)return;
  try{S=await api('/api/status')}catch(e){$('#hSub').textContent='Reconnecting...';$('#hDot').className='dot warn';return}
  if(!meta||meta.names.length!==S.eyes.count){try{meta=await api('/api/expressions')}catch(e){}}
  const st=S.sta,w=S.weather,c=S.clock,se=S.settings;
  document.documentElement.style.setProperty('--fc',TH_COL[S.eyes.theme]||TH_COL[0]);
  $('#hName').textContent=S.name;
  $('#hSub').innerHTML=greetHTML().replace(/<[^>]+>/g,'')+' \u{1F44B}';
  $('#hDot').className='dot '+(st.state==='connected'?'ok':st.state==='connecting'?'warn':'');

  // home
  $('#hGreet').innerHTML=greetHTML();
  $('#hExpr').innerHTML='Feeling <b>'+esc(S.eyes.name)+'</b>'+(meta?`<span class="tag">${TH_EMOJI[S.eyes.theme]} ${esc(meta.themeNames[S.eyes.theme])}</span>`:'');
  $('#hTime').textContent=c.valid?c.time:'--:--';
  $('#hDate').textContent=c.valid?c.date:'Needs Wi-Fi';
  $('#hWxI').innerHTML=w.have?wxEmoji(w.code,w.isDay):'&#127777;&#65039;';
  $('#hWxT').textContent=w.have?Math.round(w.temp)+deg:'--';
  $('#hWxD').textContent=w.have?w.text:(st.state==='connected'?'Loading...':'Needs Wi-Fi');
  $('#hLoc').textContent=w.haveLoc?w.loc:'Weather details';
  $('#hWxMore').innerHTML=w.have?kv('Feels like',w.feels.toFixed(0)+deg)+kv('Today',Math.round(w.tmin)+' / '+Math.round(w.tmax)+deg)+kv('Humidity',w.hum.toFixed(0)+'%')+kv('Wind',w.wind.toFixed(0)+' km/h')+kv('Sunrise',w.sunrise)+kv('Sunset',w.sunset):'<p class="mute">Connect me to Wi-Fi to see the weather.</p>';
  setChk('#qAuto',S.eyes.auto);setChk('#qShake',se.shake);setVal('#qBright',se.brightness);$('#qBv').textContent=pc(se.brightness);
  const scr={eyes:'&#128064; Awake',clock:'&#128338; Clock',off:'&#128564; Sleeping'}[c.screen]||'';
  $('#hChips').innerHTML=`<span class="chip">${st.state==='connected'?'&#128246; '+esc(st.ssid):'&#128246; '+wifiTxt[st.state]}</span><span class="chip">${S.ble.connected?(S.ble.paired?'&#128241; Phone paired':'&#128241; Pairing...'):'&#128241; Bluetooth ready'}</span><span class="chip">${scr}</span>`;

  // wi-fi
  $('#wNow').innerHTML=st.state==='connected'
    ?`<div style="font-size:40px">&#128246;</div><div style="font-size:18px;font-weight:800;margin-top:6px">${esc(st.ssid)}</div><div class="mute">Connected ${bars(st.rssi)}</div>`
    :`<div style="font-size:40px">${st.state==='connecting'?'&#9203;':'&#128268;'}</div><div style="font-size:18px;font-weight:800;margin-top:6px">${wifiTxt[st.state]||st.state}</div><div class="mute">${st.state==='connecting'?esc(st.ssid):'Find your network below'}</div>`;
  if(tab==='wifi')renderSaved();

  // style
  $('#gPrev').innerHTML=greetHTML()+' &#128075;';
  setVal('#cName',se.name);
  $('#gHint').textContent=w.haveLoc?'Greeting picked for '+w.loc:'Location unknown yet - I\'ll say Hello';
  renderThemes();

  // settings
  setVal('#sBright',se.brightness);$('#sBv').textContent=pc(se.brightness);
  setChk('#sLabels',se.labels);setVal('#sClock',String(se.clockAfter));setVal('#sTout',String(se.screenTimeout));
  setChk('#sShake',se.shake);setVal('#sMode',se.shakeRandom?'1':'0');setVal('#sSens',se.sens);$('#sSv').textContent=sensTxt(se.sens);
  setVal('#cCity',se.city);$('#sLocNow').textContent=w.haveLoc?(w.manual?'Set to ':'Detected: ')+w.loc:'Not known yet';
  $('#sVer').textContent='v'+S.fw;$('#sOta').textContent=(otaTxt[S.ota.state]||S.ota.state)+(S.ota.latest?' (latest v'+S.ota.latest+')':'');
  $('#sAbout').textContent=S.name+' \u00b7 v'+S.fw;
  if(S.devOpen&&!tok)tok='open';
}

function renderThemes(){
  if(!meta)return;
  const mask=S.settings.themes;
  $('#thList').innerHTML=meta.themeNames.map((tn,ti)=>{
    const on=!!(mask&(1<<ti)),it=meta.names.filter((n,i)=>meta.themes[i]===ti);
    return `<div class="card thcard ${on?'on':''}" style="--tc:${TH_COL[ti]}">
      <div class="theme"><span class="ic" style="background:${on?TH_COL[ti]+'33':''}">${TH_EMOJI[ti]}</span>
        <div class="grow"><b>${esc(tn)}</b><small>${esc(meta.themeDesc[ti])}</small></div>
        <label class="sw"><input type="checkbox" ${on?'checked':''} onchange="toggleTheme(${ti},this.checked)"><span></span></label></div>
      <details><summary>${it.length} expressions</summary><div class="chips">${it.map(n=>`<span class="chip">${esc(n)}</span>`).join('')}</div></details></div>`}).join('');
}
function toggleTheme(ti,on){
  let m=S.settings.themes;m=on?(m|(1<<ti)):(m&~(1<<ti));
  if(!m){toast('Keep at least one theme on',1);renderThemes();return}
  S.settings.themes=m;setS({themes:m});
}

// ---------- wi-fi ----------
async function loadSaved(){try{saved=(await api('/api/wifi')).networks;renderSaved()}catch(e){}}
function renderSaved(){
  const cur=S&&S.sta.ssid,state=S&&S.sta.state;
  $('#wSaved').innerHTML=saved.length?saved.map(n=>{
    const here=n===cur,a=esc(JSON.stringify(n));
    const sub=here?(state==='connected'?'<small style="color:var(--ok)">Connected</small>':state==='connecting'?'<small style="color:var(--warn)">Connecting...</small>':'<small>Saved</small>'):'<small>Saved</small>';
    return `<div class="row"><span class="ic">${here&&state==='connected'?'&#9989;':'&#128246;'}</span><div class="grow"><b>${esc(n)}</b>${sub}</div>
      <select class="more" onchange="netAct(this.value,${a});this.value=''"><option value="" selected hidden>&#8943;</option>${here&&state==='connected'?'':'<option value="c">Connect now</option>'}<option value="p">Change password</option><option value="f">Forget</option></select></div>`}).join('')
    :'<div class="row"><span class="ic">&#128566;</span><div class="grow"><b>Nothing saved yet</b><small>Find your network below</small></div></div>';
}
function netAct(v,n){if(v==='c')post('/api/wifi/connect',{ssid:n});else if(v==='p')openNet(n,true);else if(v==='f')forget(n)}
async function forget(n){if(!confirm('Forget "'+n+'"?'))return;await post('/api/wifi/forget',{ssid:n});loadSaved()}
async function scan(){
  const b=$('#scanBtn'),box=$('#wScan');b.disabled=true;b.innerHTML='&#9203; Looking...';
  try{await api('/api/scan',{});let r;for(let i=0;i<20;i++){await new Promise(z=>setTimeout(z,1000));r=await api('/api/scan');if(!r.scanning)break}
    const L=r.networks||[];box.style.display='block';
    box.innerHTML=L.length?L.map(n=>{const a=esc(JSON.stringify(n.ssid));return `<div class="row tap" onclick="openNet(${a},${n.secure})"><span class="ic">${bars(n.rssi)}</span><div class="grow"><b>${esc(n.ssid)}</b><small>${saved.includes(n.ssid)?'Saved \u00b7 ':''}${n.secure?'&#128274; Secured':'Open'}</small></div><span style="color:var(--dim);font-size:20px">&#8250;</span></div>`}).join(''):'<div class="row"><div class="grow"><b>No networks found</b><small>Try again in a moment</small></div></div>';
  }catch(e){toast(e.message,1)}
  b.disabled=false;b.innerHTML='&#128269; Find networks';
}
function openNet(name,secure){
  netName=name;netSecure=secure;
  $('#mTitle').textContent=name||'Hidden network';
  $('#mSub').textContent=name?(secure?'Enter the Wi-Fi password':'Open network - no password needed'):'Type the network name and password';
  $('#mSsidBox').style.display=name?'none':'block';$('#mPassBox').style.display=secure?'block':'none';
  $('#mSsid').value='';$('#mPass').value='';$('#mErr').textContent='';
  $('#sheet').classList.add('on');setTimeout(()=>(name?(secure?$('#mPass'):null):$('#mSsid'))?.focus(),80);
}
function closeNet(){$('#sheet').classList.remove('on')}
async function saveNet(){
  const ssid=netName||$('#mSsid').value,pass=netSecure?$('#mPass').value:'';
  if(!ssid.trim()||ssid.length>32)return $('#mErr').textContent='Network name must be 1-32 characters';
  if(pass&&(pass.length<8||pass.length>63))return $('#mErr').textContent='Wi-Fi passwords are 8-63 characters';
  closeNet();await post('/api/wifi/add',{ssid,pass});loadSaved();
  setTimeout(()=>toast('Saved! If this page stops, rejoin my Wi-Fi'),2700);
}

// ---------- developer ----------
function renderDevLock(){
  const open=!!tok;$('#dLogin').style.display=open?'none':'block';$('#dArea').style.display=open?'block':'none';
  if(open)devRefresh();
}
async function devLogin(){
  try{const j=await api('/api/dev/login',{user:$('#dUser').value,pass:$('#dPass').value});
    tok=j.token;sessionStorage.setItem('devTok',tok);$('#dPass').value='';toast('Developer unlocked');renderDevLock();
  }catch(e){toast(e.message,1)}
}
async function devLogout(){try{await api('/api/dev/logout',{})}catch(e){}tok='';sessionStorage.removeItem('devTok');renderDevLock();toast('Locked')}
function devPanel(p){panel=p;$$('.dp').forEach(d=>d.style.display=d.id==='p-'+p?'block':'none');devRefresh();if(p==='logs')pollLog()}
async function devRefresh(){
  if(!tok||busy)return;
  try{D=await api('/api/dev/status')}catch(e){return}
  const st=D.sta,ap=D.ap,c=D.clock,w=D.weather;
  $('#dDev').innerHTML=kv('Name',esc(D.name))+kv('Firmware','v'+D.fw)+kv('Uptime',up(D.uptime))+kv('Free heap',kb(D.heap)+' (min '+kb(D.minHeap)+')')+kv('Free PSRAM',kb(D.psram))+kv('Chip temp',D.temp.toFixed(1)+' '+deg)+kv('Last reset',esc(D.reset))+kv('Core / IDF',esc(D.sdk));
  $('#dSta').innerHTML=kv('State',wifiTxt[st.state]||st.state)+kv('Network',esc(st.ssid)||'-')+kv('IP address',st.ip?`<a href="http://${st.ip}/">${st.ip}</a>`:'-')+kv('Signal',st.ip?st.rssi+' dBm '+bars(st.rssi):'-')+kv('Local name',`<a href="http://${D.mdns}/">${D.mdns}</a>`)+kv('MAC',st.mac);
  $('#dAp').innerHTML=kv('Name',esc(ap.ssid))+kv('Address',ap.ip)+kv('Clients',ap.clients)+kv('Channel',ap.channel);
  $('#dTw').innerHTML=kv('Time',c.valid?c.time:'not synced')+kv('Date',c.valid?c.date:'-')+kv('Time zone',c.tz)+kv('Location',w.haveLoc?esc(w.loc)+(w.manual?' (manual)':' (auto)'):'unknown')+kv('Greeting',esc(w.greet))+(w.have?kv('Weather',w.temp.toFixed(1)+' '+deg+', '+esc(w.text))+kv('Updated',w.age+' s ago'):'')+(w.err?kv('Last error',esc(w.err)):'');
  $('#dMisc').innerHTML=kv('Bluetooth',D.ble.connected?(D.ble.paired?'Phone paired':'Phone connected (PIN on screen)'):'Advertising')+kv('Screen',c.screen)+kv('Expression',esc(D.eyes.name)+' (#'+(D.eyes.index+1)+')')+kv('Auto-cycle',D.eyes.auto?'on':'off')+kv('Themes mask','0x'+D.settings.themes.toString(16));
  if(meta){
    const mask=D.settings.themes;
    $('#dExGrid').innerHTML=meta.themeNames.map((tn,ti)=>{const on=mask&(1<<ti);
      return `<div class="gh">${TH_EMOJI[ti]} ${esc(tn)}${on?'':' \u00b7 off'}</div>`+meta.names.map((n,i)=>[n,i]).filter(x=>meta.themes[x[1]]===ti)
        .map(([n,i])=>`<button class="chip ${i===D.eyes.index?'on':''} ${on?'':'dim'}" onclick="post('/api/expr',{index:${i}})">${esc(n)}</button>`).join('')}).join('');
  }
  if(D.imu.ok){
    const ax=(l,v)=>{const cl=Math.max(-2,Math.min(2,v)),wd=Math.abs(cl)/4*100,left=cl<0?50-wd:50;return `<div class="ax"><b>${l}</b><div class="track"><b style="left:${left}%;width:${wd}%"></b></div><span>${v.toFixed(2)} g</span></div>`};
    $('#dAx').innerHTML=ax('X',D.imu.ax)+ax('Y',D.imu.ay)+ax('Z',D.imu.az)+kv('|a|',D.imu.g.toFixed(2)+' g (about 1.00 g when still)');
    const si=$('#dShake'),old=si.dataset.n;si.textContent='Shakes detected: '+D.imu.shakes+' \u00b7 sensitivity '+D.settings.sens+'/10';
    if(old!==undefined&&+old!==D.imu.shakes){si.classList.remove('flash');void si.offsetWidth;si.classList.add('flash')}si.dataset.n=D.imu.shakes;
  }else $('#dAx').innerHTML='<p class="mute">Accelerometer not detected (QMI8658 on SDA 6 / SCL 7).</p>';
  $('#dOta').innerHTML=kv('Status',otaTxt[D.ota.state]||D.ota.state)+kv('Latest online',D.ota.latest?'v'+esc(D.ota.latest):'-')+kv('Message',esc(D.ota.msg)||'-');
}
function upload(){
  const f=$('#fFile').files[0],msg=$('#fMsg'),pg=$('#fProg'),bar=pg.firstElementChild;
  if(!f)return toast('Choose a .bin file first',1);
  if(/merged|bootloader|partitions/i.test(f.name))return toast('Use the plain .ino.bin file',1);
  if(!confirm('Install '+f.name+' ('+Math.round(f.size/1024)+' KB)?'))return;
  busy=true;$('#fBtn').disabled=true;pg.style.display='block';bar.style.width='0';msg.textContent='Uploading...';
  const fd=new FormData();fd.append('firmware',f,f.name);
  const x=new XMLHttpRequest();x.open('POST','/api/update?size='+f.size);if(tok)x.setRequestHeader('X-Dev-Token',tok);
  x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded/e.total*100);bar.style.width=p+'%';msg.textContent='Uploading and flashing... '+p+'%'}};
  x.onload=()=>{let j={};try{j=JSON.parse(x.responseText)}catch(e){}
    if(x.status===200){msg.textContent='Installed! Rebooting - this page reloads by itself.';waitBack()}
    else{msg.textContent='Failed: '+(j.msg||x.status);busy=false;$('#fBtn').disabled=false}};
  x.onerror=()=>{msg.textContent='Upload failed (connection lost).';busy=false;$('#fBtn').disabled=false};
  x.send(fd);
}
async function reboot(){if(!confirm('Reboot the device?'))return;await post('/api/reboot',{});busy=true;waitBack()}
function waitBack(){setTimeout(async function t(){try{await api('/api/status');location.reload()}catch(e){setTimeout(t,2000)}},4000)}
async function pollLog(){
  if(busy||!tok||$('#lPause').checked)return;
  try{const r=await api('/api/log?since='+logSince);if(!r.lines.length)return;
    const el=$('#log'),stick=el.scrollTop+el.clientHeight>=el.scrollHeight-20;
    if(r.first>logSince+1&&logSince)el.insertAdjacentHTML('beforeend','<span class="lt">... (older lines dropped)</span>\n');
    el.insertAdjacentHTML('beforeend',r.lines.map(l=>`<span class="lt">[${(l.t/1000).toFixed(1)}]</span> ${esc(l.m)}`).join('\n')+'\n');
    logSince=r.last;if(stick)el.scrollTop=el.scrollHeight;
  }catch(e){}
}

refresh().then(loadSaved);
setInterval(refresh,2000);
setInterval(()=>{if(tab==='dev'&&tok&&panel!=='logs')devRefresh()},1500);
setInterval(()=>{if(tab==='dev'&&panel==='logs')pollLog()},1000);
setInterval(()=>{if(S&&tab==='wifi')renderSaved()},3000);
</script>
</body>
</html>
)HTML";
