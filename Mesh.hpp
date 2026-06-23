#pragma once

/*
 * Mesh.hpp
 * ---------------------------------------------------------------------------
 * Lightweight RAII wrapper around OpenGL mesh resources.
 *
 * This class owns:
 * - VAO: Vertex Array Object
 * - VBO: Vertex Buffer Object
 * - optional EBO: Element Buffer Object / index buffer
 *
 * The class uses modern OpenGL Direct State Access functions:
 * - glCreateVertexArrays
 * - glCreateBuffers
 * - glNamedBufferData
 * - glVertexArrayVertexBuffer
 * - glVertexArrayAttribFormat
 * - glVertexArrayElementBuffer
 *
 * This is important for the assignment because DSA is required and old
 * compatibility/fixed-pipeline OpenGL must not be used.
 *
 * Mesh is non-copyable, because OpenGL object IDs are owned resources.
 * Copying them would risk double deletion. The destructor releases all OpenGL
 * buffers and the VAO.
 * ---------------------------------------------------------------------------
 */

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
    /*
     * Fixed vertex attribute locations.
     *
     * All meshes and shaders use the same attribute layout:
     * location 0 = vertex position
     * location 1 = vertex normal
     * location 2 = texture coordinates
     *
     * Vertex shaders must match these locations.
     */
    // force attribute slots in shaders for all meshes, shaders etc.
    static constexpr GLuint attribute_location_position{0};
    static constexpr GLuint attribute_location_normal{1};
    static constexpr GLuint attribute_location_texture_coords{2};

    /*
     * Disabled default constructor.
     *
     * A Mesh object should only exist when it has valid vertex data and valid
     * OpenGL resources. This follows RAII: constructed object = usable object.
     */
    // No default constructor. RAII - if constructed, it will be correctly initialized
    // and can be rendered. OpenGL resources are guaranteed to be deallocated using destructor.
    // Double-free errors are prevented by making class non-copyable (therefore
    // double destruction of the same OpenGL buffer is prevented).
    Mesh() = delete;

    /*
     * Creates a non-indexed mesh.
     *
     * Parameters:
     * - vertices: vertex buffer data containing position, normal and UV
     * - primitive_type: OpenGL primitive type, usually GL_TRIANGLES
     *
     * This constructor creates:
     * - VAO
     * - VBO
     *
     * Then it describes the Vertex structure layout to OpenGL.
     */
    // mesh from vert
    Mesh(std::vector<Vertex> const &vertices, GLenum primitive_type) : primitive_type_{primitive_type}
    {
        vertices_ = vertices;

        // Create VAO and buffers
        glCreateVertexArrays(1, &vao_);
        glCreateBuffers(1, &vbo_);

        /*
         * Upload all vertex data to the GPU.
         *
         * GL_STATIC_DRAW is used because mesh geometry does not change every frame.
         */
        glNamedBufferData(vbo_,
                          static_cast<GLsizeiptr>(vertices_.size() * sizeof(Vertex)),
                          vertices_.data(),
                          GL_STATIC_DRAW);

        // binding index 0: vertex buffer
        glVertexArrayVertexBuffer(vao_, 0, vbo_, 0, static_cast<GLsizei>(sizeof(Vertex)));

        /*
         * Configure vertex attributes.
         *
         * offsetof(Vertex, ...) is used so the GPU reads the correct fields
         * from the interleaved Vertex structure.
         */

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

    /*
     * Creates an indexed mesh.
     *
     * This constructor first creates the normal vertex mesh using the other
     * constructor and then adds an EBO/index buffer.
     *
     * Indexed drawing avoids duplicated vertices and is used for OBJ models.
     */
    // Mesh with indirect vert addressing
    Mesh(std::vector<Vertex> const &vertices, std::vector<GLuint> const &indices, GLenum primitive_type) :
        Mesh(vertices, primitive_type)
    {
        indices_ = indices;

        /*
         * Upload index data and attach it to the VAO.
         *
         * After glVertexArrayElementBuffer, this VAO remembers which EBO
         * belongs to it.
         */
        glCreateBuffers(1, &ebo_);
        glNamedBufferData(ebo_,
                          static_cast<GLsizeiptr>(indices_.size() * sizeof(GLuint)),
                          indices_.data(),
                          GL_STATIC_DRAW);

        glVertexArrayElementBuffer(vao_, ebo_);
    }

    /*
     * Draws the mesh.
     *
     * If there is no EBO, the mesh is rendered with glDrawArrays.
     * If an EBO exists, the mesh is rendered with glDrawElements.
     */
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

    /*
     * Releases all owned OpenGL resources.
     *
     * Deleting zero is safe in OpenGL, so it is fine even for meshes without EBO.
     */
    ~Mesh()
    {
        glDeleteBuffers(1, &ebo_);
        glDeleteBuffers(1, &vbo_);
        glDeleteVertexArrays(1, &vao_);
    }

private:
    /*
     * Primitive type used for rendering.
     * Usually GL_TRIANGLES in this project.
     */
    GLenum primitive_type_{GL_POINTS};

    /*
     * CPU-side copies of vertices and indices.
     *
     * They are also useful for draw counts and debugging.
     */
    std::vector<Vertex> vertices_;
    std::vector<GLuint> indices_;

    /*
     * OpenGL object IDs.
     *
     * vao_ stores vertex attribute configuration.
     * vbo_ stores vertex data.
     * ebo_ stores indices, if the mesh is indexed.
     */
    GLuint vao_{0};
    GLuint vbo_{0};
    GLuint ebo_{0};
};
