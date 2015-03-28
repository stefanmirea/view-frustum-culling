/* Stefan-Gabriel Mirea - 331CC */
#version 330

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec3 in_normal;
layout(location = 2) in vec2 in_texcoord;

#define M_PI 3.1415926535897932384626433832795

/* numarul de triunghiuri ale unei "usi" a navei mama (trebuie sa fie par) */
#define MOTHERSHIP_DOOR_TRIANGLES 24

uniform mat4 model_matrix, view_matrix, projection_matrix;
/* valoare intre 0 si pi / 2 care specifica gradul de deschidere al usii navei mama */
uniform float mothership_open;
/* constante din main */
uniform float mothership_diameter;
uniform int mothership_vert_num;
/* valideaza valorile de mai sus (obiectul curent este nava mama si este timpul deschiderii /
 * inchiderii usii) */
uniform int mothership_open_valid;

out vec2 texcoord;
out vec3 world_pos;
out vec3 world_normal;

void main()
{
    texcoord = in_texcoord;
    vec3 model_position = in_position;
    vec3 model_normal = in_normal;
    if (mothership_open_valid == 1)
        if (gl_VertexID % 16 == 15)
        {
            int door_num = (gl_VertexID / 16) / MOTHERSHIP_DOOR_TRIANGLES;
            float ang = 2 * M_PI * MOTHERSHIP_DOOR_TRIANGLES / mothership_vert_num * (0.5 +
                        door_num);
            /* punctul fata de care se face rotirea */
            vec3 rot_center = vec3(0.135 * cos(ang), -0.043, 0.135 * sin(ang)) *
                              mothership_diameter;
            /* argumentul redus al punctului obtinut dupa translatia care aduce rot_center in
             * centrul sistemului */
            float transl_arg = ang + M_PI;
            if (transl_arg > 2 * M_PI)
                transl_arg -= 2 * M_PI;
            /* unghiul cu care se face rotirea pe verticala */
            model_position = mat3(cos(transl_arg), 0.0, sin(transl_arg),
                                  0.0, 1.0, 0.0,
                                  -sin(transl_arg), 0, cos(transl_arg)) *
                             mat3(cos(mothership_open), -sin(mothership_open), 0.0,
                                  sin(mothership_open), cos(mothership_open), 0.0,
                                  0.0, 0.0, 1.0) *
                             vec3(0.135 * mothership_diameter, 0.0, 0.0) + rot_center;
            model_normal = mat3(cos(ang), 0.0, sin(ang), 0.0, 1.0, 0.0, -sin(ang), 0.0, cos(ang)) *
                           vec3(sin(mothership_open), -cos(mothership_open), 0.0);
        }
        else if (gl_VertexID % 16 == 14)
        {
            /* argumentul punctului curent in xOz */
            float ang = (gl_VertexID / 16) * 1.0f / mothership_vert_num * 2 * M_PI;
            model_normal = mat3(cos(ang), 0.0, sin(ang), 0.0, 1.0, 0.0, -sin(ang), 0.0, cos(ang)) *
                           vec3(sin(mothership_open), -cos(mothership_open), 0.0);
        }

    vec4 world_pos_vec4 = model_matrix * vec4(model_position, 1.0);

    gl_Position = projection_matrix * view_matrix * world_pos_vec4;
    world_pos = world_pos_vec4.xyz;
    world_normal = normalize(mat3(model_matrix) * model_normal);
}
