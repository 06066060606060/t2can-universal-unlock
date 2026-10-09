const char INDEX_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="ko">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#151515">
<title>TESLA UNLOCK</title>
<style>
:root{--bg:#f2f2f0;--card:#fff;--ink:#171717;--muted:#777;--line:#dededb;--green:#2b8a55;--amber:#b27a19;--red:#c9403a;--shadow:0 8px 30px rgba(0,0,0,.07)}
@media(prefers-color-scheme:dark){:root{--bg:#151515;--card:#202020;--ink:#f2f2f2;--muted:#999;--line:#343434;--shadow:none}}
*{box-sizing:border-box;-webkit-tap-highlight-color:transparent}html,body{margin:0;background:var(--bg);color:var(--ink);font-family:-apple-system,BlinkMacSystemFont,"Helvetica Neue",Arial,sans-serif}button,input{font:inherit}button{cursor:pointer}.wrap{max-width:520px;margin:auto;min-height:100vh;padding:calc(12px + env(safe-area-inset-top)) 14px calc(26px + env(safe-area-inset-bottom))}.top{height:48px;display:flex;align-items:center;justify-content:space-between}.logo{font-size:15px;font-weight:700;letter-spacing:-.02em}.live{display:flex;align-items:center;gap:7px;font-size:11px;color:var(--muted)}.live:before{content:"";width:7px;height:7px;border-radius:50%;background:var(--red)}.live.ok:before{background:var(--green)}
.drive{padding:24px 8px 18px;text-align:center}.small{font-size:10px;color:var(--muted);font-weight:650;letter-spacing:.06em}.mode{font-size:46px;line-height:1.03;font-weight:510;letter-spacing:-.055em;margin:8px 0 20px}.mode.good{color:var(--green)}.mode.warn{color:var(--amber)}.mode.bad{color:var(--red)}.mode.compact{font-size:30px;letter-spacing:-.04em}
.metrics{display:grid;grid-template-columns:repeat(2,1fr);gap:1px;background:var(--line);border:1px solid var(--line);border-radius:18px;overflow:hidden;box-shadow:var(--shadow)}.metric{background:var(--card);padding:14px 7px;text-align:center}.metric .k{font-size:9px;color:var(--muted);font-weight:650}.metric .v{font-size:17px;margin-top:5px;font-weight:580;letter-spacing:-.03em}.good{color:var(--green)!important}.warn{color:var(--amber)!important}.bad{color:var(--red)!important}
.quick{margin-top:14px;background:var(--card);border-radius:20px;overflow:hidden;border:1px solid var(--line);box-shadow:var(--shadow)}.qrow{min-height:60px;padding:0 16px;display:flex;align-items:center;justify-content:space-between;gap:10px}.qrow+.qrow{border-top:1px solid var(--line)}.ql{display:flex;align-items:center;gap:12px;min-width:0}.ico{width:28px;height:28px;border-radius:50%;background:var(--bg);display:grid;place-items:center;font-size:12px;font-weight:700;flex:0 0 auto}.qt{font-size:14px;font-weight:560}.qs{font-size:10px;color:var(--muted);margin-top:2px;white-space:nowrap}.rightctl{display:flex;align-items:center;gap:9px;min-width:0}.prio{font-size:9.5px;font-weight:680;color:var(--green);white-space:nowrap;text-align:right}.prio b{display:block;font-size:7.5px;color:var(--muted);font-weight:620;margin-bottom:2px;letter-spacing:.05em}.toggle{position:relative;width:43px;height:25px;flex:0 0 auto}.toggle input{display:none}.track{position:absolute;inset:0;border-radius:99px;background:#b8b8b3;transition:.18s}.track:after{content:"";position:absolute;width:19px;height:19px;left:3px;top:3px;border-radius:50%;background:#fff;transition:.18s;box-shadow:0 1px 3px rgba(0,0,0,.25)}.toggle input:checked+.track{background:var(--green)}.toggle input:checked+.track:after{transform:translateX(18px)}
.gatecard{margin-top:14px;background:var(--card);border:1px solid var(--line);border-radius:20px;padding:15px;box-shadow:var(--shadow)}.gatehead{display:flex;align-items:center;justify-content:space-between;gap:12px}.gatetitle{font-size:13px;font-weight:590}.gatepill{border:1px solid var(--green);color:var(--green);border-radius:999px;padding:7px 11px;font-size:11px;font-weight:700;white-space:nowrap}.gatepill.closed{border-color:var(--muted);color:var(--muted)}.gategrid{display:grid;grid-template-columns:repeat(4,1fr);gap:1px;background:var(--line);border:1px solid var(--line);border-radius:14px;overflow:hidden;margin-top:12px}.gateitem{background:var(--card);padding:11px 5px;text-align:center}.gk{font-size:8px;color:var(--muted);font-weight:650;letter-spacing:.04em}.gv{font-size:11px;margin-top:4px;font-weight:650;white-space:nowrap}
.canline{margin-top:14px;display:grid;grid-template-columns:1fr 1fr;gap:8px}.can{background:var(--card);border:1px solid var(--line);border-radius:18px;padding:15px;box-shadow:var(--shadow)}.canTop{display:flex;align-items:center;justify-content:space-between}.canName{font-size:12px;font-weight:590}.badge{font-size:9px;color:var(--green);font-weight:700}.canVal{font-size:22px;margin-top:10px;font-weight:520;letter-spacing:-.04em}.canVal.online{color:var(--green)}.canVal.offline{color:var(--red)}.online{color:var(--green)}.offline{color:var(--red)}.canSub{font-size:9px;color:var(--muted);margin-top:4px}
.drawer{margin-top:14px;background:var(--card);border:1px solid var(--line);border-radius:20px;overflow:hidden;box-shadow:var(--shadow)}details+details{border-top:1px solid var(--line)}summary{list-style:none;cursor:pointer;padding:17px 16px;display:flex;justify-content:space-between;align-items:center;font-size:13px;font-weight:570}summary::-webkit-details-marker{display:none}.arrow{font-size:15px;color:var(--muted);transition:.18s}details[open] .arrow{transform:rotate(90deg)}.body{padding:0 16px 15px}.r{display:flex;justify-content:space-between;gap:16px;padding:10px 0;font-size:11px;border-top:1px solid var(--line)}.rk{color:var(--muted)}.rv{text-align:right;font-variant-numeric:tabular-nums;overflow-wrap:anywhere}.subhead{font-size:9px;color:var(--muted);font-weight:700;letter-spacing:.08em;margin:13px 0 7px}.controlgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px;padding-top:6px;padding-bottom:6px}.btn{border:0;border-radius:13px;padding:12px;background:var(--bg);color:var(--ink);font-size:11px;font-weight:600}.btn.primary{background:var(--ink);color:var(--bg)}.btn.danger{color:var(--red)}.fieldgrid{display:grid;grid-template-columns:1fr 1fr;gap:8px}.field{background:var(--bg);border-radius:13px;padding:9px 10px}.field label{display:block;font-size:8px;color:var(--muted);font-weight:650;margin-bottom:5px}.field input{width:100%;border:0;outline:0;background:transparent;color:var(--ink);font-size:13px}.field.full{grid-column:1/-1}.ota{margin-top:8px}.ota input[type=file]{width:100%;font-size:10px;color:var(--muted);margin:6px 0 8px}.progress{height:4px;background:var(--line);border-radius:99px;overflow:hidden;display:none}.progress.show{display:block}.progress>i{display:block;height:100%;width:0;background:var(--green)}.otaMsg{font-size:9px;color:var(--muted);margin-top:7px}.linkbtn{display:flex;align-items:center;justify-content:center;text-decoration:none}
.confirmOverlay{position:fixed;inset:0;background:rgba(0,0,0,.52);display:none;align-items:center;justify-content:center;padding:20px;z-index:100}
.confirmOverlay.show{display:flex}
.confirmBox{width:min(100%,390px);background:var(--card);color:var(--ink);border:1px solid var(--line);border-radius:20px;padding:22px;box-shadow:0 16px 50px rgba(0,0,0,.25)}
.confirmTitle{font-size:16px;font-weight:700;margin-bottom:10px}
.confirmText{font-size:13px;line-height:1.45;color:var(--red);font-weight:650}
.confirmActions{display:grid;grid-template-columns:1fr 1fr;gap:9px;margin-top:20px}
.confirmActions .btn{padding:12px}
.toast{position:fixed;left:50%;bottom:calc(24px + env(safe-area-inset-bottom));transform:translate(-50%,20px);background:#111;color:#fff;padding:10px 14px;border-radius:999px;font-size:11px;opacity:0;pointer-events:none;transition:.2s;z-index:20}.toast.show{opacity:.94;transform:translate(-50%,0)}.bottom{text-align:center;margin-top:18px;font-size:9px;color:var(--muted)}
@media(max-width:380px){.mode{font-size:40px}.mode.compact{font-size:27px}.qrow{padding:0 13px}.qs{max-width:140px;overflow:hidden;text-overflow:ellipsis}.prio{font-size:8.5px}.gategrid{grid-template-columns:1fr 1fr}.canline{grid-template-columns:1fr}}
</style>
</head>
<body>
<div class="wrap">
  <div class="top"><div class="logo">TESLA UNLOCK</div><div class="live" id="conn">Connecting</div></div>

  <section class="drive">
    <div class="small">AUTOPILOT</div>
    <div class="mode" id="apMode">—</div>
    <div class="metrics">
      <div class="metric"><div class="k">BLINKER</div><div class="v" id="blinkerState">—</div></div>
      <div class="metric"><div class="k">BUTTON</div><div class="v" id="buttonState">—</div></div>
    </div>
  </section>

  <section class="quick">
    <div class="qrow"><div class="ql"><div class="ico">↔</div><div><div class="qt">Auto Blinker</div></div></div><label class="toggle"><input id="blinkToggle" type="checkbox"><span class="track"></span></label></div>
    <div class="qrow"><div class="ql"><div class="ico">S</div><div><div class="qt">EU Unlock</div><div class="qs">Summon TX Priority</div></div></div><div class="rightctl"><div class="prio"><b>PRIORITY</b><span id="priorityState">—</span></div><label class="toggle"><input id="summonToggle" type="checkbox"><span class="track"></span></label></div></div>
    <div class="qrow"><div class="ql"><div class="ico">T</div><div><div class="qt">TLSSC</div><div class="qs">AP active only</div></div></div><label class="toggle"><input id="tlsscToggle" type="checkbox"><span class="track"></span></label></div>
    <div class="qrow"><div class="ql"><div class="ico">N</div><div><div class="qt">Nag Killer</div><div class="qs">torque echo</div></div></div><label class="toggle"><input id="nagQuickToggle" type="checkbox"><span class="track"></span></label></div>
  </section>

  <section class="gatecard">
    <div class="gatehead"><div class="gatetitle">SUMMON GATE</div><div class="gatepill closed" id="gatePill">CLOSED</div></div>
    <div class="gategrid">
      <div class="gateitem"><div class="gk">PARKED</div><div class="gv" id="gPark">OFF</div></div>
      <div class="gateitem"><div class="gk">SUMMONING</div><div class="gv" id="gSummon">OFF</div></div>
      <div class="gateitem"><div class="gk">ACA</div><div class="gv" id="gAca">INACTIVE</div></div>
      <div class="gateitem"><div class="gk">SPR</div><div class="gv" id="gSpr">NOT SEEN</div></div>
    </div>
  </section>

  <div class="canline">
    <section class="can"><div class="canTop"><div class="canName">J2 · BODY</div><div class="badge" id="canABadge">—</div></div><div class="canVal" id="canAStatus">—</div><div class="canSub">CAN A · MCP2515</div></section>
    <section class="can"><div class="canTop"><div class="canName">J3 · CHASSIS</div><div class="badge" id="canBBadge">—</div></div><div class="canVal" id="canBStatus">—</div><div class="canSub">CAN B · MCP2515</div></section>
    <section class="can"><div class="canTop"><div class="canName">J4 · PARTY</div><div class="badge" id="canCBadge">—</div></div><div class="canVal" id="canCStatus">—</div><div class="canSub">CAN C · MCP2515</div></section>
  </div>

  <section class="drawer">
    <details>
      <summary><span>Live details</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="r"><span class="rk">HandsOn real</span><span class="rv" id="liveHo">—</span></div>
        <div class="r"><span class="rk">Last injected</span><span class="rv" id="liveInj">—</span></div>
                <div class="r"><span class="rk">Summon TX OK / FAIL</span><span class="rv" id="liveSumTx">—</span></div>
      </div>
    </details>

    <details>
      <summary><span>Nag Killer</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="r"><span class="rk">Status</span><span class="rv" id="nagStatus">—</span></div>
        <div class="r"><span class="rk">J4 frames / target 0x370 RX</span><span class="rv" id="nagRx">—</span></div>
        <div class="r"><span class="rk">J4 warm-up</span><span class="rv" id="nagWarmup">—</span></div>
        <div class="r"><span class="rk">Torque real / injected</span><span class="rv" id="nagTorque">—</span></div>
        <div class="r"><span class="rk">Echo TX OK / FAIL</span><span class="rv" id="nagTx">—</span></div>
        <div class="subhead">MODE</div>
        <div class="controlgrid"><button class="btn" id="nagModeA">A</button><button class="btn" id="nagModeB">B</button><button class="btn" id="nagModeC">C</button><button class="btn" id="nagToggle">Enable</button></div>
        <div class="fieldgrid"><div class="field"><label>TARGET CAN ID</label><input id="nagTarget" value="0x370"></div><div class="field"><label>HANDS-ON RATE %</label><input id="nagRate" type="number" min="0" max="100"></div><div class="field"><label>BURST MS</label><input id="nagBurst" type="number" min="50" max="10000"></div><div class="field"><label>PAUSE MS</label><input id="nagPause" type="number" min="0" max="10000"></div></div>
        <button class="btn primary" id="nagApply" style="width:100%;margin-top:8px">Apply configuration</button>
      </div>
    </details>



    <details>
      <summary><span>Auto Blinker</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="subhead">AUTO BLINKER · STATES 3 / 4 / 5</div>
        <div class="fieldgrid">
          <div class="field"><label>DELAY BEFORE TURN TRIGGER (S)</label><input id="blinkTriggerDelay" type="number" min="0" max="30" step="0.1" inputmode="decimal"></div>
          <div class="field"><label>AP / NOA ACTIVATION DELAY (S)</label><input id="blinkApEntryDelay" type="number" min="0" max="30" step="0.1" inputmode="decimal"></div>
          <div class="field"><label>AFTER LANE CHANGE DELAY (S)</label><input id="blinkLaneChangeDelay" type="number" min="0" max="60" step="0.1" inputmode="decimal"></div>
        </div>
        <button class="btn primary" id="blinkApply" style="width:100%;margin-top:8px">Apply Delays</button>
        <div class="r"><span class="rk">Gate</span><span class="rv" id="blinkGate">—</span></div>
        <div class="r"><span class="rk">AP / NOA delay remaining</span><span class="rv" id="blinkApEntryRemain">—</span></div>
        <div class="r"><span class="rk">Lane change delay remaining</span><span class="rv" id="blinkLaneChangeRemain">—</span></div>
        <div class="r"><span class="rk">Auto armed</span><span class="rv" id="blinkArmed">—</span></div>
        <div class="r"><span class="rk">Countdown</span><span class="rv" id="blinkRemain">—</span></div>
        <div class="r"><span class="rk">Behavior</span><span class="rv" id="blinkBehavior">—</span></div>
        <div class="r"><span class="rk">Active turn</span><span class="rv" id="blinkTurn">—</span></div>
        <div class="subhead">TURN SIGNAL TYPE</div>
        <div class="controlgrid" style="padding-top:0">
          <button class="btn" id="blinkTransportStalk">STALK · 0x249</button>
          <button class="btn" id="blinkTransportStalkless">STALKLESS · 0x3C2</button>
        </div>
        <div class="subhead" style="margin-top:8px">Turn Signal Type</div>
        <div class="r"><span class="rk">Selected type</span><span class="rv" id="blinkTransportModeState">STALK · 0x249</span></div>
        <div class="subhead">STALK TRANSPORT</div>
        <div class="r"><span class="rk">Active transport</span><span class="rv" id="blinkTransport">—</span></div>
        <div class="r"><span class="rk">0x249 RX</span><span class="rv" id="live249">—</span></div>
        <div class="r"><span class="rk">0x3C2 RX</span><span class="rv" id="blink3c2">—</span></div>
        <div class="r"><span class="rk">Stalkless left</span><span class="rv" id="stalklessLeft">—</span></div>
        <div class="r"><span class="rk">Stalkless right</span><span class="rv" id="stalklessRight">—</span></div>
        <div class="r"><span class="rk">Stalkless TX OK / FAIL</span><span class="rv" id="stalklessTx">—</span></div>
        <div class="r"><span class="rk">0x249 checksum</span><span class="rv" id="blinkCk">—</span></div>
        <div class="r"><span class="rk">0x249 raw</span><span class="rv" id="blinkRaw">—</span></div>
      </div>
    </details>

    <details>
      <summary><span>Lane Change</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="subhead">BLINDSPOT CONFIG · AP / NOA ONLY</div>
        <div class="controlgrid">
          <button class="btn" id="blindspotStandard">STANDARD</button>
          <button class="btn" id="blindspotAggressive">AGGRESSIVE</button>
          <button class="btn" id="blindspotMadMax">MAD_MAX</button>
        </div>
        <div class="r"><span class="rk">Selected config</span><span class="rv" id="blindspotState">STANDARD</span></div>
        <div class="r"><span class="rk">Injected config</span><span class="rv" id="blindspotInjected">—</span></div>
        <div class="r"><span class="rk">Front interior button</span><span class="rv" id="laneButtonDetail">—</span></div>
        <div class="r"><span class="rk">Lane-change cancel count</span><span class="rv" id="laneCancelCount">0</span></div>
      </div>
    </details>

      <details>
      <summary><span>TLSSC Restore For banned car only</span><span class="arrow">›</span></summary>
      <div class="body">
        <div style="color:red" class="subhead">Do not use on non banned car you will be banned instantly.</div>
        <button class="btn" id="tlsscRestoreToggle" style="width:100%">Enable TLSSC Restore</button>
      </div>
    </details>

    <details>
      <summary><span>Summon transport</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="r"><span class="rk">Priority state</span><span class="rv" id="sumPriorityDetail">—</span></div>
        <div class="r"><span class="rk">Fresh parked</span><span class="rv" id="sumFreshPark">—</span></div>
        <div class="r"><span class="rk">TX queue now / max</span><span class="rv" id="sumTxQueue">—</span></div>
        <div class="r"><span class="rk">Non-Summon shed</span><span class="rv" id="sumShed">—</span></div>
        <div class="r"><span class="rk">Queue flush</span><span class="rv" id="sumFlush">—</span></div>
        <div class="r"><span class="rk">Full enter / exit</span><span class="rv" id="sumFull">—</span></div>
        <div class="r"><span class="rk">RX 0x118 / 0x186 / 0x399 / 0x3F8</span><span class="rv" id="sumRxIds">—</span></div>
        <div class="r"><span class="rk">R79 0x3FD AP RX mux1 / mux0</span><span class="rv" id="r79Rx">—</span></div>
        <div class="r"><span class="rk">R79 0x3FD TX OK / FAIL</span><span class="rv" id="r79Tx">—</span></div>
      </div>
    </details>

    <details>
      <summary><span>System</span><span class="arrow">›</span></summary>
      <div class="body">
        <div class="r"><span class="rk">Firmware</span><span class="rv" id="sysFw">—</span></div>
        <div class="r"><span class="rk">Uptime</span><span class="rv" id="sysUptime">—</span></div>
        <div class="r"><span class="rk">Free heap</span><span class="rv" id="sysHeap">—</span></div>
        <div class="r"><span class="rk">Hard Reinitialize</span><span class="rv" id="sysReinit">—</span></div>
        <div class="r"><span class="rk">Last hard reason</span><span class="rv" id="sysReason">—</span></div>
        <div class="r"><span class="rk">Last RX J2 / J3 / J4</span><span class="rv" id="sysBusAge">—</span></div>
        <div class="controlgrid">
          <button class="btn" id="btnResetStats">Reset Stats</button>
          <button class="btn" id="btnReinit">Hard Reinitialize</button>
          <a class="btn linkbtn" href="/api/system/boot-capture.csv">Boot Capture CSV</a>
          <button class="btn danger" id="btnReboot">Reboot T-2CAN</button>
        </div>
        <div class="subhead">FIRMWARE UPDATE</div>
        <div class="ota"><input id="otaFile" type="file" accept=".bin,application/octet-stream"><button class="btn primary" id="otaUpload" style="width:100%">Upload Firmware</button><div class="progress" id="otaProgress"><i id="otaBar"></i></div><div class="otaMsg" id="otaMsg"></div></div>
      </div>
    </details>
  </section>
  <div class="bottom">T2CAN UNLOCK</div>
</div>
<div class="confirmOverlay" id="tlsscConfirm" role="dialog" aria-modal="true" aria-labelledby="tlsscConfirmTitle">
  <div class="confirmBox">
    <div class="confirmTitle" id="tlsscConfirmTitle">TLSSC Restore Warning</div>
    <div class="confirmText">Do not use on non banned car you will be banned instantly</div>
    <div class="confirmActions">
      <button class="btn" id="tlsscCancel">Cancel</button>
      <button class="btn primary" id="tlsscConfirmBtn">Confirm</button>
    </div>
  </div>
</div>
<div class="toast" id="toast"></div>
<script>
const $=id=>document.getElementById(id);let otaUploading=false,lastOk=0,lastBlink=null,lastSum=null,lastSys=null,lastDas=null;
function ok(){lastOk=Date.now();$('conn').textContent='Connected';$('conn').className='live ok'}function toast(t){const x=$('toast');x.textContent=t;x.classList.add('show');clearTimeout(toast.h);toast.h=setTimeout(()=>x.classList.remove('show'),1500)}
setInterval(()=>{if(Date.now()-lastOk>2500){$('conn').textContent='Disconnected';$('conn').className='live'}},700);
function fmtUptime(s){s=Number(s)||0;const h=Math.floor(s/3600),m=Math.floor((s%3600)/60),ss=s%60;return h?`${h}h ${m}m ${ss}s`:m?`${m}m ${ss}s`:`${ss}s`}
function modeInfo(ds,noa){switch(Number(ds)){case 0:case 1:case 2:return['OFF',''];case 3:return['AUTOSTEER','good'];case 4:return['AUTOSTEER · RESTRICTED','warn compact'];case 5:return[noa?'NOA':'NOA · STALE',noa?'good':'warn'];case 6:return['FSD','good'];case 8:return['ABORTING','warn'];case 9:return['ABORTED','warn'];case 14:return['FAULT','bad'];case 15:return['SNA','warn'];default:return['STATE '+ds,'warn']}}
const TURN={0:'IDLE',2:'RIGHT',4:'UP_2',6:'LEFT',7:'LEFT legacy',8:'DOWN_2'};
async function jget(url){const r=await fetch(url,{cache:'no-store'});if(!r.ok)throw Error(r.status);ok();return r.json()}async function post(url){const r=await fetch(url,{method:'POST'});if(!r.ok&&!([202].includes(r.status)))throw Error(r.status);ok();return r}

async function fetchBlink(){try{const s=lastBlink=await jget('/api/blinkA/stats');const [m,c]=modeInfo(s.dasState,s.noaActive);$('apMode').textContent=m;$('apMode').className='mode '+c;$('blinkToggle').checked=!!s.enabled;$('blinkTransportModeState').textContent=Number(s.transportMode)===2?'STALKLESS · 0x3C2':'STALK · 0x249';$('blinkTransportStalk').className='btn '+(Number(s.transportMode)===1?'primary':'');$('blinkTransportStalkless').className='btn '+(Number(s.transportMode)===2?'primary':'');$('blinkerState').textContent=!s.enabled?'OFF':s.gateOpen?'READY':s.stateGateOpen?'WAIT':'STANDBY';$('blinkerState').className='v '+(s.enabled&&s.gateOpen?'good':'');if(document.activeElement!==$('blinkTriggerDelay'))$('blinkTriggerDelay').value=(Number(s.triggerDelayMs)/1000).toFixed(1);if(document.activeElement!==$('blinkApEntryDelay'))$('blinkApEntryDelay').value=(Number(s.apEntryDelayMs)/1000).toFixed(1);if(document.activeElement!==$('blinkLaneChangeDelay'))$('blinkLaneChangeDelay').value=(Number(s.laneChangeDelayMs)/1000).toFixed(1);$('blinkGate').textContent=s.gateOpen?'READY · STATES 3/4/5':s.stateGateOpen?'WAITING DELAY':'BLOCKED';$('blinkApEntryRemain').textContent=(s.apEntryRemainMs||0)+' ms';$('blinkLaneChangeRemain').textContent=(s.laneChangeRemainMs||0)+' ms';$('blinkArmed').textContent=s.autoArmed?'ARMED':'IDLE';$('blinkRemain').textContent=s.autoArmed?s.autoRemainMs+' ms':'—';$('blinkBehavior').textContent=s.behaviorType;$('blinkTurn').textContent=TURN[s.activeTurn]??s.activeTurn;
const transport=s.transportName||'—';
$('blinkTransport').textContent=transport;
$('blinkTransport').className='rv '+(transport.includes('STALKLESS')?'good':transport.includes('STALK')?'good':'');
$('live249').textContent=s.rx249??0;
$('blink3c2').textContent=s.rx3C2??0;
$('stalklessLeft').textContent=s.stalklessLeftButton?'PRESSED':'IDLE';
$('stalklessRight').textContent=s.stalklessRightButton?'PRESSED':'IDLE';
$('stalklessTx').textContent=(s.stalklessTxOk??0)+' / '+(s.stalklessTxFail??0);
$('blinkCk').textContent=!s.seen249?'—':(s.cksumSelfTest?'OK':'MISMATCH');
$('blinkRaw').textContent=s.realRaw||'—';$('buttonState').textContent=s.laneChangeButtonPressed?'PRESSED':'IDLE';$('buttonState').className='v '+(s.laneChangeButtonPressed?'good':'');const bcfg=['STANDARD','AGGRESSIVE','MAD_MAX'][Number(s.ulcBlindSpotInjectConfig)||0];$('blindspotState').textContent=bcfg;$('blindspotInjected').textContent=bcfg+' · AP/NOA';$('laneButtonDetail').textContent=s.laneChangeButtonPressed?'PRESSED':'IDLE';$('laneCancelCount').textContent=s.laneChangeCancelCount||0;$('blindspotStandard').className='btn '+(Number(s.ulcBlindSpotInjectConfig)===0?'primary':'');$('blindspotAggressive').className='btn '+(Number(s.ulcBlindSpotInjectConfig)===1?'primary':'');$('blindspotMadMax').className='btn '+(Number(s.ulcBlindSpotInjectConfig)===2?'primary':'')}catch(e){}}
async function fetchDas(){try{const s=lastDas=await jget('/api/das/stats');$('blink24a').textContent=s.visualDebugRx}catch(e){}}
async function fetchSum(){try{const s=lastSum=await jget('/api/summon/stats');$('summonToggle').checked=!!s.enabled;$('tlsscToggle').checked=!!s.tlssc;const r=$('tlsscRestoreToggle');r.textContent=s.tlsscRestore?'Disable TLSSC Restore':'Enable TLSSC Restore';r.className='btn '+(s.tlsscRestore?'primary':'');$('priorityState').textContent=s.priorityStateName||s.priorityState;$('sumPriorityDetail').textContent=s.priorityStateName||s.priorityState;$('sumFreshPark').textContent=s.priorityFreshParked?'YES':'NO';$('sumTxQueue').textContent=s.txQueueNow+' / '+s.txQueueMax;$('sumShed').textContent=s.nonSummonShed+' · S '+s.standbyShed+' · F '+s.fullShed;$('sumFlush').textContent=s.summonQueueFlush;$('sumFull').textContent=s.priorityFullEnter+' / '+s.priorityFullExit;$('sumRxIds').textContent=[s.rx280,s.rx390,s.rx921,s.rx1016].join(' / ');$('r79Rx').textContent=(s.r79RxMux1||0)+' / '+(s.r79RxMux0||0);$('r79Tx').textContent=(s.r79TxOk||0)+' / '+(s.r79TxFail||0);$('liveSumTx').textContent=s.txOk+' / '+s.txFail;const gp=$('gatePill');gp.textContent=s.gate?'OPEN':'CLOSED';gp.className='gatepill '+(s.gate?'':'closed');[['gPark',s.parked,'ON','OFF'],['gSummon',s.summon,'ON','OFF'],['gAca',s.aca,'ACTIVE','INACTIVE'],['gSpr',s.spr,'SEEN','NOT SEEN']].forEach(([id,v,a,b])=>{const e=$(id);e.textContent=v?a:b;e.className='gv '+(v?'good':'')});}catch(e){}}
async function fetchSys(){try{const s=lastSys=await jget('/api/system/stats');$('sysFw').textContent=s.fwVersion||'—';$('sysUptime').textContent=fmtUptime(s.uptimeS);$('sysHeap').textContent=Math.round((s.freeHeap||0)/1024)+' KB';$('sysReinit').textContent=s.canHardReinit+' / fail '+s.canHardReinitFail;$('sysReason').textContent=s.canLastHardReason;$('sysBusAge').textContent=[s.bodyAgeMs,s.chassisAgeMs,s.partyAgeMs].map(v=>(Number(v)||0)+' ms').join(' / ');
const canAOnline=!!s.mcpTrafficOnline;
$('canAStatus').textContent=canAOnline?'ONLINE':'OFFLINE';
$('canAStatus').className='canVal '+(canAOnline?'online':'offline');
$('canABadge').textContent=s.mcpReady?'READY':'NOT READY';
const canBOnline=!!s.chassisTrafficOnline;
$('canBStatus').textContent=canBOnline?'ONLINE':'OFFLINE';
$('canBStatus').className='canVal '+(canBOnline?'online':'offline');
$('canBBadge').textContent=s.chassisReady?'READY':'NOT READY';
const canCOnline=!!s.partyTrafficOnline;
$('canCStatus').textContent=canCOnline?'ONLINE':'OFFLINE';
$('canCStatus').className='canVal '+(canCOnline?'online':'offline');
$('canCBadge').textContent=s.partyReady?'READY':'NOT READY';
}catch(e){console.warn('fetchSys failed',e)}}

async function fetchNag(){try{const [c,s]=await Promise.all([jget('/api/nag/config'),jget('/api/nag/stats')]);$('nagStatus').textContent=c.enabled?'ENABLED · J4 PARTY':'DISABLED';$('nagStatus').className='rv '+(c.enabled?'good':'');$('nagQuickToggle').checked=!!c.enabled;$('nagRx').textContent=(s.canFrames||0)+' / '+(s.rx||0);$('nagWarmup').textContent=s.warmupReady?'READY':((s.warmupFrames||0)+' frames · waiting');$('nagTorque').textContent=Number(s.torque||0).toFixed(2)+' / '+Number(s.injNm||0).toFixed(2)+' Nm';$('nagTx').textContent=(s.txOk||0)+' / '+(s.txFail||0);$('liveHo').textContent=s.rx?String(s.ho):'—';$('liveInj').textContent=s.txOk?(String(s.injHo)+' · '+Number(s.injNm||0).toFixed(2)+' Nm'):'—';$('nagToggle').textContent=c.enabled?'Disable':'Enable';$('nagToggle').className='btn '+(c.enabled?'primary':'');['A','B','C'].forEach((x,i)=>$('nagMode'+x).className='btn '+(Number(c.mode)===i?'primary':''));if(document.activeElement!==$('nagTarget'))$('nagTarget').value='0x'+Number(c.targetId).toString(16).toUpperCase();if(document.activeElement!==$('nagRate'))$('nagRate').value=c.hoRatePct;if(document.activeElement!==$('nagBurst'))$('nagBurst').value=c.burstMs;if(document.activeElement!==$('nagPause'))$('nagPause').value=c.pauseMs;window.nagConfig=c;}catch(e){}}
async function nagSetMode(m){try{await post('/api/nag/mode?mode='+m);fetchNag()}catch(e){toast('Nag mode update failed')}}
['A','B','C'].forEach((x,i)=>$('nagMode'+x).onclick=()=>nagSetMode(i));
$('nagToggle').onclick=async()=>{const c=window.nagConfig||{};try{await post('/api/nag/update?enabled='+(c.enabled?'0':'1')+'&targetId='+encodeURIComponent($('nagTarget').value)+'&hoRatePct='+$('nagRate').value+'&burstMs='+$('nagBurst').value+'&pauseMs='+$('nagPause').value);fetchNag()}catch(e){toast('Nag update failed')}};
$('nagQuickToggle').onchange=async e=>{const c=window.nagConfig||{};try{await post('/api/nag/update?enabled='+(e.target.checked?'1':'0')+'&targetId='+encodeURIComponent($('nagTarget').value)+'&hoRatePct='+$('nagRate').value+'&burstMs='+$('nagBurst').value+'&pauseMs='+$('nagPause').value);fetchNag()}catch(x){e.target.checked=!!c.enabled;toast('Nag update failed')}};
$('nagApply').onclick=async()=>{const c=window.nagConfig||{};try{await post('/api/nag/update?enabled='+(c.enabled?'1':'0')+'&targetId='+encodeURIComponent($('nagTarget').value)+'&hoRatePct='+$('nagRate').value+'&burstMs='+$('nagBurst').value+'&pauseMs='+$('nagPause').value);fetchNag();toast('J4 configuration saved')}catch(e){toast('Nag update failed')}};

$('blinkToggle').onchange=async e=>{try{await post(e.target.checked?'/api/blinkA/enable':'/api/blinkA/disable');fetchBlink()}catch(x){}};
$('summonToggle').onchange=async e=>{try{await post(e.target.checked?'/api/summon/enable':'/api/summon/disable');fetchSum()}catch(x){}};
$('tlsscToggle').onchange=async e=>{try{await post(e.target.checked?'/api/summon/tlssc-enable':'/api/summon/tlssc-disable');fetchSum()}catch(x){}};
const tlsscConfirm=$('tlsscConfirm');
const tlsscCancel=$('tlsscCancel');
const tlsscConfirmBtn=$('tlsscConfirmBtn');

function closeTlsscConfirm(){tlsscConfirm.classList.remove('show')}

async function setTlsscRestore(){
  try{
    await post('/api/tlssc-restore/enable');
    await fetchSum();
    toast('TLSSC Restore enabled');
  }catch(e){
    toast('TLSSC Restore update failed');
  }
}

tlsscCancel.onclick=closeTlsscConfirm;
tlsscConfirmBtn.onclick=async()=>{closeTlsscConfirm();await setTlsscRestore()};
tlsscConfirm.onclick=e=>{if(e.target===tlsscConfirm)closeTlsscConfirm()};

$('tlsscRestoreToggle').onclick=async()=>{
  const enabled=!!(lastSum&&lastSum.tlsscRestore);
  if(enabled){
    try{
      await post('/api/tlssc-restore/disable');
      await fetchSum();
      toast('TLSSC Restore disabled');
    }catch(e){
      toast('TLSSC Restore update failed');
    }
    return;
  }
  tlsscConfirm.classList.add('show');
};

async function setBlinkTransport(transport){
  try{
    await post('/api/blinkA/transport?transport='+transport);
    await fetchBlink();
    toast(transport===2?'Turn Signal: Stalkless 0x3C2':'Turn Signal: Stalk 0x249');
  }catch(e){toast('Turn signal type update failed')}
}
$('blinkTransportStalk').onclick=()=>setBlinkTransport(1);
$('blinkTransportStalkless').onclick=()=>setBlinkTransport(2);
async function setBlindspotConfig(cfg){
  try{
    await post('/api/laneChange/blindspot?cfg='+cfg);
    await fetchBlink();
    toast('Blindspot: '+['STANDARD','AGGRESSIVE','MAD_MAX'][cfg]);
  }catch(e){toast('Blindspot update failed')}
}
$('blindspotStandard').onclick=()=>setBlindspotConfig(0);
$('blindspotAggressive').onclick=()=>setBlindspotConfig(1);
$('blindspotMadMax').onclick=()=>setBlindspotConfig(2);
$('blinkApply').onclick=async()=>{let triggerSec=parseFloat($('blinkTriggerDelay').value),apSec=parseFloat($('blinkApEntryDelay').value),laneSec=parseFloat($('blinkLaneChangeDelay').value);if(!Number.isFinite(triggerSec)||!Number.isFinite(apSec)||!Number.isFinite(laneSec))return toast('Invalid delay');triggerSec=Math.max(0,Math.min(30,triggerSec));apSec=Math.max(0,Math.min(30,apSec));laneSec=Math.max(0,Math.min(60,laneSec));$('blinkTriggerDelay').value=triggerSec.toFixed(1);$('blinkApEntryDelay').value=apSec.toFixed(1);$('blinkLaneChangeDelay').value=laneSec.toFixed(1);try{const r=await post('/api/blinkA/delays?triggerMs='+Math.round(triggerSec*1000)+'&apMs='+Math.round(apSec*1000)+'&laneMs='+Math.round(laneSec*1000));if(r.ok){await fetchBlink();toast('Auto Blinker delays saved')}}catch(e){toast('Delay update failed')}};
$('btnResetStats').onclick=async()=>{if(!confirm('Reset runtime diagnostic counters?'))return;try{await post('/api/system/reset-stats');toast('Stats reset')}catch(e){}};$('btnReinit').onclick=async()=>{if(!confirm('Hard reinitialize CAN A and CAN B?'))return;try{await post('/api/system/reinit-can');toast('Reinitialize requested')}catch(e){}};$('btnReboot').onclick=async()=>{if(!confirm('Reboot T-2CAN now?'))return;try{await post('/api/system/reboot')}catch(e){}toast('Rebooting…');setTimeout(()=>location.reload(),5000)};
$('otaUpload').onclick=()=>{const f=$('otaFile').files[0],msg=$('otaMsg'),pr=$('otaProgress'),bar=$('otaBar');if(!f)return toast('Choose .bin file');const fd=new FormData();fd.append('update',f,f.name);const x=new XMLHttpRequest();otaUploading=true;pr.classList.add('show');msg.textContent='Uploading…';x.open('POST','/update',true);x.upload.onprogress=e=>{if(e.lengthComputable){const p=Math.round(e.loaded/e.total*100);bar.style.width=p+'%';msg.textContent='Uploading '+p+'%'}};x.onload=()=>{let good=x.status===200;try{good=good&&JSON.parse(x.responseText).ok}catch(e){}if(good){bar.style.width='100%';msg.textContent='Flash successful · rebooting';setTimeout(()=>location.reload(),6000)}else{msg.textContent='OTA failed';otaUploading=false}};x.onerror=()=>{msg.textContent='Upload error';otaUploading=false};x.send(fd)};

fetchBlink();fetchDas();fetchSum();fetchSys();fetchNag();setInterval(()=>{if(!otaUploading){fetchBlink()}},500);setInterval(()=>{if(!otaUploading){fetchDas();fetchSum();fetchNag()}},800);setInterval(()=>{if(!otaUploading)fetchSys()},3000);
</script>
</body></html>
)HTML";
