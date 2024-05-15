const express = require('express');
const path = require('path');
const http = require('http');
const WebSocket = require('ws');
const coap = require('coap');

const app = express();
const PORT = 3000;

let latestData = { temperature: null, humidity: null };

// Serve static files from the "public" directory
app.use(express.static(path.join(__dirname, 'public')));

// Middleware to handle both JSON and plain text data
app.use(express.json());
app.use(express.text({ type: 'text/plain' }));

// Create HTTP server
const server = http.createServer(app);

// Create WebSocket server
const wss = new WebSocket.Server({ server });

// Handle WebSocket connections
wss.on('connection', (ws) => {
  console.log('Client connected');

  ws.on('message', (message) => {
    const { command } = JSON.parse(message);
    handleCoapRequest(command);
  });

  ws.on('close', () => {
    console.log('Client disconnected');
  });
});

function handleCoapRequest(command) {
  const req = coap.request({
    hostname: '192.168.151.224', // Replace with your ESP32 IP address
    port: 5683,
    method: 'PUT',
    pathname: '/light',
  });

  req.write(command);
  req.on('response', (res) => {
    res.pipe(process.stdout);
    res.on('end', () => {
      console.log('CoAP request sent successfully');
    });
  });

  req.end();
}

// Define your REST endpoint
app.post('/api/data', (req, res) => {
  let temperature, humidity;

  if (typeof req.body === 'string') {
    // Handle plain text data
    const data = req.body.trim();
    console.log("Received data:", data);

    if (data.includes(',')) {
      [temperature, humidity] = data.split(',').map(item => item.trim());
    } else {
      console.error("Invalid data format");
      return res.sendStatus(400); // Bad request
    }
  } else if (typeof req.body === 'object') {
    // Handle JSON data
    ({ temperature, humidity } = req.body);
  } else {
    console.error("Unsupported data format");
    return res.sendStatus(400); // Bad request
  }

  latestData = { temperature, humidity }; // Store the latest data

  console.log("Température:", temperature, "°C", "Humidité:", humidity, "%");

  // Broadcast data to all connected WebSocket clients
  wss.clients.forEach((client) => {
    if (client.readyState === WebSocket.OPEN) {
      client.send(JSON.stringify({ temperature, humidity }));
    }
  });

  res.sendStatus(200);
});

// Endpoint to retrieve the latest data
app.get('/api/data', (req, res) => {
  res.json(latestData);
});

// Handle LED control endpoint
app.put('/light/:command', (req, res) => {
  const { command } = req.params;
  console.log(`Received LED command: ${command}`);
  handleCoapRequest(command);
  res.sendStatus(204); // No content response
});

// Start the server
server.listen(PORT, '192.168.151.40', () => {
  console.log(`Server running on http://192.168.151.40:${PORT}`);
});