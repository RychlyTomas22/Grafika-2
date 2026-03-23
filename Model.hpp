#pragma once

#include <filesystem>
#include <string>
#include <vector>
#include <memory>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>  // translate/scale
#include <glm/gtx/euler_angles.hpp>      // yawPitchRoll

#include "assets.hpp"
#include "Mesh.hpp"
#include "ShaderProgram.hpp"

class Model {
public:
    // origin point of whole model
    glm::vec3 pivot_position{}; // [0,0,0] of the object
    glm::vec3 eulerAngles{};    // pitch, yaw, roll (radians)
    glm::vec3 scale{1.0f};

    struct mesh_package {
        std::shared_ptr<Mesh> mesh;                 // geometry & topology, vertex attributes
        std::shared_ptr<ShaderProgram> shader;      // which shader to use to draw this part of the model

        glm::vec3 origin{0.0f};                     // mesh origin relative to origin of the whole model
        glm::vec3 eulerAngles{0.0f};                // mesh rotation relative to orientation of the whole model
        glm::vec3 scale{1.0f};                      // mesh scale relative to scale of the whole model

        // cache
        mutable bool dirty{true};
        mutable glm::mat4 cached_local{1.0f};

        void mark_dirty() const { dirty = true; }
    };

    std::vector<mesh_package> meshes;

    Model() = default;

    void mark_dirty() const { model_dirty_ = true; }

    void addMesh(std::shared_ptr<Mesh> mesh,
                 std::shared_ptr<ShaderProgram> shader,
                 glm::vec3 origin = glm::vec3(0.0f),
                 glm::vec3 eulerAngles = glm::vec3(0.0f),
                 glm::vec3 scale = glm::vec3(1.0f))
    {
        mesh_package pkg{};
        pkg.mesh = std::move(mesh);
        pkg.shader = std::move(shader);
        pkg.origin = origin;
        pkg.eulerAngles = eulerAngles;
        pkg.scale = scale;
        pkg.dirty = true;
        meshes.push_back(std::move(pkg));
    }

    // update based on running time
    void update(const float /*delta_t*/) {
        // future: animate transforms then call mark_dirty()/pkg.mark_dirty()
    }

    void draw() const {
        const glm::mat4 model_m = get_model_matrix_();

        // call draw() on mesh (all meshes)
        for (auto const& pkg : meshes) {
            const glm::mat4 local_m = get_local_matrix_(pkg);
            const glm::mat4 final_m = model_m * local_m;

            pkg.shader->use(); // select proper shader
            pkg.shader->setUniform("uM_m", final_m);
            pkg.mesh->draw();  // draw mesh
        }
    }

private:
    mutable bool model_dirty_{true};
    mutable glm::mat4 cached_model_{1.0f};

    static glm::mat4 compose_trs_(const glm::vec3& t,
                                 const glm::vec3& euler_radians,
                                 const glm::vec3& s)
    {
        const glm::mat4 T = glm::translate(glm::mat4(1.0f), t);
        const glm::mat4 R = glm::yawPitchRoll(euler_radians.y, euler_radians.x, euler_radians.z);
        const glm::mat4 S = glm::scale(glm::mat4(1.0f), s);
        return T * R * S;
    }

    glm::mat4 get_model_matrix_() const {
        if (!model_dirty_) return cached_model_;
        cached_model_ = compose_trs_(pivot_position, eulerAngles, scale);
        model_dirty_ = false;
        return cached_model_;
    }

    glm::mat4 get_local_matrix_(mesh_package const& pkg) const {
        if (!pkg.dirty) return pkg.cached_local;
        pkg.cached_local = compose_trs_(pkg.origin, pkg.eulerAngles, pkg.scale);
        pkg.dirty = false;
        return pkg.cached_local;
    }
};