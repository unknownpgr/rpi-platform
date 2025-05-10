#!/usr/bin/env node

const path = require("path");
const express = require("express");
const WebSocket = require("ws");
const Robot = require("./robot");

const PORT = 80;
const UI_PATH = path.join(__dirname, "..", "src-ui", "dist");
const app = express();

// Serve static files from public directory
app.use(express.static(UI_PATH));

// Fallback to index.html
app.get("{*_}", (req, res) => {
  res.sendFile(path.join(UI_PATH, "index.html"));
});

// Create HTTP server
const server = app.listen(PORT, () => {
  console.log(`Server listening on port ${PORT}`);
});

// Create WebSocket server and robot
const wss = new WebSocket.Server({ server });
const robot = new Robot();

// Setup event handlers
wss.on("connection", (ws) => {
  function handleRobotEvent(event) {
    ws.send(JSON.stringify(event));
  }

  function handleWebSocketMessage(message) {
    const command = message.toString();
    robot.sendCommand(command);
  }

  robot.addListener(handleRobotEvent);
  ws.on("message", handleWebSocketMessage);
  ws.on("close", () => robot.removeListener(handleRobotEvent));
});
