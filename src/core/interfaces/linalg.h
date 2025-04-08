#pragma once
#include <stdbool.h>

/**
 * Solve the least squares problem for the equation D * x = y.
 * D is a matrix of size (y_dim, x_dim), row-major,
 * y is a vector of size (y_dim), and x is a vector of size (x_dim).
 * x will be filled with the solution.
 *
 * If it fails, it returns false.
 */
bool pseudo_inverse(int y_dim, int x_dim, double *D, double *y, double *x);