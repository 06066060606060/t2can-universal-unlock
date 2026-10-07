(()=>{
  const send=value=>Promise.resolve(new Response(JSON.stringify(value),{status:200,headers:{'Content-Type':'application/json'}}));
  const query=new URLSearchParams(location.search),requestedModel=Number(query.get('previewModel'));
  const model=[1,2,3,4,5].includes(requestedModel)?requestedModel:1;
  const topology=model===1?1:Number(query.get('previewTopology'))===3?3:2;
  const turn=topology===3?0:model===4&&Number(query.get('previewTurn'))!==1?2:1;
  let ap=['OFF','AUTOSTEER','NOA'].includes(query.get('previewAp'))?query.get('previewAp'):'NOA';
  let dasState={OFF:0,AUTOSTEER:3,NOA:5}[ap],apActive=ap!=='OFF',noaActive=ap==='NOA';
  const topologyName={1:'Party + VH',2:'Body + Chassis',3:'Party + Chassis'}[topology];
  const canA=topology===2?'BODY':'PARTY',canB=topology===1?'VH':'CHASSIS';
  const nagTorqueSupported=topology!==2,nagTsl9Supported=topology!==3,nagSupported=nagTorqueSupported||nagTsl9Supported,advancedEapSupported=topology!==3,pedalMapSupported=topology!==3;
  window.__t2PreviewState={model,topology,turn,ap};
  const driverMonitoring={enabled:true,supported:true,active:apActive};
  const visionControl={requestDisabled:false,bus:0,supported:true,bodySupported:model!==1&&topology===2,busSupported:true,busSelectorVisible:model!==1,labEnabled:true,apActive,apFresh:true,stockValid:true,stockBit:1,rxAgeMs:24,rxCount:1240,gateOpen:false,txOk:0,txFail:0,state:'OFF'};
  const country={countrySupported:true,countryMode:0,countryGateOpen:false};
  const nag={enabled:false,ignoreApState:false,method:nagTorqueSupported?0:1,tsl9Sequence:0,pauseAtZeroSpeed:false,mode:7,humanVariant:1,hoRatePct:50,burstMs:300,pauseMs:2000,targetId:0x488,apStateId:0x399,steeringId:0x370,torque:[]};
  const nagStats={torque:0.03,ho:0,injNm:apActive?-0.22:0,injHo:apActive?1:0,rx:90222,txOk:89687,txFail:0,lastTxAgeMs:apActive?15:13900,maxTxGapMs:40,apActive,canAState:1,dasStateValid:true,dasState,dasAgeMs:15,sessionTxOk:18697,lastSkip:'NONE',lastSkipAgeMs:999999,vehicleSpeedValid:true,vehicleSpeedKph:71.2,vehicleSpeedFresh:true,stoppedGate:false,stopCarrierActive:false,humanPhase:apActive?'PEAK':'WAIT',humanMotion:'DRIVING',humanEventType:'HUMAN',humanEventCount:31,humanDirection:-1,humanOutputNm:apActive?-0.22:0,humanCarrier:false,humanSessionAgeMs:18500};
  const scroll={supported:true,routeName:topology===2?'BODY':canB,enabled:false,nagTsl9Selected:nag.enabled&&nag.method===1,sequenceName:nag.enabled&&nag.method===1?'V8.2 +1 / 0 / -1 / 0':'UP / DOWN',intervalSeconds:30,visualRepeatSeconds:2,visualWarningActive:false,visualWarningEpoch:0,warningRemainingMs:0,mux1Rx:1240,physicalDeferrals:0,txUpOk:0,txDownOk:0,txFail:0,lastTxAgeMs:999999,lastTxValue:0};
  const profile={setupMode:false,migrationNotice:false,profile:model,topology,turn,topologyName,canA,canB,nagSupported,nagTorqueSupported,nagTsl9Supported,advancedEapSupported,euUnlockSupported:true,pedalMapSupported,apDriveProfileSupported:pedalMapSupported};
  const features={lab:true,s3xy:true,doorCancel:false,bannedSupported:model!==1,banned:false,tlsscRestoreSupported:false,tlsscRestore:false,pedalMapSupported,apDriveProfileSupported:pedalMapSupported,apDriveProfile:false,apDriveProfileRegenRaw:10};
  const s3xy={bluetoothEnabled:true,bleInitialized:true,autoEnabled:true,pairedCount:2,connectedCount:2,maxDevices:3,devices:[{id:1,name:'Driver Button',address:'AA:BB:CC:DD:EE:FF',state:'READY',connected:true,secure:true,subscribed:true,autoConnect:true,rssi:-48,notifyCount:27,singleAction:'left_blinker',doubleAction:'right_blinker',longAction:'noa_cancel'}],ulcTxOk:0,ulcTxFail:0,ulcAccepted:0,ulcBlocked:0,ulcLastDir:0};
  const r79={fixedPolicy:true,txState:apActive?'ACTIVE':'SUSPENDED',txReason:apActive?'AP engaged':'AP OFF',gearName:'D',dasStateValid:true,dasState,txOk:42118,txFail:0,lastTxValid:true,lastTxAgeMs:apActive?12:13900,transport:'CAN B · '+canB,periodic:'20 Hz',smartMode:1,bit18Mode:1,bit18ModeName:'FORCE 0',runtimeState:apActive?'ACTIVE':'SUSPENDED',stockTemplateValid:true,stockMux1Rx:90222,fastAttempts:42118,fastTxOk:42118,fastTxFail:0,periodicTxOk:48069,periodicTxFail:0,quietArm:12,quietFire:12,quietGuardSkip:0};
  const homeFast={nag:{torque:.03,apActive,canAState:1},blink:{dasState,dasStateValid:true,displayState:dasState,displayStateValid:true,canReinitializing:false,enabled:true,noaSessionStateName:noaActive?'READY':'INACTIVE'},summon:{canState:1,canStateName:'RUNNING'},cantraffic:{mcpTrafficSeen:true,mcpTrafficOnline:true,mcpTrafficAgeMs:18,twaiTrafficSeen:true,twaiTrafficOnline:true,twaiTrafficAgeMs:22},r79};
  const homeSlow={blink:{enabled:true,delayMs:300},summon:{tlssc:false},s3xy,r79,lab3f8:{alcMode:0}};
  window.__t2PreviewSetAp=next=>{
    if(!['OFF','AUTOSTEER','NOA'].includes(next))return;
    ap=next;dasState={OFF:0,AUTOSTEER:3,NOA:5}[ap];apActive=ap!=='OFF';noaActive=ap==='NOA';
    window.__t2PreviewState.ap=ap;
    Object.assign(nagStats,{apActive,dasState,injNm:apActive?-0.22:0,injHo:apActive?1:0,lastTxAgeMs:apActive?15:13900,humanPhase:apActive?'PEAK':'WAIT',humanOutputNm:apActive?-0.22:0});
    Object.assign(r79,{txState:apActive?'ACTIVE':'SUSPENDED',txReason:apActive?'AP engaged':'AP OFF',dasState,lastTxAgeMs:apActive?12:13900,runtimeState:apActive?'ACTIVE':'SUSPENDED'});
    driverMonitoring.active=driverMonitoring.enabled&&apActive;
    Object.assign(visionControl,{apActive,gateOpen:visionControl.requestDisabled&&apActive,state:!visionControl.requestDisabled?'OFF':apActive?'READY':'WAIT_AP'});
    homeFast.nag.apActive=apActive;
    Object.assign(homeFast.blink,{dasState,displayState:dasState,noaSessionStateName:noaActive?'READY':'INACTIVE'});
  };
  window.fetch=async(input,init={})=>{
    const raw=typeof input==='string'?input:input.url;
    const u=new URL(raw,location.href);
    if(u.pathname==='/api/profile/status')return send(profile);
    if(u.pathname==='/api/features/status')return send(features);
    if(u.pathname==='/api/wifi/status')return send({ok:true,ssid:'T2CAN-DEMO',ip:'192.168.4.1',passwordIsDefault:false});
    if(u.pathname==='/api/country/stats')return send(country);
    if(u.pathname==='/api/country/update'){country.countryMode=Number(u.searchParams.get('mode'));return send(country);}
    if(u.pathname==='/api/nag/config')return send(nag);
    if(u.pathname==='/api/driver-monitoring/config'){
      if(u.searchParams.has('enabled'))driverMonitoring.enabled=u.searchParams.get('enabled')==='1';
      driverMonitoring.active=driverMonitoring.enabled&&apActive;
      return send(driverMonitoring);
    }
    if(u.pathname==='/api/vision-control/stats')return send(visionControl);
    if(u.pathname==='/api/vision-control/update'){
      if(u.searchParams.has('disabled'))visionControl.requestDisabled=u.searchParams.get('disabled')==='1';
      if(u.searchParams.has('bus'))visionControl.bus=Number(u.searchParams.get('bus'))===1?1:0;
      visionControl.gateOpen=visionControl.requestDisabled&&apActive;
      visionControl.state=!visionControl.requestDisabled?'OFF':apActive?'READY':'WAIT_AP';
      return send(visionControl);
    }
    if(u.pathname==='/api/nag/stats')return send(nagStats);
    if(u.pathname==='/api/nag/update'){
      if(u.searchParams.has('ignoreApState'))nag.ignoreApState=u.searchParams.get('ignoreApState')==='1';
      if(u.searchParams.has('enabled'))nag.enabled=u.searchParams.get('enabled')==='1';
      if(u.searchParams.has('pauseAtZeroSpeed'))nag.pauseAtZeroSpeed=u.searchParams.get('pauseAtZeroSpeed')==='1';
      if(u.searchParams.has('method'))nag.method=Number(u.searchParams.get('method'))===1?1:0;
      if(u.searchParams.has('tsl9Sequence'))nag.tsl9Sequence=Number(u.searchParams.get('tsl9Sequence'))===1?1:0;
      scroll.nagTsl9Selected=nag.enabled&&nag.method===1;
      scroll.sequenceName=scroll.nagTsl9Selected?'V8.2 +1 / 0 / -1 / 0':'UP / DOWN';
      return send(nag);
    }
    if(u.pathname==='/api/ap-right-scroll/stats')return send(scroll);
    if(u.pathname==='/api/ap-right-scroll/update'){
      if(u.searchParams.has('enabled'))scroll.enabled=u.searchParams.get('enabled')==='1';
      if(u.searchParams.has('intervalSeconds'))scroll.intervalSeconds=Number(u.searchParams.get('intervalSeconds'));
      if(u.searchParams.has('visualRepeatSeconds'))scroll.visualRepeatSeconds=Number(u.searchParams.get('visualRepeatSeconds'));
      scroll.nagTsl9Selected=nag.enabled&&nag.method===1;
      scroll.sequenceName=scroll.nagTsl9Selected?'V8.2 +1 / 0 / -1 / 0':'UP / DOWN';
      scroll.gateOpen=scroll.enabled&&apActive;
      return send(scroll);
    }
    if(u.pathname==='/api/nag/mode'){nag.mode=Number(u.searchParams.get('m'));return send(nag);}
    if(u.pathname==='/api/s3xy/stats')return send(s3xy);
    if(u.pathname==='/api/r79/stats')return send(r79);
    if(u.pathname==='/api/snapshot'){
      const groups=u.searchParams.get('groups');
      if(groups==='home-fast')return send({fast:homeFast,...(u.searchParams.has('slow')?{slow:homeSlow}:{})});
      if(groups==='home-slow')return send(homeSlow);
      if(groups==='lab-lite')return send({r79,alc:{alcValid:true,alcRaw:apActive?8:0,alcAgeMs:10,lane239Valid:true,lane239AgeMs:14,leftLaneExists:1,leftLineUsageRaw:2,leftForkRaw:0,rightLaneExists:1,rightLineUsageRaw:2,rightForkRaw:0},dmsNag:driverMonitoring});
      if(groups==='settings-lite')return send({system:{fwVersion:'3.8.4'},blink:{delayMs:300},summon:{sessionActive:false},s3xy,lab3f8:{alcMode:0},r79});
      if(groups==='system')return send({system:{fwVersion:'3.8.4'}});
      return send({});
    }
    return send({ok:true});
  };
})();
