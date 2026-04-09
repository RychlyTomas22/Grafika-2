#pragma once

#include "non_copyable.hpp"
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Texture: private NonCopyable
{
public:
    enum class Interpolation {
        nearest,
        linear,
        linear_mipmap_linear,
    };

    Texture() = default;
    Texture(const cv::Mat & image, Interpolation interpolation = Interpolation::linear_mipmap_linear); // default = best texture filtering
    Texture(const glm::vec3 & vec); // synthetic single-color RGB texture
    Texture(const glm::vec4 & vec); // synthetic single-color RGBA texture
    Texture(const std::filesystem::path & path, Interpolation interpolation = Interpolation::linear_mipmap_linear);

    ~Texture();

    void bind(void);
    GLuint get_name() const;
    int get_height(void);
    int get_width(void);
    void set_interpolation(Interpolation interpolation);
    void replace_image(const cv::Mat& image);
private:
    cv::Mat load_image(const std::filesystem::path& path);
    static GLuint gen_ckboard(void);  // create default texture

    // NOTE: do NOT call gen_ckboard() in static init (OpenGL context not ready yet)
    GLuint name_{ 0 }; // set default-constructed texture to ckboard pattern lazily
};