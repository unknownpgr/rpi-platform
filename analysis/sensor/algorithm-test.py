#! /usr/bin/env python3
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

with open("sensor_history-2.txt", "r") as file:
    sensor_history = np.loadtxt(file)

sensor_history = sensor_history[::16, :]
sensor_history = sensor_history[1000:1500, :]


def mu(ds):
    ds = 1 - np.abs(ds) * 3
    ds = np.maximum(ds, 0)
    return ds


def optimized(values, positions, mu, xs):
    n = len(values)
    result = np.zeros(len(xs))
    for i in range(n):
        result += (values[i] - mu(xs - positions[i])) ** 2
    return result


def prior(positions, current_position):
    p1 = ((positions - current_position) ** 2) * 8
    p2 = (positions**2) * 4
    return p1 + p2


fig, ax = plt.subplots()
(likelihood,) = ax.plot([], [])
(sensor_values,) = ax.plot([], [])
(estimated_position,) = ax.plot([], [], color="red")
frame_text = ax.text(0.02, 0.95, "", transform=ax.transAxes)
ax.set_xlim(-1, 1)
ax.set_ylim(0, 1)
ax.set_xlabel("Sensor Position")
ax.set_ylabel("Value")
ax.set_title("Sensor Data Animation")

ps = np.linspace(-1, 1, 16)  # sensor positions
xs = np.linspace(-1, 1, 100)  # candidate positions

current_position = 0


def update(frame):
    global current_position

    vs = sensor_history[frame, :]
    ys = optimized(vs, ps, mu, xs)
    prior_values = prior(xs, current_position)
    posterior_values = ys + prior_values
    x_hat = xs[np.argmin(posterior_values)]
    current_position = x_hat

    likelihood_values = np.exp(-posterior_values)
    likelihood_values = likelihood_values / np.sum(likelihood_values)
    likelihood_values *= 10

    likelihood.set_data(xs, likelihood_values)
    sensor_values.set_data(ps, vs)
    frame_text.set_text(f"T={frame}")
    estimated_position.set_data([x_hat, x_hat], [0, 1])
    return (likelihood, sensor_values, estimated_position, frame_text)


ani = FuncAnimation(
    fig, update, frames=range(sensor_history.shape[0]), interval=100, blit=True
)
plt.show()
