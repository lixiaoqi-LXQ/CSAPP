/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>
#include "cachelab.h"

#define __DEBUG__

#define MN3232_BX 8
#define MN3232_BY 8

#define MN6464_BX 4
#define MN6464_BY 4

#define MN6167_BX 16
#define MN6167_BY 16

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
    int I, J;
    int di, dj;
    int v1, v2, v3, v4, v5, v6, v7, v8;

    if (M == 32 && N == 32) {
        for (I = 0; I < M; I += MN3232_BX) {
            for (J = 0; J < N; J += MN3232_BY) {
                for (di = 0; di < MN3232_BX; di++) {
                    v1               = A[I + di][J + 0];
                    v2               = A[I + di][J + 1];
                    v3               = A[I + di][J + 2];
                    v4               = A[I + di][J + 3];
                    v5               = A[I + di][J + 4];
                    v6               = A[I + di][J + 5];
                    v7               = A[I + di][J + 6];
                    v8               = A[I + di][J + 7];
                    B[J + 0][I + di] = v1;
                    B[J + 1][I + di] = v2;
                    B[J + 2][I + di] = v3;
                    B[J + 3][I + di] = v4;
                    B[J + 4][I + di] = v5;
                    B[J + 5][I + di] = v6;
                    B[J + 6][I + di] = v7;
                    B[J + 7][I + di] = v8;
                    /* for (dj = 0; dj < MN3232_BY; dj++) {
                     *     if ((J + dj) >= N)
                     *         break;
                     *     B[J + dj][I + di] = A[I + di][J + dj];
                     * } */
                }
            }
        }

    } else if (M == 64 && N == 64) {
        {
            for (I = 0; I < N; I += 8)
                for (J = 0; J < M; J += 8) {
                    for (di = I; di < I + 4; ++di) {
                        v1               = A[di][J + 0];
                        v2               = A[di][J + 1];
                        v3               = A[di][J + 2];
                        v4               = A[di][J + 3];
                        v5               = A[di][J + 4];
                        v6               = A[di][J + 5];
                        v7               = A[di][J + 6];
                        v8               = A[di][J + 7];

                        B[J + 0][di]     = v1;
                        B[J + 1][di]     = v2;
                        B[J + 2][di]     = v3;
                        B[J + 3][di]     = v4;
                        B[J + 0][di + 4] = v5;
                        B[J + 1][di + 4] = v6;
                        B[J + 2][di + 4] = v7;
                        B[J + 3][di + 4] = v8;
                    }
                    for (dj = J; dj < J + 4; ++dj) {
                        v1               = A[I + 4][dj];
                        v2               = A[I + 5][dj];
                        v3               = A[I + 6][dj];
                        v4               = A[I + 7][dj];
                        v5               = B[dj][I + 4];
                        v6               = B[dj][I + 5];
                        v7               = B[dj][I + 6];
                        v8               = B[dj][I + 7];

                        B[dj][I + 4]     = v1;
                        B[dj][I + 5]     = v2;
                        B[dj][I + 6]     = v3;
                        B[dj][I + 7]     = v4;
                        B[dj + 4][I]     = v5;
                        B[dj + 4][I + 1] = v6;
                        B[dj + 4][I + 2] = v7;
                        B[dj + 4][I + 3] = v8;
                    }
                    for (di = I + 4; di < I + 8; ++di) {
                        v1           = A[di][J + 4];
                        v2           = A[di][J + 5];
                        v3           = A[di][J + 6];
                        v4           = A[di][J + 7];
                        B[J + 4][di] = v1;
                        B[J + 5][di] = v2;
                        B[J + 6][di] = v3;
                        B[J + 7][di] = v4;
                    }
                }
        }
    } else if (M == 61 && N == 67) {
        for (J = 0; J < N; J += MN6167_BY) {
            for (I = 0; I < M; I += MN6167_BX) {
                for (dj = 0; dj < MN6167_BY; dj++) {
                    if ((J + dj) >= N)
                        break;
                    for (di = 0; di < MN6167_BX;) {
                        if ((I + di) >= M)
                            break;
                        if (I + di + 8 >= M) {
                            A[J + dj][I + di] = B[I + di][J + dj];
                            di++;
                        } else {
                            v1                    = A[J + dj][I + di + 0];
                            v2                    = A[J + dj][I + di + 1];
                            v3                    = A[J + dj][I + di + 2];
                            v4                    = A[J + dj][I + di + 3];
                            v5                    = A[J + dj][I + di + 4];
                            v6                    = A[J + dj][I + di + 5];
                            v7                    = A[J + dj][I + di + 6];
                            v8                    = A[J + dj][I + di + 7];
                            B[I + di + 0][J + dj] = v1;
                            B[I + di + 1][J + dj] = v2;
                            B[I + di + 2][J + dj] = v3;
                            B[I + di + 3][J + dj] = v4;
                            B[I + di + 4][J + dj] = v5;
                            B[I + di + 5][J + dj] = v6;
                            B[I + di + 6][J + dj] = v7;
                            B[I + di + 7][J + dj] = v8;
                            di += 8;
                        }
                    }
                }
            }
        }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp     = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions()
{
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/*
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
