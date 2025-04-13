const fs = require("fs/promises");

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

  async initialize() {
    // Configure stdin handler
    process.stdin.on("data", (data) => this.handleInput(data.toString()));

    // Connect to shared memory
    try {
      this.shmFd = await fs.open(sharedMemoryPath, "r+");
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
    this.pipeFd = +process.argv[2];

    // Setup state change handler
    setInterval(() => this.handleStateChange(), 100);
  }

  handleInput(input) {
    this.inputBuffer += input;

    switch (this.status) {
      case STATUS_INITIALIZING: {
        const index = this.inputBuffer.indexOf("--------");
        if (index < 0) break;

        this.mappingStructure = this.inputBuffer
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
        this.inputBuffer = this.inputBuffer.slice(index + 9);
        this.status = STATUS_INITIALIZED;
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

  async handleStateChange() {
    const newBuffer = Buffer.alloc(sharedMemorySize);
    await this.shmFd.read(newBuffer, 0, sharedMemorySize, 0);

    // Check if the buffer has changed
    if (newBuffer.equals(this.shmBuffer)) return;
    this.shmBuffer = newBuffer;

    const newState = {};

    this.mappingStructure.forEach(({ key, offset, type }) => {
      let value;
      switch (type) {
        case "uint8_t":
          value = this.shmBuffer.readUInt8(offset);
          break;
        case "uint16_t":
          value = this.shmBuffer.readUInt16LE(offset);
          break;
        case "uint32_t":
          value = this.shmBuffer.readUInt32LE(offset);
          break;
        case "float":
          value = this.shmBuffer.readFloatLE(offset);
          break;
        case "double":
          value = this.shmBuffer.readDoubleLE(offset);
          break;
        case "bool":
          value = this.shmBuffer.readUInt8(offset) !== 0;
          break;
        case "uint16_t[16]":
          value = [];
          for (let i = 0; i < 16; i++) {
            value.push(this.shmBuffer.readUInt16LE(offset + i * 2));
          }
          break;
        case "double[16]":
          value = [];
          for (let i = 0; i < 16; i++) {
            value.push(this.shmBuffer.readDoubleLE(offset + i * 8));
          }
          break;
        default:
          // Ignore unknown types
          return;
      }

      // Build the state object
      let obj = newState;
      for (let i = 0; i < key.length - 1; i++) {
        if (!obj[key[i]]) obj[key[i]] = {};
        obj = obj[key[i]];
      }
      obj[key[key.length - 1]] = value;
    });

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
    fs.writeSync(this.pipeFd, command + "\n");
  }
}

module.exports = Robot;
