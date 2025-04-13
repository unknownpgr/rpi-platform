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

  private state: RobotState = {
    state: RobotStatus.IDLE,
    sensor_state: {
      state: 0,
      raw_data: Array(16).fill(0),
      sensor_data: Array(16).fill(0),
      calibration: {
        sensor_low: Array(16).fill(0),
        sensor_high: Array(16).fill(0),
      },
      is_calibrated: false,
    },
    drive_state: {
      position: 0,
      speed: 0,
      battery_voltage: 0,
    },
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
        this.notify({ type: "input", data: data.data });
        break;
    }
  }

  private handleWebSocketDisconnect() {
    this.ws?.close();
    this.connectionStatus = "connecting";
    this.notify({ type: "connectionStatus", data: this.connectionStatus });
    this.initializeWebSocket();
  }

  private notify(data: ServerEvent) {
    this.listeners.forEach((listener) => listener(data));
  }

  public addListener(listener: (data: ServerEvent) => void) {
    this.listeners.push(listener);
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
