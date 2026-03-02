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

    float menu_x, menu_y;
    float row_height;
} UIContext;

typedef struct {
    int r;
    int g;
    int b;
} UIColor;

extern UIContext ui;

void ui_begin_frame(GLFWwindow* window);
void ui_end_frame();

bool ui_draw_rect(float x, float y, float w, float h, UIColor color);
bool ui_draw_rect_outline(float x, float y, float w, float h, UIColor color); 

bool ui_button(int id, Font* font, const char* label, float x, float y, float w, float h);
bool ui_checkbox(int id, Font* font, const char* label, bool* value, float x, float y);
bool ui_slider(int id, float* value, float min, float max, float x, float y, float w);

#endif
