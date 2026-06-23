#pragma once

/*
 * Texture.hpp
 * ---------------------------------------------------------------------------
 * RAII wrapper around an OpenGL 2D texture.
 *
 * Responsibilities:
 * - load image files through OpenCV
 * - upload image data to OpenGL texture objects
 * - support RGB, RGBA and grayscale textures
 * - support synthetic one-color textures
 * - support a default checkerboard texture
 * - configure texture filtering and wrapping
 * - release OpenGL texture object in destructor
 *
 * The implementation uses OpenGL Direct State Access texture functions such as:
 * - glCreateTextures
 * - glTextureStorage2D
 * - glTextureSubImage2D
 * - glTextureParameteri
 * - glBindTextureUnit
 *
 * That fits the assignment requirement for DSA usage.
 * ---------------------------------------------------------------------------
 */

#include "non_copyable.hpp"
#include <filesystem>
#include <opencv2/opencv.hpp>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Texture: private NonCopyable
{
public:
    /*
     * Texture filtering modes.
     */
    enum class Interpolation {
        nearest,
        linear,
        linear_mipmap_linear,
    };

    /*
     * Default constructor creates an empty Texture object.
     *
     * The actual checkerboard OpenGL texture is created lazily in bind(),
     * because OpenGL context must already exist before texture creation.
     */
    Texture() = default;

    /*
     * Creates a texture from an existing OpenCV image.
     */
    Texture(const cv::Mat & image, Interpolation interpolation = Interpolation::linear_mipmap_linear); // default = best texture filtering

    /*
     * Creates a synthetic 1x1 RGB texture from a color.
     */
    Texture(const glm::vec3 & vec); // synthetic single-color RGB texture

    /*
     * Creates a synthetic 1x1 RGBA texture from a color with alpha.
     */
    Texture(const glm::vec4 & vec); // synthetic single-color RGBA texture

    /*
     * Loads an image from file and creates an OpenGL texture.
     */
    Texture(const std::filesystem::path & path, Interpolation interpolation = Interpolation::linear_mipmap_linear);

    /*
     * Deletes the owned OpenGL texture object.
     */
    ~Texture();

    /*
     * Binds texture to texture unit 0.
     *
     * If this object does not own a texture yet, it binds the default
     * checkerboard texture.
     */
    void bind(void);

    /*
     * Returns raw OpenGL texture object name/ID.
     */
    GLuint get_name() const;

    /*
     * Returns texture dimensions queried from OpenGL.
     */
    int get_height(void);
    int get_width(void);

    /*
     * Changes texture filtering mode.
     */
    void set_interpolation(Interpolation interpolation);

    /*
     * Replaces texture data.
     *
     * The new image must have the same size and compatible channel format.
     */
    void replace_image(const cv::Mat& image);

private:
    /*
     * Loads image data from disk through OpenCV.
     */
    cv::Mat load_image(const std::filesystem::path& path);

    /*
     * Creates the shared checkerboard fallback texture.
     */
    static GLuint gen_ckboard(void);  // create default texture

    // NOTE: do NOT call gen_ckboard() in static init (OpenGL context not ready yet)
    GLuint name_{ 0 }; // set default-constructed texture to ckboard pattern lazily
};
