#pragma once

#include <GL/glew.h>
#include <GL/wglew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

//vertex description
struct Vertex {
    glm::vec3 Position{};
    glm::vec3 Normal{};
    glm::vec2 TexCoords{};

    bool operator == (const Vertex& v1) const {
        return (Position == v1.Position
            && Normal == v1.Normal
            && TexCoords == v1.TexCoords);
    }
};