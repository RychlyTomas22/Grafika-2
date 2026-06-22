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
#include <nlohmann/json.hpp>
#include <fstream>
#include <cmath>

#include "app.hpp"
#include "gl_err_callback.h"
#include "gl_utils.h"
#include "glfw_callbacks.h"

#include "OBJloader.hpp"
#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"

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
        first_mouse_ = true;
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
static glm::mat4 make_perspective(float fov_deg, float aspect, float znear, float zfar)
{
    return glm::perspective(glm::radians(fov_deg), aspect, znear, zfar);
}

static glm::vec4 json_vec4_or_default(
    const nlohmann::json& j,
    const std::string& key,
    glm::vec4 fallback
)
{
    if (!j.contains(key) || !j.at(key).is_array() || j.at(key).size() != 4) {
        return fallback;
    }

    return glm::vec4(
        j.at(key).at(0).get<float>(),
        j.at(key).at(1).get<float>(),
        j.at(key).at(2).get<float>(),
        j.at(key).at(3).get<float>()
    );
}

void App::load_config_()
{
    const std::filesystem::path config_path = "resources/config.json";

    if (!std::filesystem::exists(config_path)) {
        std::cout << "Config file not found, using defaults: "
                  << config_path.string() << "\n";
        return;
    }

    std::ifstream file(config_path);
    if (!file.is_open()) {
        std::cout << "Cannot open config file, using defaults: "
                  << config_path.string() << "\n";
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();

    nlohmann::json cfg = nlohmann::json::parse(buffer.str());

    if (cfg.contains("window")) {
        const auto& w = cfg.at("window");

        window_width_ = w.value("width", window_width_);
        window_height_ = w.value("height", window_height_);
        vsync_on_ = w.value("vsync", vsync_on_);
        msaa_samples_ = w.value("msaa_samples", msaa_samples_);

        if (w.contains("title")) {
            base_title_ = w.at("title").get<std::string>();
        }
    }

    if (cfg.contains("camera")) {
        const auto& c = cfg.at("camera");

        cam_speed_ = c.value("speed", cam_speed_);
        fov_deg_ = c.value("fov_deg", fov_deg_);
        znear_ = c.value("znear", znear_);
        zfar_ = c.value("zfar", zfar_);
    }

    if (cfg.contains("rendering")) {
        const auto& r = cfg.at("rendering");
        clear_color_ = json_vec4_or_default(r, "clear_color", clear_color_);
    }

    std::cout << "Config loaded from: " << config_path.string() << "\n";
}

bool App::init() {

    load_config_();
    // GL init
    {

        if (!glfwInit()) {
            throw std::runtime_error("GLFW not initialized properly!");
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

        // hide window during initialization
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        // antialiasing
        glfwWindowHint(GLFW_SAMPLES, msaa_samples_);


        window = glfwCreateWindow(
     window_width_,
     window_height_,
     base_title_.c_str(),
     nullptr,
     nullptr
 );

        if (!window)
        {
            throw std::runtime_error("GLFW window not created properly!");
        }

        glfwMakeContextCurrent(window);
        glfwSwapInterval(vsync_on_ ? 1 : 0);
        glfwSetWindowUserPointer(window, this);
        glfwcbRegisterAll(window);

        glfwGetFramebufferSize(window, &fb_width_, &fb_height_);

        const float aspect = static_cast<float>(fb_width_) / static_cast<float>(fb_height_);
        proj_ = make_perspective(fov_deg_, aspect, znear_, zfar_);

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

        glEnable(GL_MULTISAMPLE);

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

    // show window after heavy init is done
    glfwShowWindow(window);


    return true;
}

void App::init_assets(void) {
    //
    // Initialize pipeline: compile, link and use shaders
    //
    // load shaders from files (.vert/.frag)
    //
    std::filesystem::path vs = "resources/shaders/point2.vert";
    std::filesystem::path fs = "resources/shaders/point_or_directional2.frag";

    if (!std::filesystem::exists(vs) || !std::filesystem::exists(fs)) {
        throw std::runtime_error(
            "Shader files missing. Expected resources/shaders/point2.vert and point_or_directional2.frag. "
            "Set working directory to project root."
        );
    }

   shader_ = std::make_shared<ShaderProgram>(vs, fs);

    auto load_mesh = [](const std::filesystem::path& path) -> std::shared_ptr<Mesh> {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("OBJ file missing: " + path.string());
        }

        std::vector<Vertex> vertices;
        std::vector<GLuint> indices;

        if (!loadOBJ(path, vertices, indices)) {
            throw std::runtime_error("OBJ loading failed: " + path.string());
        }

        return std::make_shared<Mesh>(vertices, indices, GL_TRIANGLES);
    };

    auto load_texture = [](const std::filesystem::path& path) -> std::shared_ptr<Texture> {
        if (!std::filesystem::exists(path)) {
            throw std::runtime_error("Texture file missing: " + path.string());
        }

        return std::make_shared<Texture>(path);
    };

    auto cube_mesh   = load_mesh("resources/models/cube_triangles_vnt.obj");
    auto teapot_mesh   = load_mesh("resources/models/teapot_tri_vnt.obj");
    auto bunny_mesh   = load_mesh("resources/models/bunny_tri_vnt.obj");

    auto texture_atlas = load_texture("resources/textures/tex_2048.png");
    auto onyx = load_texture("resources/textures/Onyx010_1K-JPG_Color.jpg");


    // Atlas helper.
    // x, y coordinates in atlas.
    // x = column from left
    // y = row from top
    const float atlas_cols = 16.0f;
    const float atlas_rows = 16.0f;

    const glm::vec2 tile_scale{
        1.0f / atlas_cols,
        1.0f / atlas_rows
    };

    auto tile_offset = [](int x, int y) -> glm::vec2 {
        const float cols = 16.0f;
        const float rows = 16.0f;

        return glm::vec2{
            static_cast<float>(x) / cols,
            1.0f - (static_cast<float>(y) + 1.0f) / rows
        };
    };

    scene_objects_.clear();

    // Atlas cube 1: tile from atlas
    scene_objects_.push_back(SceneObject{
        cube_mesh,
        texture_atlas,
        glm::vec3(-2.2f, 0.0f, 0.0f),
        glm::vec3(0.55f, 0.55f, 0.55f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        tile_offset(0, 0),
        tile_scale
    });

    // Atlas cube 2: different tile from atlas
    scene_objects_.push_back(SceneObject{
        cube_mesh,
        texture_atlas,
        glm::vec3(-0.8f, 0.0f, 0.0f),
        glm::vec3(0.55f, 0.55f, 0.55f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        -0.8f,
        0.25f,
        2.0f,
        0.0f,
        tile_offset(1, 0),
        tile_scale
    });

    // Atlas cube 3: different tile from atlas
    scene_objects_.push_back(SceneObject{
        cube_mesh,
        texture_atlas,
        glm::vec3(0.6f, 0.0f, 0.0f),
        glm::vec3(0.55f, 0.55f, 0.55f),
        glm::vec3(0.0f, 1.0f, 1.0f),
        0.6f,
        0.15f,
        1.4f,
        1.5f,
        tile_offset(2, 0),
        tile_scale
    });

    // Sphere: different model loaded from file
    scene_objects_.push_back(SceneObject{
        bunny_mesh,
        onyx,
        glm::vec3(5.0f, 0.0f, 0.0f),
        glm::vec3(0.55f, 0.55f, 0.55f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        -0.6f,
        0.4f,
        2.0f,
        0.0f,
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 1.0f)
    });

    // Teapot: another different model loaded from file
    scene_objects_.push_back(SceneObject{
        teapot_mesh,
        onyx,
        glm::vec3(0.0f, -3.0f, -1.8f),
        glm::vec3(0.25f, 0.25f, 0.25f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        0.7f,
        0.2f,
        1.4f,
        1.5f,
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 1.0f)
    });
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

static void save_screenshot_bgr(const std::string& path, int w, int h)
{
    cv::Mat img(h, w, CV_8UC3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK); // default back buffer
    glReadPixels(0, 0, w, h, GL_BGR, GL_UNSIGNED_BYTE, img.data);
    cv::flip(img, img, 0); // OpenGL origin bottom-left
    cv::imwrite(path, img);
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

        if (!shader_ || scene_objects_.empty()) {
            throw std::runtime_error("Assets not initialized (shader_ or scene_objects_ missing).");
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

            const float dtf = static_cast<float>(dt);
            const float v = cam_speed_ * dtf;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cam_pos_ += v * cam_front_;
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cam_pos_ -= v * cam_front_;

            glm::vec3 right = glm::normalize(glm::cross(cam_front_, cam_up_));
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cam_pos_ += v * right;
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cam_pos_ -= v * right;

            glm::mat4 view = glm::lookAt(cam_pos_, cam_pos_ + cam_front_, cam_up_);

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

                GLboolean msaa_enabled = glIsEnabled(GL_MULTISAMPLE);
                GLint sample_buffers = 0;
                GLint samples = 0;
                glGetIntegerv(GL_SAMPLE_BUFFERS, &sample_buffers);
                glGetIntegerv(GL_SAMPLES, &samples);

                ImGui::Begin("HUD", nullptr, flags);
                ImGui::Text("FPS: %.1f", fps_value_);
                ImGui::Text("dt:  %.4f s", dt);
                ImGui::Text("FOV: %.1f", fov_deg_);
                ImGui::Separator();

                ImGui::Text("MSAA: %s", msaa_enabled ? "ON" : "OFF");
                ImGui::Text("Sample buffers: %d", sample_buffers);
                ImGui::Text("Samples: %d", samples);

                ImGui::Separator();

                ImGui::Checkbox("Show ImGui (F1)", &show_imgui);
                if (ImGui::Checkbox("VSync (V)", &vsync_on_)) {
                    glfwSwapInterval(vsync_on_ ? 1 : 0);
                }

                if (ImGui::Button(is_fullscreen_ ? "Windowed (F11)" : "Fullscreen (F11)")) {
                    toggle_fullscreen_();
                }

                ImGui::Checkbox("Animate (P)", &animate_color_);
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

            shader_->use();
            shader_->setUniform("tex0", 0);
            shader_->setUniform("uV_m", view);
            shader_->setUniform("uP_m", proj_);

            const float tf = static_cast<float>(t);

            // light 0
            glm::mat4 light_m0(1.0f);
            light_m0 = glm::rotate(light_m0, tf * 1.0f, glm::vec3(0.0f, 1.0f, 0.0f));

            glm::vec3 light_world0 =
                glm::vec3(light_m0 * glm::vec4(point_light_.base_position, 1.0f));

            // light 1
            glm::mat4 light_m1(1.0f);
            light_m1 = glm::rotate(light_m1, -tf * 0.7f, glm::vec3(0.0f, 1.0f, 0.0f));

            glm::vec3 light_world1 =
                glm::vec3(light_m1 * glm::vec4(point_light1_.base_position, 1.0f));

            // light 2
            glm::mat4 light_m2(1.0f);
            light_m2 = glm::rotate(light_m2, tf * 1.3f, glm::vec3(1.0f, 0.0f, 0.0f));

            glm::vec3 light_world2 =
                glm::vec3(light_m2 * glm::vec4(point_light2_.base_position, 1.0f));

            // transfer to VIEW space
            glm::vec3 light_view0 = glm::vec3(view * glm::vec4(light_world0, 1.0f));
            glm::vec3 light_view1 = glm::vec3(view * glm::vec4(light_world1, 1.0f));
            glm::vec3 light_view2 = glm::vec3(view * glm::vec4(light_world2, 1.0f));

            // directional light
            glm::vec3 sun_world_dir = glm::normalize(glm::vec3(
                std::cos(tf * 0.25f),
                -0.7f,
                std::sin(tf * 0.25f)
            ));

            // direction has w = 0.0 because it is direction, not position
            glm::vec3 sun_view_dir =
                glm::normalize(glm::vec3(view * glm::vec4(sun_world_dir, 0.0f)));

            shader_->setUniform("sun_direction", sun_view_dir);
            shader_->setUniform("sun_ambient_intensity", sun_.ambient);
            shader_->setUniform("sun_diffuse_intensity", sun_.diffuse);
            shader_->setUniform("sun_specular_intensity", sun_.specular);



            shader_->setUniform("light_position0", light_view0);
            shader_->setUniform("light_position1", light_view1);
            shader_->setUniform("light_position2", light_view2);

            // light 0
            shader_->setUniform("ambient_intensity0", point_light_.ambient);
            shader_->setUniform("diffuse_intensity0", point_light_.diffuse);
            shader_->setUniform("specular_intensity0", point_light_.specular);

            // light 1
            shader_->setUniform("ambient_intensity1", point_light1_.ambient);
            shader_->setUniform("diffuse_intensity1", point_light1_.diffuse);
            shader_->setUniform("specular_intensity1", point_light1_.specular);

            // light 2
            shader_->setUniform("ambient_intensity2", point_light2_.ambient);
            shader_->setUniform("diffuse_intensity2", point_light2_.diffuse);
            shader_->setUniform("specular_intensity2", point_light2_.specular);

            // materiál
            shader_->setUniform("ambient_material", material_ambient_);
            shader_->setUniform("diffuse_material", material_diffuse_);
            shader_->setUniform("specular_material", material_specular_);
            shader_->setUniform("specular_shinines", material_shininess_);

            // spotlight / camera headlight
            const float spot_cutoff_cos =
                static_cast<float>(std::cos(glm::radians(spot_light_.cutoff_deg)));

            shader_->setUniform("spot_diffuse_intensity", spot_light_.diffuse);
            shader_->setUniform("spot_specular_intensity", spot_light_.specular);
            shader_->setUniform("spot_cutoff_cos", spot_cutoff_cos);
            shader_->setUniform("spot_exponent", spot_light_.exponent);

            for (const auto& object : scene_objects_) {
                glm::mat4 model(1.0f);

                const float vertical_offset =
                    object.vertical_amplitude * std::sin(tf * object.vertical_speed + object.vertical_phase);

                model = glm::translate(
                    model,
                    object.position + glm::vec3(0.0f, vertical_offset, 0.0f)
                );

                model = glm::rotate(
                    model,
                    tf * object.rotation_speed,
                    object.rotation_axis
                );

                model = glm::scale(model, object.scale);

                object.texture->bind();

                shader_->setUniform("uM_m", model);
                shader_->setUniform("uv_offset", object.uv_offset);
                shader_->setUniform("uv_scale", object.uv_scale);

                object.mesh->draw();
            }

            // render ImGui on top
            if (show_imgui && imgui_inited_) {
                ImGui::Render();
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            }
            if (screenshot_next_frame_) {
                save_screenshot_bgr(screenshot_path_, fb_width_, fb_height_);
                screenshot_next_frame_ = false;
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

    case GLFW_KEY_F1:
        // show/hide ImGui
        show_imgui = !show_imgui;
        break;

    case GLFW_KEY_P:
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

    case GLFW_KEY_M: // MSAA toggle
        if (glIsEnabled(GL_MULTISAMPLE)) glDisable(GL_MULTISAMPLE);
        else glEnable(GL_MULTISAMPLE);
        break;

    case GLFW_KEY_F2: // screenshot
        save_screenshot_bgr("resources/screenshots/screenshot.png", fb_width_, fb_height_);
        break;

        case GLFW_KEY_F3:
            glDisable(GL_MULTISAMPLE);
            screenshot_path_ = "resources/screenshots/screenshot_no_msaa.png";
            screenshot_next_frame_ = true;
            break;

        case GLFW_KEY_F4:
            glEnable(GL_MULTISAMPLE);
            screenshot_path_ = "resources/screenshots/screenshot_msaa.png";
            screenshot_next_frame_ = true;
            break;

    default:
        break;
    }
}

void App::on_fbsize(int width, int height) {
    fb_width_ = (width > 0) ? width : 1;
    fb_height_ = (height > 0) ? height : 1;
    glViewport(0, 0, fb_width_, fb_height_);

    const float aspect = static_cast<float>(fb_width_) / static_cast<float>(fb_height_);
    proj_ = make_perspective(fov_deg_, aspect, znear_, zfar_);
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

    if (!cursor_captured_) {
        first_mouse_ = true;
        return;
    }

    if (first_mouse_) {
        last_x_ = x;
        last_y_ = y;
        first_mouse_ = false;
        return;
    }

    const double dx = x - last_x_;
    const double dy = last_y_ - y;
    last_x_ = x;
    last_y_ = y;

    yaw_deg_   += static_cast<float>(dx) * mouse_sensitivity_;
    pitch_deg_ += static_cast<float>(dy) * mouse_sensitivity_;

    // flip prevention
    if (pitch_deg_ > 89.0f) pitch_deg_ = 89.0f;
    if (pitch_deg_ < -89.0f) pitch_deg_ = -89.0f;

    const float yaw   = glm::radians(yaw_deg_);
    const float pitch = glm::radians(pitch_deg_);

    glm::vec3 front;
    front.x = std::cos(yaw) * std::cos(pitch);
    front.y = std::sin(pitch);
    front.z = std::sin(yaw) * std::cos(pitch);

    cam_front_ = glm::normalize(front);
}



void App::on_scroll(double /*xoffset*/, double yoffset)
{
    // Mouse wheel changes field of view.
    fov_deg_ -= static_cast<float>(yoffset) * 2.0f;

    if (fov_deg_ < 25.0f) {
        fov_deg_ = 25.0f;
    }

    if (fov_deg_ > 90.0f) {
        fov_deg_ = 90.0f;
    }

    const float aspect =
        static_cast<float>(fb_width_) / static_cast<float>(fb_height_);

    proj_ = make_perspective(fov_deg_, aspect, znear_, zfar_);
}

App::~App()
{
    scene_objects_.clear();
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