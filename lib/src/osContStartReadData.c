#include "libultra_internal.h"
#include "osContInternal.h"
#include "PR/ique.h"
#include <macros.h>
#include "controller.h"

ALIGNED8 OSPifRam __osContPifRam;

extern u8 __osContLastCmd;
extern u8 __osMaxControllers;

extern int gDebugNumber;

void __osPackReadData(int* isOculusConnected);

s32 osContStartReadData(OSMesgQueue *mesg, int* isOculusConnected) {
#ifdef VERSION_CN
    s32 ret;
#else
    s32 ret = 0;
    s32 i;
#endif
    __osSiGetAccess();
    if (__osContLastCmd != CONT_CMD_READ_BUTTON) {
        __osPackReadData(isOculusConnected);
        ret = __osSiRawStartDma(OS_WRITE, __osContPifRam.ramarray);
        osRecvMesg(mesg, NULL, OS_MESG_BLOCK);
    }
#ifndef VERSION_CN
    for (i = 0; i < ARRAY_COUNT(__osContPifRam.ramarray) + 1; i++) {
        __osContPifRam.ramarray[i] = 0xff;
    }
    __osContPifRam.pifstatus = 0;
#endif

    ret = __osSiRawStartDma(OS_READ, __osContPifRam.ramarray);
#ifdef VERSION_CN
    __osContLastCmd = 0xfd;
#else
    __osContLastCmd = CONT_CMD_READ_BUTTON;
#endif
    __osSiRelAccess();
    return ret;
}

void __osContReadOrientation(int* data, float rotationMtx[4][4]) {
    float scale;

    guMtxIdentF(rotationMtx);
    
    // // up vector
    rotationMtx[1][0] = data[0];
    rotationMtx[1][1] = data[1];
    rotationMtx[1][2] = data[2];

    // forward vector
    rotationMtx[2][0] = data[3];
    rotationMtx[2][1] = data[4];
    rotationMtx[2][2] = data[5];

    // normalize up
    guNormalize(&rotationMtx[1][0], &rotationMtx[1][1], &rotationMtx[1][2]);

    scale = rotationMtx[2][0] * rotationMtx[1][0] +
        rotationMtx[2][1] * rotationMtx[1][1] +
        rotationMtx[2][2] * rotationMtx[1][2];

    // modify forward so it is perpendicular to up
    rotationMtx[2][0] -= scale * rotationMtx[1][0];
    rotationMtx[2][1] -= scale * rotationMtx[1][1];
    rotationMtx[2][2] -= scale * rotationMtx[1][2];

    scale = rotationMtx[2][0] * rotationMtx[2][0] +
        rotationMtx[2][1] * rotationMtx[2][1] +
        rotationMtx[2][2] * rotationMtx[2][2];

    if (scale <= 0.00001f && scale >= -0.00001f) {
        guMtxIdentF(rotationMtx);
        return;
    }

    // normalize forward
    guNormalize(&rotationMtx[2][0], &rotationMtx[2][1], &rotationMtx[2][2]);

    // | x  y  z  |
    // | ux uy uz |
    // | fx fy fz |

    // side = up x forward
    rotationMtx[0][0] = rotationMtx[1][1] * rotationMtx[2][2] - rotationMtx[1][2] * rotationMtx[2][1];
    rotationMtx[0][1] = rotationMtx[1][2] * rotationMtx[2][0] - rotationMtx[1][0] * rotationMtx[2][2];
    rotationMtx[0][2] = rotationMtx[1][0] * rotationMtx[2][1] - rotationMtx[1][1] * rotationMtx[2][0];
}

void osContGetReadData(OSContPad *pad, float rotationMtx[4][4], int* isOculusConnected) {
    u8 *cmdBufPtr;
    OSContPackedRead response;
    s32 i;
    cmdBufPtr = (u8 *) __osContPifRam.ramarray;

    response = * (OSContPackedRead *) cmdBufPtr;

    pad->errnum = (response.rxLen & 0xc0) >> 4;
    if (pad->errnum == 0) {
        pad->button = response.button;
        pad->stick_x = response.rawStickX;
        pad->stick_y = response.rawStickY;
    }
    cmdBufPtr += sizeof(OSContPackedRead);
    ++pad;

    for (i = 1; i < __osMaxControllers; i += 1) {
        pad->button = 0;
        pad->errnum = 0;
        pad->stick_x = 0;
        pad->stick_y = 0;
        ++pad;
    }

    // check for read orientation command
    if (cmdBufPtr[3] == 0x16) {
        __osContReadOrientation((int*)(cmdBufPtr + 4), rotationMtx);
    } else {
        guMtxIdentF(rotationMtx);
        // *isOculusConnected = cmdBufPtr[4] != 0x06;
        *isOculusConnected += 1;
    }
}

void __osPackReadData(int* isOculusConnected) {
    u8 *cmdBufPtr;
    OSContPackedRead request;
    s32 i;
    cmdBufPtr = (u8 *) __osContPifRam.ramarray;

#ifdef VERSION_CN
    for (i = 0; i < ARRAY_COUNT(__osContPifRam.ramarray); i++) {
#else
    for (i = 0; i < ARRAY_COUNT(__osContPifRam.ramarray) + 1; i++) {
#endif
        __osContPifRam.ramarray[i] = 0;
    }

    __osContPifRam.pifstatus = 1;
    request.padOrEnd = 255;
    request.txLen = 1;
    request.rxLen = 4;
    request.command = 1;
    request.button = 65535;
    request.rawStickX = -1;
    request.rawStickY = -1;
    * (OSContPackedRead *) cmdBufPtr = request;
    cmdBufPtr += sizeof(OSContPackedRead);

    gDebugNumber = *isOculusConnected;
    
    if (*isOculusConnected) {
        *cmdBufPtr++ = 0xFF; // padding
        *cmdBufPtr++ = 0x01; // tx bytes
        *cmdBufPtr++ = 0x18; // rx bytes
        *cmdBufPtr++ = 0x16; // read orientation command

        for (i = 0; i < 24; ++i) {
            *cmdBufPtr++ = 0xFF; // recieve data
        }
    } else {
        *cmdBufPtr++ = 0xFF; // padding
        *cmdBufPtr++ = 0x01; // tx bytes
        *cmdBufPtr++ = 0x03; // rx bytes
        *cmdBufPtr++ = 0x00; // init command
        *cmdBufPtr++ = 0xFF; // recieve data
        *cmdBufPtr++ = 0xFF;
        *cmdBufPtr++ = 0xFF;
    }

    *cmdBufPtr = 254; // end of message
}