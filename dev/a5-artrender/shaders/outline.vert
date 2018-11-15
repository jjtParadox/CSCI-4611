#version 330

// CSci-4611 Assignment 5:  Art Render


uniform mat4 model_view_matrix;
uniform mat4 normal_matrix;
uniform mat4 proj_matrix;
uniform float thickness;

layout(location = 0) in vec3 vertex;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec3 left_normal;
layout(location = 3) in vec3 right_normal;

void main() {
    vec3 v = (model_view_matrix * vec4(vertex, 1)).xyz;
    vec3 e = normalize(-v);
    vec3 l_n = normalize((normal_matrix * vec4(left_normal, 1)).xyz);
    vec3 r_n = normalize((normal_matrix * vec4(right_normal, 1)).xyz);

    float l_dot = dot(e, l_n);
    float r_dot = dot(e, r_n);

    vec3 vert = vertex;
    if ( (l_dot < 0 && r_dot > 0) || (l_dot > 0 && r_dot < 0) ) {
        vert += thickness * normal;
    }
    gl_Position = proj_matrix * model_view_matrix * vec4(vert,1);
}
