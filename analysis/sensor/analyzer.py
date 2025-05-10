#! /usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

with open("sensor_history-2.txt", "r") as file:
    sensor_history = np.loadtxt(file)

sample = False
if sample:
    sensor_history = sensor_history[::16, :]


filter_mode = "median"

if filter_mode == "iir":
    iir_gain = 0.005
    filtered = [np.zeros(16)]
    for i in range(1, len(sensor_history)):
        new_row = iir_gain * sensor_history[i] + (1 - iir_gain) * filtered[-1]
        filtered.append(new_row)
    sensor_history = np.array(filtered)
elif filter_mode == "mean":
    n_steps = 100
    shifted = []
    for i in range(n_steps, len(sensor_history)):
        shifted.append(sensor_history[i - n_steps : i])
    stacked = np.stack(shifted, axis=0)
    sensor_history = stacked.mean(axis=1)
elif filter_mode == "median":
    n_steps = 50
    shifted = []
    for i in range(n_steps, len(sensor_history)):
        shifted.append(sensor_history[i - n_steps : i])
    stacked = np.stack(shifted, axis=0)
    sensor_history = np.median(stacked, axis=1)


threshold = 0.1

fig, ax = plt.subplots()
(line,) = ax.plot([], [])
(binary_line,) = ax.plot([], [])
ax.axhline(threshold, color="red", linestyle="--")
ax.set_xlim(0, 15)
ax.set_ylim(sensor_history.min(), sensor_history.max())
ax.set_xlabel("Sensor Index")
ax.set_ylabel("Value")
ax.set_title("Sensor Data Animation")

frame_text = ax.text(0.02, 0.95, "", transform=ax.transAxes)


def update(frame):
    x = np.arange(16)
    y = sensor_history[frame]
    line.set_data(x, y)
    binary_line.set_data(x, y > threshold)
    frame_text.set_text(f"Frame: {frame}/{len(sensor_history)-1}")
    return (line, binary_line, frame_text)


ani = FuncAnimation(fig, update, frames=len(sensor_history), interval=1, blit=True)

plt.show()
