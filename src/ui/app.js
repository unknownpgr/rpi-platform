const express = require("express");
const WebSocket = require("ws");
const path = require("path");
const fs = require("fs");

const sharedMemoryPath = "/dev/shm/state";
const sharedMemorySize = 4096;
const buffer = Buffer.alloc(sharedMemorySize);
const sharedMemory = fs.openSync(sharedMemoryPath, "r+");
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

const READ_MAPPING = 0x00;
const FORWARD = 0x01;

let state = READ_MAPPING;
let inputBuffer = "";
let mappingStructure = [];

// WebSocket connection handler
wss.on("connection", (ws) => {
  ws.send(JSON.stringify({ type: "log", data: inputBuffer }));
});

// Redirect stdin to stdout
process.stdin.on("data", (data) => {
  inputBuffer += data.toString();

  switch (state) {
    case READ_MAPPING:
      const index = inputBuffer.indexOf("--------");
      if (index !== -1) {
        mappingStructure = inputBuffer
          .slice(0, index)
          .split("\n")
          .map((x) => x.trim())
          .filter((x) => x.length > 0)
          .map((x) => {
            const [key, value, type] = x.split(":");
            return {
              key: key.split("."),
              offset: +value,
              type: type,
            };
          });
        inputBuffer = inputBuffer.slice(index + 9);
        state = FORWARD;
      }
      break;
    case FORWARD:
      if (inputBuffer.length > 3000) {
        inputBuffer = inputBuffer.slice(-3000);
      }
      wss.clients.forEach((client) => {
        client.send(JSON.stringify({ type: "log", data: data.toString() }));
      });
      break;
  }
});

setInterval(() => {
  fs.readSync(sharedMemory, buffer, 0, sharedMemorySize, 0);

  const state = {};

  mappingStructure.forEach((x) => {
    const { key, offset, type } = x;
    let value;
    switch (type) {
      case "uint8_t":
        value = buffer.readUInt8(offset);
        break;
      case "uint16_t":
        value = buffer.readUInt16LE(offset);
        break;
      case "uint32_t":
        value = buffer.readUInt32LE(offset);
        break;
      case "float":
        value = buffer.readFloatLE(offset);
        break;
      case "double":
        value = buffer.readDoubleLE(offset);
        break;
      case "bool":
        value = buffer.readUInt8(offset) !== 0;
        break;
      case "uint16_t[16]":
        value = [];
        for (let i = 0; i < 16; i++) {
          value.push(buffer.readUInt16LE(offset + i * 2));
        }
        break;
      default:
        return;
    }

    let obj = state;
    for (let i = 0; i < key.length - 1; i++) {
      if (!obj[key[i]]) {
        obj[key[i]] = {};
      }
      obj = obj[key[i]];
    }
    obj[key[key.length - 1]] = value;
  });

  wss.clients.forEach((client) =>
    client.send(JSON.stringify({ type: "state", data: state }))
  );
}, 100);
