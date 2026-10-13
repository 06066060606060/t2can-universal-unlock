const fs=require('fs'),vm=require('vm'),assert=require('assert');
const source=fs.readFileSync(require('path').join(__dirname,'../dashboard_source.html'),'utf8');
const start=source.indexOf('// BEGIN CAN PERFORMANCE RENDER'),end=source.indexOf('// END CAN PERFORMANCE RENDER');
assert(start>=0 && end>start);
const ids=new Set([...source.matchAll(/\bid=(?:"([^"]+)"|([^\s>]+))/g)].map(m=>m[1]||m[2]));
const nodes=new Map();
const $=id=>{assert(ids.has(id),'missing render target: '+id);if(!nodes.has(id))nodes.set(id,{textContent:''});return nodes.get(id)};
const context={$};vm.createContext(context);
vm.runInContext(source.slice(start,end),context);
context.renderCanPerformance({});
const bus={rxFrames:100,rxErrors:1,rxOverflowEvents:2,txQueued:95,txBusy:3,txErrors:2,recoveries:4,recoveryErrors:1};
context.renderCanPerformance({canPerformance:{body:bus,chassis:{...bus,rxFrames:200,recoveries:7},party:{...bus,rx370:30,tx370:12}}});
for(const name of ['body','chassis','party']){
  assert.equal($(`canPerf_${name}_Rx`).textContent,name==='chassis'?'200':'100');
  assert.equal($(`canPerf_${name}_Tx`).textContent,'95');
  assert.equal($(`canPerf_${name}_Fail`).textContent,'1 / 5');
  assert.equal($(`canPerf_${name}_Recovery`).textContent,name==='chassis'?'7 / 1':'4 / 1');
  assert.equal($(`canPerf_${name}_Overflow`).textContent,'2');
}
assert.equal($('canPerf_party_Rx370').textContent,'30');
assert.equal($('canPerf_party_Tx370').textContent,'12');
// Older firmware must show unavailable counters rather than invented zeros or NAG aliases.
context.renderCanPerformance({canPerformance:{body:{rxFrames:0,rxErrors:0,txQueued:0,txBusy:0,txErrors:0}}});
assert.equal($('canPerf_body_Rx').textContent,'0');
assert.equal($('canPerf_body_Recovery').textContent,'--');
assert.equal($('canPerf_party_Rx370').textContent,'--');
assert.equal($('canPerf_party_Tx370').textContent,'--');
assert.equal($('canPerf_party_Rx').textContent,'--');
context.renderCanPerformance({canPerformance:{party:{...bus,rx370:0,tx370:0,recoveries:0,recoveryErrors:0}}});
assert.equal($('canPerf_party_Rx370').textContent,'0');
assert.equal($('canPerf_party_Recovery').textContent,'0 / 0');
assert(!ids.has('canPerf_body_Latency') && !ids.has('sysMcpBusOffSnap'));
for(const match of source.matchAll(/<script\b[^>]*>([\s\S]*?)<\/script>/gi))new vm.Script(match[1]);
console.log('PASS: simplified bus counters, RX/TX failures, recoveries, independent 0x370 counters, old firmware and script syntax');
