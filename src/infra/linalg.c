#include <linalg.h>

#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))

extern void dgelss_(
    int *m, int *n, int *nrhs,
    double *A, int *lda,
    double *B, int *ldb,
    double *S, double *rcond, int *rank,
    double *work, int *lwork,
    int *info);

bool pseudo_inverse(int y_dim, int x_dim, double *D_original, double *y_original, double *x)
{
    // Transpose and copy D to get the correct dimensions for LAPACK
    // because LAPACK expects column-major order and it will destroy the original D
    double *D = (double *)malloc(x_dim * y_dim * sizeof(double));
    for (int i = 0; i < y_dim; i++)
    {
        for (int j = 0; j < x_dim; j++)
        {
            D[j * y_dim + i] = D_original[i * x_dim + j];
        }
    }

    // Likewise, clone y to avoid modifying the original y
    double *y = (double *)malloc(y_dim * sizeof(double));
    for (int i = 0; i < y_dim; i++)
    {
        y[i] = y_original[i];
    }

    // Parameters for dgelss_
    int m = y_dim;                                            // Number of rows in D
    int n = x_dim;                                            // Number of columns in D
    int nrhs = 1;                                             // Number of right-hand sides (columns of B)
    int lda = m;                                              // Number of rows in D
    int ldb = m;                                              // Number of rows in y
    double rcond = -1.0;                                      // Condition number threshold for rank determination
    int rank;                                                 // Calculated rank of the matrix
    int info;                                                 // Error flag
    int lwork = -1;                                           // Workspace size
    double *S = (double *)malloc(MIN(m, n) * sizeof(double)); // Singular values

    // Query the optimal workspace size
    {
        double wkopt;
        dgelss_(&m, &n, &nrhs, D, &lda, y, &ldb, S, &rcond, &rank, &wkopt, &lwork, &info);
        lwork = (int)wkopt;
    }

    // Allocate workspace
    double *work = (double *)malloc(lwork * sizeof(double));

    // Solve the least squares problem
    dgelss_(&m, &n, &nrhs, D, &lda, y, &ldb, S, &rcond, &rank, work, &lwork, &info);

    // Error in dgelss_
    if (info != 0)
    {
        free(S);
        free(work);
        return false;
    }

    // Copy the result to x
    for (int i = 0; i < n; i++)
    {
        x[i] = y[i];
    }

    // Free allocated memory
    free(S);
    free(work);
    free(D);
    free(y);

    // Return true if the rank is equal to the minimum of m and n
    // indicating that the matrix is full rank
    // and the pseudo-inverse was computed successfully
    return (rank == MIN(m, n));
}
