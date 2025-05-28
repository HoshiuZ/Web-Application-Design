const WebSocket = require('ws');
const wss = new WebSocket.Server({ port: 8080 });

const clients = new Map(); // 存储每个连接的信息

wss.on('connection', (ws) => {
  ws.on('message', (message) => {
    const data = JSON.parse(message);

    if (data.type === 'register') {
      clients.set(ws, { id: data.id, role: data.role });
      console.log(`[注册] ${data.role}: ${data.id}`);
    }

    if (data.type === 'chat') {
      // 广播聊天消息
      for (let client of clients.keys()) {
        if (client.readyState === WebSocket.OPEN) {
          client.send(JSON.stringify(data));
        }
      }
    }

    if (data.type === 'command') {
      // 用户向节点发送控制命令
      for (let [client, info] of clients.entries()) {
        if (info.id === data.target && info.role === 'node') {
          client.send(JSON.stringify(data));
          break;
        }
      }
    }

    if (data.type === 'status') {
      // 节点返回状态，发送给用户
      for (let [client, info] of clients.entries()) {
        if (info.id === data.target && info.role === 'user') {
          client.send(JSON.stringify(data));
          break;
        }
      }
    }
  });

  ws.on('close', () => {
    const info = clients.get(ws);
    console.log(`[断开] ${info?.role}: ${info?.id}`);
    clients.delete(ws);
  });
});

console.log("WebSocket server is running on ws://localhost:8080");
