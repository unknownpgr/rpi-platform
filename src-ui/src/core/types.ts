export enum RobotStatus {
  EXIT = 0x00,
  IDLE = 0x01,
  CALIBRATION = 0x02,
  DRIVING = 0x03,
}

export enum SensorStatus {
  READING = 0,
  CALI_LOW = 1,
  CALI_HIGH = 2,
}

export interface SensorCalibration {
  sensor_low: number[];
  sensor_high: number[];
}

export interface SensorState {
  state: SensorStatus;
  raw_data: number[];
  sensor_data: number[];
  calibration: SensorCalibration;
  is_calibrated: boolean;
}

export interface DriveState {
  position: number;
  speed: number;
  battery_voltage: number;
}

export interface RobotState {
  state: RobotStatus;
  sensor_state: SensorState;
  drive_state: DriveState;
}
