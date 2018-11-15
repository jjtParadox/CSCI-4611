#version 330

in vec3 position_in_eye_space;
in vec3 normal_in_eye_space;

out vec4 color;

uniform vec3 light_in_eye_space;
uniform vec4 Ia, Id, Is;

uniform vec4 ka, kd, ks;
uniform float s;


void main() {
    // normalized normal
    vec3 n = normalize(normal_in_eye_space);

    // unit vector from the vertex to the light
    vec3 l = normalize(light_in_eye_space - position_in_eye_space);

    // unit vector from the vertex to the eye point, which is at 0,0,0 in "eye space"
    vec3 e = normalize(-position_in_eye_space);

    // halfway vector
    vec3 h = normalize(l + e);

    // calculate color using the light intensity equation
    color = ka * Ia + kd * Id * max(0,dot(n, l)) + ks * Is * pow(dot(h, n), s);
}
