#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "ui.h"
#include "font.h"

#include <string.h>

UIContext ui = {0};

void ui_update(GLFWwindow* window) {
    ui.last_mouse_x = ui.mouse_x;
    ui.last_mouse_y = ui.mouse_y;
    
    double x, y;
    glfwGetCursorPos(window, &x, &y);

    ui.mouse_x = x;
    ui.mouse_y = y;

    bool is_down = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    ui.mouse_clicked = (is_down && !ui.mouse_down);
    ui.mouse_down = is_down;

    if (!ui.mouse_down) {
        ui.dragging_item = 0;
    }
}

bool ui_button(int id, Font* font, const char* label, float x, float y, float w, float h) {
    bool result = false;

    double mx = 0, my = 0;
    glfwGetCursorPos(glfwGetCurrentContext(), &mx, &my);

    // Check if mouse is inside
    if (mx >= x && mx <= x + w && my >= y && my <= y + h) {
        ui.hot_item = id;
        if (ui.active_item == 0 && ui.mouse_clicked) {
            ui.active_item = id;
        }
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
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
    glEnd();
    glColor3f(1.0f, 1.0f, 1.0f);

    float text_x = x + (w * 0.1f);
    float text_y = y - (h * 0.6f);
    font_draw(font, label, (text_x + 1.0f) * 640.0f, (1.0f - text_y) * 360.0f);

    return result;
}

bool ui_slider(int id, float* value, float min, float max, float x, float y, float w)
{
    double mx, my;
    glfwGetCursorPos(glfwGetCurrentContext(), &mx, &my);

    if (ui.mouse_down && mx >= x && mx <= x + w && my >= y - 10 && my <= y + 10)
    {
        const float pct = (float)((mx - x) / w);
        *value = min + (max - min) * pct;
        return true;
    }

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.2f, 0.2f, 0.2f);
    glBegin(GL_QUADS);
        glVertex2f(x, y); glVertex2f(x + w, y);
    glEnd();

    float knob_x = x + ((*value - min) / (max - min)) * w;
    glBegin(GL_QUADS);
        glVertex2f(knob_x - 5, y - 10);
        glVertex2f(knob_x + 5, y - 10);
        glVertex2f(knob_x + 5, y + 10);
        glVertex2f(knob_x - 5, y + 10);
    glEnd();

}
