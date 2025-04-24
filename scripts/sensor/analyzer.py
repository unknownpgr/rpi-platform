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

# 그래프 설정
fig, ax = plt.subplots()
(line,) = ax.plot([], [])
(binary_line,) = ax.plot([], [])
ax.axhline(threshold, color="red", linestyle="--")
ax.set_xlim(0, 15)  # 0부터 15까지의 x축 범위
ax.set_ylim(
    sensor_history.min(), sensor_history.max()
)  # y축 범위는 데이터의 최소/최대값으로 설정
ax.set_xlabel("Sensor Index")
ax.set_ylabel("Value")
ax.set_title("Sensor Data Animation")

# 프레임 카운터 텍스트 추가
frame_text = ax.text(0.02, 0.95, "", transform=ax.transAxes)


# 애니메이션 함수
def update(frame):
    x = np.arange(16)  # 0부터 15까지의 x값
    y = sensor_history[frame]  # 현재 프레임의 데이터
    line.set_data(x, y)
    binary_line.set_data(x, y > threshold)
    frame_text.set_text(f"Frame: {frame}/{len(sensor_history)-1}")
    return (line, binary_line, frame_text)


# 애니메이션 생성
ani = FuncAnimation(
    fig, update, frames=len(sensor_history), interval=1, blit=True
)  # 1ms 간격으로 업데이트

plt.show()
