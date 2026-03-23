#pragma once

#include <GL/glew.h>
// platform specific functions (in this case Windows)

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>
// Register all callbacks used by the app.
void glfwcbRegisterAll(GLFWwindow* window);

// Individual callbacks (C-style). These forward events to App stored in glfw window user pointer.
void glfwcbKey(GLFWwindow* w, int key, int scancode, int action, int mods);
void glfwcbFramebufferSize(GLFWwindow* w, int width, int height);
void glfwcbMouseButton(GLFWwindow* w, int button, int action, int mods);
void glfwcbCursorPos(GLFWwindow* w, double x, double y);
void glfwcbScroll(GLFWwindow* w, double xoffset, double yoffset);
