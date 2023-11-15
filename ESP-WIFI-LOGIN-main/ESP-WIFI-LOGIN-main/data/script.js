
var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
window.addEventListener('load', onLoad);

scanButton();

// handletForm();

// function handletForm() {
//   var form = document.getElementById("ssid_pass");
  
//   form.addEventListener('submit', submitForm);
// }

// function submitForm(event) {
//   event.preventDefault();
//   // form.style.display = "none";
//   document.getElementById("submit_form").innerHTML = "<b>Form submit successful</b>";
//   ssid = document.getElementById("ssid").value;
//   document.getElementById("connect_result").innerHTML = `<b>Connecting to ${ssid}</b>`;
//   return false;
// }

function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen    = onOpen;
  websocket.onclose   = onClose;
  websocket.onmessage = onMessage; // <-- add this line
}

function onOpen(event) {
  console.log('Connection opened');
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  console.log(event.data);
  let msg = JSON.parse(event.data);
  console.log(msg);
  // console.log(typeof(msg));
  // console.log(msg.ssid[1]);

 switch (msg.type) {
  case 1:
    let html = "Scan done<br>";

    for (let i = 0; i < msg.number; i++) {
      html = html.concat(`<div class="box"><p><span id="">${msg.ssid[i]}</span><button class="button" onclick ="fill_SSID('${msg.ssid[i]}')">Connect</button></p></div>`);
    }

    document.getElementById("list_scan").innerHTML = html;

    break;
 
  default:
    break;
 }
}


function onLoad(event) {
  initWebSocket();

}

function fill_SSID(ssid) {
  document.getElementById("ssid").value = ssid;
}

function scanButton() {
  document.getElementById("scan_button").addEventListener('click', ()=>{
    console.log("Click Scan Button");
    document.getElementById("list_scan").innerHTML = "Scanning......";
    websocket.send('scan');
  })}


function toggle(){
  websocket.send('toggle');
}
