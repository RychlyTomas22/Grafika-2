#pragma once

/*
 * Model.hpp
 * ---------------------------------------------------------------------------
 * Higher-level model container.
 *
 * Mesh represents only one drawable geometry buffer.
 * Model represents a whole object that may be composed from multiple meshes.
 *
 * Each mesh part can have:
 * - its own mesh
 * - its own shader
 * - local transform relative to the whole model
 *
 * The Model itself also has a global transform:
 * - pivot_position
 * - eulerAngles
 * - scale
 *
 * This file is useful when a complex object is made from multiple parts.
 * The current project can also draw simple SceneObject objects directly, but
 * this class is still a reusable abstraction for grouped models.
 * ---------------------------------------------------------------------------
 */

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
    /*
     * Global transform of the whole model.
     *
     * pivot_position is the world-space origin of the model.
     * eulerAngles stores pitch/yaw/roll in radians.
     * scale scales the whole model.
     */
    // origin point of whole model
    glm::vec3 pivot_position{}; // [0,0,0] of the object
    glm::vec3 eulerAngles{};    // pitch, yaw, roll (radians)
    glm::vec3 scale{1.0f};

    /*
     * One mesh part of a larger model.
     *
     * A complex model can be made from multiple mesh_package entries.
     * Each part has its own local transform relative to the model origin.
     */
    struct mesh_package {
        std::shared_ptr<Mesh> mesh;                 // geometry & topology, vertex attributes
        std::shared_ptr<ShaderProgram> shader;      // which shader to use to draw this part of the model

        glm::vec3 origin{0.0f};                     // mesh origin relative to origin of the whole model
        glm::vec3 eulerAngles{0.0f};                // mesh rotation relative to orientation of the whole model
        glm::vec3 scale{1.0f};                      // mesh scale relative to scale of the whole model

        // cache
        /*
         * Local matrix cache.
         *
         * The local transform matrix is recomputed only when dirty is true.
         * This avoids recalculating the same matrix every draw call when the
         * transform has not changed.
         */
        mutable bool dirty{true};
        mutable glm::mat4 cached_local{1.0f};

        /*
         * Marks this mesh part transform as changed.
         */
        void mark_dirty() const { dirty = true; }
    };

    /*
     * All mesh parts that belong to this model.
     */
    std::vector<mesh_package> meshes;

    Model() = default;

    /*
     * Marks the whole model transform as changed.
     */
    void mark_dirty() const { model_dirty_ = true; }

    /*
     * Adds one mesh part to the model.
     *
     * Parameters:
     * - mesh: geometry to draw
     * - shader: shader used for this mesh part
     * - origin: local position relative to the model origin
     * - eulerAngles: local rotation in radians
     * - scale: local scale
     */
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

    /*
     * Updates the model.
     *
     * Currently empty, but prepared for future animation. If this function
     * changes pivot_position, eulerAngles, scale or any mesh_package transform,
     * it should call mark_dirty() or pkg.mark_dirty().
     */
    // update based on running time
    void update(const float /*delta_t*/) {
        // future: animate transforms then call mark_dirty()/pkg.mark_dirty()
    }

    /*
     * Draws all mesh parts of the model.
     *
     * For every mesh part:
     * 1. compute final model matrix
     * 2. activate the part shader
     * 3. send uM_m uniform
     * 4. draw the mesh
     */
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
    /*
     * Cached global model transform.
     *
     * Mutable is used because cache can be updated inside const draw().
     */
    mutable bool model_dirty_{true};
    mutable glm::mat4 cached_model_{1.0f};

    /*
     * Builds a transform matrix from translation, Euler rotation and scale.
     *
     * Matrix order:
     * T * R * S
     *
     * This means scale is applied first, then rotation, then translation.
     */
    static glm::mat4 compose_trs_(const glm::vec3& t,
                                 const glm::vec3& euler_radians,
                                 const glm::vec3& s)
    {
        const glm::mat4 T = glm::translate(glm::mat4(1.0f), t);
        const glm::mat4 R = glm::yawPitchRoll(euler_radians.y, euler_radians.x, euler_radians.z);
        const glm::mat4 S = glm::scale(glm::mat4(1.0f), s);
        return T * R * S;
    }

    /*
     * Returns the global model matrix.
     *
     * Recomputed only when model_dirty_ is true.
     */
    glm::mat4 get_model_matrix_() const {
        if (!model_dirty_) return cached_model_;
        cached_model_ = compose_trs_(pivot_position, eulerAngles, scale);
        model_dirty_ = false;
        return cached_model_;
    }

    /*
     * Returns the local matrix of one mesh part.
     *
     * Recomputed only when pkg.dirty is true.
     */
    glm::mat4 get_local_matrix_(mesh_package const& pkg) const {
        if (!pkg.dirty) return pkg.cached_local;
        pkg.cached_local = compose_trs_(pkg.origin, pkg.eulerAngles, pkg.scale);
        pkg.dirty = false;
        return pkg.cached_local;
    }
};
