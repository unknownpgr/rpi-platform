const fs = require("fs");
const readState = require("./state-reader.js");

const sharedMemoryPath = "/dev/shm/state";
const sharedMemorySize = 4096;

const STATUS_INITIALIZING = "INITIALIZING";
const STATUS_INITIALIZED = "INITIALIZED";
const STATUS_ERROR = "ERROR";

class Robot {
  constructor() {
    this.status = STATUS_INITIALIZING;
    this.mappingStructure = [];
    this.state = {};
    this.shmFd = null;
    this.shmBuffer = Buffer.alloc(sharedMemorySize);
    this.inputBuffer = "";
    this.listeners = [];

    this.initialize();
  }

  initialize() {
    // Connect to shared memory
    try {
      this.shmFd = fs.openSync(sharedMemoryPath, "r+");
    } catch (err) {
      if (err.code === "ENOENT") {
        this.status = STATUS_ERROR;
        this.notify({ type: "status", data: this.status });
        return;
      } else {
        throw err;
      }
    }

    // Setup pipe
    const [readPipeFd, writePipeFd] = process.argv[2].split(",");
    this.readPipeFd = +readPipeFd;
    this.writePipeFd = +writePipeFd;

    // Configure read pipe handler
    const readStream = fs.createReadStream(null, { fd: this.readPipeFd });
    readStream.on("data", (data) => this.handleInput(data.toString()));

    // Setup state change handler
    setInterval(() => this.handleStateChange(), 100);
  }

  handleInput(input) {
    this.inputBuffer += input;
    console.log(input);

    switch (this.status) {
      case STATUS_INITIALIZING: {
        const regex = /^\[([0-9,]+)\]$/;
        const match = this.inputBuffer.match(regex);
        if (match) {
          this.offsets = match[1].split(",").map((x) => +x);
          this.status = STATUS_INITIALIZED;
        }
        break;
      }

      case STATUS_INITIALIZED: {
        if (this.inputBuffer.length > 3000) {
          this.inputBuffer = this.inputBuffer.slice(-3000);
        }
        this.notify({ type: "input", data: input });
        break;
      }

      case STATUS_ERROR: {
        this.inputBuffer = "";
        break;
      }
    }
  }

  handleStateChange() {
    const newBuffer = Buffer.alloc(sharedMemorySize);
    fs.readSync(this.shmFd, newBuffer, 0, sharedMemorySize, 0);

    // Check if the buffer has changed
    if (newBuffer.equals(this.shmBuffer)) return;
    this.shmBuffer = newBuffer;

    const newState = readState(this.shmBuffer, this.offsets);
    this.state = newState;
    this.notify({ type: "state", data: this.state });
  }

  notify(event) {
    this.listeners.forEach((listener) => listener(event));
  }

  addListener(listener) {
    this.listeners.push(listener);
    this.notify({ type: "status", data: this.status });
    if (this.status === STATUS_INITIALIZED) {
      this.notify({ type: "state", data: this.state });
      this.notify({ type: "input", data: this.inputBuffer });
    }
  }

  removeListener(listener) {
    this.listeners = this.listeners.filter((l) => l !== listener);
  }

  sendCommand(command) {
    if (this.status !== STATUS_INITIALIZED) return;
    fs.writeSync(this.writePipeFd, command + "\n");
  }
}

module.exports = Robot;
