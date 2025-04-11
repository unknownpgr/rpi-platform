const express = require("express");
const WebSocket = require("ws");
const path = require("path");
const fs = require("fs");
const { off } = require("process");

const sharedMemoryPath = "/dev/shm/state";
const sharedMemorySize = 4096;
const buffer = Buffer.alloc(sharedMemorySize);
const sharedMemory = fs.openSync(sharedMemoryPath, "r+");

function parseSharedMemory(buffer) {
  let offset = 0;
  const run_program = buffer.readUInt8(offset) !== 0;
  offset += 8;

  const state = buffer.readUInt8(offset);
  offset += 1;

  const is_calibrated = buffer.readUInt8(offset) !== 0;
  offset += 1;

  const sensor_values = [];
  for (let i = 0; i < 16; i++) {
    sensor_values.push(buffer.readUInt16LE(offset));
    offset += 2;
  }

  const calibration = {
    low: [],
    high: [],
  };

  for (let i = 0; i < 16; i++) {
    calibration.low.push(buffer.readUInt16LE(offset));
    offset += 2;
  }

  for (let i = 0; i < 16; i++) {
    calibration.high.push(buffer.readUInt16LE(offset));
    offset += 2;
  }

  const battery_voltage = buffer.readDoubleLE(8);

  return {
    run_program,
    state,
    is_calibrated,
    sensor_values,
    calibration,
    battery_voltage,
  };
}

const pipeFd = +process.argv[2];

const app = express();
const port = 80;

// Serve static files from public directory
app.use(express.static(path.join(__dirname, "public")));

// Redirect `/` to `/index.html`
app.get("/", (req, res) => {
  res.redirect("/index.html");
});

app.get("/command", (req, res) => {
  const command = req.query.command;
  fs.writeSync(pipeFd, command + "\n");
  res.send("OK");
});

// Create HTTP server
const server = app.listen(port, () => {
  console.log(`Server listening on port ${port}`);
});

// Create WebSocket server
const wss = new WebSocket.Server({ server });

let logBuffer = "";

// WebSocket connection handler
wss.on("connection", (ws) => {
  ws.send(logBuffer);
  ws.on("message", (message) => {});
  ws.on("close", () => {
    console.log("Client disconnected");
  });
});

// Redirect stdin to stdout
process.stdin.on("data", (data) => {
  wss.clients.forEach((client) => client.send(data.toString()));
  logBuffer += data.toString();
  if (logBuffer.length > 1024) {
    logBuffer = logBuffer.slice(-1024);
  }
});

setInterval(() => {
  fs.readSync(sharedMemory, buffer, 0, sharedMemorySize, 0);
  const state = parseSharedMemory(buffer);
  // wss.clients.forEach((client) => client.send(JSON.stringify(state)));
}, 100);
