const http = require('http');
const WebSocket = require('ws');
const fs = require('fs');
const path = require('path');

const server = http.createServer((req, res) => {
  const file = req.url === '/' ? '/index.html' : req.url;
  fs.readFile(path.join(__dirname, file), (err, data) => {
    if (err) {
      res.writeHead(404);
      res.end("Not found");
    } else {
      res.writeHead(200);
      res.end(data);
    }
  });
});

const wss = new WebSocket.Server({ server });

wss.on('connection', ws => {
  ws.on('message', msg => {
    const messageStr = msg.toString();

    let user = "匿名";
    let text = messageStr;

    try {
      const parsed = JSON.parse(messageStr);
      user = parsed.user || "匿名";
      text = parsed.text || "";
    } catch (e) {
    }

    const broadcastMsg = `[${user}]: ${text}`;

    // 广播给所有客户端
    wss.clients.forEach(client => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(broadcastMsg);
      }
    });
  });
});

server.listen(3000, () => {
  console.log("Server is listening on port 3000");
});
