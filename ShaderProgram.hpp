// ===== file: ShaderProgram.hpp =====
#pragma once

/*
 * ShaderProgram.hpp
 * ---------------------------------------------------------------------------
 * RAII wrapper around an OpenGL shader program.
 *
 * Responsibilities:
 * - compile vertex and fragment shaders
 * - link shader program
 * - activate/deactivate shader program
 * - set uniforms by name
 * - cache uniform locations
 * - delete shader program in destructor
 *
 * The class is non-copyable because it owns an OpenGL program object.
 * Copying would risk double deletion of the same OpenGL resource.
 * ---------------------------------------------------------------------------
 */

#include <string>
#include <filesystem>
#include <unordered_map>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "non_copyable.hpp"

class ShaderProgram : private NonCopyable {
public:
    /*
     * No default constructor.
     *
     * A ShaderProgram object should only exist when a real OpenGL program
     * was compiled and linked successfully.
     */
    // No default constructor. RAII - if constructed, it will be correctly initialized
    // and can be rendered. OpenGL resources are guaranteed to be deallocated using destructor.
    // Double-free errors are prevented by making class non-copyable (therefore
    // double destruction of the same OpenGL shader is prevented).
    ShaderProgram(void) = delete; //does nothing

    /*
     * Creates shader program directly from GLSL source strings.
     */
    // you can add more constructors for pipeline with GS, TS etc.
    ShaderProgram(std::string const & vertex_shader_code, std::string const & fragment_shader_code);

    //std::filesystem::path & read_text_file(std::filesystem::path::iterator::reference path); // check if correct

    /*
     * Creates shader program from vertex and fragment shader files.
     */
    ShaderProgram(std::filesystem::path const & VS_file, std::filesystem::path const & FS_file);

    /*
     * Activates this shader program.
     *
     * currently_used avoids unnecessary repeated glUseProgram calls.
     */
    // activate shader
    void use(void) {
        if (ID == currently_used) // already being used
            return;
        else {
            glUseProgram(ID);
            currently_used = ID;
        }
    };

    /*
     * Sets vec2 uniform.
     */
    void setUniform(const std::string& name, const glm::vec2& val);

    /*
     * Deactivates currently active shader program.
     */
    // deactivate current shader program (i.e. activate shader no. 0)
    void deactivate(void) {
        glUseProgram(0);
        currently_used = 0;
    };

    /*
     * Deletes owned OpenGL shader program.
     */
    ~ShaderProgram(void) {  //deallocate shader program
        deactivate();
        glDeleteProgram(ID);
        ID = 0;
    }

    /*
     * Returns OpenGL program object ID.
     */
    GLuint getID(void) { return ID; }

    /*
     * Queries vertex attribute location.
     */
    GLint  getAttribLocation(const std::string & name);

    // set uniform according to name
    // https://docs.gl/gl4/glUniform
    /*
     * Uniform setters.
     *
     * These overloads allow C++ code to send common data types to GLSL
     * uniforms by name.
     */
    void setUniform(const std::string & name, const GLfloat val);
    void setUniform(const std::string & name, const GLint val);
    void setUniform(const std::string & name, const glm::vec3 & val);
    void setUniform(const std::string & name, const glm::vec4 & val);
    void setUniform(const std::string & name, const glm::mat3 & val);
    void setUniform(const std::string & name, const glm::mat4 & val);
    void setUniform(const std::string & name, const std::vector<GLint> & val);
    void setUniform(const std::string & name, const std::vector<GLfloat> & val);
    void setUniform(const std::string & name, const std::vector<glm::vec3> & val);

private:
    /*
     * OpenGL program ID.
     *
     * 0 means no valid shader program.
     */
    GLuint ID{0}; // default = 0, empty shader

    /*
     * Tracks currently bound shader program.
     *
     * Inline static means there is one shared value for all ShaderProgram
     * objects.
     */
    inline static GLuint currently_used{0};

    /*
     * Uniform location cache.
     *
     * glGetUniformLocation is relatively expensive and can spam errors if
     * called repeatedly. This map stores both valid locations and -1 results.
     */
    std::unordered_map<std::string, GLint> uniform_location_cache;

    /*
     * Gets uniform location from cache or from OpenGL.
     */
    GLint getUniformLocation(const std::string & name);

    /*
     * Reads an entire text file into a string.
     */
    std::string textFileRead(const std::filesystem::path & filename); // load text file

    /*
     * Compiles one GLSL shader object.
     */
    GLuint compile_shader(const std::string & source_code, const GLenum type);

    /*
     * Returns shader compilation log.
     */
    std::string getShaderInfoLog(const GLuint obj);

    /*
     * Links compiled shader objects into one program.
     */
    GLuint link_shader(const std::vector<GLuint> shader_ids);

    /*
     * Returns program linking log.
     */
    std::string getProgramInfoLog(const GLuint obj);
};
