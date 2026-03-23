// icp.cpp
// author: JJ
// include anywhere, in any order
#include <iostream>
#include <chrono>
#include <stack>
#include <random>
#include <sstream>
#include <iomanip>
#include <filesystem>

// OpenCV (does not depend on GL)
#include <opencv2\opencv.hpp>

// OpenGL Extension Wrangler: allow all multiplatform GL functions
#include <GL/glew.h>
// platform specific functions (in this case Windows)

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>

// OpenGL math (and other additional GL libraries, at the end)
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "gl_err_callback.h"

#include "assets.hpp"
//---------------------------------------------------------------------
#include <imgui.h>               // main ImGUI header
#include <imgui_impl_glfw.h>     // GLFW bindings
#include <imgui_impl_opengl3.h>  // OpenGL bindings

#include "app.hpp"
#include "gl_err_callback.h"
#include "gl_utils.h"
#include "glfw_callbacks.h"

#include "OBJloader.hpp"
#include "ShaderProgram.hpp"
#include "Mesh.hpp"

App::App()
{
    std::cout << "Constructed...\n";
}

void App::set_cursor_captured_(bool captured)
{
    if (!window) return;
    glfwSetInputMode(window, GLFW_CURSOR, captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
    cursor_captured_ = captured;

    if (captured) {
        esc_primed_to_quit_ = false;
    }
}

void App::toggle_fullscreen_()
{
    if (!window) return;

    if (!is_fullscreen_) {
        glfwGetWindowPos(window, &windowed_x_, &windowed_y_);
        glfwGetWindowSize(window, &windowed_w_, &windowed_h_);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (!mode) return;

        glfwSetWindowMonitor(
            window,
            monitor,
            0,
            0,
            mode->width,
            mode->height,
            mode->refreshRate
        );

        is_fullscreen_ = true;
    } else {
        glfwSetWindowMonitor(
            window,
            nullptr,
            windowed_x_,
            windowed_y_,
            windowed_w_,
            windowed_h_,
            0
        );

        is_fullscreen_ = false;
    }
}

bool App::init() {

    // GL init

    {

        if (!glfwInit()) {
            throw std::runtime_error("GLFW not initialized properly!");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

        // Task 1.3: hide window during initialization
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        window = glfwCreateWindow(800, 600, "OpenGL context", nullptr, nullptr);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(vsync_on_ ? 1 : 0);

        if (!window)
        {
            throw std::runtime_error("GLFW not initialized properly!");
        }
        glfwSetWindowUserPointer(window, this);
        glfwcbRegisterAll(window);

        glfwGetFramebufferSize(window, &fb_width_, &fb_height_);
        glViewport(0, 0, fb_width_, fb_height_);

        // init glew
        // http://glew.sourceforge.net/basic.html
        // DONETODO: add error checking!

        GLenum err = glewInit();
        if (GLEW_OK != err)
        {
            /* Problem: glewInit failed, something is seriously wrong. */
            fprintf(stderr, "Error: %p\n", glewGetErrorString(err));
            throw std::runtime_error("GLFW not initialized properly!");
        }
        (void)glGetError();

        if (!GLEW_ARB_direct_state_access)
            throw std::runtime_error("No DSA :-(");

        // //TODO: get info about your GL context
        //
        // if (GLEW_ARB_debug_output)
        // {
        //     glDebugMessageCallback(MessageCallback, 0);
        //     glEnable(GL_DEBUG_OUTPUT);
        //
        //     //default is asynchronous debug output, use this to simulate glGetError() functionality
        //     //glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
        //
        //     std::cout << "GL_DEBUG enabled." << std::endl;
        // }
        // else
        //     std::cout << "GL_DEBUG NOT SUPPORTED!" << std::endl;
        glutilPrintContextInfoOrThrow(4, 6, 1);
        glutilTryEnableDebugOutput(MessageCallback, nullptr);
    }

    init_assets();

    init_imgui();

    // Task 1.3: show window after heavy init is done
    glfwShowWindow(window);

    return true;
}

void App::init_assets(void) {
    //
    // Initialize pipeline: compile, link and use shaders
    //
    // load shaders from files (.vert/.frag)
    //
    std::filesystem::path vs = "resources/shaders/basic_core_2.vert";
    std::filesystem::path fs = "resources/shaders/basic_uniform_2.frag";

    if (!std::filesystem::exists(vs) || !std::filesystem::exists(fs)) {
        throw std::runtime_error("Shader files missing. Expected: resources/shaders/simple.vert + simple.frag. "
                                 "Set working directory to project root or copy resources next to executable.");
    }
    shader_ = std::make_shared<ShaderProgram>(vs, fs);

    //
    // load triangle vertex data from .OBJ and create VAO/VBO/EBO using DSA
    //
    std::filesystem::path obj = "resources/models/triangle.obj";
    if (!std::filesystem::exists(obj)) {
        throw std::runtime_error("OBJ file missing. Expected: resources/models/triangle.obj. "
                                 "Set working directory to project root or copy resources next to executable.");
    }

    std::vector<Vertex> vertices;
    std::vector<GLuint> indices;
    if (!loadOBJ(obj, vertices, indices)) {
        throw std::runtime_error("OBJ loading failed: " + obj.string());
    }

    // Use also EBO (indirect vertex addressing)
    mesh_ = std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES);
}

void App::init_imgui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();
    imgui_inited_ = true;
    std::cout << "ImGUI version: " << ImGui::GetVersion() << "\n";
}

int App::run()
{
    try {
        glEnable(GL_DEPTH_TEST);

        fps_last_t_ = glfwGetTime();
        title_last_t_ = fps_last_t_;
        fps_accum_dt_ = 0.0;
        fps_frames_ = 0;
        fps_value_ = 0.0;

        if (!shader_ || !mesh_) {
            throw std::runtime_error("Assets not initialized (shader_/mesh_ is null).");
        }

        while (!glfwWindowShouldClose(window)) {
            const double t = glfwGetTime();
            const double dt = t - fps_last_t_;
            fps_last_t_ = t;
            fps_accum_dt_ += dt;
            fps_frames_ += 1;

            if (fps_accum_dt_ >= 0.50) {
                fps_value_ = static_cast<double>(fps_frames_) / fps_accum_dt_;
                fps_accum_dt_ = 0.0;
                fps_frames_ = 0;
            }

            // Title update is fine in windowed, but fullscreen has no visible title bar.
            if (!is_fullscreen_ && (t - title_last_t_) >= 0.25) {
                std::ostringstream oss;
                oss << base_title_ << " | FPS: " << std::fixed << std::setprecision(1) << fps_value_;
                glfwSetWindowTitle(window, oss.str().c_str());
                title_last_t_ = t;
            }

            // ImGui frame
            if (show_imgui && imgui_inited_) {
                ImGui_ImplOpenGL3_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                ImGuiWindowFlags flags =
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoSavedSettings;

                ImGui::Begin("HUD", nullptr, flags);
                ImGui::Text("FPS: %.1f", fps_value_);
                ImGui::Text("dt:  %.4f s", dt);
                ImGui::Separator();

                ImGui::Checkbox("Show ImGui (D)", &show_imgui);
                if (ImGui::Checkbox("VSync (V)", &vsync_on_)) {
                    glfwSwapInterval(vsync_on_ ? 1 : 0);
                }

                if (ImGui::Button(is_fullscreen_ ? "Windowed (F11)" : "Fullscreen (F11)")) {
                    toggle_fullscreen_();
                }

                ImGui::Checkbox("Animate (A)", &animate_color_);
                ImGui::ColorEdit4("Triangle", &tri_color_.x);
                ImGui::ColorEdit4("Clear", &clear_color_.x);

                ImGui::End();
            }

            // draw scene
            glClearColor(clear_color_.r, clear_color_.g, clear_color_.b, clear_color_.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glm::vec4 col = tri_color_;
            if (animate_color_) {
                const float tt = static_cast<float>(t);
                const float w = 2.0f;
                const float rmod = 0.5f + 0.5f * std::sin(tt * w + 0.0f);
                const float gmod = 0.5f + 0.5f * std::sin(tt * w + 2.0943951f);
                const float bmod = 0.5f + 0.5f * std::sin(tt * w + 4.1887902f);
                col.r *= rmod;
                col.g *= gmod;
                col.b *= bmod;
            }

            // Fragment shader expects: uniform vec4 ucolor;
            shader_->setUniform("ucolor", col);
            shader_->use();
            mesh_->draw();

            // render ImGui on top
            if (show_imgui && imgui_inited_) {
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }

            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    } catch (std::exception const& e) {
        std::cerr << "App failed : " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Finished OK...\n";
    return EXIT_SUCCESS;
}

void App::error_callback(int error, const char* description)
{
    std::cerr << "GLFW error " << error << ": " << description << "\n";
}

void App::on_key(int key, int /*scancode*/, int action, int /*mods*/)
{
    if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

    switch (key) {
    case GLFW_KEY_ESCAPE:
        // ESC releases cursor first, second ESC quits
        if (cursor_captured_) {
            set_cursor_captured_(false);
            return;
        }
        if (!esc_primed_to_quit_) {
            esc_primed_to_quit_ = true;
            return;
        }
        glfwSetWindowShouldClose(window, GLFW_TRUE);
        break;

    case GLFW_KEY_TAB:
        // cursor release/catch
        set_cursor_captured_(!cursor_captured_);
        break;

    case GLFW_KEY_F11:
        // toggle windowed/fullscreen
        toggle_fullscreen_();
        break;

    case GLFW_KEY_D:
        // show/hide ImGui
        show_imgui = !show_imgui;
        break;

    case GLFW_KEY_A:
        animate_color_ = !animate_color_;
        break;

    case GLFW_KEY_C: {
        std::uniform_real_distribution<float> u(0.0f, 1.0f);
        clear_color_ = {u(rng_), u(rng_), u(rng_), 1.0f};
        break;
    }

    case GLFW_KEY_V:
        vsync_on_ = !vsync_on_;
        glfwSwapInterval(vsync_on_ ? 1 : 0);
        break;

    default:
        break;
    }
}

void App::on_fbsize(int width, int height) {
    fb_width_ = (width > 0) ? width : 1;
    fb_height_ = (height > 0) ? height : 1;
    glViewport(0, 0, fb_width_, fb_height_);
}

void App::on_mouse_button(int button, int action, int /*mods*/)
{
    if (action != GLFW_PRESS) return;

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (!cursor_captured_) {
            set_cursor_captured_(true);
            return;
        }

        left_mouse_down_ = true;
        std::uniform_real_distribution<float> u(0.0f, 1.0f);
        tri_color_ = {u(rng_), u(rng_), u(rng_), 1.0f};
        return;
    }

    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        set_cursor_captured_(false);
        return;
    }
}

void App::on_cursor_pos(double x, double y)
{
    cursor_x_ = x;
    cursor_y_ = y;
}

void App::on_scroll(double /*xoffset*/, double /*yoffset*/)
{
}

App::~App()
{
    mesh_.reset();
    shader_.reset();

    if (imgui_inited_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        imgui_inited_ = false;
    }

    if (window) {
        glfwDestroyWindow(window);
        window = nullptr;
    }

    glfwTerminate();
}