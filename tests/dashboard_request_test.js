const assert = require('assert');
const fs = require('fs');
const path = require('path');
const vm = require('vm');
const source = fs.readFileSync(path.join(__dirname, '../dashboard_source.html'), 'utf8');
const start = source.indexOf('const DASHBOARD_REQUEST_TIMEOUT_MS='), end = source.indexOf('let reconnectWatchSeq', start);
assert(start >= 0 && end > start);
let timer, cleared = 0, connected = 0, response;
const context = {AbortController, setTimeout: callback => {timer=callback;return 1}, clearTimeout:()=>++cleared, ok:()=>++connected, fetch:async()=>response};
vm.createContext(context);
vm.runInContext(source.slice(start,end),context);
async function main() {
  let resolveBody;
  response = {ok:true, json:()=>new Promise(resolve=>resolveBody=resolve)};
  const pending = context.jget('/test');
  await new Promise(setImmediate);
  assert.equal(connected,0,'headers alone must not mark a stalled body as connected');
  resolveBody({ok:true});
  assert.deepEqual(await pending,{ok:true});
  assert.equal(connected,1);
  assert.equal(cleared,1);
  // Body parsing and network failures both release the timeout in finally.
  response = {ok:true, json:async()=>{throw new SyntaxError('invalid JSON')}};
  await assert.rejects(context.jget('/test'),/invalid JSON/);
  assert.equal(connected,1);
  context.fetch = async (url,options) => {
    const abortable = () => new Promise((resolve,reject) => options.signal.addEventListener('abort',()=>reject(new Error('aborted')),{once:true}));
    return {ok:true,json:abortable};
  };
  const stalledBody = context.jget('/test');
  await new Promise(setImmediate);
  timer();
  await assert.rejects(stalledBody,/aborted/);
  assert.equal(cleared,3);
  assert.equal(connected,1);
  console.log('PASS: GET timeout covers JSON body; connectivity follows complete valid responses; failure clears timeout');
}
main().catch(error=>{console.error(error);process.exitCode=1});
