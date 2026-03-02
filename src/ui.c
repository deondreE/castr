#include <glad/glad.h>
#include "ui.h"
#include <stdio.h>

UIContext ui = { 0 };

static bool is_inside(float x, float y, float w, float h) {
    return (ui.mouse_x >= x && ui.mouse_x <= x + w && ui.mouse_y >= y && ui.mouse_y <= y + h);
}

void ui_begin_frame(GLFWwindow* window) {
    ui.last_mouse_x = ui.mouse_x;
    ui.last_mouse_y = ui.mouse_y;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    ui.mouse_x = x;
    ui.mouse_y = y;
    bool is_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    ui.mouse_clicked = (is_down && !ui.mouse_down);
    ui.mouse_down = is_down;
    ui.hot_item = 0;
}

void ui_end_frame() {
    if (!ui.mouse_down) {
        ui.active_item = 0;
    }
}

bool ui_draw_rect(float x, float y, float w, float h, UIColor color) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    return true;
}

bool ui_draw_rect_outline(float x, float y, float w, float h, UIColor color) {
    glDisable(GL_TEXTURE_2D);
    glColor3f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    return true;
}

bool ui_button(int id, Font* font, const char* label, float x, float y, float w, float h) {
    bool result = false;
    bool hover = is_inside(x, y, w, h);

    if (hover) {
        ui.hot_item = id;
        if (ui.mouse_clicked) ui.active_item = id;
    }

    UIColor color = { 50, 50, 50 };
    if (ui.active_item == id) {
        color.r = 100;
        if (!ui.mouse_down && hover) result = true;
    }
    else if (ui.hot_item == id) {
        color.r = 70; color.g = 70; color.b = 70;
    }

    ui_draw_rect(x, y, w, h, color);
    ui_draw_rect_outline(x, y, w, h, (UIColor) { 120, 120, 120 });

    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    font_draw(font, label, x + 10, y + h - 10);
    return result;
}

bool ui_checkbox(int id, Font* font, const char* label, bool* value, float x, float y) {
    float size = 20.0f;
    bool hover = is_inside(x, y, size, size);

    if (hover) {
        ui.hot_item = id;
        if (ui.mouse_clicked) {
            *value = !(*value);
            return true;
        }
    }

    ui_draw_rect(x, y, size, size, (UIColor) { 25, 25, 25 });
    ui_draw_rect_outline(x, y, size, size, (UIColor) { 200, 200, 200 });

    if (*value) {
        ui_draw_rect(x + 4, y + 4, size - 8, size - 8, (UIColor) { 50, 200, 50 });
    }

    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    font_draw(font, label, x + size + 10, y + size - 5);
    return false;
}

bool ui_slider(int id, float* value, float min, float max, float x, float y, float w) {
    float h = 20.0f;
    bool changed = false;
    bool hover = is_inside(x, y, w, h);

    if (hover && ui.mouse_down) {
        ui.active_item = id;
    }

    if (ui.active_item == id) {
        float pct = (float)((ui.mouse_x - x) / w);
        if (pct < 0.0f) pct = 0.0f;
        if (pct > 1.0f) pct = 1.0f;
        *value = min + (max - min) * pct;
        changed = true;
    }

    ui_draw_rect(x, y + 8, w, 4, (UIColor) { 30, 30, 30 });
    float knob_x = x + ((*value - min) / (max - min)) * w;
    ui_draw_rect(knob_x - 5, y, 10, h, (UIColor) { 180, 180, 180 });

    return changed;
}