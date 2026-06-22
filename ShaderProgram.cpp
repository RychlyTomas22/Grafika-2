/*
 * ShaderProgram.cpp
 * ---------------------------------------------------------------------------
 * Implementation of the ShaderProgram RAII wrapper.
 *
 * The class handles:
 * - loading shader code from strings or files
 * - compiling vertex and fragment shaders
 * - linking them into an OpenGL shader program
 * - setting uniforms using Direct State Access calls
 * - caching uniform locations to avoid repeated glGetUniformLocation calls
 *
 * The project uses modern OpenGL functions such as glProgramUniform*.
 * These functions set uniforms directly on a program object and do not require
 * the program to be currently active with glUseProgram.
 * ---------------------------------------------------------------------------
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <stdexcept>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "ShaderProgram.hpp"

// set uniform according to name
// https://docs.gl/gl4/glUniform

/*
 * Optional Mesh.hpp include.
 *
 * If Mesh.hpp is available, ShaderProgram binds predefined vertex attribute
 * locations before linking. This keeps shader inputs and VAO attribute
 * locations consistent.
 */
#if __has_include("Mesh.hpp")
    #include "Mesh.hpp"
    #define SHADERPROGRAM_HAS_MESH 1
#else
    #define SHADERPROGRAM_HAS_MESH 0
#endif

/*
 * Creates a shader program directly from source code strings.
 *
 * The constructor compiles both shaders and links them into one GPU program.
 * If compilation or linking fails, an exception is thrown.
 */
ShaderProgram::ShaderProgram(const std::string & vertex_shader_code, const std::string & fragment_shader_code) {
    // compile shaders and store IDs for linker
    auto vertex_shader   = compile_shader(vertex_shader_code, GL_VERTEX_SHADER);
    auto fragment_shader = compile_shader(fragment_shader_code, GL_FRAGMENT_SHADER);

    std::vector<GLuint> shader_ids{vertex_shader, fragment_shader};

    // link all compiled shaders into shader program
    ID = link_shader(shader_ids);
}

/*
 * Creates a shader program from two shader files.
 *
 * The files are read as text and then passed to the source-code constructor.
 */
ShaderProgram::ShaderProgram(const std::filesystem::path & VS_file, const std::filesystem::path & FS_file) :
    ShaderProgram{textFileRead(VS_file), textFileRead(FS_file)} {}

// Get location or write error to console
GLint ShaderProgram::getUniformLocation(const std::string & name) {
    // deferred (lazy) cache generation

    // Check if the location is already cached
    auto it = uniform_location_cache.find(name);
    if (it != uniform_location_cache.end()) {
        return it->second;
    }

    /*
     * Query uniform location from OpenGL.
     *
     * A result of -1 usually means:
     * - uniform does not exist
     * - uniform was optimized out by the GLSL compiler
     * - name is wrong
     */
    // Get the location and cache it
    auto loc = glGetUniformLocation(ID, name.c_str());
    if (loc == -1) {
        std::cerr << "No uniform with name: " << name << '\n';
    }
    uniform_location_cache[name] = loc; // cache even -1 to avoid repeated spam
    return loc;
}

/*
 * Returns vertex attribute location by name.
 *
 * Mostly useful for debugging. In this project, attribute locations are
 * normally fixed before shader linking.
 */
GLint ShaderProgram::getAttribLocation(const std::string & name) {
    GLint loc = glGetAttribLocation(ID, name.c_str());
    if (loc == -1) {
        std::cerr << "No vertex attribute with name: " << name << ", or reserved name (starting with gl_)\n";
        return loc;
    }
    return loc;
}

/*
 * Uniform setters.
 *
 * These functions use DSA glProgramUniform* calls. The shader program does not
 * need to be currently bound by glUseProgram to update a uniform.
 */

void ShaderProgram::setUniform(const std::string& name, const glm::vec2& val) {
    auto loc = getUniformLocation(name);
    glProgramUniform2fv(ID, loc, 1, glm::value_ptr(val));
}

// Uniform setting

void ShaderProgram::setUniform(const std::string& name, const GLfloat val) {
    auto loc = getUniformLocation(name);
    glProgramUniform1f(ID, loc, val);
}

void ShaderProgram::setUniform(const std::string& name, const GLint val) {
    auto loc = getUniformLocation(name);
    glProgramUniform1i(ID, loc, val);
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec3 & val) {
    auto loc = getUniformLocation(name);
    glProgramUniform3fv(ID, loc, 1, glm::value_ptr(val));
}

void ShaderProgram::setUniform(const std::string& name, const glm::vec4 & in_vec4) {
    auto loc = getUniformLocation(name);
    glProgramUniform4fv(ID, loc, 1, glm::value_ptr(in_vec4));
}

void ShaderProgram::setUniform(const std::string& name, const glm::mat3 & val) {
    auto loc = getUniformLocation(name);
    glProgramUniformMatrix3fv(ID, loc, 1, GL_FALSE, glm::value_ptr(val));
}

void ShaderProgram::setUniform(const std::string& name, const glm::mat4 & val) {
    auto loc = getUniformLocation(name);
    glProgramUniformMatrix4fv(ID, loc, 1, GL_FALSE, glm::value_ptr(val));
}

void ShaderProgram::setUniform(const std::string & name, const std::vector<GLint>& val) {
    auto loc = getUniformLocation(name);
    glProgramUniform1iv(ID, loc, static_cast<GLsizei>(val.size()), reinterpret_cast<GLint const*>(val.data()));
}

void ShaderProgram::setUniform(const std::string & name, const std::vector<GLfloat>& val) {
    auto loc = getUniformLocation(name);
    glProgramUniform1fv(ID, loc, static_cast<GLsizei>(val.size()), reinterpret_cast<GLfloat const*>(val.data()));
}

void ShaderProgram::setUniform(const std::string & name, const std::vector<glm::vec3>& val) {
    auto loc = getUniformLocation(name);
    if (val.empty())
        return;
    glProgramUniform3fv(ID, loc, static_cast<GLsizei>(val.size()), glm::value_ptr(val[0]));
}

/*
 * Reads and returns the compile log of one shader object.
 */
std::string ShaderProgram::getShaderInfoLog(const GLuint obj) {
    int log_length = 0;
    std::string s;
    glGetShaderiv(obj, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 0) {
        std::vector<char> v(log_length);
        glGetShaderInfoLog(obj, log_length, nullptr, v.data());
        s.assign(begin(v), end(v));
    }
    return s;
}

/*
 * Reads and returns the link log of one shader program.
 */
std::string ShaderProgram::getProgramInfoLog(const GLuint obj) {
    int log_length = 0;
    std::string s;
    glGetProgramiv(obj, GL_INFO_LOG_LENGTH, &log_length);
    if (log_length > 0) {
        std::vector<char> v(log_length);
        glGetProgramInfoLog(obj, log_length, nullptr, v.data());
        s.assign(begin(v), end(v));
    }
    return s;
}

/*
 * Compiles one shader.
 *
 * Parameters:
 * - source_code: GLSL source code
 * - type: GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.
 *
 * Returns:
 * - OpenGL shader object ID
 *
 * Throws:
 * - std::runtime_error if compilation fails
 */
GLuint ShaderProgram::compile_shader(const std::string & source_code, const GLenum type) {
    char const *src_cstr = source_code.c_str();

    GLuint shader_ID = glCreateShader(type);

    glShaderSource(shader_ID, 1, &src_cstr, nullptr);
    glCompileShader(shader_ID);
    {
        GLint status;
        glGetShaderiv(shader_ID, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            std::cerr << getShaderInfoLog(shader_ID) << std::endl;
            glDeleteShader(shader_ID);
            throw std::runtime_error("Shader compilation failed.");
        }
    }

    return shader_ID;
}

/*
 * Links compiled shaders into one shader program.
 *
 * The function:
 * 1. creates program object
 * 2. attaches compiled shaders
 * 3. binds known vertex attribute locations
 * 4. links the program
 * 5. detaches and deletes shader objects
 * 6. checks link status
 *
 * After linking, individual shader objects are no longer needed.
 */
GLuint ShaderProgram::link_shader(const std::vector<GLuint> shader_ids) {
    GLuint prog_ID = glCreateProgram();

    for (const auto & id : shader_ids)
        glAttachShader(prog_ID, id);

    // force OpenGL to use specific slots(locations) for certain vertex attributes,
    // must be set before linking
#if SHADERPROGRAM_HAS_MESH
    glBindAttribLocation(prog_ID, Mesh::attribute_location_position, "aPos");
    glBindAttribLocation(prog_ID, Mesh::attribute_location_normal, "aNorm");
    glBindAttribLocation(prog_ID, Mesh::attribute_location_texture_coords, "aTex");

    glBindAttribLocation(prog_ID, Mesh::attribute_location_position, "aPosition");
    glBindAttribLocation(prog_ID, Mesh::attribute_location_normal, "aNormal");
    glBindAttribLocation(prog_ID, Mesh::attribute_location_texture_coords, "aTexCoord");
#endif

    glLinkProgram(prog_ID);

    for (const auto& id : shader_ids) {
        glDetachShader(prog_ID, id);
        glDeleteShader(id);
    }

    // check link result, print info & throw error (if any)
    {
        GLint status;
        glGetProgramiv(prog_ID, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            std::cerr << "Error linking shader program." << std::endl;
            std::cerr << getProgramInfoLog(prog_ID) << std::endl;
            glDeleteProgram(prog_ID);
            throw std::runtime_error("Shader linking failed.");
        }
    }
    return prog_ID;
}

/*
 * Reads a whole text file into a std::string.
 *
 * Used for loading .vert and .frag shader files.
 */
std::string ShaderProgram::textFileRead(const std::filesystem::path& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error(std::string("Error opening file: ") + filepath.string());
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
