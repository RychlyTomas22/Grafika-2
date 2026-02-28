#pragma once
#include "assets.hpp"

// OpenGL Extension Wrangler: allow all multiplatform GL functions
#include <GL/glew.h>
// platform specific functions (in this case Windows)

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>

// OpenGL math (and other additional GL libraries, at the end)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <random>
#include <string>

class App {
public:
    App();
    ~App();
    bool init(void);
    void init_imgui();
    int run(void);
    void init_assets(void);
    //new GL stuff
    GLFWwindow* window = nullptr;

    // GLFW callback handlers (wired via glfwSet*Callback)
    void error_callback(int error, const char* description);
    void on_key(int key, int scancode, int action, int mods);
    void on_fbsize(int width, int height);
    void on_mouse_button(int button, int action, int mods);
    void on_cursor_pos(double x, double y);
    void on_scroll(double xoffset, double yoffset);


private:

    bool vsync_on_ = true;
    bool show_imgui{true};

    // Task 1.2: mouse cursor capture / release
    bool cursor_captured_{false};
    bool esc_primed_to_quit_{false};

    bool is_fullscreen_{false};
    int windowed_x_{100};
    int windowed_y_{100};
    int windowed_w_{800};
    int windowed_h_{600};

    bool imgui_inited_{false};
    void set_cursor_captured_(bool captured);
    void toggle_fullscreen_();
    GLFWmonitor* best_monitor_for_window_() const;

    // runtime state controlled via callbacks
    glm::vec4 tri_color_{ 1.0f, 1.0f, 1.0f, 1.0f };
    glm::vec4 clear_color_{ 0.08f, 0.08f, 0.10f, 1.0f };
    bool animate_color_{ true };
    bool left_mouse_down_{ false };

    int fb_width_{ 800 };
    int fb_height_{ 600 };
    double cursor_x_{ 0.0 };
    double cursor_y_{ 0.0 };


    std::mt19937 rng_{ std::random_device{}() };
    std::string base_title_{ "OpenGL context" };

    // FPS / title update
    double fps_last_t_{ 0.0 };
    double fps_accum_dt_{ 0.0 };
    int fps_frames_{ 0 };
    double fps_value_{ 0.0 };
    double title_last_t_{ 0.0 };


    GLuint shader_prog_ID{ 0 };
    GLuint VBO_ID{ 0 };
    GLuint VAO_ID{ 0 };

    std::vector<vertex> triangle_vertices =
    {
        {{0.0f,  0.5f,  0.0f}},
        {{0.5f, -0.5f,  0.0f}},
        {{-0.5f, -0.5f,  0.0f}}
    };

};