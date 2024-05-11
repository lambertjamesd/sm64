#ifndef HEADSET_H
#define HEADSET_H

extern float gHeadsetRotation[4][4];

void headset_read_orientation(float rotation[4][4]);
void headset_recenter();

#endif