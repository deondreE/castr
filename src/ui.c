#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "ui.h"
#include "font.h"

#include <string.h>

UIContext ui = {0};

void ui_update(GLFWwindow* window) {
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    
    // Convert GLFW coordinates to normalized -1 to 1 for OpenGL
    int w, h;
    glfwGetWindowSize(window, &w, &h);
    ui.mouse_x = (x / (w / 2.0)) - 1.0;
    ui.mouse_y = 1.0 - (y / (h / 2.0)); 

    bool is_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    ui.mouse_clicked = (is_down && !ui.mouse_down);
    ui.mouse_down = is_down;
}

bool ui_button(int id, Font* font, const char* label, float x, float y, float w, float h) {
    bool result = false;

    // Check if mouse is inside
    if (ui.mouse_x >= x && ui.mouse_x <= x + w &&
        ui.mouse_y >= y - h && ui.mouse_y <= y) {
        ui.hot_item = id;
        if (ui.active_item == 0 && ui.mouse_clicked) {
            ui.active_item = id;
        }
    } else {
        if (ui.hot_item == id) ui.hot_item = 0;
    }

    // Colors
    float color[3] = {0.3f, 0.3f, 0.3f}; // Normal
    if (ui.hot_item == id) {
        color[0] = 0.4f; color[1] = 0.4f; color[2] = 0.4f; // Hover
    }
    if (ui.active_item == id) {
        color[0] = 0.6f; color[1] = 0.1f; color[2] = 0.1f; // Click
        if (!ui.mouse_down) {
            result = true;
            ui.active_item = 0;
        }
    }

    // Render the button rectangle
    glDisable(GL_TEXTURE_2D);
    glColor3fv(color);
    glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y - h);
        glVertex2f(x, y - h);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);

    float text_x = x + (w * 0.1f);
    float text_y = y - (h * 0.6f);
    font_draw(font, label, (text_x + 1.0f) * 640.0f, (1.0f - text_y) * 360.0f);

    return result;
}