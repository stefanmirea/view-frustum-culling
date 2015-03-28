//--------------------------------------------------------------------------------------------------
// Descriere: fisier main - include definita listener-ului Tema, precum si a anumitor clase interne
// ajutatoare (MeshType, InfSurface, Mesh, Character). Nu am putut folosi surse multiple, deoarece
// multe dintre aceste clase folosesc functii / clase din scheletul de laborator, iar in schelet
// acestea sunt definite in headere (ar fi fost nevoie sa modific scheletul separand declaratiile
// de implementari, altfel fisierele obiect ar fi definit simboluri comune).
//
// Autor: Stefan-Gabriel Mirea, 331CC
// Data: 4 ian 2015
//--------------------------------------------------------------------------------------------------

/* incarcator de meshe */
#include "lab_mesh_loader.hpp"

/* geometrie: drawSolidCube, drawWireTeapot... */
#include "lab_geometry.hpp"

/* incarcator de shadere */
#include "lab_shader_loader.hpp"

/* interfata cu glut, ne ofera fereastra, input, context opengl */
#include "lab_glut.hpp"

/* camera */
#include "lab_camera.hpp"

/* texturi */
#include "lab_texture_loader.hpp"

/* headere utile */
#include <ctime>
#include <bitset>
#include <cstdlib>
#include <algorithm>

/* numarul implicit de case de-a lungul unei laturi a patratului: trebuie sa fie un numar
 * par >= 6 */
#define DEFAULT_ARRAY_DIM          16

/* dimensiunea unei celule (ce contine o casa sau o portiune din strada) (m) */
#define CELL_SIZE                  17.5f

/* distanta pe Oz intre doua case consecutive care nu sunt despartite de o strada (m) (in functie de
 * ea se va calcula automat si distanta pe Ox, incat harta sa fie patratica) */
#define GAP_SIZE                   7.0f

/* vitezele de variatie ale parametrilor personajului */
#define DELTA_ANG                  70.0f    /* grade/s */
#define DELTA_TILT                 111.111f /* grade/s */
#define DELTA_DIST                 30.0f    /* m/s     */

/* codul de culoare folosit de view2 */
#define NO_CODE                    0 /* view1 e activa => se respecta texturile */
#define CODE_VISIBLE               1 /* desenare cu verde */
#define CODE_HIDDEN                2 /* desenare cu rosu */
#define CODE_UFO                   3 /* pentru desenarea navelor cu o alta culoare */

/* distanta maxima pana la care se poate vedea prin ceata (ea va fi si distanta cu care se va
 * incrementa suprafata pamantului pe ambele directii pentru a simula un teren infinit) (m) */
#define MAX_DIST                   350.0f

/* diametrele si numerele de varfuri ale poligoanelor regulate care aproximeaza cercurile de-a
 * lungul carora se face extrude pentru crearea navelor */
#define MOTHERSHIP_DIAMETER        43.59f /* m */
#define MOTHERSHIP_VERT_NUM        240    /* trebuie sa fie multiplu al lui
                                           * MOTHERSHIP_DOOR_TRIANGLES din shader_vertex.glsl */
#define UFO_DIAMETER               6.275f /* m */
#define UFO_VERT_NUM               24     /* trebuie sa fie par */

/* specifica daca OZN-ul se va inclina in timpul deplasarilor fata / spate sau stanga / dreapta */
#define ENABLE_TILT                true

/* unghiul maxim cu care se poate inclina personajul in timpul deplasarii (grade) */
#define MAX_TILT                   22.0f

/* perioada de clipire a personajului cand camera view2 e activa (s) */
#define BLINK_PERIOD               0.7f

/* identificatori pentru axe */
#define AXIS_X                     0
#define AXIS_Z                     1

/* numarul tipurilor de obiecte */
#define NUM_OBJ_TYPES              18

/* tipurile de obiecte */
#define OBJ_MOTHERSHIP             0  /* OZN-ul de pe rectorat */
#define OBJ_ONE_STORY_HOUSE        1  /* casa fara etaje */
#define OBJ_TWO_STORY_HOUSE        2  /* casa cu un etaj */
#define OBJ_UFO                    3  /* personajul principal */
#define OBJ_STREET_END             4  /* sfarsitul unei strazi, implicit conectat in ambele parti la
                                       * case */
#define OBJ_STREET_FULL_CONNECTED  5  /* strada conectata in ambele parti la case */
#define OBJ_STREET_SEMI_CONNECTED  6  /* strada conectata la o singura casa */
#define OBJ_STREET_THREE_WAY       7  /* strada in forma de T */
#define OBJ_STREET_FOUR_WAY        8  /* strada in forma de + */
#define OBJ_STREET_STRAIGHT_CSxCS  9  /* strada continua, neconectata la case, dimensiuni
                                       * CELL_SIZE x CELL_SIZE */
#define OBJ_STREET_STRAIGHT_XGxCS  10 /* idem, x_gap x CELL_SIZE */
#define OBJ_STREET_STRAIGHT_GSxCS  11 /* idem, GAP_SIZE x CELL_SIZE */
#define OBJ_GREENSPACE_CSxCS       12 /* spatiu verde CELL_SIZE x CELL_SIZE */
#define OBJ_GREENSPACE_CSxGS       13 /* spatiu verde CELL_SIZE x GAP_SIZE */
#define OBJ_GREENSPACE_XGxGS       14 /* spatiu verde x_gap x GAP_SIZE */
#define OBJ_GREENSPACE_XGxCS       15 /* spatiu verde x_gap x CELL_SIZE */
#define OBJ_MARGIN                 16 /* celulele ce bordeaza matricea de elemente */
#define OBJ_CORNER                 17 /* celulele ce reprezinta colturile matricei de elemente */

/* identificatori pentru scenele filmului de la inceput, modul de joc si pentru scena de tranzitie
 * intre joc si eventuala reluare a filmului */
#define SCENE_INTRODUCTION         0
#define SCENE_MOTHERSHIP_COMES_OUT 1
#define SCENE_MOTHERSHIP_STOPS     2
#define SCENE_SPAWN                3
#define GAME_STARTED               4
#define SCENE_FADE_OUT             5

/* tastele sageti */
#define KEY_LEFT                   100
#define KEY_UP                     101
#define KEY_RIGHT                  102
#define KEY_DOWN                   103

using namespace std;

class Tema : public lab::glut::WindowListener {

/* clase interne */
private:
    /* retine geometria pentru un tip de obiect */
    class MeshType {
    public:
        unsigned int vbo, ibo, vao, num_indices;
        vector<lab::VertexFormat> vertices;
        vector<unsigned int> indices;
        unsigned int texture;

        MeshType(const vector<lab::VertexFormat> &vertices, const vector<unsigned int> &indices,
            const unsigned int texture)
            : vertices(vertices),
              indices(indices),
              texture(texture)
        {
            buildGeom();
        }
        MeshType(vector<lab::VertexFormat> &vertices, const vector<unsigned int> &indices,
                 const unsigned int texture, const float scale)
            : indices(indices),
              texture(texture)
        {
            for (unsigned int i = 0; i < vertices.size(); ++i)
            {
                vertices[i].position_x *= scale;
                vertices[i].position_y *= scale;
                vertices[i].position_z *= scale;
            }
            this->vertices = vertices;
            buildGeom();
        }
        /* creeaza vao, vbo, ibo si num_indices pe baza vectorilor de vertecsi si indecsi */
        void buildGeom()
        {
            /* vertex array object -> un obiect ce reprezinta un container pentru starea de
             * desenare */
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            /* vertex buffer object -> un obiect in care tinem vertecsii */
            glGenBuffers(1, &vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(lab::VertexFormat), &vertices[0],
                         GL_STATIC_DRAW);

            /* index buffer object -> un obiect in care tinem indecsii */
            glGenBuffers(1, &ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                         &indices[0], GL_STATIC_DRAW);

            /* legatura intre atributele vertecsilor si pipeline, datele noastre sunt
             * INTERLEAVED. */
            glEnableVertexAttribArray(0);
            /* trimite pozitii pe pipe 0 */
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(lab::VertexFormat), (void*)0);
            glEnableVertexAttribArray(1);
            /* trimite normale pe pipe 1 */
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(lab::VertexFormat),
                                  (void*)(sizeof(float) * 3));
            /* trimite texcoords pe pipe 2 */
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(lab::VertexFormat),
                                  (void*)(2 * sizeof(float)*3));

            num_indices = indices.size();
        }

        void destroyGeom()
        {
            glDeleteBuffers(1, &vbo);
            glDeleteBuffers(1, &ibo);
            glDeleteVertexArrays(1, &vao);
        }
    };

    /* o suprafata care se redimensioneaza incat sa para infinita */
    class InfSurface : public MeshType {
    public:
        /* la cel nivel a fost extinsa suprafata, ca numar de MAX_DIST (pot fi unul sau doua grade
         * de libertate) */
        int ext_level_x, ext_level_z;

        vector<glm::vec3> world_vertices;
        InfSurface(const vector<lab::VertexFormat> &vertices, const vector<unsigned int> &indices,
                   const unsigned int texture)
            : MeshType(vertices, indices, texture) {}
        glm::vec3 getPos(const lab::VertexFormat &v) const
        {
            return glm::vec3(v.position_x, v.position_y, v.position_z);
        }
        /* creeaza vectorul world_vertices, compatibil cu metoda isVisible() a frustumului */
        void computeWorldVertices()
        {
            world_vertices = vector<glm::vec3>{
                getPos(vertices[0]),
                getPos(vertices[1]),
                getPos(vertices[2]),
                getPos(vertices[3])
            };
        }
        void updateGeom()
        {
            destroyGeom();
            buildGeom();
            computeWorldVertices();
        }
    };

    /* clasa pentru obiectele statice din scena si a navei mama */
    class Mesh {
    private:
        vector<glm::vec3> world_vertices;
    public:
        int mesh_type;
        glm::mat4 model_matrix;
        /* creeaza lista coordonatelor varfurilor in spatiul utilizator */
        void computeWorldVertices(vector<MeshType> &mesh_types)
        {
            vector<lab::VertexFormat> &vertices = mesh_types[mesh_type].vertices;
            world_vertices.resize(vertices.size());
            for (unsigned int i = 0; i < vertices.size(); ++i)
                world_vertices[i] = glm::vec3(model_matrix * glm::vec4(vertices[i].position_x,
                                                                       vertices[i].position_y,
                                                                       vertices[i].position_z,
                                                                       1.0));
        }
        vector<glm::vec3> &getWorldVertices()
        {
            return world_vertices;
        }
    };

    /* clasa pentru personaj, care spre deosebire de alte obiecte retine anumite rotatii separat de
     * matricea de modelare */
    class Character : public Mesh {
    private:
        float tilt_x, tilt_z;
        bool tilted_x, tilted_z;
    public:
        Character() : tilt_x(0), tilt_z(0), tilted_x(false), tilted_z(false) {}
        float sinDeg(const float degrees)
        {
            return sin(degrees * glm::pi<float>() / 180);
        }
        void tiltFront(const float frame_delta_tilt)
        {
            if (tilt_z - frame_delta_tilt < -MAX_TILT)
                tilt_z = -MAX_TILT;
            else
                tilt_z -= frame_delta_tilt;
            if (!tilted_z)
                tilted_z = true;
        }
        void tiltLeft(const float frame_delta_tilt)
        {
            if (tilt_x + frame_delta_tilt > MAX_TILT)
                tilt_x = MAX_TILT;
            else
                tilt_x += frame_delta_tilt;
            if (!tilted_x)
                tilted_x = true;
        }
        void tiltBack(const float frame_delta_tilt)
        {
            if (tilt_z + frame_delta_tilt > MAX_TILT)
                tilt_z = MAX_TILT;
            else
                tilt_z += frame_delta_tilt;
            if (!tilted_z)
                tilted_z = true;
        }
        void tiltRight(const float frame_delta_tilt)
        {
            if (tilt_x - frame_delta_tilt < -MAX_TILT)
                tilt_x = -MAX_TILT;
            else
                tilt_x -= frame_delta_tilt;
            if (!tilted_x)
                tilted_x = true;
        }
        void translateUpward(const float frame_delta_dist)
        {
            model_matrix = glm::translate(model_matrix, glm::vec3(0.0, frame_delta_dist, 0.0));
        }
        void translateOnwardWithoutTilt(const float frame_delta_dist)
        {
            model_matrix = glm::translate(model_matrix, glm::vec3(0.0, 0.0, -frame_delta_dist));
        }
        void translateSidewaysWithoutTilt(const float frame_delta_dist)
        {
            model_matrix = glm::translate(model_matrix, glm::vec3(frame_delta_dist, 0.0, 0.0));
        }
        void translateOnward(const float frame_delta_dist)
        {
            if (tilted_z)
                model_matrix = glm::translate(model_matrix,
                    glm::vec3(0.0, 0.0, frame_delta_dist * sinDeg(tilt_z) / sinDeg(MAX_TILT)));
        }
        void translateSideways(const float frame_delta_dist)
        {
            if (tilted_x)
                model_matrix = glm::translate(model_matrix,
                    glm::vec3(-frame_delta_dist * sinDeg(tilt_x) / sinDeg(MAX_TILT), 0.0, 0.0));
        }
        void rotateOx(const float frame_delta_ang)
        {
            model_matrix = glm::rotate(model_matrix, frame_delta_ang, glm::vec3(1.0, 0.0, 0.0));
        }
        void rotateOy(const float frame_delta_ang)
        {
            /* rotirile pe Oy se fac in spatiul utilizator (prin inmultirea matricei de modelare la
             * stanga), pentru a nu afecta rotirea camerei pe Oz */
            glm::vec3 character_pos(model_matrix * glm::vec4(0.0, 0.0, 0.0, 1.0));
            glm::mat4 rotation_mat = glm::translate(glm::mat4(), character_pos);
            rotation_mat = glm::rotate(rotation_mat, frame_delta_ang, glm::vec3(0.0, 1.0, 0.0));
            rotation_mat = glm::translate(rotation_mat, -character_pos);
            model_matrix = rotation_mat * model_matrix;
        }
        void cancelTilt(const int axis, const float frame_delta_tilt)
        {
            bool *tilted = axis == AXIS_X ? &tilted_x : &tilted_z;
            float *tilt  = axis == AXIS_X ? &tilt_x   : &tilt_z;
            if (tilted)
            {
                float new_tilt = *tilt + (*tilt < 0 ? 1 : -1) * frame_delta_tilt;
                if ((*tilt < 0) ^ (new_tilt < 0))
                {
                    *tilt = 0;
                    *tilted = false;
                }
                else
                    *tilt = new_tilt;
            }
        }
        /* returneaza matricea de modelare (mentinuta separat pentru a o folosi doar pe ea la
         * orientarea camerei), impreuna cu inclinarile */
        glm::mat4 getCombinedMatrix() const
        {
            return glm::rotate(glm::rotate(model_matrix, tilt_x, glm::vec3(0.0, 0.0, 1.0)), tilt_z,
                               glm::vec3(1.0, 0.0, 0.0));
        }
        glm::vec3 getLightPosition() const
        {
            return glm::vec3(getCombinedMatrix() * glm::vec4(0.0, 0.0, -UFO_DIAMETER / 2, 1.0));
        }
        glm::vec3 getSpotDirection() const
        {
            return glm::vec3(getCombinedMatrix() * glm::vec4(0.0, 0.0, -1.0, 0.0));
        }
        void collideWithGround(const vector<lab::VertexFormat> &vertices)
        {
            glm::mat4 combined_matrix = getCombinedMatrix();
            float y_min = 1;
            for (unsigned int i = 0; i < vertices.size(); ++i)
            {
                glm::vec3 world_pos(combined_matrix * glm::vec4(vertices[i].position_x,
                                                                vertices[i].position_y,
                                                                vertices[i].position_z,
                                                                1.0));
                if (world_pos[1] < y_min)
                    y_min = world_pos[1];
            }
            if (y_min < 0)
                model_matrix = glm::translate(glm::mat4(), glm::vec3(0.0, -y_min, 0.0)) *
                               model_matrix;
        }
    };

/* variabile */
private:
    /* matrice 4x4 pentru proiectie */
    glm::mat4 projection_matrix;
    /* camere */
    lab::Camera view1, view2, *active_cam;
    /* id-ul de opengl al obiectului de tip program shader */
    unsigned int gl_program_shader;
    unsigned int screen_width, screen_height;

    /* variabile pentru evidenta timpului */
    float last_time, start_time;
    LARGE_INTEGER perf_freq;

    /* numarul de case de-a lungul unei laturi a patratului */
    int array_dim;
    /* distanta pe Ox intre doua case consecutive nedespartite de o strada */
    float x_gap;
    /* lungimea laturii grilei de obiecte */
    float grid_size;
    /* grosimea marginilor care bordeaza matricea de obiecte */
    float margin_size;

    /* tastele apasate la un momentdat */
    bitset<256> pressed_keys;
    /* pentru tastele apasate, spune daca sunt sau nu speciale */
    bitset<256> is_special;

    /* matricea continand casele si strazile dintre ele */
    vector<vector<Mesh>> world;
    /* meshuri marginale */
    vector<Mesh> margins, corners;
    /* nava mama */
    Mesh mothership;

    /* locul unde se va desfasura actiunea filmului introductiv */
    float center_x, center_z;

    /* texturi */
    unsigned int mothership_texture_color, mothership_texture_alpha, street_texture,
        house_texture_color, house_texture_alpha, ufo_texture, grass_texture, margin_texture,
        corner_texture, forever_texture;

    vector<MeshType> mesh_types;
    vector<InfSurface> infinite_surfaces;
    Character character;

    /* aspectul ecranului */
    float aspect;

    /* scena din filmul initial */
    int current_scene = SCENE_INTRODUCTION;
    /* retine daca s-a pus pauza la filmul initial */
    bool paused = false;

/* metode */
public:
    /* constructor - e apelat cand e instantiata clasa */
    Tema(int array_dim) : array_dim(array_dim)
    {
        /* setari pentru desenare, clear color seteaza culoarea de clear pentru ecran
         * (format R, G, B, A) */
        glClearColor(1, 1, 1, 1);
        /* clear depth si depth test (nu le studiem momentan, dar avem nevoie de ele!) */
        glClearDepth(1);
        /* sunt folosite pentru a determina obiectele cele mai apropiate de camera (la curs:
         * algoritmul pictorului, algoritmul zbuffer) */
        glEnable(GL_DEPTH_TEST);

        /* incarca un shader din fisiere si gaseste locatiile matricelor relativ la programul
         * creat */
        gl_program_shader = lab::loadShader("shadere\\shader_vertex.glsl",
                                            "shadere\\shader_fragment.glsl");

        /* incarca texturile */
        mothership_texture_color = lab::loadTextureBMP("resurse\\mothership.bmp");
        mothership_texture_alpha = lab::loadTextureBMP("resurse\\mothershipalpha.bmp");
        street_texture           = lab::loadTextureBMP("resurse\\streets.bmp");
        house_texture_color      = lab::loadTextureBMP("resurse\\house.bmp");
        house_texture_alpha      = lab::loadTextureBMP("resurse\\housealpha.bmp");
        ufo_texture              = lab::loadTextureBMP("resurse\\ufo.bmp");
        grass_texture            = lab::loadTextureBMP("resurse\\grass.bmp");
        margin_texture           = lab::loadTextureBMP("resurse\\margin.bmp");
        corner_texture           = lab::loadTextureBMP("resurse\\corner.bmp");
        forever_texture          = lab::loadTextureBMP("resurse\\forever.bmp");

        createWorld();
        mothership.mesh_type = OBJ_MOTHERSHIP;
        character.mesh_type  = OBJ_UFO;

        active_cam = &view1;

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        glPointSize(10);

        QueryPerformanceFrequency(&perf_freq);
    }

    /* destructor - e apelat cand e distrusa clasa */
    ~Tema()
    {
        /* distruge shader */
        glDeleteProgram(gl_program_shader);

        /* distruge mesh-urile */
        for (int i = 0; i < NUM_OBJ_TYPES; ++i)
            mesh_types[i].destroyGeom();
        for (unsigned int i = 0; i < infinite_surfaces.size(); ++i)
            infinite_surfaces[i].destroyGeom();

        /* distruge texturile */
        glDeleteTextures(1, &mothership_texture_color);
        glDeleteTextures(1, &mothership_texture_alpha);
        glDeleteTextures(1, &street_texture);
        glDeleteTextures(1, &house_texture_color);
        glDeleteTextures(1, &house_texture_alpha);
        glDeleteTextures(1, &ufo_texture);
        glDeleteTextures(1, &grass_texture);
        glDeleteTextures(1, &margin_texture);
        glDeleteTextures(1, &corner_texture);
        glDeleteTextures(1, &forever_texture);
    }

    //----------------------------------------------------------------------------------------------
    //metode de constructie geometrie --------------------------------------------------------------

    /* creeaza geometria tuturor tipurilor de obiecte */
    void createGeometry()
    {
        for (int i = 0; i < NUM_OBJ_TYPES; ++i)
        {
            if (i == OBJ_MOTHERSHIP)
            {
                vector<lab::VertexFormat> vertices;
                vector<unsigned int> indices;
                float degree = 0, delta_degree = 2 * glm::pi<float>() / MOTHERSHIP_VERT_NUM;

                for (int j = 0; j <= MOTHERSHIP_VERT_NUM; ++j)
                {
                    float tx = 0.0309f + 0.9382f * (MOTHERSHIP_VERT_NUM - j) / MOTHERSHIP_VERT_NUM;
                    vertices.insert(vertices.end(), {
                        lab::VertexFormat(lab::PolarCoord(  0.0f, degree),  0.184f, lab::PolarCoord(  0.0f, degree),    1.0f, tx, 0.7835f),
                        lab::VertexFormat(lab::PolarCoord(0.069f, degree),  0.174f, lab::PolarCoord(0.138f, degree),   0.99f, tx, 0.7361f),
                        lab::VertexFormat(lab::PolarCoord(0.069f, degree),  0.174f, lab::PolarCoord(0.497f, degree),  0.868f, tx, 0.7361f),
                        lab::VertexFormat(lab::PolarCoord(0.118f, degree),  0.146f, lab::PolarCoord(0.497f, degree),  0.868f, tx,  0.698f),
                        lab::VertexFormat(lab::PolarCoord(0.118f, degree),  0.146f, lab::PolarCoord(  1.0f, degree),    0.0f, tx,  0.698f),
                        lab::VertexFormat(lab::PolarCoord(0.118f, degree),  0.126f, lab::PolarCoord(  1.0f, degree),    0.0f, tx, 0.6839f),
                        lab::VertexFormat(lab::PolarCoord(0.118f, degree),  0.126f, lab::PolarCoord(0.224f, degree),  0.975f, tx, 0.6839f),
                        lab::VertexFormat(lab::PolarCoord(0.254f, degree),  0.095f, lab::PolarCoord( 0.28f, degree),   0.96f, tx, 0.5893f),
                        lab::VertexFormat(lab::PolarCoord( 0.41f, degree),  0.041f, lab::PolarCoord(0.359f, degree),  0.933f, tx, 0.4779f),
                        lab::VertexFormat(lab::PolarCoord(  0.5f, degree),    0.0f, lab::PolarCoord(0.414f, degree),   0.91f, tx, 0.4104f),
                        lab::VertexFormat(lab::PolarCoord(  0.5f, degree),    0.0f, lab::PolarCoord(0.225f, degree), -0.974f, tx, 0.4104f),
                        lab::VertexFormat(lab::PolarCoord(0.386f, degree), -0.026f, lab::PolarCoord(0.166f, degree), -0.986f, tx, 0.3315f),
                        lab::VertexFormat(lab::PolarCoord(0.269f, degree), -0.039f, lab::PolarCoord(0.066f, degree), -0.998f, tx, 0.2518f),
                        lab::VertexFormat(lab::PolarCoord(0.135f, degree), -0.043f, lab::PolarCoord(0.031f, degree), -0.999f, tx, 0.1608f),
                        lab::VertexFormat(lab::PolarCoord(0.135f, degree), -0.043f, lab::PolarCoord(  0.0f, degree),   -1.0f, tx, 0.1608f),
                        lab::VertexFormat(lab::PolarCoord(  0.0f, degree), -0.043f, lab::PolarCoord(  0.0f, degree),   -1.0f, tx, 0.0699f),
                    });
                    if (j < MOTHERSHIP_VERT_NUM)
                    {
                        unsigned int crt_offset = j * 16, next_offset = (j + 1) * 16;
                        indices.insert(indices.end(), {
                            crt_offset  +  0, crt_offset +  1, next_offset +  1,
                            next_offset +  2, crt_offset +  2, next_offset +  3, crt_offset +  2, next_offset +  3, crt_offset +  3,
                            next_offset +  4, crt_offset +  4, next_offset +  5, crt_offset +  4, next_offset +  5, crt_offset +  5,
                            next_offset +  6, crt_offset +  6, next_offset +  7, crt_offset +  6, next_offset +  7, crt_offset +  7,
                            next_offset +  7, crt_offset +  7, next_offset +  8, crt_offset +  7, next_offset +  8, crt_offset +  8,
                            next_offset +  8, crt_offset +  8, next_offset +  9, crt_offset +  8, next_offset +  9, crt_offset +  9,
                            next_offset + 10, crt_offset + 10, next_offset + 11, crt_offset + 10, next_offset + 11, crt_offset + 11,
                            next_offset + 11, crt_offset + 11, next_offset + 12, crt_offset + 11, next_offset + 12, crt_offset + 12,
                            next_offset + 12, crt_offset + 12, next_offset + 13, crt_offset + 12, next_offset + 13, crt_offset + 13,
                            next_offset + 14, crt_offset + 14, crt_offset  + 15,
                        });
                    }
                    degree += delta_degree;
                }
                /* steag */
                vertices.insert(vertices.end(), {
                    lab::VertexFormat(  0.0f, 0.283f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0309f, 0.9301f),
                    lab::VertexFormat(0.051f, 0.283f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0468f, 0.9301f),
                    lab::VertexFormat(  0.0f, 0.174f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0309f, 0.8535f),
                    lab::VertexFormat(0.051f, 0.174f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0468f, 0.8535f),
                });
                unsigned int offset = 16 * (MOTHERSHIP_VERT_NUM + 1);
                indices.insert(indices.end(), {
                    offset + 0, offset + 1, offset + 2,
                    offset + 1, offset + 2, offset + 3,
                });
                mesh_types.push_back(MeshType(vertices, indices, mothership_texture_color,
                                              MOTHERSHIP_DIAMETER));
            }
            else if (i == OBJ_ONE_STORY_HOUSE || i == OBJ_TWO_STORY_HOUSE)
            {
                vector<lab::VertexFormat> vertices;
                vector<unsigned int> indices;
                if (i == OBJ_ONE_STORY_HOUSE)
                    _loadObjFile("resurse\\one_story_house.obj", vertices, indices);
                else
                    _loadObjFile("resurse\\two_story_house.obj", vertices, indices);
                mesh_types.push_back(MeshType(vertices, indices, house_texture_color,
                                              CELL_SIZE / 10));
            }
            else if (i == OBJ_UFO)
            {
                /* coordonatele centrelor bazelor in cadrul texturii */
                float up_cen_x = 0.5f, up_cen_y = 0.8567f, down_cen_x = 0.5f, down_cen_y = 0.1596f;
                /* razele bazelor in coordonatele de textura, unde ambele baze sunt elipse
                 * necirculare */
                float up_rad_x = 0.2724f, up_rad_y = 0.0869f, down_rad_x = 0.3234f,
                    down_rad_y = 0.1032f;
                /* abscisele ce delimiteaza textura laterala */
                float mid_left_x = 0.1766f, mid_right_x = 0.8234f;

                vector<lab::VertexFormat> vertices{
                    lab::VertexFormat(0.0f,  0.112f, 0.0f, 0.0f,  1.0f, 0.0f,   up_cen_x,   up_cen_y),
                    lab::VertexFormat(0.0f, -0.079f, 0.0f, 0.0f, -1.0f, 0.0f, down_cen_x, down_cen_y),
                };
                vector<unsigned int> indices;
                float degree = 0, delta_degree = 2 * glm::pi<float>() / UFO_VERT_NUM;

                for (int j = 0; j < UFO_VERT_NUM; ++j)
                {
                    unsigned int crt_offset = vertices.size();
                    if (j % 2 == 0)
                        crt_offset += 12;
                    unsigned int next_offset = j == UFO_VERT_NUM - 1 ? 2 : crt_offset + 14;
                    vertices.push_back(lab::VertexFormat(lab::PolarCoord(0.129f, degree), 0.112f,
                                                         lab::PolarCoord(0.0f, degree), 1.0f,
                                                         up_cen_x + cos(degree) * up_rad_x,
                                                         up_cen_y - sin(degree) * up_rad_y));
                    float tx_init = j % 2 == 0 ? mid_right_x : 0.5f;
                    for (float tx = tx_init; tx > 0; tx -= mid_right_x - mid_left_x)
                        vertices.insert(vertices.end(), {
                            lab::VertexFormat(lab::PolarCoord(0.129f, degree),  0.112f, lab::PolarCoord( 0.82f, degree),  0.572f, tx, 0.7698f),
                            lab::VertexFormat(lab::PolarCoord(0.152f, degree),  0.079f, lab::PolarCoord( 0.82f, degree),  0.572f, tx, 0.7428f),
                            lab::VertexFormat(lab::PolarCoord(0.152f, degree),  0.079f, lab::PolarCoord(0.281f, degree),   0.96f, tx, 0.7428f),
                            lab::VertexFormat(lab::PolarCoord(0.251f, degree),   0.05f, lab::PolarCoord(0.199f, degree),   0.98f, tx, 0.6735f),
                            lab::VertexFormat(lab::PolarCoord(0.305f, degree),  0.044f, lab::PolarCoord(0.155f, degree),  0.988f, tx,  0.637f),
                            lab::VertexFormat(lab::PolarCoord(0.411f, degree),  0.023f, lab::PolarCoord(0.221f, degree),  0.975f, tx, 0.5642f),
                            lab::VertexFormat(lab::PolarCoord(  0.5f, degree),    0.0f, lab::PolarCoord(0.221f, degree),  0.975f, tx, 0.5028f),
                            lab::VertexFormat(lab::PolarCoord(  0.5f, degree),    0.0f, lab::PolarCoord(0.221f, degree), -0.975f, tx, 0.5028f),
                            lab::VertexFormat(lab::PolarCoord(0.411f, degree), -0.023f, lab::PolarCoord(0.221f, degree), -0.975f, tx, 0.4414f),
                            lab::VertexFormat(lab::PolarCoord(0.305f, degree), -0.044f, lab::PolarCoord(0.155f, degree), -0.988f, tx, 0.3685f),
                            lab::VertexFormat(lab::PolarCoord(0.251f, degree),  -0.05f, lab::PolarCoord(0.199f, degree),  -0.98f, tx,  0.332f),
                            lab::VertexFormat(lab::PolarCoord(0.152f, degree), -0.079f, lab::PolarCoord(0.281f, degree),  -0.96f, tx, 0.2628f),
                        });
                    vertices.push_back(lab::VertexFormat(lab::PolarCoord(0.152f, degree), -0.079f,
                                                         lab::PolarCoord(0.0f, degree), -1.0f,
                                                         down_cen_x + cos(degree) * down_rad_x,
                                                         down_cen_y + sin(degree) * down_rad_y));
                    indices.insert(indices.end(), {
                        next_offset +  0, j % 2 == 1 ? crt_offset : crt_offset - 12, 0,
                        next_offset +  1, crt_offset +  1, next_offset +  2, crt_offset +  1, next_offset +  2, crt_offset +  2,
                        next_offset +  3, crt_offset +  3, next_offset +  4, crt_offset +  3, next_offset +  4, crt_offset +  4,
                        next_offset +  4, crt_offset +  4, next_offset +  5, crt_offset +  4, next_offset +  5, crt_offset +  5,
                        next_offset +  5, crt_offset +  5, next_offset +  6, crt_offset +  5, next_offset +  6, crt_offset +  6,
                        next_offset +  6, crt_offset +  6, next_offset +  7, crt_offset +  6, next_offset +  7, crt_offset +  7,
                        next_offset +  8, crt_offset +  8, next_offset +  9, crt_offset +  8, next_offset +  9, crt_offset +  9,
                        next_offset +  9, crt_offset +  9, next_offset + 10, crt_offset +  9, next_offset + 10, crt_offset + 10,
                        next_offset + 10, crt_offset + 10, next_offset + 11, crt_offset + 10, next_offset + 11, crt_offset + 11,
                        next_offset + 11, crt_offset + 11, next_offset + 12, crt_offset + 11, next_offset + 12, crt_offset + 12,
                        j % 2 == 0 ? next_offset + 13 : next_offset + 25, crt_offset + 13, 1,
                    });
                    degree += delta_degree;
                }
                mesh_types.push_back(MeshType(vertices, indices, ufo_texture, UFO_DIAMETER));
            }
            else
            {
                int texture;
                float dim_x, dim_z;
                float tx_off, tx_inc, ty_off, ty_inc;
                if (i >= OBJ_STREET_END && i <= OBJ_STREET_STRAIGHT_GSxCS)
                {
                    texture = street_texture;
                    dim_x = dim_z = CELL_SIZE;
                    switch (i)
                    {
                        case OBJ_STREET_END:
                            tx_off = 0.0146f, ty_off = 0.6699f;
                            break;
                        case OBJ_STREET_FULL_CONNECTED:
                            tx_off =    0.5f, ty_off = 0.6699f;
                            break;
                        case OBJ_STREET_SEMI_CONNECTED:
                            tx_off = 0.0146f, ty_off = 0.3397f;
                            break;
                        case OBJ_STREET_THREE_WAY:
                            tx_off =    0.5f, ty_off = 0.3397f;
                            break;
                        case OBJ_STREET_FOUR_WAY:
                            tx_off = 0.0146f, ty_off = 0.0096f;
                            break;
                        case OBJ_STREET_STRAIGHT_CSxCS:
                            tx_off =    0.5f, ty_off = 0.0096f;
                            break;
                        case OBJ_STREET_STRAIGHT_XGxCS:
                            dim_x = x_gap;
                            tx_off =    0.5f, ty_off = 0.0096f;
                            break;
                        case OBJ_STREET_STRAIGHT_GSxCS:
                            dim_x = GAP_SIZE;
                            tx_off =    0.5f, ty_off = 0.0096f;
                            break;
                    }
                    tx_inc = dim_x * 0.4854f / CELL_SIZE, ty_inc = dim_z * 0.3205f / CELL_SIZE;
                }
                else if (i >= OBJ_GREENSPACE_CSxCS && i <= OBJ_GREENSPACE_XGxCS)
                {
                    texture = grass_texture;
                    tx_off = ty_off = 0;
                    switch (i)
                    {
                        case OBJ_GREENSPACE_CSxCS:
                            dim_x = dim_z = CELL_SIZE;
                            break;
                        case OBJ_GREENSPACE_CSxGS:
                            dim_x = CELL_SIZE, dim_z = GAP_SIZE;
                            break;
                        case OBJ_GREENSPACE_XGxGS:
                            dim_x = x_gap, dim_z = GAP_SIZE;
                            break;
                        case OBJ_GREENSPACE_XGxCS:
                            dim_x = x_gap, dim_z = CELL_SIZE;
                            break;
                    }
                    tx_inc = dim_x / CELL_SIZE;
                    ty_inc = dim_z / CELL_SIZE;
                }
                else if (i == OBJ_MARGIN)
                {
                    texture = margin_texture;
                    dim_x = grid_size, dim_z = margin_size;
                    tx_off = 0, tx_inc = grid_size / margin_size;
                    ty_off = 0.0283f, ty_inc = 0.9434f;
                }
                else /* i == OBJ_CORNER */
                {
                    texture = corner_texture;
                    dim_x = dim_z = margin_size;
                    tx_off = ty_off = 0.0283f;
                    tx_inc = ty_inc = 0.9434f;
                }
                mesh_types.push_back(MeshType(
                    vector<lab::VertexFormat>{
                        lab::VertexFormat( dim_x / 2, 0,  dim_z / 2, 0, 1, 0, tx_off + tx_inc, ty_off),
                        lab::VertexFormat( dim_x / 2, 0, -dim_z / 2, 0, 1, 0, tx_off + tx_inc, ty_off + ty_inc),
                        lab::VertexFormat(-dim_x / 2, 0, -dim_z / 2, 0, 1, 0, tx_off,          ty_off + ty_inc),
                        lab::VertexFormat(-dim_x / 2, 0,  dim_z / 2, 0, 1, 0, tx_off,          ty_off)},
                    vector<unsigned int>{0, 1, 3, 1, 2, 3},
                    texture));
            }
        }
    }

    /* determina center_x si center_z, relativ la care se fac animatiile de la inceput */
    void findCenter()
    {
        if (array_dim % 8 == 0)
            center_x = center_z = grid_size / 2;
        else if (array_dim % 8 == 2)
        {
            center_x = (grid_size - 3 * CELL_SIZE - x_gap) / 2;
            center_z = grid_size / 2 + CELL_SIZE * 1.5f + GAP_SIZE * 0.5f;
        }
        else if (array_dim % 8 == 4)
        {
            center_x = grid_size / 2 + x_gap * 1.5f + CELL_SIZE * 2.5f;
            center_z = grid_size / 2;
        }
        else
        {
            center_x = (grid_size + 2 * (CELL_SIZE + x_gap)) / 2;
            center_z = (grid_size + 3 * CELL_SIZE + GAP_SIZE) / 2;
        }
    }

    /* creeaza obiectele si pozitioneaza camera view2 */
    void createWorld()
    {
        srand((unsigned int)time(NULL));

        /* calculez x_gap */
        float grid_size_Ox = array_dim * CELL_SIZE;
        int gap_num_Oz;
        if (array_dim % 4 == 0)
        {
            gap_num_Oz = array_dim * 3 / 4;
            grid_size_Ox += (array_dim / 4 - 1) * CELL_SIZE + gap_num_Oz * GAP_SIZE;
        }
        else
        {
            gap_num_Oz = (array_dim + 2) * 3 / 4 - 2;
            grid_size_Ox += ((array_dim + 2) / 4 - 1) * CELL_SIZE + gap_num_Oz * GAP_SIZE;
        }
        float grid_size_Oz = (array_dim * 1.5f) * CELL_SIZE + (array_dim / 2 - 1) * GAP_SIZE;
        x_gap = GAP_SIZE + (grid_size_Oz - grid_size_Ox) / gap_num_Oz;

        grid_size = grid_size_Oz;
        margin_size = grid_size / static_cast<int>(grid_size / CELL_SIZE);

        createGeometry();
        findCenter();

        float off_z = 0;
        world.resize(2 * array_dim - 1);
        for (int i = 0; i < 2 * array_dim - 1; ++i)
        {
            float off_x = 0;
            world[i].resize(2 * array_dim - 1);
            for (int j = 0; j < 2 * array_dim - 1; ++j)
            {
                if (i % 2 == 0 && j % 2 == 0)
                {
                    /* creez o casa si o orientez spre strada */
                    /* casele vor avea zero sau un etaj in mod aleatoriu, cu exceptia celor doua din
                     * prima scena: cea din stanga nu va avea etaj, cea de-a doua va avea */
                    bool left_house, right_house;
                    if (array_dim % 8 == 0)
                    {
                        left_house  = i == array_dim && j == array_dim - 6;
                        right_house = i == array_dim && j == array_dim - 4;
                    }
                    else if (array_dim % 8 == 2)
                    {
                        left_house  = i == array_dim + 2 && j == array_dim - 8;
                        right_house = i == array_dim + 2 && j == array_dim - 6;
                    }
                    else if (array_dim % 8 == 4)
                    {
                        left_house  = i == array_dim && j == array_dim - 2;
                        right_house = i == array_dim && j == array_dim;
                    }
                    else
                    {
                        left_house  = i == array_dim + 2 && j == array_dim - 4;
                        right_house = i == array_dim + 2 && j == array_dim - 2;
                    }
                    if (left_house)
                        world[i][j].mesh_type = OBJ_ONE_STORY_HOUSE;
                    else if (right_house)
                        world[i][j].mesh_type = OBJ_TWO_STORY_HOUSE;
                    else if (rand() % 2 == 1)
                        world[i][j].mesh_type = OBJ_ONE_STORY_HOUSE;
                    else
                        world[i][j].mesh_type = OBJ_TWO_STORY_HOUSE;
                    int row = i / 2;
                    int col = j / 2;
                    world[i][j].model_matrix = glm::translate(world[i][j].model_matrix, glm::vec3(
                        off_x + CELL_SIZE * 0.5,
                        0.0,
                        off_z + CELL_SIZE * 0.5));
                    float angle;
                    if (row == 0)
                        angle = 0.0f;
                    else if (row == array_dim - 1)
                        angle = 180.0f;
                    else if (col == 0 || col == array_dim - 1 || col % 4 == 1 || col % 4 == 2)
                        angle = row % 2 == 0 ? 0.0f : 180.0f;
                    else if (col % 4 == 3)
                        angle = 90.0f;
                    else
                        angle = -90.0f;
                    world[i][j].model_matrix = glm::rotate(world[i][j].model_matrix, angle,
                                                           glm::vec3(0.0, 1.0, 0.0));
                }
                else if (i % 4 == 1 && (j == 0 || j == 2 * array_dim - 2))
                {
                    world[i][j].mesh_type = OBJ_STREET_END;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + CELL_SIZE * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                    if (j == 2 * array_dim - 2)
                        world[i][j].model_matrix = glm::rotate(world[i][j].model_matrix, 180.0f,
                                                               glm::vec3(0.0, 1.0, 0.0));
                }
                else if (i % 4 == 1 && (j % 8 == 2 || j % 8 == 4))
                {
                    world[i][j].mesh_type = OBJ_STREET_FULL_CONNECTED;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + CELL_SIZE * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                }
                else if ((i == 0 || i == 2 * array_dim - 2) && j % 8 == 7)
                {
                    world[i][j].mesh_type = OBJ_GREENSPACE_CSxCS;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + CELL_SIZE * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                }
                else if (i % 2 == 0 && j % 8 == 7)
                {
                    world[i][j].mesh_type = OBJ_STREET_FULL_CONNECTED;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + CELL_SIZE * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                    world[i][j].model_matrix = glm::rotate(world[i][j].model_matrix, 90.0f,
                                                           glm::vec3(0.0, 1.0, 0.0));
                }
                else if ((i == 1 || i == 2 * array_dim - 3) && (j % 8 == 0 || j % 8 == 6))
                {
                    world[i][j].mesh_type = OBJ_STREET_SEMI_CONNECTED;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + CELL_SIZE * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                    if (i == 2 * array_dim - 3)
                        world[i][j].model_matrix = glm::rotate(world[i][j].model_matrix, 180.0f,
                                                               glm::vec3(0.0, 1.0, 0.0));
                }
                else if ((i == 1 || i == 2 * array_dim - 3) && (j % 8 == 7))
                {
                    world[i][j].mesh_type = OBJ_STREET_THREE_WAY;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + CELL_SIZE * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                    if (i == 2 * array_dim - 3)
                        world[i][j].model_matrix = glm::rotate(world[i][j].model_matrix, 180.0f,
                                                               glm::vec3(0.0, 1.0, 0.0));
                }
                else if (i % 4 == 1 && j % 8 == 7)
                {
                    world[i][j].mesh_type = OBJ_STREET_FOUR_WAY;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + CELL_SIZE * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                }
                else if (i % 4 == 1 && (j % 8 == 0 || j % 8 == 6))
                {
                    world[i][j].mesh_type = OBJ_STREET_STRAIGHT_CSxCS;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + CELL_SIZE * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                }
                else if (i % 4 == 1 && (j % 8 == 1 || j % 8 == 3 || j % 8 == 5))
                {
                    world[i][j].mesh_type = OBJ_STREET_STRAIGHT_XGxCS;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + x_gap * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                }
                else if (i % 4 == 3)
                    if (j % 8 == 7)
                    {
                        world[i][j].mesh_type = OBJ_STREET_STRAIGHT_GSxCS;
                        world[i][j].model_matrix = glm::translate(
                            world[i][j].model_matrix,
                            glm::vec3(off_x + CELL_SIZE * 0.5, 0.0, off_z + GAP_SIZE * 0.5));
                        world[i][j].model_matrix = glm::rotate(world[i][j].model_matrix, 90.0f,
                                                               glm::vec3(0.0, 1.0, 0.0));
                    }
                    else
                    {
                        float dim_x;
                        if (j % 2 == 0)
                        {
                            dim_x = CELL_SIZE;
                            world[i][j].mesh_type = OBJ_GREENSPACE_CSxGS;
                        }
                        else
                        {
                            dim_x = x_gap;
                            world[i][j].mesh_type = OBJ_GREENSPACE_XGxGS;
                        }
                        world[i][j].model_matrix = glm::translate(
                            world[i][j].model_matrix,
                            glm::vec3(off_x + dim_x * 0.5, 0.0, off_z + GAP_SIZE * 0.5));
                    }
                else if (i % 2 == 0 && (j % 8 == 1 || j % 8 == 3 || j % 8 == 5))
                {
                    world[i][j].mesh_type = OBJ_GREENSPACE_XGxCS;
                    world[i][j].model_matrix = glm::translate(
                        world[i][j].model_matrix,
                        glm::vec3(off_x + x_gap * 0.5, 0.0, off_z + CELL_SIZE * 0.5));
                }
                world[i][j].computeWorldVertices(mesh_types);
                if (j % 8 == 1 || j % 8 == 3 || j % 8 == 5)
                    off_x += x_gap;
                else
                    off_x += CELL_SIZE;
            }
            if (i % 4 < 3)
                off_z += CELL_SIZE;
            else
                off_z += GAP_SIZE;
        }

        /* creez meshurile marginale */
        margins.resize(4);
        for (int i = 0; i < 4; ++i)
        {
            margins[i].mesh_type = OBJ_MARGIN;
            float trans_x = i == 0 ? -margin_size / 2 : i == 1 ?
                            grid_size + margin_size / 2 : grid_size / 2;
            float trans_z = i == 2 ? -margin_size / 2 : i == 3 ?
                            grid_size + margin_size / 2 : grid_size / 2;
            margins[i].model_matrix = glm::translate(margins[i].model_matrix,
                                                     glm::vec3(trans_x, 0.0, trans_z));
            float ang = 0;
            switch (i)
            {
                case 0: ang =  90; break;
                case 1: ang = -90; break;
                case 3: ang = 180; break;
            }
            margins[i].model_matrix = glm::rotate(margins[i].model_matrix, ang,
                                                  glm::vec3(0.0, 1.0, 0.0));
            margins[i].computeWorldVertices(mesh_types);
        }

        corners.resize(4);
        for (int i = 0; i < 4; ++i)
        {
            corners[i].mesh_type = OBJ_CORNER;
            float trans_x = i % 2 == 0 ? -margin_size / 2 : grid_size + margin_size / 2;
            float trans_z = i / 2 == 0 ? -margin_size / 2 : grid_size + margin_size / 2;
            corners[i].model_matrix = glm::translate(corners[i].model_matrix,
                                                     glm::vec3(trans_x, 0.0, trans_z));
            float ang = 0;
            switch (i)
            {
                case 0: ang = 90; break;
                case 2: ang = 180; break;
                case 3: ang = -90; break;
            }
            corners[i].model_matrix = glm::rotate(corners[i].model_matrix, ang,
                                                  glm::vec3(0.0, 1.0, 0.0));
            corners[i].computeWorldVertices(mesh_types);
        }

        /* creez suprafetele redimensionabile in orice directie */
        for (int i = 0; i < 8; ++i)
        {
            float dim_x, dim_z;
            if (i == 1 || i == 6)
                dim_x = grid_size + 2 * margin_size;
            else
                dim_x = MAX_DIST;
            if (i == 3 || i == 4)
                dim_z = grid_size + 2 * margin_size;
            else
                dim_z = MAX_DIST;
            float off_x, off_z;
            if (i == 0 || i == 3 || i == 5)
                off_x = -margin_size - dim_x / 2;
            else if (i == 1 || i == 6)
                off_x = -margin_size + dim_x / 2;
            else
                off_x = grid_size + margin_size + dim_x / 2;
            if (i == 0 || i == 1 || i == 2)
                off_z = -margin_size - dim_z / 2;
            else if (i == 3 || i == 4)
                off_z = -margin_size + dim_z / 2;
            else
                off_z = grid_size + margin_size + dim_z / 2;
            float t1, t2, t3, t4;
            switch (i)
            {
                case 0:
                case 1:
                    t1 = 1, t2 = 0, t3 = 0, t4 = -1;
                    break;
                case 2:
                case 4:
                    t1 = 1, t2 = 0, t3 = 1, t4 = 0;
                    break;
                case 3:
                case 5:
                    t1 = 0, t2 = -1, t3 = 0, t4 = -1;
                    break;
                case 6:
                case 7:
                    t1 = 0, t2 = -1, t3 = 1, t4 = 0;
            }
            t3 *= dim_x / margin_size, t4 *= dim_x / margin_size;
            t1 *= dim_z / margin_size, t2 *= dim_z / margin_size;

            infinite_surfaces.push_back(InfSurface(
                vector<lab::VertexFormat>{
                    lab::VertexFormat(off_x + dim_x / 2, 0, off_z + dim_z / 2, 0, 1, 0, t3, t2),
                    lab::VertexFormat(off_x + dim_x / 2, 0, off_z - dim_z / 2, 0, 1, 0, t3, t1),
                    lab::VertexFormat(off_x - dim_x / 2, 0, off_z - dim_z / 2, 0, 1, 0, t4, t1),
                    lab::VertexFormat(off_x - dim_x / 2, 0, off_z + dim_z / 2, 0, 1, 0, t4, t2)},
                vector<unsigned int>{0, 1, 3, 1, 2, 3},
                forever_texture));
            infinite_surfaces[i].computeWorldVertices();
            infinite_surfaces[i].ext_level_x = infinite_surfaces[i].ext_level_z = 1;
        }
        view2.set(glm::vec3(grid_size / 2, grid_size * 220 / 268, grid_size / 2),
                  glm::vec3(grid_size / 2, 0.0, grid_size / 2),
                  glm::vec3(0.0, 0.0, -1.0));
    }

    //----------------------------------------------------------------------------------------------
    //metode legate mai mult de logica programului -------------------------------------------------
    bool isPressed(const unsigned char key, const bool special_key) const
    {
        return pressed_keys.test(key) && (is_special.test(key) == special_key);
    }

    //----------------------------------------------------------------------------------------------
    //metode de cadru ------------------------------------------------------------------------------

    /* functie chemata inainte de a incepe cadrul de desenare, o folosim ca sa updatam situatia
     * scenei (modelam / simulam scena) */
    void notifyBeginFrame()
    {
        static bool first_frame = true;
        LARGE_INTEGER perf_time;
        QueryPerformanceCounter(&perf_time);
        float new_time = static_cast<float>(perf_time.QuadPart) / perf_freq.QuadPart;
        if (paused && !first_frame)
        {
            start_time += new_time - last_time;
            last_time = new_time;
            return;
        }
        if (first_frame)
        {
            start_time = new_time;
            first_frame = false;
        }

        /* timpul trecut in cadrul scenei curente */
        float scene_time = new_time - start_time;

        /* trecerea la scena urmatoare */
        switch (current_scene)
        {
            case SCENE_INTRODUCTION:
                if (scene_time > 6)
                {
                    current_scene = SCENE_MOTHERSHIP_COMES_OUT;
                    start_time = new_time;
                    scene_time = 0;
                }
                break;
            case SCENE_MOTHERSHIP_COMES_OUT:
                if (scene_time > 7)
                {
                    current_scene = SCENE_MOTHERSHIP_STOPS;
                    start_time = new_time;
                    scene_time = 0;
                    view1.set(glm::vec3(center_x + CELL_SIZE, 43.86f,
                                        center_z + CELL_SIZE + GAP_SIZE / 2),
                              glm::vec3(center_x, 14.035, center_z + GAP_SIZE / 2),
                              glm::vec3(0.0, 1.0, 0.0));
                }
                break;
            case SCENE_MOTHERSHIP_STOPS:
                if (scene_time > 1.9)
                {
                    current_scene = SCENE_SPAWN;
                    start_time = new_time;
                    scene_time = 0;
                    character.model_matrix = glm::translate(character.model_matrix,
                                                            glm::vec3(center_x, 20.807, center_z));
                }
                break;
            case SCENE_SPAWN:
                if (scene_time > 9.7)
                {
                    current_scene = GAME_STARTED;
                    start_time = new_time;
                    scene_time = 0;
                }
                break;
            case SCENE_FADE_OUT:
                if (scene_time > 1)
                {
                    current_scene = SCENE_INTRODUCTION;
                    start_time = new_time;
                    scene_time = 0;
                    character = Character();
                }
        }

        /* actualizarea obiectelor conform timpului trecut in cadrul scenei curente */
        switch (current_scene)
        {
            case GAME_STARTED:
                if (!paused)
                {
                    /* controlul personajului */
                    float frame_delta_ang  = (new_time - last_time) * DELTA_ANG;
                    float frame_delta_tilt = (new_time - last_time) * DELTA_TILT;
                    float frame_delta_dist = (new_time - last_time) * DELTA_DIST;

                    if (isPressed('w', false) || isPressed(KEY_UP, true))
                        ENABLE_TILT ? character.tiltFront(frame_delta_tilt) :
                                      character.translateOnwardWithoutTilt(frame_delta_dist);
                    if (isPressed('a', false))
                        ENABLE_TILT ? character.tiltLeft(frame_delta_tilt) :
                                      character.translateSidewaysWithoutTilt(-frame_delta_dist);
                    if (isPressed('s', false) || isPressed(KEY_DOWN, true))
                        ENABLE_TILT ? character.tiltBack(frame_delta_tilt) :
                                      character.translateOnwardWithoutTilt(-frame_delta_dist);
                    if (isPressed('d', false))
                        ENABLE_TILT ? character.tiltRight(frame_delta_tilt) :
                                      character.translateSidewaysWithoutTilt(frame_delta_dist);
                    if (isPressed('r', false))
                        character.translateUpward(frame_delta_dist);
                    if (isPressed('f', false))
                        character.translateUpward(-frame_delta_dist);
                    if ((isPressed('q', false) || isPressed(KEY_LEFT,  true)) ^
                         isPressed('e', false) || isPressed(KEY_RIGHT, true))
                        character.rotateOy((isPressed('q', false) || isPressed(KEY_LEFT, true) ?
                                            1 : -1) * frame_delta_ang);
                    if (isPressed('t', false))
                        character.rotateOx(frame_delta_ang);
                    if (isPressed('g', false))
                        character.rotateOx(-frame_delta_ang);

                    if (ENABLE_TILT)
                    {
                        /* anulare automata a inclinarii, daca nu se apasa tastele
                         * corespunzatoare */
                        if (!isPressed('a', false) && !isPressed('d', false))
                            character.cancelTilt(AXIS_X, frame_delta_tilt);
                        if (!isPressed('w', false) && !isPressed('s', false) &&
                            !isPressed(KEY_UP, true) && !isPressed(KEY_DOWN, true))
                            character.cancelTilt(AXIS_Z, frame_delta_tilt);

                        /* deplasare conform inclinarii */
                        character.translateOnward(frame_delta_dist);
                        character.translateSideways(frame_delta_dist);
                    }

                    character.collideWithGround(mesh_types[OBJ_UFO].vertices);

                    view1.set(glm::vec3(character.model_matrix *
                                        glm::vec4(0.0, UFO_DIAMETER, 1.9 * UFO_DIAMETER, 1.0)),
                              glm::vec3(character.model_matrix *
                                        glm::vec4(0.0, UFO_DIAMETER, 0.0, 1.0)),
                              glm::vec3(character.model_matrix * glm::vec4(0.0, 1.0, 0.0, 0.0)));
                }
                break;
            case SCENE_INTRODUCTION:
            {
                float pos_x = center_x - 3.5f * CELL_SIZE - 3 * x_gap +
                              scene_time / 10 * (2 * CELL_SIZE + 3 * x_gap);
                glm::vec3 view1_pos(pos_x, 1.75f, center_z + GAP_SIZE / 2 + CELL_SIZE * 1.75f);
                float target_x = center_x - 3 * CELL_SIZE - 2 * x_gap +
                                 scene_time / 10 * (CELL_SIZE + x_gap);
                glm::vec3 view1_target(target_x, 5.096f, center_z + CELL_SIZE / 2 + GAP_SIZE / 2);
                view1_pos = view1_target +
                            glm::normalize(view1_pos - view1_target) * 1.4f * CELL_SIZE;
                view1.set(view1_pos, view1_target, glm::vec3(0.0, 1.0, 0.0));
                break;
            }
            case SCENE_MOTHERSHIP_COMES_OUT:
            {
                if (scene_time < 4)
                {
                    float pos_z;
                    if (scene_time < 2)
                        /* deplasare liniara */
                        pos_z = center_z - GAP_SIZE / 2 - 1.1f * CELL_SIZE +
                                0.3f * CELL_SIZE * (2 - scene_time);
                    else
                        /* deplasare franata */
                        pos_z = center_z - GAP_SIZE / 2 - 1.1f * CELL_SIZE +
                                0.075f * CELL_SIZE * (scene_time - 2) * (scene_time - 2) -
                                0.3f * CELL_SIZE * (scene_time - 2);
                    view1.set(glm::vec3(center_x, 1.75f, pos_z),
                              glm::vec3(center_x, 7.017f, pos_z - CELL_SIZE),
                              glm::vec3(0.0, 1.0, 0.0));
                }
                else
                {
                    /* pozitia navei pe Oy e o functie de gradul al doilea de pozitia pe Oz, care e
                     * o functie de gradul I de timp */
                    float a = (MAX_DIST - 20.807f) / (MAX_DIST * MAX_DIST - 140.351f * MAX_DIST +
                              4924.592f);
                    float b = 140.351f * a;
                    float c = 4924.592f * a + 20.807f;
                    float delay_z = -MAX_DIST + (scene_time - 4) / 3 * (MAX_DIST - 70.175f);
                    float pos_y = a * delay_z * delay_z + b * delay_z + c;
                    mothership.model_matrix = glm::translate(glm::mat4(),
                                                             glm::vec3(center_x, pos_y,
                                                                       center_z + delay_z));
                }
                break;
            }
            case SCENE_MOTHERSHIP_STOPS:
            {
                /* pozitia navei pe Oz e o functie de gradul II de timp */
                float b = (MAX_DIST - 70.175f) / 3;
                float a = -b * b / 280.702f;
                float c = center_z - 70.175f;
                if (scene_time < -b / (2 * a))
                {
                    float pos_z = a * scene_time * scene_time + b * scene_time + c;
                    mothership.model_matrix = glm::translate(glm::mat4(),
                                                             glm::vec3(center_x, 20.807f, pos_z));
                }
                break;
            }
            case SCENE_SPAWN:
            {
                float final_cam_y = 10 + UFO_DIAMETER;
                float cam_y = (26.316f - final_cam_y) * pow(scene_time / 9.7f - 1, 2) + final_cam_y;
                if (cam_y < final_cam_y)
                    cam_y = final_cam_y;
                float cam_z = center_z + CELL_SIZE * 2 + GAP_SIZE / 2;
                if (scene_time > 7.7)
                {
                    cam_z -= (scene_time - 7.7f) / 2 * (CELL_SIZE * 2 + GAP_SIZE / 2 -
                             1.9f * UFO_DIAMETER);
                    if (cam_z < center_z + 1.9f * UFO_DIAMETER)
                        cam_z = center_z + 1.9f * UFO_DIAMETER;
                }
                view1.set(glm::vec3(center_x, cam_y, cam_z),
                          glm::vec3(center_x, cam_y, center_z),
                          glm::vec3(0.0, 1.0, 0.0));
                if (scene_time > 3 && scene_time <= 4.5)
                    character.model_matrix = glm::translate(glm::mat4(),
                                                            glm::vec3(center_x,
                                                                      20.807 - (scene_time - 3) /
                                                                          1.5 * 10.807,
                                                                      center_z));
                else if (scene_time > 6.7 && scene_time < 8.7)
                {
                    float pos_x;
                    if (scene_time < 7.7)
                        pos_x = center_x + MOTHERSHIP_DIAMETER * sqrt(1 -
                                pow(scene_time - 7.7f, 2));
                    else
                        pos_x = center_x + MOTHERSHIP_DIAMETER;
                    float pos_z = center_z - (scene_time - 6.7f) / 2 * MAX_DIST;
                    float pos_y = 20.807f + MAX_DIST * pow((pos_z - center_z) / MAX_DIST, 2);
                    mothership.model_matrix = glm::translate(glm::mat4(),
                                                             glm::vec3(pos_x, pos_y, pos_z));
                }
            }
        }

        /* extind / restrang suprafata din jurul matricei */
        glm::vec3 view1_pos(view1.getPosition());
        int temp_ext;
        for (int i = 0; i < 8; ++i)
        {
            /* extindere / restrangere pe Ox */
            if (i != 1 && i != 6)
            {
                bool intersect;
                if (i == 0 || i == 2)
                    intersect = view1_pos[2] - MAX_DIST < -margin_size;
                else if (i == 5 || i == 7)
                    intersect = view1_pos[2] + MAX_DIST > grid_size + margin_size;
                else
                    intersect = view1_pos[2] + MAX_DIST > -margin_size &&
                                view1_pos[2] - MAX_DIST < grid_size + margin_size;
                if (intersect)
                    if (i == 0 || i == 3 || i == 5)
                        temp_ext = max(1, static_cast<int>((-(view1_pos[0] - MAX_DIST) -
                                              margin_size) / MAX_DIST + 1));
                    else
                        temp_ext = max(1, static_cast<int>((view1_pos[0] + MAX_DIST - (grid_size +
                                              margin_size)) / MAX_DIST + 1));
                else
                    temp_ext = 1;
                if (infinite_surfaces[i].ext_level_x != temp_ext)
                {
                    infinite_surfaces[i].ext_level_x = temp_ext;
                    float new_size = temp_ext * MAX_DIST;
                    float new_x_limit;
                    int ext_point1, ext_point2, txt_x_sign;
                    if (i == 0 || i == 3 || i == 5)
                    {
                        new_x_limit = -margin_size - new_size;
                        ext_point1 = 2, ext_point2 = 3;
                        txt_x_sign = -1;
                    }
                    else
                    {
                        new_x_limit = grid_size + margin_size + new_size;
                        ext_point1 = 0, ext_point2 = 1;
                        txt_x_sign = 1;
                    }
                    infinite_surfaces[i].vertices[ext_point1].position_x = new_x_limit;
                    infinite_surfaces[i].vertices[ext_point2].position_x = new_x_limit;
                    infinite_surfaces[i].vertices[ext_point1].texcoord_x = txt_x_sign * new_size /
                        margin_size;
                    infinite_surfaces[i].vertices[ext_point2].texcoord_x = txt_x_sign * new_size /
                        margin_size;
                    infinite_surfaces[i].updateGeom();
                }
            }
            /* extindere / restrangere pe Oz */
            if (i != 3 && i != 4)
            {
                bool intersect;
                if (i == 0 || i == 5)
                    intersect = view1_pos[0] - MAX_DIST < -margin_size;
                else if (i == 2 || i == 7)
                    intersect = view1_pos[0] + MAX_DIST > grid_size + margin_size;
                else
                    intersect = view1_pos[0] + MAX_DIST > -margin_size &&
                                view1_pos[0] - MAX_DIST < grid_size + margin_size;
                if (intersect)
                    if (i == 0 || i == 1 || i == 2)
                        temp_ext = max(1, static_cast<int>((-(view1_pos[2] - MAX_DIST) -
                                              margin_size) / MAX_DIST + 1));
                    else
                        temp_ext = max(1, static_cast<int>((view1_pos[2] + MAX_DIST -
                                              (grid_size + margin_size)) / MAX_DIST + 1));
                else
                    temp_ext = 1;
                if (infinite_surfaces[i].ext_level_z != temp_ext)
                {
                    infinite_surfaces[i].ext_level_z = temp_ext;
                    float new_size = temp_ext * MAX_DIST;
                    float new_z_limit;
                    int ext_point1, ext_point2, txt_y_sign;
                    if (i == 0 || i == 1 || i == 2)
                    {
                        new_z_limit = -margin_size - new_size;
                        ext_point1 = 1, ext_point2 = 2;
                        txt_y_sign = 1;
                    }
                    else
                    {
                        new_z_limit = grid_size + margin_size + new_size;
                        ext_point1 = 0, ext_point2 = 3;
                        txt_y_sign = -1;
                    }
                    infinite_surfaces[i].vertices[ext_point1].position_z = new_z_limit;
                    infinite_surfaces[i].vertices[ext_point2].position_z = new_z_limit;
                    infinite_surfaces[i].vertices[ext_point1].texcoord_y = txt_y_sign * new_size /
                        margin_size;
                    infinite_surfaces[i].vertices[ext_point2].texcoord_y = txt_y_sign * new_size /
                        margin_size;
                    infinite_surfaces[i].updateGeom();
                }
            }
        }

        last_time = new_time;
    }

    /* deseneaza toate mesh-urile ce iau parte la view frustum culling */
    void drawMesh(Mesh &mesh)
    {
        bool draw = view1.isVisible(mesh.getWorldVertices());
        if (draw || active_cam == &view2)
        {
            /* trimit matricea de modelare la shader */
            glUniformMatrix4fv(glGetUniformLocation(gl_program_shader,"model_matrix"),1,false,
                               glm::value_ptr(mesh.model_matrix));

            if (active_cam == &view1)
            {
                /* texturare */
                glActiveTexture(GL_TEXTURE0 + 0);
                glBindTexture(GL_TEXTURE_2D, mesh_types[mesh.mesh_type].texture);
                glUniform1i(glGetUniformLocation(gl_program_shader, "textura1"), 0);
            }
            else
                glUniform1i(glGetUniformLocation(gl_program_shader, "code"),
                                                 draw ? CODE_VISIBLE : CODE_HIDDEN);

            /* bind obiect */
            glBindVertexArray(mesh_types[mesh.mesh_type].vao);
            glDrawElements(GL_TRIANGLES, mesh_types[mesh.mesh_type].num_indices, GL_UNSIGNED_INT,
                           0);
        }
    }

    /* functia de afisare (lucram cu banda grafica) */
    void notifyDisplayFrame()
    {
        /* pe tot ecranul */
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, screen_width, screen_height);

        /* foloseste shaderul */
        glUseProgram(gl_program_shader);

        /* trimite variabile uniforme la shader */
        if (current_scene == SCENE_INTRODUCTION && last_time - start_time < 2)
            glUniform1f(glGetUniformLocation(gl_program_shader, "fade"),
                                             1 - 0.5f * (last_time - start_time));
        else if (current_scene == SCENE_FADE_OUT)
            glUniform1f(glGetUniformLocation(gl_program_shader, "fade"), last_time - start_time);
        else
            glUniform1f(glGetUniformLocation(gl_program_shader, "fade"), 0);
        glUniform1i(glGetUniformLocation(gl_program_shader, "mothership_open_valid"), 0);
        glUniformMatrix4fv(glGetUniformLocation(gl_program_shader,"view_matrix"), 1, false,
                           glm::value_ptr(active_cam->getViewMatrix()));
        glUniformMatrix4fv(glGetUniformLocation(gl_program_shader,"projection_matrix"), 1, false,
                           glm::value_ptr(projection_matrix));
        if (active_cam == &view1)
        {
            glm::vec3 cam_pos = view1.getPosition();
            glUniform1i(glGetUniformLocation(gl_program_shader, "enable_fog"), 1);
            glUniform3f(glGetUniformLocation(gl_program_shader, "eye_position"), cam_pos[0],
                        cam_pos[1], cam_pos[2]);
            glUniform1f(glGetUniformLocation(gl_program_shader, "max_dist"), MAX_DIST);
            glUniform1i(glGetUniformLocation(gl_program_shader, "code"), NO_CODE);
            glm::vec3 light_pos = character.getLightPosition();
            glUniform3f(glGetUniformLocation(gl_program_shader, "light_position"), light_pos[0],
                        light_pos[1], light_pos[2]);
            glUniform1f(glGetUniformLocation(gl_program_shader, "material_kd"), 0.5);
            glUniform1f(glGetUniformLocation(gl_program_shader, "material_ks"), 0.5);
            if (current_scene == SCENE_SPAWN || current_scene == GAME_STARTED ||
                current_scene == SCENE_FADE_OUT)
            {
                glUniform1i(glGetUniformLocation(gl_program_shader, "enable_spot_light"), 1);
                glm::vec3 spot_dir = character.getSpotDirection();
                glUniform3f(glGetUniformLocation(gl_program_shader, "D"), spot_dir[0], spot_dir[1],
                            spot_dir[2]);
            }
            else
                glUniform1i(glGetUniformLocation(gl_program_shader, "enable_spot_light"), 0);
        }
        else
            glUniform1i(glGetUniformLocation(gl_program_shader, "enable_fog"), 0);

        view1.updateFrustum(projection_matrix);

        /* exponentul de reflexie speculara pentru case si pamant */
        glUniform1i(glGetUniformLocation(gl_program_shader, "material_shininess"), 5);

        /* desenez casele */
        glActiveTexture(GL_TEXTURE0 + 1);
        glBindTexture(GL_TEXTURE_2D, house_texture_alpha);
        glUniform1i(glGetUniformLocation(gl_program_shader, "has_alpha"), 1);
        glUniform1i(glGetUniformLocation(gl_program_shader, "textura2"), 1);
        for (unsigned int i = 0; i < world.size(); ++i)
            for (unsigned int j = 0; j < world[i].size(); ++j)
                if (i % 2 == 0 && j % 2 == 0)
                    drawMesh(world[i][j]);

        /* desenez obiectele intermediare (strazi, iarba) */
        glUniform1i(glGetUniformLocation(gl_program_shader, "has_alpha"), 0);
        for (unsigned int i = 0; i < world.size(); ++i)
            for (unsigned int j = 0; j < world[i].size(); ++j)
                if (i % 2 != 0 || j % 2 != 0)
                    drawMesh(world[i][j]);

        /* desenez marginile */
        for (int i = 0; i < 4; ++i)
        {
            drawMesh(margins[i]);
            drawMesh(corners[i]);
        }

        /* desenez dreptunghiurile care vor da senzatia de infinitate */
        if (active_cam == &view1)
            for (int i = 0; i < 8; ++i)
                if (view1.isVisible(infinite_surfaces[i].world_vertices))
                {
                    /* trimit matricea de modelare la shader */
                    glUniformMatrix4fv(glGetUniformLocation(gl_program_shader,"model_matrix"), 1,
                                       false, glm::value_ptr(glm::mat4()));

                    /* texturare */
                    glActiveTexture(GL_TEXTURE0 + 0);
                    glBindTexture(GL_TEXTURE_2D, infinite_surfaces[i].texture);
                    glUniform1i(glGetUniformLocation(gl_program_shader, "textura1"), 0);

                    /* bind obiect */
                    glBindVertexArray(infinite_surfaces[i].vao);
                    glDrawElements(GL_TRIANGLES, infinite_surfaces[i].num_indices, GL_UNSIGNED_INT,
                                   0);
                }

        /* desenez personajul */
        if (current_scene == SCENE_SPAWN || current_scene == GAME_STARTED ||
            current_scene == SCENE_FADE_OUT)
        {
            glUniform1i(glGetUniformLocation(gl_program_shader, "material_shininess"), 200);
            if (active_cam == &view1 || (fmod(last_time, BLINK_PERIOD) < BLINK_PERIOD / 2))
            {
                /* trimit matricea de modelare combinata cu inclinarile la shader */
                glUniformMatrix4fv(glGetUniformLocation(gl_program_shader, "model_matrix"), 1,
                                   false, glm::value_ptr(character.getCombinedMatrix()));

                if (active_cam == &view1)
                {
                    /* texturare */
                    glActiveTexture(GL_TEXTURE0 + 0);
                    glBindTexture(GL_TEXTURE_2D, mesh_types[OBJ_UFO].texture);
                    glUniform1i(glGetUniformLocation(gl_program_shader, "textura1"), 0);
                }
                else
                    glUniform1i(glGetUniformLocation(gl_program_shader, "code"), CODE_UFO);

                /* bind obiect */
                glBindVertexArray(mesh_types[OBJ_UFO].vao);
                glDrawElements(GL_TRIANGLES, mesh_types[OBJ_UFO].num_indices, GL_UNSIGNED_INT, 0);
            }
        }

        /* desenez nava mama */
        float scene_time = last_time - start_time;
        if (current_scene != SCENE_INTRODUCTION && current_scene != GAME_STARTED &&
            current_scene != SCENE_FADE_OUT && !(current_scene == SCENE_MOTHERSHIP_COMES_OUT &&
            scene_time < 4))
        {
            glUniform1i(glGetUniformLocation(gl_program_shader, "material_shininess"), 200);
            if (current_scene == SCENE_SPAWN && scene_time > 1)
            {
                glUniform1i(glGetUniformLocation(gl_program_shader, "mothership_open_valid"), 1);
                if (scene_time < 3)
                    glUniform1f(glGetUniformLocation(gl_program_shader, "mothership_open"),
                                (scene_time - 1) / 2 * glm::pi<float>() / 2);
                else if (scene_time < 4.5)
                    glUniform1f(glGetUniformLocation(gl_program_shader, "mothership_open"),
                                glm::pi<float>() / 2);
                else if (scene_time < 6.5)
                    glUniform1f(glGetUniformLocation(gl_program_shader, "mothership_open"),
                                (1 - (scene_time - 4.5f) / 2) * glm::pi<float>() / 2);
                else
                    glUniform1f(glGetUniformLocation(gl_program_shader, "mothership_open"), 0);
                glUniform1f(glGetUniformLocation(gl_program_shader, "mothership_diameter"),
                            MOTHERSHIP_DIAMETER);
                glUniform1i(glGetUniformLocation(gl_program_shader, "mothership_vert_num"),
                            MOTHERSHIP_VERT_NUM);
            }
            glUniformMatrix4fv(glGetUniformLocation(gl_program_shader, "model_matrix"), 1, false,
                               glm::value_ptr(mothership.model_matrix));
            if (active_cam == &view1)
            {
                glActiveTexture(GL_TEXTURE0 + 0);
                glBindTexture(GL_TEXTURE_2D, mothership_texture_color);
                glUniform1i(glGetUniformLocation(gl_program_shader, "textura1"), 0);
                glActiveTexture(GL_TEXTURE0 + 1);
                glBindTexture(GL_TEXTURE_2D, mothership_texture_alpha);
                glUniform1i(glGetUniformLocation(gl_program_shader, "has_alpha"), 1);
                glUniform1i(glGetUniformLocation(gl_program_shader, "textura2"), 1);
            }
            else
                glUniform1i(glGetUniformLocation(gl_program_shader, "code"), CODE_UFO);
            glBindVertexArray(mesh_types[OBJ_MOTHERSHIP].vao);
            glDrawElements(GL_TRIANGLES, mesh_types[OBJ_MOTHERSHIP].num_indices, GL_UNSIGNED_INT,
                           0);
        }
    }

    /* functie chemata dupa ce am terminat cadrul de desenare (poate fi folosita pt modelare /
     * simulare) */
    void notifyEndFrame() {}

    /* functei care e chemata cand se schimba dimensiunea ferestrei initiale */
    void notifyReshape(int width, int height, int previos_width, int previous_height)
    {
        /* reshape */
        if(height==0) height=1;
        screen_width = width;
        screen_height = height;
        aspect = width * 1.0f / height;
        projection_matrix = glm::perspective(75.0f, aspect,0.1f,
                                             active_cam == &view1 ? MAX_DIST : 10000.0f);
    }

    //----------------------------------------------------------------------------------------------
    //metode de input output -----------------------------------------------------------------------

    /* tasta apasata */
    void notifyKeyPressed(unsigned char key_pressed, int mouse_x, int mouse_y)
    {
        pressed_keys.set(key_pressed);
        switch (key_pressed)
        {
            case 27:
                /* ESC inchide glut */
                lab::glut::close();
                break;
            case 32:
                paused = !paused;
                break;
            case 13:
                if (!paused && current_scene != GAME_STARTED)
                {
                    current_scene = GAME_STARTED;
                    start_time = last_time;
                    character.model_matrix = glm::translate(glm::mat4(),
                                                            glm::vec3(center_x, 10.0f, center_z));
                }
                break;
            case 'n':
                if (!paused && current_scene == GAME_STARTED)
                {
                    current_scene = SCENE_FADE_OUT;
                    start_time = last_time;
                }
                break;
            case '1':
                active_cam = &view1;
                glClearColor(1, 1, 1, 1);
                projection_matrix = glm::perspective(75.0f, aspect,0.1f, MAX_DIST);
                break;
            case '2':
                active_cam = &view2;
                glClearColor(0, 0, 0, 1);
                projection_matrix = glm::perspective(75.0f, aspect,0.1f, 10000.0f);
        }
    }

    /* tasta ridicata */
    void notifyKeyReleased(unsigned char key_released, int mouse_x, int mouse_y)
    {
        pressed_keys.set(key_released, 0);
    }

    /* tasta speciala (up / down / F1 / F2 / ...) apasata */
    void notifySpecialKeyPressed(int key_pressed, int mouse_x, int mouse_y)
    {
        if(key_pressed == GLUT_KEY_F1) lab::glut::enterFullscreen();
        if(key_pressed == GLUT_KEY_F2) lab::glut::exitFullscreen();

        pressed_keys.set(key_pressed);
        is_special.set(key_pressed);
    }

    /* tasta speciala ridicata */
    void notifySpecialKeyReleased(int key_released, int mouse_x, int mouse_y)
    {
        pressed_keys.set(key_released, 0);
        is_special.set(key_released, 0);
    }
    /* drag cu mouse-ul */
    void notifyMouseDrag(int mouse_x, int mouse_y) {}
    /* am miscat mouseul (fara sa apas vreun buton) */
    void notifyMouseMove(int mouse_x, int mouse_y) {}
    /* am apasat pe un buton */
    void notifyMouseClick(int button, int state, int mouse_x, int mouse_y) {}
    /* scroll cu mouse-ul */
    void notifyMouseScroll(int wheel, int direction, int mouse_x, int mouse_y) {}
};

int main(int argc, char *argv[])
{
    /* initializeaza GLUT (fereastra + input + context OpenGL) */
    lab::glut::WindowInfo window(std::string("Tema 4 EGC"), 1024, 576, 171, 60, true);
    lab::glut::ContextInfo context(3, 3, false);
    lab::glut::FramebufferInfo framebuffer(true, true, true, true);
    lab::glut::init(window, context, framebuffer);

    /* initializeaza GLEW (ne incarca functiile openGL, altfel ar trebui sa facem asta manual!) */
    glewExperimental = true;
    glewInit();
    std::cout << "GLEW:initializare" << std::endl;

    /* cream clasa noastra si o punem sa asculte evenimentele de la GLUT */
    /* DUPA GLEW!!! ca sa avem functiile de OpenGL incarcate inainte sa ii fie apelat constructorul
     * (care creeaza obiecte OpenGL) */
    int array_dim = DEFAULT_ARRAY_DIM;
    if (argc > 1)
    {
        array_dim = atoi(argv[1]);
        if (array_dim % 2 == 1 || array_dim < 6)
        {
            cerr << "Numarul de case de-a lungul unei laturi trebuie sa fie par si mai mare sau"
                    " egal cu 6.\n";
            cerr << "Valoarea data (" << array_dim <<
                    ") se ignora; se considera valoarea implicita de " << DEFAULT_ARRAY_DIM <<
                    ".\n";
            array_dim = DEFAULT_ARRAY_DIM;
        }
    }
    Tema tema(array_dim);
    lab::glut::setListener(&tema);

    /* taste */
    cout << "Taste:\n";
    cout << "\tEsc .......... iesire\n";
    cout << "\tSpace ........ pauza (inclusiv in filmul de la inceput)\n";
    cout << "\tEnter ........ sarire peste filmul de la inceput\n";
    cout << "\tn ............ restart (doar dupa terminarea filmului)\n";
    cout << "\t1 ............ view1\n";
    cout << "\t2 ............ view2\n";
    cout << "\tw / sus ...... deplasare in fata\n";
    cout << "\ts / jos ...... deplasare inapoi\n";
    cout << "\ta ............ deplasare la stanga\n";
    cout << "\td ............ deplasare la dreapta\n";
    cout << "\tr ............ deplasare in sus\n";
    cout << "\tf ............ deplasare in jos\n";
    cout << "\tq / stanga ... rotatie pe Oy la stanga\n";
    cout << "\te / dreapta .. rotatie pe Oy la dreapta\n";
    cout << "\tt ............ rotatie pe Ox in sus\n";
    cout << "\tg ............ rotatie pe Ox in jos\n";

    /* run */
    lab::glut::run();

    return 0;
}
