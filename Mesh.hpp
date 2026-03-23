#pragma once

#include <string>
#include <vector>
#include <cstddef>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "assets.hpp"
#include "non_copyable.hpp"

class Mesh: private NonCopyable
{
public:
    // force attribute slots in shaders for all meshes, shaders etc.
    static constexpr GLuint attribute_location_position{0};
    static constexpr GLuint attribute_location_normal{1};
    static constexpr GLuint attribute_location_texture_coords{2};

    // No default constructor. RAII - if constructed, it will be correctly initialized
    // and can be rendered. OpenGL resources are guaranteed to be deallocated using destructor.
    // Double-free errors are prevented by making class non-copyable (therefore
    // double destruction of the same OpenGL buffer is prevented).
    Mesh() = delete;

    // mesh from vert
    Mesh(std::vector<Vertex> const &vertices, GLenum primitive_type) : primitive_type_{primitive_type}
    {
        vertices_ = vertices;

        // Create VAO and buffers
        glCreateVertexArrays(1, &vao_);
        glCreateBuffers(1, &vbo_);

        glNamedBufferData(vbo_,
                          static_cast<GLsizeiptr>(vertices_.size() * sizeof(Vertex)),
                          vertices_.data(),
                          GL_STATIC_DRAW);

        // binding index 0: vertex buffer
        glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, static_cast<GLsizei>(sizeof(Vertex)));

        // Position
        glEnableVertexArrayAttrib(vao_, attribute_location_position);
        glVertexArrayAttribFormat(vao_,
                                  attribute_location_position,
                                  glm::vec3::length(),
                                  GL_FLOAT,
                                  GL_FALSE,
                                  static_cast<GLuint>(offsetof(Vertex, Position)));
        glVertexArrayAttribBinding(vao_, attribute_location_position, 0);

        // Normal
        glEnableVertexArrayAttrib(vao_, attribute_location_normal);
        glVertexArrayAttribFormat(vao_,
                                  attribute_location_normal,
                                  glm::vec3::length(),
                                  GL_FLOAT,
                                  GL_FALSE,
                                  static_cast<GLuint>(offsetof(Vertex, Normal)));
        glVertexArrayAttribBinding(vao_, attribute_location_normal, 0);

        // TexCoords
        glEnableVertexArrayAttrib(vao_, attribute_location_texture_coords);
        glVertexArrayAttribFormat(vao_,
                                  attribute_location_texture_coords,
                                  glm::vec2::length(),
                                  GL_FLOAT,
                                  GL_FALSE,
                                  static_cast<GLuint>(offsetof(Vertex, TexCoords)));
        glVertexArrayAttribBinding(vao_, attribute_location_texture_coords, 0);
    }

    // Mesh with indirect vert addressing
    Mesh(std::vector<Vertex> const &vertices, std::vector<GLuint> const &indices, GLenum primitive_type) :
        Mesh(vertices, primitive_type)
    {
        indices_ = indices;

        glCreateBuffers(1, &ebo_);
        glNamedBufferData(ebo_,
                          static_cast<GLsizeiptr>(indices_.size() * sizeof(GLuint)),
                          indices_.data(),
                          GL_STATIC_DRAW);

        glVertexArrayElementBuffer(vao_, ebo_);
    }

    void draw() const
    {
        glBindVertexArray(vao_);
        if (ebo_ == 0) {
            glDrawArrays(primitive_type_, 0, static_cast<GLsizei>(vertices_.size()));
        } else {
            glDrawElements(primitive_type_,
                           static_cast<GLsizei>(indices_.size()),
                           GL_UNSIGNED_INT,
                           nullptr);
        }
    }

    ~Mesh()
    {
        glDeleteBuffers(1, &ebo_);
        glDeleteBuffers(1, &vbo_);
        glDeleteVertexArrays(1, &vao_);
    }

private:
    GLenum primitive_type_{GL_POINTS};

    std::vector<Vertex> vertices_;
    std::vector<GLuint> indices_;

    GLuint vao_{0};
    GLuint vbo_{0};
    GLuint ebo_{0};
};