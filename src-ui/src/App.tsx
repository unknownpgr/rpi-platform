import { Terminal } from "@xterm/xterm";
import { useEffect, useRef } from "react";
import { Button } from "./Button";
import { Card } from "./Card";
import { Server } from "./core/server";
import { RobotStatus } from "./core/types";
import { useServer } from "./useServer";
function SensorDataRow({
  low,
  high,
  raw,
  data,
}: {
  low: number;
  high: number;
  raw: number;
  data: number;
}) {
  const max = 4096;
  const lowPosition = (low / max) * 100;
  const highPosition = (high / max) * 100;
  const rawPosition = (raw / max) * 100;
  const dataPosition = data * 100;
  return (
    <div className="flex flex-col gap-2 w-0 grow-1">
      <div className="relative h-[100px] w-full bg-gray-700 rounded-sm">
        <div
          className="absolute bottom-0 left-0 w-full bg-green-300 rounded-sm"
          style={{
            height: `${dataPosition}%`,
          }}></div>
      </div>
      <div className="w-full text-center text-white text-xs font-bold">
        {Math.round(data * 100)
          .toString()
          .padStart(2, "0")}
      </div>
      <div className="relative h-[100px] w-full bg-gray-700 rounded-sm">
        <div
          className="absolute bottom-0 left-0 w-full h-[1px] bg-gray-500"
          style={{
            height: `${rawPosition}%`,
          }}></div>
        <div
          className="absolute left-0 w-full h-[1px] bg-gray-400"
          style={{
            bottom: `${lowPosition}%`,
          }}></div>
        <div
          className="absolute left-0 w-full h-[1px] bg-gray-400 rounded-sm"
          style={{
            bottom: `${highPosition}%`,
          }}></div>
      </div>
    </div>
  );
}

function Speedometer({ speed, maxSpeed }: { speed: number; maxSpeed: number }) {
  const speedPercentage = (speed / maxSpeed) * 100;
  const angle = (speedPercentage / 100) * 240 - 120;

  return (
    <div className="flex flex-col items-center justify-center w-full">
      <div className="relative h-[150px] w-[200px] mb-4 overflow-hidden">
        <div className="absolute top-0 left-0 w-[200px] h-[200px] bg-gray-700 rounded-full overflow-hidden">
          {/* Needle */}
          <div
            className="absolute left-1/2 bottom-1/2 h-[180px] w-[2px] bg-red-400 origin-bottom transition-transform duration-300"
            style={{
              transform: `translateX(-50%) rotate(${angle}deg)`,
            }}
          />

          {/* Center point */}
          <div className="absolute bottom-0 left-1/2 bottom-1/2 w-[120px] h-[120px] bg-gray-800 rounded-full transform -translate-x-1/2 translate-y-[50%]" />

          {/* Triangle */}
          <div
            className="absolute bottom-0 left-1/2 bottom-1/2 w-[200px] h-[200px] bg-gray-800 transform -translate-x-1/2 translate-y-[100%]"
            style={{
              clipPath: `polygon(50% 0%, -123.2% 100%, 223.2% 100%)`,
            }}
          />
        </div>
        <div className="absolute left-1/2 top-1/2 text-white text-2xl font-bold -translate-x-1/2 translate-y-1/2">
          {speed.toFixed(2)}
        </div>
      </div>
    </div>
  );
}

function PositionMeter({ position }: { position: number }) {
  const positionPercentage = position * 50 + 50;

  return (
    <div className="flex flex-col items-center justify-center gap-4 items-center">
      <div className="relative h-[10px] w-80 bg-gray-700 rounded-sm">
        <div
          className="absolute top-0 left-0 w-[10px] h-full bg-green-300 rounded-sm"
          style={{
            left: `${positionPercentage}%`,
            transform: "translateX(-50%)",
          }}></div>
      </div>
      <div className="text-white text-2xl font-bold text-center">
        {position.toFixed(2)}
      </div>
    </div>
  );
}

function BatteryMeter({ voltage }: { voltage: number }) {
  let cellNumberGuess = Math.floor(voltage / 3.88);
  if (cellNumberGuess < 1) {
    cellNumberGuess = 1;
  }
  const minVoltage = cellNumberGuess * 3.7;
  const maxVoltage = cellNumberGuess * 4.2;

  let voltagePercentage = (voltage - minVoltage) / (maxVoltage - minVoltage);
  if (voltagePercentage < 0) {
    voltagePercentage = 0;
  }
  if (voltagePercentage > 1) {
    voltagePercentage = 1;
  }
  voltagePercentage = voltagePercentage * 100;

  return (
    <div>
      <div className="flex flex-row items-center justify-center gap-4 items-center">
        <div className="flex flex-col items-center justify-center gap-1 w-0 grow-1">
          <div className="text-white text-2xl font-bold text-center">
            {voltage.toFixed(2)}V
          </div>
          <div className="text-gray-400 text-sm font-bold text-center">
            {(voltage / cellNumberGuess).toFixed(2)}V per cell
          </div>
        </div>
        <div className="relative h-[40px] w-80 bg-gray-700 rounded-sm">
          <div
            className="absolute top-0 left-0 h-full bg-green-300 rounded-sm"
            style={{
              width: `${voltagePercentage}%`,
            }}></div>
        </div>
        <div className="text-white text-2xl font-bold text-center w-0 grow-1">
          {Math.round(voltagePercentage)}%
        </div>
      </div>
      <div className="text-gray-400 text-sm font-bold text-center mt-2">
        Detected: {cellNumberGuess} cells ({minVoltage.toFixed(2)}V -{" "}
        {maxVoltage.toFixed(2)}V)
      </div>
    </div>
  );
}

const server = new Server();

function App() {
  useServer(server);

  const state = server.getState();

  const terminalElementRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (terminalElementRef.current) {
      const terminal = new Terminal({
        fontSize: 12,
        cols: 80,
        rows: 12,
      });
      terminal.open(terminalElementRef.current);

      const removeListener = server.addListener((event) => {
        if (event.type === "input") terminal.write(event.data);
      });

      return () => {
        removeListener();
        terminal.dispose();
      };
    }
  }, [terminalElementRef.current]);

  if (server.getConnectionStatus() === "connecting") {
    return (
      <div className="p-10 container mx-auto">
        <h1 className="text-xl font-bold text-white mb-4">
          RPi-Platform Dashboard
        </h1>
        <div className="text-white text-2xl font-bold">
          Connecting to server...
        </div>
      </div>
    );
  }

  if (server.getServerStatus() === "INITIALIZING") {
    return (
      <div className="p-10 container mx-auto">
        <h1 className="text-xl font-bold text-white mb-4">
          RPi-Platform Dashboard
        </h1>
        <div className="text-white text-2xl font-bold">
          Server is initializing...
        </div>
      </div>
    );
  }

  if (server.getServerStatus() === "ERROR") {
    return (
      <div className="p-10 container mx-auto">
        <h1 className="text-xl font-bold text-white mb-4">
          RPi-Platform Dashboard
        </h1>
        <div className="text-white text-2xl font-bold">
          Server is in error state.
        </div>
      </div>
    );
  }

  return (
    <div className="p-10 container mx-auto">
      <h1 className="text-xl font-bold text-white mb-4">
        RPi-Platform Dashboard
      </h1>
      <div className="grid grid-cols-2 gap-4">
        <Card title="Robot State">
          <div className="text-white text-2xl font-bold">
            {RobotStatus[state.state]}
          </div>
        </Card>
        <Card title="Control">
          <div className="flex flex-wrap gap-2">
            <Button
              onClick={() => server.sendCommand("idle")}
              variant={
                state.state === RobotStatus.IDLE ? "activated" : "default"
              }>
              IDLE
            </Button>
            <Button
              variant={
                state.state === RobotStatus.CALI_LOW ? "activated" : "default"
              }
              onClick={() => server.sendCommand("cali_low")}>
              CALIBRATE_LOW
            </Button>
            <Button
              variant={
                state.state === RobotStatus.CALI_HIGH ? "activated" : "default"
              }
              onClick={() => server.sendCommand("cali_high")}>
              CALIBRATE_HIGH
            </Button>
            <Button onClick={() => server.sendCommand("cali_save")}>
              SAVE_CALIBRATION
            </Button>
            <Button
              variant={
                state.state === RobotStatus.DRIVE ? "activated" : "default"
              }
              onClick={() => server.sendCommand("drive")}>
              DRIVE
            </Button>
            <Button onClick={() => server.sendCommand("quit")}>QUIT</Button>
          </div>
        </Card>
        <Card title="Sensor Data">
          <div className="flex flex-row gap-2">
            {state.sensor_data.map((data, index) => (
              <SensorDataRow
                key={index}
                low={state.sensor_low[index]}
                high={state.sensor_high[index]}
                raw={state.sensor_raw[index]}
                data={data}
              />
            ))}
          </div>
        </Card>
        <Card title="Drive State">
          <Speedometer speed={state.speed} maxSpeed={10} />
          <br />
          <PositionMeter position={state.position} />
        </Card>
        <Card title="Battery Voltage">
          <BatteryMeter voltage={state.battery_voltage} />
        </Card>
        <Card title="Terminal">
          <div className="rounded-md overflow-hidden p-2 bg-black">
            <div ref={terminalElementRef} />
          </div>
        </Card>
      </div>
    </div>
  );
}

export default App;
