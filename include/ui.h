#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <GLFW/glfw3.h>
#include "font.h"


typedef struct {
    double mouse_x, mouse_y;
    double last_mouse_x, last_mouse_y;
    bool mouse_down;
    bool mouse_clicked;
    int hot_item;    // Mouse is hovering
    int active_item; // Mouse is clicking
    int dragging_item;
} UIContext;

extern UIContext ui;

void ui_update(GLFWwindow* window);
bool ui_button(int id, Font* font, const char* label, float x, float y, float w, float h);
bool ui_slider(int id, float* value, float min, float max, float x, float y, float w);

#endif
