// icp.cpp
// author: JJ
// include anywhere, in any order
#include <iostream>
#include <chrono>

#include <random>
#include <sstream>
#include <iomanip>
#include <filesystem>

// OpenCV (does not depend on GL)
#include <opencv2/opencv.hpp>

// OpenGL Extension Wrangler: allow all multiplatform GL functions
#include <GL/glew.h>
// platform specific functions (in this case Windows)

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>

// OpenGL math (and other additional GL libraries, at the end)
#include <glm/glm.hpp>
#include "gl_err_callback.h"

#include "assets.hpp"
//---------------------------------------------------------------------
#include <imgui.h>               // main ImGUI header
#include <imgui_impl_glfw.h>     // GLFW bindings
#include <imgui_impl_opengl3.h>  // OpenGL bindings
#include <fstream>
#include <cmath>
#include <algorithm>

#define JSON_HAS_CPP_20 0
#include <nlohmann/json.hpp>

#include "app.hpp"
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

    nlohmann::json cfg;

    try {
        file >> cfg;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Invalid config JSON, using defaults: "
                  << e.what() << "\n";
        return;
    }

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
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthFunc(GL_LEQUAL);

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
    auto plane_mesh   = load_mesh("resources/models/plane_tri_vnt.obj");

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

    particle_mesh_ = cube_mesh;
    particle_texture_ = texture_atlas;
    particle_uv_offset_ = tile_offset(8, 2);
    particle_uv_scale_ = tile_scale;

    scene_objects_.clear();



// Atlas cube 1
scene_objects_.push_back(SceneObject{
    cube_mesh,
    texture_atlas,

    glm::vec3(-2.2f, 0.0f, 0.0f),
    glm::vec3(0.55f, 0.55f, 0.55f),

    glm::vec3(0.0f, 1.0f, 0.0f),
    1.0f,

    {},

    tile_offset(0, 0),
    tile_scale,

    1.0f,
    true
});

// Atlas cube 2
scene_objects_.push_back(SceneObject{
    cube_mesh,
    texture_atlas,

    glm::vec3(-0.8f, 0.0f, 5.0f),
    glm::vec3(0.55f, 0.55f, 0.55f),

    glm::vec3(1.0f, 0.0f, 0.0f),
    -0.8f,

    {
        PositionAnimation{
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(-4.2f, 0.0f, 0.0f),
            0.8f,
            0.0f
        }
    },

    tile_offset(1, 0),
    tile_scale,

    1.0f,
    true
});

// Atlas cube 3
scene_objects_.push_back(SceneObject{
    cube_mesh,
    texture_atlas,

    glm::vec3(0.6f, 0.0f, 0.0f),
    glm::vec3(0.55f, 0.55f, 0.55f),

    glm::vec3(0.0f, 1.0f, 1.0f),
    0.6f,

    {
        PositionAnimation{
            glm::vec3(0.0f, -0.15f, 5.0f),
            glm::vec3(0.0f,  0.15f, 0.0f),
            1.4f,
            1.5f
        }
    },

    tile_offset(2, 0),
    tile_scale,

    1.0f,
    true
});

// Bunny
scene_objects_.push_back(SceneObject{
    bunny_mesh,
    onyx,

    glm::vec3(8.0f, 0.0f, 0.0f),
    glm::vec3(0.55f, 0.55f, 0.55f),

    glm::vec3(0.0f, 1.0f, 1.0f),
    -0.6f,

    {
        PositionAnimation{
            glm::vec3(0.0f, -0.30f, 0.0f),
            glm::vec3(0.0f,  0.30f, 0.0f),
            2.0f,
            0.0f
        }
    },

    glm::vec2(0.0f, 0.0f),
    glm::vec2(1.0f, 1.0f),

    4.5f,
    true
});



    // Transparent cube 1
scene_objects_.push_back(SceneObject{
        cube_mesh,
        texture_atlas,

        glm::vec3(-1.5f, 0.4f, -2.2f),
        glm::vec3(0.75f, 0.75f, 0.75f),

        glm::vec3(0.0f, 1.0f, 0.0f),
        0.25f,

        {},

        tile_offset(6, 0),
        tile_scale,

        1.0f,
        false,

        glm::vec4(0.4f, 0.9f, 1.0f, 0.35f),
        true
    });

    // Transparent plane
scene_objects_.push_back(SceneObject{
        plane_mesh,
        texture_atlas,

        glm::vec3(0.0f, -4.0f, -2.5f),
        glm::vec3(3.0f, 1.0f, 3.0f),

        glm::vec3(0.0f, 1.0f, 0.0f),
        0.0f,

        {},

        tile_offset(3, 4),
        tile_scale,

        1.0f,
        false,

        glm::vec4(0.5f, 0.8f, 1.0f, 0.30f),
        true
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

glm::vec3 App::get_object_position_(const SceneObject& object, float time) const
{
    glm::vec3 pos = object.position;

    for (const auto& anim : object.position_animations) {
        const float k =
            0.5f - 0.5f * std::cos(time * anim.speed + anim.phase);

        pos += glm::mix(anim.from_offset, anim.to_offset, k);
    }

    return pos;
}

float App::get_object_radius_(const SceneObject& object) const
{
    const float max_scale = std::max(object.scale.x, std::max(object.scale.y, object.scale.z));
    return object.collision_radius * max_scale;
}

bool App::is_inside_map_(const glm::vec3& position) const
{
    if (position.x < map_min_x_ + player_collision_radius_) return false;
    if (position.x > map_max_x_ - player_collision_radius_) return false;

    if (position.y < map_min_y_ + player_collision_radius_) return false;
    if (position.y > map_max_y_ - player_collision_radius_) return false;

    if (position.z < map_min_z_ + player_collision_radius_) return false;
    if (position.z > map_max_z_ - player_collision_radius_) return false;

    return true;
}

bool App::collides_with_scene_(const glm::vec3& position, float time) const
{
    for (const auto& object : scene_objects_) {
        if (!object.collidable) {
            continue;
        }

        const glm::vec3 object_pos = get_object_position_(object, time);
        const float object_radius = get_object_radius_(object);

        const float min_distance = player_collision_radius_ + object_radius;
        const glm::vec3 diff = position - object_pos;
        const float dist2 = glm::dot(diff, diff);

        if (dist2 < min_distance * min_distance) {
            return true;
        }
    }

    return false;
}

bool App::is_valid_player_position_(const glm::vec3& position, float time) const
{
    if (!is_inside_map_(position)) {
        return false;
    }

    if (collides_with_scene_(position, time)) {
        return false;
    }

    return true;
}

void App::try_move_camera_(const glm::vec3& movement, float time)
{
    const glm::vec3 old_position = cam_pos_;

    const glm::vec3 full_move = old_position + movement;

    if (is_valid_player_position_(full_move, time)) {
        cam_pos_ = full_move;
        return;
    }

    collision_active_ = true;

    // Sliding
    const glm::vec3 move_x = old_position + glm::vec3(movement.x, 0.0f, 0.0f);
    if (is_valid_player_position_(move_x, time)) {
        cam_pos_ = move_x;
    }

    const glm::vec3 move_z = cam_pos_ + glm::vec3(0.0f, 0.0f, movement.z);
    if (is_valid_player_position_(move_z, time)) {
        cam_pos_ = move_z;
    }
}

void App::push_camera_out_of_collisions_(float time)
{
    for (const auto& object : scene_objects_) {
        if (!object.collidable) {
            continue;
        }

        const glm::vec3 object_pos = get_object_position_(object, time);
        const float object_radius = get_object_radius_(object);
        const float min_distance = player_collision_radius_ + object_radius;

        glm::vec3 diff = cam_pos_ - object_pos;
        float dist2 = glm::dot(diff, diff);

        if (dist2 < 0.000001f) {
            diff = glm::vec3(1.0f, 0.0f, 0.0f);
            dist2 = 1.0f;
        }

        const float dist = std::sqrt(dist2);

        if (dist < min_distance) {
            const glm::vec3 push_dir = diff / dist;
            cam_pos_ = object_pos + push_dir * min_distance;
            collision_active_ = true;
        }
    }
}

void App::spawn_particles_(
    const glm::vec3& origin,
    int count,
    float speed,
    float lifetime,
    const glm::vec4& color
)
{
    std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
    std::uniform_real_distribution<float> factor(0.5f, 1.2f);
    std::uniform_real_distribution<float> size_dist(0.025f, 0.075f);

    for (int i = 0; i < count; ++i) {
        glm::vec3 dir;

        do {
            dir = glm::vec3(unit(rng_), unit(rng_), unit(rng_));
        } while (glm::dot(dir, dir) < 0.0001f);

        dir = glm::normalize(dir);

        Particle p;
        p.position = origin + dir * 0.05f;
        p.velocity = dir * speed * factor(rng_) + glm::vec3(0.0f, 0.4f, 0.0f);
        p.lifetime = lifetime * factor(rng_);
        p.max_lifetime = p.lifetime;
        p.size = size_dist(rng_);
        p.color = color;

        particles_.push_back(p);
    }

    // Safety cap so particles cannot grow forever.
    constexpr std::size_t max_particles = 600;
    if (particles_.size() > max_particles) {
        const std::size_t remove_count = particles_.size() - max_particles;
        particles_.erase(particles_.begin(), particles_.begin() + static_cast<std::ptrdiff_t>(remove_count));
    }
}

void App::update_particles_(float dt)
{
    const glm::vec3 gravity{ 0.0f, -1.8f, 0.0f };

    for (auto& particle : particles_) {
        particle.velocity += gravity * dt;
        particle.position += particle.velocity * dt;
        particle.lifetime -= dt;
    }

    particles_.erase(
        std::remove_if(
            particles_.begin(),
            particles_.end(),
            [](const Particle& particle) {
                return particle.lifetime <= 0.0f;
            }
        ),
        particles_.end()
    );
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

            const float tf = static_cast<float>(t);
            const float dtf = static_cast<float>(dt);
            const float v = cam_speed_ * dtf;

            collision_active_ = false;

            glm::vec3 forward = glm::vec3(cam_front_.x, 0.0f, cam_front_.z);
            if (glm::dot(forward, forward) > 0.0001f) {
                forward = glm::normalize(forward);
            }else {
                forward = glm::vec3(0.0f, 0.0f, -1.0f);
            }

            glm::vec3 right = glm::normalize(glm::cross(forward, cam_up_));

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
                try_move_camera_(v * forward, tf);
            }

            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
                try_move_camera_(-v * forward, tf);
            }

            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
                try_move_camera_(v * right, tf);
            }

            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
                try_move_camera_(-v * right, tf);
            }

            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                try_move_camera_(v * glm::vec3(0.0f, 1.0f, 0.0f), tf);
            }

            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
                try_move_camera_(-v * glm::vec3(0.0f, 1.0f, 0.0f), tf);
            }

            push_camera_out_of_collisions_(tf);

            // Collision particle burst with cooldown.
            if (collision_active_ && (t - last_collision_particle_time_) > 0.35) {
                spawn_particles_(
                    cam_pos_,
                    40,
                    2.4f,
                    0.75f,
                    glm::vec4(1.0f, 0.55f, 0.10f, 0.80f)
                );

                last_collision_particle_time_ = t;
            }

            // Trail behind selected moving object.
            // trail_object_index - witch object to use for partiles
            if (scene_objects_.size() > trail_object_index_ && (t - last_trail_particle_time_) > 0.035) {
                const glm::vec3 trail_pos =
                    get_object_position_(scene_objects_[trail_object_index_], tf);

                spawn_particles_(
                    trail_pos,
                    2,
                    0.45f,
                    0.7f,
                    glm::vec4(0.25f, 0.75f, 1.0f, 0.70f)
                );

                last_trail_particle_time_ = t;
            }

            update_particles_(dtf);

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
                ImGui::Text("Collision: %s", collision_active_ ? "YES" : "no");
                ImGui::Text("Player position: %.2f %.2f %.2f", cam_pos_.x, cam_pos_.y, cam_pos_.z);
                ImGui::Text("FPS: %.1f", fps_value_);
                ImGui::Text("dt:  %.4f s", dt);
                ImGui::Text("FOV: %.1f", fov_deg_);
                ImGui::Separator();

                ImGui::Text("MSAA: %s", msaa_enabled ? "ON" : "OFF");
                ImGui::Text("Sample buffers: %d", sample_buffers);
                ImGui::Text("Samples: %d", samples);
                ImGui::Separator();

                ImGui::Checkbox("Fog shader effect", &fog_enabled_);
                ImGui::SliderFloat("Fog near", &fog_near_, 0.1f, 20.0f);
                ImGui::SliderFloat("Fog far", &fog_far_, 1.0f, 40.0f);
                ImGui::Separator();

                ImGui::Text("Particles: %zu", particles_.size());
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

            //const float tf = static_cast<float>(t);

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

            // fog
            shader_->setUniform("global_ambient_intensity", glm::vec3(0.04f, 0.04f, 0.04f));

            shader_->setUniform("fog_enabled", fog_enabled_ ? 1 : 0);
            shader_->setUniform("fog_color", fog_color_);
            shader_->setUniform("fog_near", fog_near_);
            shader_->setUniform("fog_far", fog_far_);

            auto draw_object = [&](const SceneObject& object) {
                glm::mat4 model(1.0f);

                const glm::vec3 object_world_position = get_object_position_(object, tf);

                model = glm::translate(model, object_world_position);

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
                shader_->setUniform("object_color", object.color);

                object.mesh->draw();
            };

            std::vector<std::size_t> transparent_indices;
            transparent_indices.reserve(scene_objects_.size());

            // Draw opaque objects
            glDepthMask(GL_TRUE);

            for (std::size_t i = 0; i < scene_objects_.size(); ++i) {
                const SceneObject& object = scene_objects_[i];

                if (object.transparent || object.color.a < 1.0f) {
                    transparent_indices.push_back(i);
                } else {
                    draw_object(object);
                }
            }

            // Sort transparent objects from far to near.
            std::sort(
                transparent_indices.begin(),
                transparent_indices.end(),
                [&](std::size_t ia, std::size_t ib) {
                    const SceneObject& a = scene_objects_[ia];
                    const SceneObject& b = scene_objects_[ib];

                    const glm::vec3 a_pos = get_object_position_(a, tf);
                    const glm::vec3 b_pos = get_object_position_(b, tf);

                    const glm::vec3 da = a_pos - cam_pos_;
                    const glm::vec3 db = b_pos - cam_pos_;

                    const float dist_a2 = glm::dot(da, da);
                    const float dist_b2 = glm::dot(db, db);

                    return dist_a2 > dist_b2;
                }
            );


            // Depth test stays ON, but transparent objects are not written into depth buffer.
            glDepthMask(GL_FALSE);

            for (std::size_t index : transparent_indices) {
                draw_object(scene_objects_[index]);
            }

            // Draw particles as transparent tiny cubes.
            if (particle_mesh_ && particle_texture_ && !particles_.empty()) {
                std::vector<std::size_t> particle_indices;
                particle_indices.reserve(particles_.size());

                for (std::size_t i = 0; i < particles_.size(); ++i) {
                    particle_indices.push_back(i);
                }

                std::sort(
                    particle_indices.begin(),
                    particle_indices.end(),
                    [&](std::size_t ia, std::size_t ib) {
                        const glm::vec3 da = particles_[ia].position - cam_pos_;
                        const glm::vec3 db = particles_[ib].position - cam_pos_;

                        return glm::dot(da, da) > glm::dot(db, db);
                    }
                );

                particle_texture_->bind();

                for (std::size_t index : particle_indices) {
                    const Particle& particle = particles_[index];

                    const float life_ratio =
                        glm::clamp(particle.lifetime / particle.max_lifetime, 0.0f, 1.0f);

                    glm::vec4 particle_color = particle.color;
                    particle_color.a *= life_ratio;

                    glm::mat4 model(1.0f);
                    model = glm::translate(model, particle.position);
                    model = glm::scale(model, glm::vec3(particle.size));

                    shader_->setUniform("uM_m", model);
                    shader_->setUniform("uv_offset", particle_uv_offset_);
                    shader_->setUniform("uv_scale", particle_uv_scale_);
                    shader_->setUniform("object_color", particle_color);

                    particle_mesh_->draw();
                }
            }

            glDepthMask(GL_TRUE);

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