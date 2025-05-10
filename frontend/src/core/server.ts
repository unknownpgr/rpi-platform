import { RobotStatus } from "./types";

import { RobotState } from "./types";

export type ConnectionStatus = "connecting" | "connected";

export type ServerStatus = "INITIALIZING" | "INITIALIZED" | "ERROR";

export type ServerEvent =
  | {
      type: "serverStatus";
      data: ServerStatus;
    }
  | {
      type: "connectionStatus";
      data: ConnectionStatus;
    }
  | {
      type: "input";
      data: string;
    }
  | {
      type: "state";
      data: RobotState;
    };

export class Server {
  private ws: WebSocket | null = null;

  private connectionStatus: ConnectionStatus = "connecting";
  private serverStatus: ServerStatus = "INITIALIZING";
  private inputs: ServerEvent[] = [];

  private state: RobotState = {
    state: RobotStatus.IDLE,
    sensor_raw: Array(16).fill(0),
    sensor_data: Array(16).fill(0),
    position: 0,
    sensor_low: Array(16).fill(0),
    sensor_high: Array(16).fill(0),
    encoder_left: 0,
    encoder_right: 0,
    speed: 0,
    battery_voltage: 0,
  };
  private listeners: ((data: ServerEvent) => void)[] = [];

  constructor() {
    this.initializeWebSocket();
  }

  private initializeWebSocket() {
    this.ws = new WebSocket("/");
    this.ws.onopen = () => this.handleWebSocketOpen();
    this.ws.onmessage = (event) => this.handleWebSocketMessage(event);
    this.ws.onclose = () => this.handleWebSocketDisconnect();
  }

  private handleWebSocketOpen() {
    this.connectionStatus = "connected";
    this.notify({ type: "connectionStatus", data: this.connectionStatus });
  }

  private handleWebSocketMessage(event: MessageEvent<any>) {
    const data = JSON.parse(event.data);
    switch (data.type) {
      case "status":
        this.serverStatus = data.data;
        this.notify({ type: "serverStatus", data: this.serverStatus });
        break;
      case "state":
        this.state = data.data;
        this.notify({ type: "state", data: this.state });
        break;
      case "input":
        this.inputs.push({ type: "input", data: data.data });
        this.notify({ type: "input", data: data.data });
        break;
      default:
        console.log("Unknown message type:", data.type);
        break;
    }
  }

  private handleWebSocketDisconnect() {
    this.ws?.close();
    this.connectionStatus = "connecting";
    this.notify({ type: "connectionStatus", data: this.connectionStatus });
    setTimeout(() => {
      this.initializeWebSocket();
    }, 1000);
  }

  private notify(data: ServerEvent) {
    this.listeners.forEach((listener) => listener(data));
  }

  public addListener(listener: (data: ServerEvent) => void) {
    this.listeners.push(listener);
    this.inputs.forEach((input) => listener(input));
    return () => {
      this.listeners.filter((l) => l !== listener);
    };
  }

  public sendCommand(command: string) {
    this.ws?.send(command);
  }

  public getConnectionStatus() {
    return this.connectionStatus;
  }

  public getServerStatus() {
    return this.serverStatus;
  }

  public getState() {
    return this.state;
  }
}
