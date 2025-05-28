const WebSocket = require('ws');
const mqtt = require('mqtt');

const wss = new WebSocket.Server({ port: 8081 }, () => {
  console.log('WebSocket 服务器已启动：ws://139.155.108.193:8081');
});

const mqttClient = mqtt.connect('mqtt://localhost:1883');

wss.on('connection', (ws) => {
  console.log('网页客户端已连接');

  // 接收来自网页的消息
  ws.on('message', (msg) => {
    try {
      const data = JSON.parse(msg);
      if (data.type === 'chat') {
        mqttClient.publish('chatroom/general', msg);
      } else if (data.type === 'command') {
        mqttClient.publish(`command/${data.target}`, msg);
      } else {
        console.log('未知消息类型', data);
      }
    } catch (e) {
      console.error('消息解析错误:', e);
    }
  });

  // 当从 MQTT 收到消息后，发送给 WebSocket 客户端
  mqttClient.on('message', (topic, message) => {
    ws.send(message.toString());
  });
});

// MQTT 连接成功后订阅主题
mqttClient.on('connect', () => {
  console.log('已连接到 MQTT Broker');
  mqttClient.subscribe('status/+');           // 所有设备状态
  mqttClient.subscribe('chatroom/general');   // 聊天室消息
});
