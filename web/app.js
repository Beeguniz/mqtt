'use strict';
const $=id=>document.getElementById(id);
const devices=[['led1','LED 1',[25,50,90]],['led2','LED 2',[25,50,90]],['motor1','Bơm MOTOR1',[25,50,75,100]]];
let client=null, base='', state=null, lastState=0, lastTelemetry=0, pending=null;

const jsonMessages={telemetry:null,state:null};
function showJson(){
 const item=jsonMessages[$('jsonType').value];
 $('jsonView').textContent=item?JSON.stringify(item.payload,null,2):'Chưa nhận dữ liệu.';
 $('jsonInfo').textContent=item?item.topic+' · Nhận lúc '+new Date(item.receivedAt).toLocaleString('vi-VN'):'Chưa nhận dữ liệu.';
 $('downloadJson').disabled=!item;
}
function recordJson(type,topic,payload){
 jsonMessages[type]={topic,receivedAt:new Date().toISOString(),payload};
 showJson();
}
function clearJson(){jsonMessages.telemetry=null;jsonMessages.state=null;showJson();}
$('jsonType').onchange=showJson;
$('downloadJson').onclick=()=>{
 const type=$('jsonType').value,item=jsonMessages[type];
 if(!item)return;
 const blob=new Blob([JSON.stringify(item.payload,null,2)+'\n'],{type:'application/json;charset=utf-8'});
 const url=URL.createObjectURL(blob),link=document.createElement('a');
 link.href=url;link.download=type+'-'+item.receivedAt.replace(/[:.]/g,'-')+'.json';
 link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
};

function note(message){
 $('notice').textContent=message;
 const li=document.createElement('li');li.textContent=new Date().toLocaleTimeString('vi-VN')+' · '+message;
 $('log').prepend(li);while($('log').children.length>30)$('log').lastChild.remove();
}
for(const [id,name,levels] of devices){
 const card=document.createElement('article');card.className='panel device';
 card.innerHTML='<div class="device-head"><h3>'+name+'</h3><span id="'+id+'-state" class="device-state">Chưa rõ</span></div>'+
 '<p class="power" id="'+id+'-power">—</p><p id="'+id+'-time">Chờ thiết bị</p>'+
 '<label for="'+id+'-level">Công suất đặt</label><select id="'+id+'-level">'+levels.map(n=>'<option value="'+n+'">'+n+'%</option>').join('')+'</select>'+
 '<div class="actions"><button id="'+id+'-on" disabled>Bật / Áp dụng</button><button id="'+id+'-off" class="secondary" disabled>Tắt</button></div>';
 $('devices').append(card);
 $(id+'-on').onclick=()=>send(id,Number($(id+'-level').value));
 $(id+'-off').onclick=()=>send(id,0);
}
function live(){return !!client?.connected && state!==null && Date.now()-lastState<3500;}
function render(){
 const fresh=live();
 $('connection').textContent=!client?.connected?'Chưa kết nối':fresh?'ESP32 trực tuyến':'Broker đã kết nối · chờ ESP32';
 $('connection').classList.toggle('online',fresh);
 $('freshness').textContent=fresh?(state.valid?'Cảm biến hợp lệ · có thể điều khiển':'Cảm biến lỗi hoặc dữ liệu cũ · chỉ cho phép dừng'):'Chưa có trạng thái mới từ ESP32.';
 $('stopAll').disabled=!client?.connected;
 for(const [id] of devices){
  const output=fresh?state.outputs[id]:null;
  $(id+'-state').textContent=output?(output.power>0?'Đang chạy':'Đã tắt'):'Chưa rõ';
  $(id+'-power').textContent=output?output.power+'%':'—';
  $(id+'-time').textContent=output?.power?'Còn tối đa '+Math.ceil(output.remaining_ms/1000)+' giây':'Giới hạn mỗi lần chạy: 20 giây';
  $(id+'-on').disabled=!fresh || !state.valid || !!pending;
  $(id+'-off').disabled=!client?.connected;
 }
 if(!fresh || !state.valid || Date.now()-lastTelemetry>15000){$('temp').textContent='—';$('humi').textContent='—';}
 if(pending && Date.now()-pending.at>4000){pending=null;note('Chưa xác nhận được lệnh. Kiểm tra trạng thái thiết bị trước khi thử lại.');}
}
function send(target,power){
 if(!client?.connected || (power && (!live() || !state.valid || pending)))return;
 const command={target,power,session:state?.session??0,revision:state?.revision??0};
 if(power)pending={target,power,session:state.session,revision:state.revision,at:Date.now()};
 client.publish(base+'/command',JSON.stringify(command),{qos:0,retain:false},err=>{
  if(err){pending=null;note('Gửi lệnh thất bại: '+err.message);}
  else note(power?'Đã gửi '+target+' '+power+'%; chờ ESP32 xác nhận.':'Đã gửi yêu cầu dừng '+target+'; xem trạng thái phản hồi.');
  render();
 });
 render();
}
$('stopAll').onclick=()=>send('all',0);
function reset(){clearJson();state=null;lastState=0;lastTelemetry=0;pending=null;render();}
$('connectForm').onsubmit=e=>{
 e.preventDefault();
 if(typeof mqtt==='undefined'){note('Không tải được thư viện MQTT. Kiểm tra file vendor/mqtt.min.js.');return;}
 let url;try{url=new URL($('url').value.trim());if(!['ws:','wss:'].includes(url.protocol))throw Error();}catch{note('Địa chỉ phải có dạng ws://IP:9001 hoặc wss://host:port.');return;}
 if(location.protocol==='https:' && url.protocol!=='wss:'){note('Trang HTTPS cần broker WSS. Với broker LAN hiện tại, mở trang qua HTTP.');return;}
 if(client)client.end(true);
 base=$('topic').value.trim(); reset();
 const c=mqtt.connect(url.href,{
  username:$('username').value || undefined,password:$('username').value ? $('password').value : undefined,
  clientId:'greenhouse-web-'+crypto.getRandomValues(new Uint32Array(1))[0].toString(16),
  clean:true,reconnectPeriod:2000,connectTimeout:5000,queueQoSZero:false
 });
 client=c;$('connect').disabled=true;$('disconnect').disabled=false;
 c.on('connect',()=>{
  if(client!==c)return;
  reset();
  c.subscribe([base+'/state',base+'/telemetry'],{qos:0},(err,grants)=>{
   if(err || grants?.some(g=>g.qos===128)){note('Không được phép đọc topic. Kiểm tra tài khoản/ACL.');return;}
   note('Đã kết nối broker; đang chờ trạng thái ESP32.');
  });render();
 });
 c.on('message',(topic,payload,packet)=>{
  if(client!==c || packet.retain)return;
  let d;try{d=JSON.parse(payload.toString());}catch{return;}
  if(d.device_id!==base.split('/')[1])return;
  if(topic===base+'/state'){
   if(!Number.isInteger(d.session)||!Number.isInteger(d.revision)||typeof d.valid!=='boolean'||
    !devices.every(([id])=>Number.isInteger(d.outputs?.[id]?.power)&&d.outputs[id].power>=0&&d.outputs[id].power<=100&&Number.isFinite(d.outputs[id].remaining_ms)))return;
   recordJson('state',topic,d);
   state=d;lastState=Date.now();
   if(pending && (d.session!==pending.session || d.revision!==pending.revision)){
    const accepted=d.session===pending.session && d.outputs[pending.target].power===pending.power;
    note(accepted?'ESP32 đã cập nhật công suất yêu cầu.':'Trạng thái đã thay đổi; lệnh chưa được xác nhận theo yêu cầu.');
    pending=null;
   }
  }else if(topic===base+'/telemetry'){
   recordJson('telemetry',topic,d);
   lastTelemetry=Date.now();
   $('temp').textContent=d.valid===true&&Number.isFinite(d.temp)?d.temp.toFixed(1):'—';
   $('humi').textContent=d.valid===true&&Number.isFinite(d.humi)?d.humi.toFixed(1):'—';
  }
  render();
 });
 c.on('close',()=>{if(client===c){reset();note('Mất kết nối broker. Đang thử kết nối lại.');}});
 c.on('error',err=>{if(client===c)note('Lỗi MQTT: '+err.message);});
};
$('disconnect').onclick=()=>{
 const previous=client;client=null;previous?.end(true);
 $('connect').disabled=false;$('disconnect').disabled=true;reset();note('Đã ngắt giao diện. Thiết bị vẫn áp dụng giới hạn chạy 20 giây.');
};
setInterval(render,500);render();
