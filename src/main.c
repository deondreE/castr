#include <stdio.h>
#include <GLFW/glfw3.h>
#include "logger.h"

int main(void) {
    GLFWwindow* window;

    log_info("Starting Castr....");

    if (!glfwInit()) {
        return -1;
    }

    window = glfwCreateWindow(640, 480, "Castr Window", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    while (!glfwWindowShouldClose(window)) {

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
