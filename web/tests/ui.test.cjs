const fs=require('node:fs'),vm=require('node:vm'),assert=require('node:assert/strict');
const elements=new Map();
function element(){return {value:'',disabled:false,textContent:'',children:[],classList:{toggle(){}},append(){},prepend(x){this.children.unshift(x)},get lastChild(){return {remove:()=>this.children.pop()}}};}
const get=id=>{if(!elements.has(id))elements.set(id,element());return elements.get(id)};
let now=100000, interval, options, handlers={}, published=[];
const fake={connected:false,on(n,cb){handlers[n]=cb},subscribe(topics,opts,cb){cb(null,topics.map(topic=>({topic,qos:0})))},publish(t,p,o,cb){published.push({t,p:JSON.parse(p),o});cb()},end(){this.connected=false}};
class Clock extends Date {static now(){return now}}
const context=vm.createContext({document:{getElementById:get,createElement:element},Date:Clock,URL,Uint32Array,crypto:{getRandomValues:a=>{a[0]=123;return a}},location:{protocol:'http:'},setInterval:f=>interval=f,mqtt:{connect:(url,o)=>{options=o;return fake}}});
vm.runInContext(fs.readFileSync(require('node:path').join(__dirname,'../app.js'),'utf8'),context);
assert.equal(get('led1-on').disabled,true);
get('url').value='ws://localhost:9001';get('topic').value='nhom01/node01';
get('connectForm').onsubmit({preventDefault(){}});
fake.connected=true;handlers.connect();
assert.equal(options.queueQoSZero,false);
assert.equal(get('led1-on').disabled,true);
const message=(topic,d,retain=false)=>handlers.message('nhom01/node01/'+topic,Buffer.from(JSON.stringify(d)),{retain});
const s={device_id:'node01',session:9,revision:1,valid:true,outputs:{led1:{power:0,remaining_ms:0},led2:{power:0,remaining_ms:0},motor1:{power:0,remaining_ms:0}}};
message('state',s,true);assert.equal(get('led1-on').disabled,true);
message('state',s);assert.equal(get('led1-on').disabled,false);
get('led1-level').value='50';get('led1-on').onclick();
assert.equal(published[0].p.power,50);assert.equal(published[0].o.retain,false);
assert.equal(published[0].p.session,9);assert.equal(get('led1-on').disabled,true);
assert.equal(get('led1-power').textContent,'0%'); // No optimistic success.
s.revision=2;s.outputs.led1.power=50;s.outputs.led1.remaining_ms=19000;
message('state',s);assert.equal(get('led1-power').textContent,'50%');
message('telemetry',{device_id:'node01',valid:true,temp:26.1,humi:53.6});
assert.equal(get('temp').textContent,'26.1');
now+=4000;interval();assert.equal(get('led1-on').disabled,true);assert.notEqual(get('temp').textContent,'26.1');
get('stopAll').onclick();assert.equal(published.at(-1).p.target,'all');assert.equal(published.at(-1).p.power,0);
message('state',{...s,valid:false});assert.equal(get('led1-on').disabled,true);
fake.connected=false;handlers.close();let n=published.length;get('led1-on').onclick();assert.equal(published.length,n);
console.log('PASS: disconnected/retained/stale/invalid locks, command protocol, confirmed state, sensor display, STOP, no offline queue.');
