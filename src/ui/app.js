const express = require("express");
const WebSocket = require("ws");
const path = require("path");

const app = express();
const port = 80;

// Serve static files from public directory
app.use(express.static(path.join(__dirname, "public")));

// Redirect `/` to `/index.html`
app.get("/", (req, res) => {
  res.redirect("/index.html");
});

// Create HTTP server
const server = app.listen(port, () => {
  console.log(`Server listening on port ${port}`);
});

// Create WebSocket server
const wss = new WebSocket.Server({ server });

// WebSocket connection handler
wss.on("connection", (ws) => {
  ws.on("message", (message) => {});
  ws.on("close", () => {
    console.log("Client disconnected");
  });
});

// Redirect stdin to stdout
process.stdin.on("data", (data) => {
  wss.clients.forEach((client) => client.send(data.toString()));
});

console.log("UI server started");
