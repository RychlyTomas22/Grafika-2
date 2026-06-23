#pragma once

/*
 * OBJloader.hpp
 * ---------------------------------------------------------------------------
 * Declaration of a simple multiplatform OBJ loader.
 *
 * The loader reads a triangulated OBJ file and fills vectors that can be
 * passed directly to the Mesh constructor:
 *
 * std::vector<Vertex> vertices;
 * std::vector<GLuint> indices;
 * loadOBJ(path, vertices, indices);
 * Mesh mesh(vertices, indices, GL_TRIANGLES);
 *
 * Supported format:
 * - v positions
 * - vt texture coordinates
 * - vn normals
 * - f triangular faces in v/vt/vn format
 *
 * The loader is intentionally simple and is meant for prepared school-project
 * assets, not for arbitrary OBJ files from the internet.
 * ---------------------------------------------------------------------------
 */

#include <vector>
#include <filesystem>

#include <GL/glew.h>

#include "assets.hpp"

/*
 * Loads OBJ model data from file.
 *
 * Parameters:
 * - filename: filesystem path to the OBJ file
 * - vertices: output vector of unique project Vertex objects
 * - indices: output vector of GLuint indices
 *
 * Returns:
 * - true if the file was successfully loaded
 * - false if the file cannot be opened or the parser does not support it
 */
bool loadOBJ(
	const std::filesystem::path& filename,
	std::vector<Vertex>& vertices,
	std::vector<GLuint>& indices
);
