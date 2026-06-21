#version 460 core

out vec4 FragColor;

// Directional light / sun
uniform vec3 sun_direction = normalize(vec3(-0.3, -1.0, -0.2));
uniform vec3 sun_ambient_intensity = vec3(0.03, 0.03, 0.03);
uniform vec3 sun_diffuse_intensity = vec3(0.6, 0.55, 0.45);
uniform vec3 sun_specular_intensity = vec3(0.4, 0.4, 0.35);

// Light intensities for light 0
uniform vec3 ambient_intensity0;
uniform vec3 diffuse_intensity0;
uniform vec3 specular_intensity0;

// Light intensities for light 1
uniform vec3 ambient_intensity1;
uniform vec3 diffuse_intensity1;
uniform vec3 specular_intensity1;

// Light intensities for light 2
uniform vec3 ambient_intensity2;
uniform vec3 diffuse_intensity2;
uniform vec3 specular_intensity2;

// Spotlight attached to camera
uniform vec3 spot_diffuse_intensity = vec3(0.8, 0.8, 0.8);
uniform vec3 spot_specular_intensity = vec3(1.0, 1.0, 1.0);
uniform float spot_cutoff_cos = 0.94;
uniform float spot_exponent = 16.0;

// Material properties
uniform vec3 ambient_material;
uniform vec3 diffuse_material;
uniform vec3 specular_material;
uniform float specular_shinines;

// Texture
uniform sampler2D tex0;

// Input from vertex shader
in VS_OUT {
    vec3 N;
    vec3 L0;
    vec3 L1;
    vec3 L2;
    vec3 V;
    vec2 texCoord;
} fs_in;

vec3 evalPointLight(vec3 N, vec3 Lraw, vec3 V, vec3 tex,
                    vec3 ambient_intensity,
                    vec3 diffuse_intensity,
                    vec3 specular_intensity)
{
    vec3 L = normalize(Lraw);
    vec3 R = reflect(-L, N);

    vec3 ambient = ambient_material * ambient_intensity;
    vec3 diffuse = max(dot(N, L), 0.0) * diffuse_material * diffuse_intensity;
    vec3 specular = pow(max(dot(R, V), 0.0), specular_shinines) * specular_material * specular_intensity;

    return (ambient + diffuse) * tex + specular;
}

void main(void) {
    vec3 N = normalize(fs_in.N);
    vec3 V = normalize(fs_in.V);
    vec3 tex = texture(tex0, fs_in.texCoord).rgb;

    vec3 color = vec3(0.0);

    // Directional light / sun
    vec3 sun_L = normalize(-sun_direction);
    vec3 sun_R = reflect(-sun_L, N);

    float sun_diff = max(dot(N, sun_L), 0.0);

    float sun_spec = 0.0;
    if (sun_diff > 0.0) {
        sun_spec = pow(max(dot(sun_R, V), 0.0), specular_shinines);
    }

    vec3 sun_ambient =
        ambient_material * sun_ambient_intensity;

    vec3 sun_diffuse =
        sun_diff * diffuse_material * sun_diffuse_intensity;

    vec3 sun_specular =
        sun_spec * specular_material * sun_specular_intensity;

    color += (sun_ambient + sun_diffuse) * tex + sun_specular;

    color += evalPointLight(N, fs_in.L0, V, tex,
                            ambient_intensity0, diffuse_intensity0, specular_intensity0);

    color += evalPointLight(N, fs_in.L1, V, tex,
                            ambient_intensity1, diffuse_intensity1, specular_intensity1);

    color += evalPointLight(N, fs_in.L2, V, tex,
                            ambient_intensity2, diffuse_intensity2, specular_intensity2);

    // Spotlight attached to camera/headlight
    vec3 spot_L = V;

    // Camera looks in negative Z direction in view space
    vec3 spot_direction = vec3(0.0, 0.0, -1.0);

    // Direction from camera/light to fragment
    vec3 light_to_fragment = normalize(-spot_L);

    // Check whether fragment is inside spotlight cone
    float theta = dot(spot_direction, light_to_fragment);

    if (theta > spot_cutoff_cos) {
        float spot_factor = pow(theta, spot_exponent);

        vec3 spot_R = reflect(-spot_L, N);

        float spot_diff = max(dot(N, spot_L), 0.0);

        float spot_spec = 0.0;
        if (spot_diff > 0.0) {
            spot_spec = pow(max(dot(spot_R, V), 0.0), specular_shinines);
        }

        vec3 spot_diffuse =
            spot_diff * diffuse_material * spot_diffuse_intensity;

        vec3 spot_specular =
            spot_spec * specular_material * spot_specular_intensity;

        color += spot_factor * (spot_diffuse * tex + spot_specular);
    }

    FragColor = vec4(color, 1.0);
}