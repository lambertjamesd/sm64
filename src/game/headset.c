#include "headset.h"

#include <ultra64.h>

float gHeadsetRotation[4][4] = {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1},
};

float gHeadsetRecenter[4][4] = {
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1},
};

void matrixMul(float a[4][4], float b[4][4], float output[4][4]) {
    int x, y, j;

    for (x = 0; x < 4; ++x) {
        for (y = 0; y < 4; ++y) {
            output[x][y] = 0.0f;
            for (j = 0; j < 4; ++j) {
                output[x][y] += a[j][y] * b[x][j];
            }
        }
    }
}

void headset_read_orientation(float rotation[4][4]) {
    matrixMul(gHeadsetRotation, gHeadsetRecenter, rotation);
}

void headset_recenter() {
    guMtxIdentF(gHeadsetRecenter);

    gHeadsetRecenter[0][2] = gHeadsetRotation[2][0];
    gHeadsetRecenter[2][2] = gHeadsetRotation[2][2];

    guNormalize(&gHeadsetRecenter[0][2], &gHeadsetRecenter[1][2], &gHeadsetRecenter[2][2]);

    // side = up x forward
    gHeadsetRecenter[0][0] = gHeadsetRecenter[1][1] * gHeadsetRecenter[2][2] - gHeadsetRecenter[2][1] * gHeadsetRecenter[1][2];
    gHeadsetRecenter[1][0] = gHeadsetRecenter[2][1] * gHeadsetRecenter[0][2] - gHeadsetRecenter[0][1] * gHeadsetRecenter[2][2];
    gHeadsetRecenter[2][0] = gHeadsetRecenter[0][1] * gHeadsetRecenter[1][2] - gHeadsetRecenter[1][1] * gHeadsetRecenter[0][2];
}
