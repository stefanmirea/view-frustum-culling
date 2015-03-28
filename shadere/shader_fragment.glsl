/* Stefan-Gabriel Mirea - 331CC */
#version 330
layout(location = 0) out vec4 out_color;

/* codul de culoare folosit de view2 */
#define NO_CODE      0
#define CODE_VISIBLE 1
#define CODE_HIDDEN  2

#define culoareAmbientalaGlobala 1.0f
#define CUT_OFF      0.25f /* rad */

/* identificatori pentru tipul de lumina */
#define SPOT_LIGHT   0
#define SUN_LIGHT    1

uniform sampler2D textura1;
uniform sampler2D textura2;
uniform int has_alpha;
uniform int enable_fog, code;
uniform float max_dist;
uniform float fade;
uniform int mothership_open_valid;

/* variabile pentru iluminare */
uniform vec3 light_position;
uniform vec3 eye_position;
uniform int material_shininess;
uniform float material_kd;
uniform float material_ks;
uniform int enable_spot_light;
uniform vec3 D; /* pentru lumina de tip spot */

in vec2 texcoord;
in vec3 world_pos;
in vec3 world_normal;

/* calculeaza lumina de tip spot sau solara in punctul curent */
void getLight(in int type, out float light)
{
    vec3 L;
    if (type == SPOT_LIGHT)
        L = normalize(light_position - world_pos);
    else
        L = normalize(vec3(0.2, 1.5, 1.0));
    vec3 V = normalize(eye_position - world_pos);
    vec3 H = normalize(L + V);
    vec3 N = world_normal;

    int PrimesteLumina = dot(L, N) > 0 ? 1 : 0;

    float comp_emis, comp_amb, comp_difuz, comp_spec;
    comp_emis = 0;
    if (type == SPOT_LIGHT)
        comp_amb = 0;
    else
        comp_amb = material_kd * culoareAmbientalaGlobala;
    if (type == SPOT_LIGHT)
    {
        vec3 V_spot = normalize(world_pos - light_position);
        comp_difuz = 100 * max((dot(V_spot, D) - cos(CUT_OFF)) / (1 - cos(CUT_OFF)), 0);
    }
    else
        comp_difuz = material_kd * max(dot(N, L), 0) * 0.2;
    comp_spec = material_ks * PrimesteLumina * pow(max(dot(N, H), 0), material_shininess);
    /* atenuarea intensitatii luminii */
    if (type == SPOT_LIGHT)
    {
        float dist = distance(world_pos, light_position);
        float factor = 1 / (dist * dist);
        comp_difuz *= factor;
        comp_spec  *= factor;
    }
    light = comp_emis + comp_amb + comp_difuz + comp_spec;
}

/* combina lumina cu textura */
void applyLight(in vec3 light, inout vec3 color)
{
    int i;
    for (i = 0; i < 3; ++i)
        if (light[i] < 0.5)
            color[i] = 2 * light[i] * color[i];
        else
            color[i] = color[i] + 2 * (light[i] - 0.5) * (1 - color[i]);
}

void main()
{
    switch (code)
    {
        case NO_CODE:
        {
            /* nu se afiseaza triunghiurile care se vad din spate, decat in filmul de la inceput, in
             * timpul deschiderii usilor navei mama */
            if (mothership_open_valid == 0 && dot(eye_position - world_pos, world_normal) < 0)
                discard;
            vec3 tex1 = texture(textura1, texcoord).xyz;
            vec3 tex2 = texture(textura2, texcoord).xyz;
            if (has_alpha == 1 && tex2.r < 0.5)
                discard;

            float spot_light, sun_light;
            if (enable_spot_light == 1)
                getLight(SPOT_LIGHT, spot_light);
            else
                spot_light = 0;
            getLight( SUN_LIGHT,  sun_light);
            vec3 light_vec;
            light_vec = (sun_light * vec3(1.0) + spot_light * vec3(0.67, 1.0, 0.0)) / 2;
            applyLight(light_vec, tex1);
            out_color = vec4(tex1, 1.0);
            if (enable_fog == 1)
            {
                float dist = distance(world_pos, eye_position);
                /* incrementez r, g, b cu aceeasi valoare, incat culoarea sa ramana la fel daca
                 * dist = 0 si sa devina alb daca dist = max_dist */
                if (dist >= max_dist)
                    out_color = vec4(1.0);
                else
                {
                    float min = out_color[0];
                    if (out_color[1] < min)
                        min = out_color[1];
                    if (out_color[2] < min)
                        min = out_color[2];
                    float incr = (1 - min) * dist / max_dist;
                    out_color[0] += incr; if (out_color[0] > 1) out_color[0] = 1;
                    out_color[1] += incr; if (out_color[1] > 1) out_color[1] = 1;
                    out_color[2] += incr; if (out_color[2] > 1) out_color[2] = 1;
                }
            }
            out_color = vec4((1 - fade) * vec3(out_color) + fade * vec3(1.0), out_color[3]);
            break;
        }
        case CODE_VISIBLE:
            if (world_pos[1] > 0.1)
                out_color = vec4(0.0, 1.0, 0.0, 1.0);
            else
                out_color = vec4(0.0, 0.25, 0.0, 1.0);
            break;
        case CODE_HIDDEN:
            if (world_pos[1] > 0.1)
                out_color = vec4(1.0, 0.0, 0.0, 1.0);
            else
                out_color = vec4(0.25, 0.0, 0.0, 1.0);
            break;
        default:
            out_color = vec4(1.0);
    }
}
