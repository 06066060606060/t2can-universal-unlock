const fs=require('fs'),vm=require('vm'),assert=require('assert');
const source=fs.readFileSync(require('path').join(__dirname,'../dashboard_source.html'),'utf8');
const start=source.indexOf('// BEGIN CAN PERFORMANCE RENDER'),end=source.indexOf('// END CAN PERFORMANCE RENDER');
assert(start>=0 && end>start);
const nodes=new Map();
const $=id=>{if(!nodes.has(id))nodes.set(id,{textContent:''});return nodes.get(id)};
const context={$};vm.createContext(context);
vm.runInContext(source.slice(start,end),context);
context.renderCanPerformance({});assert.equal($('canPerfMode').textContent,'Unavailable');
const bus={rxFrames:100,rxErrors:1,rxOverflowEvents:2,rx0OverflowEvents:2,rx1OverflowEvents:0,txQueued:95,txBusy:3,txErrors:2,txStatusSamples:100,txFullSamples:5,txPendingLast:1,txPendingMax:3,rxToEnqueue:{count:95,meanUs:40,p95UpperUs:128,maxUs:250},pollGap:{count:99,meanUs:100,maxUs:1500},rxProcess:{count:100,meanUs:20,maxUs:40}};
context.renderCanPerformance({canPerformance:{compilerOptimization:'O2',body:bus,chassis:bus,party:bus}});
assert.equal($('canPerfMode').textContent,'-O2');
for(const name of ['body','chassis','party']){
  assert.equal($(`canPerf_${name}_Latency`).textContent,'40 / <=128 / 250 us');
  assert.equal($(`canPerf_${name}_Tx`).textContent,'95 / 3 / 2');
  assert.equal($(`canPerf_${name}_Full`).textContent,'5 / 100 (5.0%)');
  assert(source.includes(`id=canPerf_${name}_Latency`));
}
context.renderCanPerformance({canPerformance:{compilerOptimization:'Os',body:{...bus,rxToEnqueue:{count:0},txStatusSamples:0}}});
assert.equal($('canPerfMode').textContent,'-Os');
assert.equal($('canPerf_body_Latency').textContent,'--');
assert.equal($('canPerf_body_Full').textContent,'--');
assert.equal($('canPerf_party_Rx').textContent,'--');
// Parse every script without executing network calls or device actions.
for(const match of source.matchAll(/<script\b[^>]*>([\s\S]*?)<\/script>/gi))new vm.Script(match[1]);
console.log('PASS: all dashboard scripts parse; three bus render, O2/Os label, empty/older firmware');
