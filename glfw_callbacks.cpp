#include "glfw_callbacks.h"
#include "app.hpp"

void glfwcbKey(GLFWwindow* w, int key, int scancode, int action, int mods) {
    App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->on_key(key, scancode, action, mods);
}

void glfwcbFramebufferSize(GLFWwindow* w, int width, int height) {
    App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->on_fbsize(width, height);
}

void glfwcbMouseButton(GLFWwindow* w, int button, int action, int mods) {
    App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->on_mouse_button(button, action, mods);
}

void glfwcbCursorPos(GLFWwindow* w, double x, double y) {
    App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->on_cursor_pos(x, y);
}

void glfwcbScroll(GLFWwindow* w, double xoffset, double yoffset) {
    App* app = reinterpret_cast<App*>(glfwGetWindowUserPointer(w));
    if (app) app->on_scroll(xoffset, yoffset);
}

void glfwcbRegisterAll(GLFWwindow* window) {
    glfwSetKeyCallback(window, glfwcbKey);
    glfwSetFramebufferSizeCallback(window, glfwcbFramebufferSize);
    glfwSetMouseButtonCallback(window, glfwcbMouseButton);
    glfwSetCursorPosCallback(window, glfwcbCursorPos);
    glfwSetScrollCallback(window, glfwcbScroll);
}
