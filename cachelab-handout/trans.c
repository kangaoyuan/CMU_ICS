/*
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 *
 * 10(cache bits) == 5(set bits) + 5(block bits);
 */
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);

/*
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded.
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    if (N == 32 && M == 32) {
        // Here 8 is selected by calculation.
        const int len = 8;
        for (int i = 0; i < N; i += len) {
            for (int j = 0; j < M; j += len) {
                /*
                 *for (int ii = i; ii < i + len; ++ii) {
                 *    for(int jj = j; jj < j + len; ++jj){
                 *        int tmp = A[ii][jj];
                 *        B[jj][ii] = tmp;
                 *        //B[jj][ii] = A[ii][jj];
                 *    }
                 *}
                 */

                /*
                 *for (int k = 0; k < 8; ++k) {
                 *    int a = A[i + k][j];
                 *    int b = A[i + k][j + 1];
                 *    int c = A[i + k][j + 2];
                 *    int d = A[i + k][j + 3];
                 *    int e = A[i + k][j + 4];
                 *    int f = A[i + k][j + 5];
                 *    int g = A[i + k][j + 6];
                 *    int h = A[i + k][j + 7];
                 *    B[j][i + k] = a;
                 *    B[j + 1][i + k] = b;
                 *    B[j + 2][i + k] = c;
                 *    B[j + 3][i + k] = d;
                 *    B[j + 4][i + k] = e;
                 *    B[j + 5][i + k] = f;
                 *    B[j + 6][i + k] = g;
                 *    B[j + 7][i + k] = h;
                 *}
                 */

                for (int k = 0; k < len; ++k) {
                    int a = A[i + k][j];
                    int b = A[i + k][j + 1];
                    int c = A[i + k][j + 2];
                    int d = A[i + k][j + 3];
                    int e = A[i + k][j + 4];
                    int f = A[i + k][j + 5];
                    int g = A[i + k][j + 6];
                    int h = A[i + k][j + 7];
                    B[j + k][i] = a;
                    B[j + k][i + 1] = b;
                    B[j + k][i + 2] = c;
                    B[j + k][i + 3] = d;
                    B[j + k][i + 4] = e;
                    B[j + k][i + 5] = f;
                    B[j + k][i + 6] = g;
                    B[j + k][i + 7] = h;
                }

                for (int k = 0; k < len; k++) {
                    for (int l = 0; l < k; l++) {
                        int tmp = B[j + k][i + l];
                        B[j + k][i + l] = B[j + l][i + k];
                        B[j + l][i + k] = tmp;
                    }
                }
            }
        }
    }

    else if (N == 64 && M == 64) {
        // Here 4 is selected by calculation.
        const int len = 4;
        for (int i = 0; i < N; i += 2 * len) {
            for (int j = 0; j < M; j += 2 * len) {
                for (int k = 0; k < len; ++k) {
                    int a = A[i + k][j];
                    int b = A[i + k][j + 1];
                    int c = A[i + k][j + 2];
                    int d = A[i + k][j + 3];
                    int e = A[i + k][j + 4];
                    int f = A[i + k][j + 5];
                    int g = A[i + k][j + 6];
                    int h = A[i + k][j + 7];
                    B[j][i + k] = a;
                    B[j + 1][i + k] = b;
                    B[j + 2][i + k] = c;
                    B[j + 3][i + k] = d;
                    B[j][i + k + len] = e;
                    B[j + 1][i + k + len] = f;
                    B[j + 2][i + k + len] = g;
                    B[j + 3][i + k + len] = h;
                }
                for (int k = 0; k < len; ++k) {
                    int a = B[j + k][i + 4];
                    int b = B[j + k][i + 5];
                    int c = B[j + k][i + 6];
                    int d = B[j + k][i + 7];

                    int e = A[i + 4][j + k];
                    int f = A[i + 5][j + k];
                    int g = A[i + 6][j + k];
                    int h = A[i + 7][j + k];

                    B[j + k][i + 4] = e;
                    B[j + k][i + 5] = f;
                    B[j + k][i + 6] = g;
                    B[j + k][i + 7] = h;

                    B[j + k + len][i] = a;
                    B[j + k + len][i + 1] = b;
                    B[j + k + len][i + 2] = c;
                    B[j + k + len][i + 3] = d;

                    B[j + k + len][i + 4] = A[i + 4][j + k + len];
                    B[j + k + len][i + 5] = A[i + 5][j + k + len];
                    B[j + k + len][i + 6] = A[i + 6][j + k + len];
                    B[j + k + len][i + 7] = A[i + 7][j + k + len];
                }
            }
        }
    }

    else{
        // 61 * 67
        // Here 8 is selected by calculation.
        const int len = 8;
        int M_ = M - (M % len);
        for (int j = 0; j < M_; j += len) {
            for (int k = 0; k < N; ++k) {
                int a = A[k][j];
                int b = A[k][j + 1];
                int c = A[k][j + 2];
                int d = A[k][j + 3];
                int e = A[k][j + 4];
                int f = A[k][j + 5];
                int g = A[k][j + 6];
                int h = A[k][j + 7];

                B[j][k] = a;
                B[j + 1][k] = b;
                B[j + 2][k] = c;
                B[j + 3][k] = d;
                B[j + 4][k] = e;
                B[j + 5][k] = f;
                B[j + 6][k] = g;
                B[j + 7][k] = h;
            }
        }

        for (int i = 0; i < N; i++) {
            for (int j = M_; j < M; j++) {
                int tmp = A[i][j];
                B[j][i] = tmp;
            }
        }
    }
}

/*
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started.
 */

/*
 * trans - A simple baseline transpose function, not optimized for the
 * cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
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
void registerFunctions() {
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
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
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
