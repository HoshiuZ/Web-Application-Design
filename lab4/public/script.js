const ws = new WebSocket('ws://139.155.108.193:8080');

ws.onopen = () => {
  ws.send(JSON.stringify({ type: 'register', id: 'webUser1', role: 'user' }));
};

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  const output = document.getElementById('output');

  if (data.type === 'chat') {
    output.textContent += `[聊天] ${data.from}: ${data.message}\n`;
  } else if (data.type === 'status') {
    output.textContent += `[状态] 来自设备${data.id}：${data.status}\n`;
  }
};

function sendChat() {
  const msg = document.getElementById('msgInput').value;
  ws.send(JSON.stringify({ type: 'chat', from: 'webUser1', message: msg }));
}

function sendCommand() {
  const target = document.getElementById('nodeId').value;
  const cmd = document.getElementById('cmdInput').value;
  ws.send(JSON.stringify({ type: 'command', id: 'webUser1', target: target, command: cmd }));
}
