/*
 * OBJloader.cpp
 * ---------------------------------------------------------------------------
 * Multiplatform simple OBJ file loader.
 *
 * This version replaces MSVC-specific functions:
 * - fopen_s
 * - fscanf_s
 *
 * with standard C++:
 * - std::ifstream
 * - std::getline
 * - std::istringstream
 *
 * Supported OBJ subset:
 * - vertex positions:        v  x y z
 * - texture coordinates:     vt u v
 * - normals:                 vn x y z
 * - triangular faces only:   f v/t/n v/t/n v/t/n
 *
 * Unsupported / ignored OBJ content:
 * - object names: o
 * - groups: g
 * - smoothing groups: s
 * - material libraries: mtllib
 * - material usage: usemtl
 * - comments: #
 * - quads or polygons with more than 3 vertices
 *
 * The loader converts OBJ data into:
 * - vertices: unique Vertex entries with Position, Normal and TexCoords
 * - indices: index buffer used by Mesh for glDrawElements
 *
 * This parser is intentionally simple and is meant for prepared project assets,
 * not for every possible OBJ file from the internet.
 * ---------------------------------------------------------------------------
 */

#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "OBJloader.hpp"

/*
 * Parses one OBJ face vertex token.
 *
 * Expected token format:
 * vertexIndex/uvIndex/normalIndex
 *
 * Example:
 * 12/4/9
 *
 * OBJ uses 1-based indices, but this function only reads the raw values.
 * Conversion to 0-based vector indexing happens in loadOBJ().
 */
static bool parse_face_token(
    const std::string& token,
    unsigned int& vertex_index,
    unsigned int& uv_index,
    unsigned int& normal_index
)
{
    std::string normalized = token;

    std::replace(normalized.begin(), normalized.end(), '/', ' ');

    std::istringstream token_stream(normalized);

    return static_cast<bool>(token_stream >> vertex_index >> uv_index >> normal_index);
}

/*
 * Loads a simple triangulated OBJ file.
 *
 * Parameters:
 * - filename: path to the .obj file
 * - vertices: output vector of unique vertices
 * - indices: output vector of triangle indices
 *
 * Return value:
 * - true  = model was loaded successfully
 * - false = file could not be opened or the format is unsupported
 *
 * Important:
 * OBJ arrays start at index 1. C++ vectors start at index 0.
 * Therefore every OBJ index is converted using index - 1.
 */
bool loadOBJ(
    const std::filesystem::path& filename,
    std::vector<Vertex>& vertices,
    std::vector<GLuint>& indices
)
{
    std::cout << "Loading model: " << filename.string() << std::endl;

    /*
     * Temporary arrays store raw OBJ data exactly as it is read from the file.
     *
     * Face definitions later reference these arrays by index.
     */
    std::vector<glm::vec3> temp_vertices;
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

    /*
     * Make sure output vectors do not contain data from a previous load.
     */
    vertices.clear();
    indices.clear();

    /*
     * Open OBJ file using standard C++ streams.
     *
     * This is portable across Windows, Linux and macOS.
     */
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Impossible to open the file: " << filename.string() << "\n";
        return false;
    }

    /*
     * Main parsing loop.
     *
     * The parser reads the file line by line. Each line starts with a token:
     * - v  = vertex position
     * - vt = texture coordinate
     * - vn = vertex normal
     * - f  = face
     *
     * Unknown tokens are ignored.
     */
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream line_stream(line);
        std::string line_header;
        line_stream >> line_header;

        if (line_header.empty() || line_header[0] == '#') {
            continue;
        }

        if (line_header == "v") {
            glm::vec3 vertex{};

            if (!(line_stream >> vertex.x >> vertex.y >> vertex.z)) {
                std::cerr << "Invalid vertex position line in OBJ: " << line << "\n";
                return false;
            }

            temp_vertices.push_back(vertex);
        }
        else if (line_header == "vt") {
            glm::vec2 uv{};

            if (!(line_stream >> uv.x >> uv.y)) {
                std::cerr << "Invalid texture coordinate line in OBJ: " << line << "\n";
                return false;
            }

            temp_uvs.push_back(uv);
        }
        else if (line_header == "vn") {
            glm::vec3 normal{};

            if (!(line_stream >> normal.x >> normal.y >> normal.z)) {
                std::cerr << "Invalid normal line in OBJ: " << line << "\n";
                return false;
            }

            temp_normals.push_back(normal);
        }
        else if (line_header == "f") {
            /*
             * Read a triangular face.
             *
             * Expected face format:
             * f vertex/uv/normal vertex/uv/normal vertex/uv/normal
             *
             * Example:
             * f 1/1/1 2/2/1 3/3/1
             */
            std::vector<std::string> face_tokens;
            std::string token;

            while (line_stream >> token) {
                face_tokens.push_back(token);
            }

            if (face_tokens.size() != 3) {
                std::cerr << "Only triangular faces are supported. Invalid face line: "
                          << line << "\n";
                return false;
            }

            unsigned int vertex_index[3]{};
            unsigned int uv_index[3]{};
            unsigned int normal_index[3]{};

            for (int i = 0; i < 3; ++i) {
                if (!parse_face_token(
                        face_tokens[i],
                        vertex_index[i],
                        uv_index[i],
                        normal_index[i]
                    )) {
                    std::cerr << "Unsupported face token format: "
                              << face_tokens[i] << "\n";
                    return false;
                }

                /*
                 * Validate OBJ indices before using them.
                 *
                 * Index 0 is invalid in OBJ, because OBJ starts at 1.
                 */
                if (vertex_index[i] == 0 || vertex_index[i] > temp_vertices.size() ||
                    uv_index[i] == 0 || uv_index[i] > temp_uvs.size() ||
                    normal_index[i] == 0 || normal_index[i] > temp_normals.size()) {
                    std::cerr << "Face index out of range in line: " << line << "\n";
                    return false;
                }
            }

            /*
             * Convert the 3 OBJ face vertices into project Vertex objects.
             *
             * The loader also removes duplicate vertices. If the same
             * Position/Normal/TexCoords combination already exists, it reuses
             * the existing index instead of pushing a duplicate vertex.
             */
            for (int i = 0; i < 3; ++i) {
                GLuint current_index{};
                Vertex current_vertex{};

                current_vertex.Position = temp_vertices[vertex_index[i] - 1]; // OBJ array start from 1
                current_vertex.Normal = temp_normals[normal_index[i] - 1];
                current_vertex.TexCoords = temp_uvs[uv_index[i] - 1];

                // avoid duplicit vertices
                auto t = std::find_if(
                    vertices.begin(),
                    vertices.end(),
                    [&current_vertex](const Vertex& v2) -> bool {
                        return current_vertex == v2;
                    }
                );

                if (t == vertices.end()) {
                    vertices.push_back(current_vertex);
                    current_index = static_cast<GLuint>(vertices.size() - 1);
                }
                else {
                    current_index = static_cast<GLuint>(t - vertices.begin());
                }

                indices.push_back(current_index);
            }
        }
        else {
            // ignore other lines: g, o, s, mtllib, usemtl, #, etc.
        }
    }

    std::cout << "Model loaded: " << filename.string() << std::endl;

    return true;
}
