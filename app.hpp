#pragma once

/*
 * app.hpp
 * ---------------------------------------------------------------------------
 * Header file for the main OpenGL application class.
 *
 * This file contains:
 * - App class declaration
 * - public application lifecycle methods
 * - GLFW callback handlers
 * - window, fullscreen, VSync, MSAA and ImGui state
 * - camera state and input-related variables
 * - scene object representation
 * - animation data
 * - collision configuration and helper declarations
 * - fog shader-effect settings
 * - particle-effect data
 * - light and material definitions
 *
 * Most implementation details are in app.cpp. This header mainly defines the
 * persistent state that the application needs between frames.
 * ---------------------------------------------------------------------------
 */

// GLFW toolkit
// Uses GL calls to open GL context, i.e. GLEW __MUST__ be first.
#include <GLFW/glfw3.h>

// OpenGL math (and other additional GL libraries, at the end)
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <string>
#include <memory>

#include "ShaderProgram.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"

/*
 * Main application object.
 *
 * The App class owns the GLFW window, OpenGL resources, scene objects,
 * input state, camera state, lighting settings, collision state, particle
 * effects and ImGui state.
 *
 * The basic lifetime is:
 * 1. App constructor
 * 2. init()
 * 3. run()
 * 4. destructor cleanup
 */
class App {
public:
    App();
    ~App();

    /*
     * Initializes the application.
     *
     * Main responsibilities:
     * - load JSON configuration
     * - initialize GLFW
     * - create OpenGL 4.6 Core Profile context
     * - initialize GLEW
     * - enable debug output and required OpenGL states
     * - load all scene assets
     * - initialize ImGui
     */
    bool init(void);

    /*
     * Initializes ImGui for the current GLFW/OpenGL context.
     * Used for the HUD with FPS, collision state, particles and render controls.
     */
    void init_imgui();

    /*
     * Main application loop.
     *
     * Handles:
     * - frame timing and FPS calculation
     * - keyboard and mouse driven camera movement
     * - collisions
     * - particle spawning and updating
     * - lighting uniform updates
     * - scene rendering
     * - transparent rendering pass
     * - ImGui rendering
     */
    int run(void);

    /*
     * Loads shaders, meshes, textures and creates scene objects.
     *
     * The application expects resources to be available relative to the
     * project root working directory.
     */
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

    /*
     * General runtime toggles.
     *
     * vsync_on_ controls glfwSwapInterval().
     * show_imgui controls whether the HUD is rendered.
     */
    bool vsync_on_ = true;
    bool show_imgui{true};

    /*
     * Loads values from resources/config.json.
     *
     * The config file controls window size, title, fullscreen startup,
     * VSync, MSAA, camera settings and clear color.
     */
    void load_config_();

    // Task 1.2: mouse cursor capture / release
    bool cursor_captured_{false};
    bool esc_primed_to_quit_{false};

    /*
     * Fullscreen state.
     *
     * The previous windowed position and size are stored so that switching
     * from fullscreen back to windowed mode restores the old window state.
     */
    bool is_fullscreen_{false};
    int windowed_x_{100};
    int windowed_y_{100};
    int windowed_w_{800};
    int windowed_h_{600};

    /*
     * ImGui and helper functions for cursor/fullscreen handling.
     */
    bool imgui_inited_{false};
    void set_cursor_captured_(bool captured);
    void toggle_fullscreen_();

    /*
     * Screenshot state.
     *
     * Some screenshots are saved immediately, while screenshot_next_frame_
     * allows MSAA state to be changed before capturing the next rendered frame.
     */
    bool screenshot_next_frame_{false};
    std::string screenshot_path_;

    // runtime state controlled via callbacks

    glm::vec4 clear_color_{ 0.08f, 0.08f, 0.10f, 1.0f };
    bool left_mouse_down_{ false };
    //bool animate_color_{ false };
    //glm::vec4 tri_color_{ 1.0f, 1.0f, 1.0f, 1.0f };

    /*
     * Current framebuffer size and last known mouse cursor position.
     *
     * Framebuffer size is used for glViewport and projection matrix aspect.
     */
    int fb_width_{ 800 };
    int fb_height_{ 600 };
    double cursor_x_{ 0.0 };
    double cursor_y_{ 0.0 };

    /*
     * Random generator used for random colors and particle directions.
     */
    std::mt19937 rng_{ std::random_device{}() };

    // Window config
    std::string base_title_{ "OpenGL context" };
    int window_width_{ 800 };
    int window_height_{ 600 };
    int msaa_samples_{ 4 };
    bool start_fullscreen_{ false };

    /*
     * PositionAnimation describes one independent position animation.
     *
     * The object moves between:
     * position + from_offset
     * and
     * position + to_offset
     *
     * speed controls how fast the animation runs.
     * phase shifts the animation in time so multiple objects do not have to
     * move in the same rhythm.
     */
    struct PositionAnimation {
        glm::vec3 from_offset{ 0.0f, 0.0f, 0.0f };
        glm::vec3 to_offset{ 0.0f, 0.0f, 0.0f };

        float speed{ 1.0f };
        float phase{ 0.0f };
    };

    // loaded assets
    /*
     * SceneObject represents one renderable object in the world.
     *
     * It contains:
     * - mesh and texture references
     * - transform data: position, scale, rotation axis and rotation speed
     * - optional position animations
     * - texture-atlas UV offset and scale
     * - collision settings
     * - color tint and alpha
     * - transparency flag
     *
     * Transparent objects are rendered after opaque objects in a sorted pass.
     */
    struct SceneObject {
        std::shared_ptr<Mesh> mesh;
        std::shared_ptr<Texture> texture;

        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 scale{ 1.0f, 1.0f, 1.0f };

        glm::vec3 rotation_axis{ 0.0f, 1.0f, 0.0f };
        float rotation_speed{ 0.0f };

        std::vector<PositionAnimation> position_animations;

        glm::vec2 uv_offset{ 0.0f, 0.0f };
        glm::vec2 uv_scale{ 1.0f, 1.0f };

        float collision_radius{ 1.0f };
        bool collidable{ true };

        glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
        bool transparent{ false };
    };

    // Collisions
    /*
     * Collision system.
     *
     * The player/camera is approximated as a sphere with radius
     * player_collision_radius_.
     *
     * Scene objects are also approximated by spheres. Their final collision
     * radius is collision_radius multiplied by the largest scale component.
     *
     * The map limits prevent the camera from leaving the playable area.
     */
    float player_collision_radius_{ 0.25f };

    float map_min_x_{ -15.0f };
    float map_max_x_{  15.0f };
    float map_min_y_{ -15.5f };
    float map_max_y_{  15.0f };
    float map_min_z_{ -15.0f };
    float map_max_z_{  15.0f };

    /*
     * True during a frame when movement or push-out detects a collision.
     * Used by the HUD and particle effect spawning.
     */
    bool collision_active_{ false };

    /*
     * Scene object transform/collision helpers.
     */
    glm::vec3 get_object_position_(const SceneObject& object, float time) const;
    float get_object_radius_(const SceneObject& object) const;

    /*
     * Collision tests.
     */
    bool is_inside_map_(const glm::vec3& position) const;
    bool collides_with_scene_(const glm::vec3& position, float time) const;
    bool is_valid_player_position_(const glm::vec3& position, float time) const;

    /*
     * Camera movement helpers.
     *
     * try_move_camera_ attempts full movement first and then tries simple
     * X/Z sliding if full movement is blocked.
     *
     * push_camera_out_of_collisions_ fixes cases where a moving object enters
     * the player/camera collision sphere.
     */
    void try_move_camera_(const glm::vec3& movement, float time);
    void push_camera_out_of_collisions_(float time);

    // Custom shader effect: distance fog
    /*
     * Distance fog shader effect.
     *
     * The fragment shader blends the final lit color with fog_color_ according
     * to the distance from the camera.
     *
     * fog_near_ = distance where fog starts becoming visible.
     * fog_far_  = distance where the object is almost fully fog-colored.
     */
    bool fog_enabled_{ true };
    glm::vec3 fog_color_{ 0.08f, 0.08f, 0.10f };
    float fog_near_{ 11.0f };
    float fog_far_{ 25.0f };

    /*
     * One CPU-side particle.
     *
     * Particles are rendered as small transparent cubes. They have a position,
     * velocity, lifetime, size and color. Alpha is faded according to remaining
     * lifetime during rendering.
     */
    struct Particle {
        glm::vec3 position{ 0.0f, 0.0f, 0.0f };
        glm::vec3 velocity{ 0.0f, 0.0f, 0.0f };

        float lifetime{ 1.0f };
        float max_lifetime{ 1.0f };
        float size{ 0.05f };

        glm::vec4 color{ 1.0f, 0.6f, 0.1f, 1.0f };
    };

    // Particle effect
    /*
     * Particle rendering resources and runtime storage.
     *
     * The particles reuse a normal mesh and texture, so no separate particle
     * shader is needed.
     */
    std::shared_ptr<Mesh> particle_mesh_;
    std::shared_ptr<Texture> particle_texture_;

    glm::vec2 particle_uv_offset_{ 0.0f, 0.0f };
    glm::vec2 particle_uv_scale_{ 1.0f, 1.0f };

    std::vector<Particle> particles_;

    /*
     * Cooldowns for particle spawning.
     *
     * Collision particles should not spawn every frame while the player is
     * touching an obstacle. Trail particles also spawn at a controlled rate.
     */
    double last_collision_particle_time_{ -100.0 };
    double last_trail_particle_time_{ 0.0 };

    /*
     * Index of the scene object used for the particle trail.
     * 1 means the second object in scene_objects_.
     */
    std::size_t trail_object_index_{ 1 };

    /*
     * Creates a burst of particles at origin.
     *
     * Used both for:
     * - collision burst
     * - trail effect behind the selected moving object
     */
    void spawn_particles_(
        const glm::vec3& origin,
        int count,
        float speed,
        float lifetime,
        const glm::vec4& color
    );

    /*
     * Updates all active particles and removes dead particles.
     */
    void update_particles_(float dt);

    // loaded assets
    /*
     * Main shader and all scene objects.
     */
    std::shared_ptr<ShaderProgram> shader_;
    std::vector<SceneObject> scene_objects_;

    // FPS / title update
    /*
     * FPS measurement.
     *
     * FPS is averaged over a short interval and displayed in the window title
     * and ImGui HUD.
     */
    double fps_last_t_{ 0.0 };
    double fps_accum_dt_{ 0.0 };
    int fps_frames_{ 0 };
    double fps_value_{ 0.0 };
    double title_last_t_{ 0.0 };

    // camera
    /*
     * Camera and projection state.
     *
     * cam_pos_ is the player position.
     * cam_front_ is the current look direction.
     * yaw_deg_ and pitch_deg_ are updated from mouse movement.
     * fov_deg_ can be changed by the mouse wheel.
     */
    float fov_deg_{60.0f};
    float znear_{0.1f};
    float zfar_{100.0f};
    glm::mat4 proj_{1.0f};
    glm::vec3 cam_pos_{0.0f, 0.0f, 2.0f};
    glm::vec3 cam_front_{0.0f, 0.0f, -1.0f};
    glm::vec3 cam_up_{0.0f, 1.0f, 0.0f};
    float cam_speed_{2.5f};
    bool first_mouse_{true};
    double last_x_{0.0};
    double last_y_{0.0};
    float yaw_deg_{-90.0f};
    float pitch_deg_{0.0f};
    float mouse_sensitivity_{0.12f};

    /*
     * Directional light, used as animated sunlight.
     *
     * The base direction is stored here, but the actual direction can be
     * animated in app.cpp and sent to the shader every frame.
     */
    struct DirectionalLight {
        glm::vec3 direction{ -0.5f, -1.0f, -0.3f };
        glm::vec3 ambient{ 0.2f, 0.2f, 0.2f };
        glm::vec3 diffuse{ 0.8f, 0.8f, 0.8f };
        glm::vec3 specular{ 1.0f, 1.0f, 1.0f };
    };

    DirectionalLight sun_{};

    /*
     * Point light.
     *
     * base_position is transformed/animated in app.cpp. The transformed view
     * space position is then sent to the shader.
     *
     * constant/linear/quadratic are prepared attenuation parameters. They can
     * be used in the shader if attenuation is enabled.
     */
    struct PointLight {
        glm::vec3 base_position{ 2.0f, 1.0f, 0.0f };
        glm::vec3 ambient{ 0.05f, 0.05f, 0.05f };
        glm::vec3 diffuse{ 1.0f, 1.0f, 1.0f };
        glm::vec3 specular{ 1.0f, 1.0f, 1.0f };
        float constant{ 1.0f };
        float linear{ 0.09f };
        float quadratic{ 0.032f };
    };

    /*
     * Spotlight / reflector attached to the camera.
     *
     * The spotlight direction is computed in view space in the shader.
     */
    struct SpotLight {
        glm::vec3 diffuse{ 0.8f, 0.8f, 0.8f };
        glm::vec3 specular{ 1.0f, 1.0f, 1.0f };

        float cutoff_deg{ 20.0f };
        float exponent{ 16.0f };
    };

    SpotLight spot_light_{};

    /*
     * Material parameters used by the lighting shader.
     */
    glm::vec3 material_ambient_{ 0.05f, 0.05f, 0.05f };
    glm::vec3 material_diffuse_{ 1.0f, 1.0f, 1.0f };
    glm::vec3 material_specular_{ 1.0f, 1.0f, 1.0f };
    float material_shininess_{ 32.0f };

    /*
     * Three point lights used to satisfy the lighting requirement.
     *
     * Their base positions are animated in app.cpp before being sent as
     * view-space light_position uniforms.
     */
    PointLight point_light_{
        glm::vec3( 2.5f,  0.8f,  0.0f),
        glm::vec3(0.05f, 0.05f, 0.05f),
        glm::vec3(1.00f, 1.00f, 1.00f),
        glm::vec3(1.00f, 1.00f, 1.00f),
        1.0f, 0.09f, 0.032f
    };

    PointLight point_light1_{
        glm::vec3(-3.0f, -0.3f,  0.0f),
        glm::vec3(0.00f, 0.03f, 0.00f),
        glm::vec3(0.30f, 1.00f, 0.30f),
        glm::vec3(0.30f, 1.00f, 0.30f),
        1.0f, 0.09f, 0.032f
    };

    PointLight point_light2_{
        glm::vec3( 1.8f,  1.6f,  0.0f),
        glm::vec3(0.00f, 0.00f, 0.03f),
        glm::vec3(0.30f, 0.30f, 1.00f),
        glm::vec3(0.30f, 0.30f, 1.00f),
        1.0f, 0.09f, 0.032f
    };
};
