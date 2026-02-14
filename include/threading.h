#ifndef THREADING_H
#define THREADING_H

#include <windows.h>
#include <stdbool.h>

typedef struct {
    unsigned char* frame_buffer;
    bool has_new_frame;
    bool encoder_has_work;
    bool running;
    CRITICAL_SECTION lock;
    CONDITION_VARIABLE data_ready;
    int width, height;
} SharedState;

#endif
