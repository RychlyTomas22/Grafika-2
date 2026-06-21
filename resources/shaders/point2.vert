#version 460 core

// Vertex attributes
in vec4 aPosition;
in vec3 aNormal;
in vec2 aTexCoord;

// Matrices 
uniform mat4 uM_m, uV_m, uP_m;

// 3 point lights
uniform vec3 light_position0;
uniform vec3 light_position1;
uniform vec3 light_position2;

// Outputs to the fragment shader
out VS_OUT {
    vec3 N;
    vec3 L0;
    vec3 L1;
    vec3 L2;
    //vec3 LS;
    vec3 V;
    vec2 texCoord;
} vs_out;

void main(void) {
    // Model-view matrix
    mat4 mv_m = uV_m * uM_m;

    // Vertex position in view space
    vec4 P = mv_m * aPosition;

    // Correct normal transform
    mat3 normal_m = transpose(inverse(mat3(mv_m)));
    vs_out.N = normalize(normal_m * aNormal);

    // Vectors from current point to each light
    vs_out.L0 = light_position0 - P.xyz;
    vs_out.L1 = light_position1 - P.xyz;
    vs_out.L2 = light_position2 - P.xyz;

    // Vector from point to camera
    vs_out.V = -P.xyz;
    //vs_out.LS = -P.xyz;

    // Texture coordinates
    vs_out.texCoord = aTexCoord;

    // Final clip-space position
    gl_Position = uP_m * P;
}