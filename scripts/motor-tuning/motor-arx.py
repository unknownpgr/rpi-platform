#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import matplotlib.pyplot as plt
from scipy.linalg import solve_discrete_are


def read_data():
    with open("motor-arx-data", "r") as file:
        data = file.read()
    parts = data.split("----------")
    data = []
    for row in parts[1].split("\n"):
        if row.strip():
            row = row.split(",")
            row = [float(x.strip()) for x in row]
            data.append(row)
    data = np.array(data)
    return data


def solve_arx(data, arx_order):
    rows = len(data) - arx_order
    cols = arx_order * 2

    # Create the regressor matrix
    Y = []
    D = []
    for i in range(rows):
        row = []
        for j in range(arx_order):
            row.append(data[i + j, 1])
        for j in range(arx_order):
            row.append(data[i + j, 0])
        D.append(row)
        Y.append(data[i + arx_order, 1])
    Y = np.array(Y)
    D = np.array(D)

    # Solve the least squares problem
    theta = np.linalg.lstsq(D, Y, rcond=None)[0]
    return theta


def get_lqr(arx_gains):
    """
    Example of a discrete-time state-space system with augmented state

    x_aug = ┌        ┐
            │ y[t]   │ # current output
            │ y[t-1] │ # previous output
            │ u[t-1] │ # previous input
            │ z[t]   │ # cumulative error (sum of r[t]-y[t])
            └        ┘

    x_aug[t+1] = A_aug * x_aug[t] + B_aug * u[t] + E * r[t]

    A_aug = ┌                ┐
            │ x0  x1  x3   0 │
            │  1   0   0   0 │
            │  0   0   0   0 │
            │ -1   0   0   1 │
            └                ┘

    B_aug = ┌     ┐
            │ x2  │
            │  0  │
            │  1  │
            │  0  │
            └     ┘

    E =     ┌     ┐
            │  0  │
            │  0  │
            │  0  │
            │  1  │
            └     ┘

    """

    assert len(arx_gains) % 2 == 0
    arx_order = len(arx_gains) // 2

    y_gains = list(arx_gains[:arx_order])
    u_gains = list(arx_gains[arx_order:])

    # Define the system matrices
    A = []
    A.append(y_gains + u_gains[1:] + [0])
    for i in range(1, arx_order):
        A.append([0] * (i - 1) + [1] + [0] * (arx_order * 2 - i))
    for i in range(arx_order - 1):
        A.append([0] * arx_order * 2)
    A.append([0] * arx_order * 2)
    A[-1][0] = -1
    A[-1][-1] = 1
    A = np.array(A)

    B = []
    B.append([u_gains[0]])
    for i in range(1, arx_order * 2):
        B.append([0])
    B[-2][0] = 1
    B = np.array(B)

    # Define the cost function matrices

    # State cost
    Q = np.array(
        [
            [1, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 0],
            [0, 0, 0, 1],
        ]
    )

    # Control cost
    R = np.array([[0.1]])

    # Solve the discrete algebraic Riccati equation
    P = solve_discrete_are(A, B, Q, R)

    # Calculate the optimal gain matrix K
    K = np.linalg.inv(B.T @ P @ B + R) @ (B.T @ P @ A)

    return K


data = read_data()
N = len(data)
y = data[:, 1]
u = data[:, 0]

# ARX model
ARX_ORDER = 2
theta = solve_arx(data, ARX_ORDER)
print("ARX coefficients:")
print(theta)

# LQR controller
K = get_lqr(theta)
print("LQR controller gains:")
print(K)

# Simulate the system
y_sim = np.zeros(N)
y_sim[:ARX_ORDER] = y[:ARX_ORDER]

for i in range(ARX_ORDER, N):
    # Calculate the output using the ARX model
    y_sim[i] = np.dot(theta[:ARX_ORDER], y_sim[i - ARX_ORDER : i]) + np.dot(
        theta[ARX_ORDER:], u[i - ARX_ORDER : i]
    )

# Plot the results
plt.figure(figsize=(10, 5))

plt.subplot(2, 1, 1)
plt.plot(u, label="u")
plt.xlabel("Time")
plt.ylabel("Input")

plt.subplot(2, 1, 2)
plt.plot(y, label="Real Output")
plt.xlabel("Time")
plt.ylabel("Output")
plt.plot(y_sim, label="Simulated Output")
plt.legend()

plt.tight_layout()
plt.show()
plt.clf()
