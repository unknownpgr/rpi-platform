export enum RobotStatus {
  EXIT = 0x00,
  IDLE = 0x01,
  CALI_LOW = 0x02,
  CALI_HIGH = 0x03,
  DRIVE = 0x04,
}

export interface RobotState {
  state: RobotStatus;
  sensor_low: number[];
  sensor_high: number[];
  sensor_raw: number[];
  sensor_data: number[];
  position: number;
  speed: number;
  battery_voltage: number;
  encoder_left: number;
  encoder_right: number;
}
