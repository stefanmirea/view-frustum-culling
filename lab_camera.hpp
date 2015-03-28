//-------------------------------------------------------------------------------------------------
// Descriere: header in care este definita si implementata camera
// Nota:
//        camera este capabila de operatie de rotatie si translatie pe toate axele
//        camera este capabila de rotatii FP(first person) cat si TP(third person)
//
// Autor: Lucian Petrescu
// Data: 14 oct 2013
//-------------------------------------------------------------------------------------------------


#pragma once
#include "dependente\glm\glm.hpp"
#include "dependente\glm\gtc\type_ptr.hpp"
#include "dependente\glm\gtc\matrix_transform.hpp"

namespace lab{

    struct CamVertexFormat{ 
        glm::vec3 position, color; 
        CamVertexFormat(const glm::vec3 &p, const glm::vec3 &c){ 
            position=p; color=c;
        } 
    };

    class Camera{
    public:
        Camera() : frustum(*this) {
            //initializeaza camera
            position = glm::vec3(0,0,50);
            forward = glm::vec3(0,0,-1);
            up = glm::vec3(0,1,0);
            right = glm::vec3(1,0,0);

            //creaza geometry si update
            createGeometry();
            updateGeometry();
        }
        Camera(const glm::vec3 &position, const glm::vec3 &center, const glm::vec3 &up) : frustum(*this) {
            //set camera
            set(position, center,up);

            //update geometrie si update
            createGeometry();
            updateGeometry();
        }
        ~Camera(){
            //distruge geometrie
            destroyGeometry();
        }

        void set(const glm::vec3 &position, const glm::vec3 &center, const glm::vec3 &up){
            //update camera
            this->position = position;
            forward = glm::normalize(center-position);
            right = glm::cross(forward, up);
            this->up=glm::cross(right,forward);

            //update geometrie
            updateGeometry();
        }

        glm::vec3 getPosition()
        {
            return this->position;
        }

        glm::mat4 getViewMatrix(){
            return glm::lookAt(position,position + glm::normalize(forward), up);
        }

        /* actualizarea planelor frustumului */
        void updateFrustum(const glm::mat4 projection_matrix)
        {
            frustum.update(projection_matrix);
        }

        bool isVisible(const std::vector<glm::vec3> &vertices) const
        {
            return frustum.isVisible(vertices);
        }

    private:
        //creeaza geometrie pentru axele camerei
        void createGeometry(){
            glGenVertexArrays(1,&vao);
            glBindVertexArray(vao);
            glGenBuffers(1,&vbo);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glGenBuffers(1,&ibo);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
            std::vector<unsigned int> indices; for(unsigned int i=0;i<6;i++) indices.push_back(i);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6*sizeof(unsigned int),&indices[0], GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(CamVertexFormat),(void*)0);
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(CamVertexFormat),(void*)(sizeof(float)*3));    
        }
        //distruge geometria axelor camerei
        void destroyGeometry(){
            glDeleteVertexArrays(1,&vao);
            glDeleteBuffers(1,&vbo);
            glDeleteBuffers(1,&ibo);
        }
        //updateaza geometria axelor camerei
        void updateGeometry(){
            glBindVertexArray(vao);
            float axis_lenght=8;
            std::vector<CamVertexFormat> vertices;
            vertices.push_back( CamVertexFormat( position, glm::vec3(0,0,1)));
            vertices.push_back( CamVertexFormat( position + glm::normalize(forward)*axis_lenght, glm::vec3(0,0,1)));
            vertices.push_back( CamVertexFormat( position, glm::vec3(1,0,0)));
            vertices.push_back( CamVertexFormat( position + glm::normalize(right)*axis_lenght, glm::vec3(1,0,0)));
            vertices.push_back( CamVertexFormat( position, glm::vec3(0,1,0)));
            vertices.push_back( CamVertexFormat( position + glm::normalize(up)*axis_lenght, glm::vec3(0,1,0)));
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, 6*sizeof(CamVertexFormat),&vertices[0],GL_STATIC_DRAW);
        }
    public:
        //deseneaza geometria axelor camerei
        void drawGeometry(){
            glBindVertexArray(vao);
            glDrawElements(GL_LINES, 6,GL_UNSIGNED_INT, (void*)0);
        }

    private:
        //camera
        glm::vec3 position;
        glm::vec3 up;
        glm::vec3 forward;
        glm::vec3 right;

        //geometrie camera
        unsigned int vao,vbo,ibo;

        /* coeficientii din ecuatia implicita a unui plan: a * x + b * y + c * z + d = 0, unde
         * (x, y, z) e un punct in spatiul utilizator */
        struct Plane {
            float a, b, c, d;
        };

        class Frustum {
        private:
            std::vector<Plane> planes;
            Camera &parent;
            glm::mat4 last_projection_matrix, last_view_matrix;
        public:
            Frustum(Camera &parent) : parent(parent), planes(std::vector<Plane>(6)) {}
            /* actualizeaza ecuatiile planelor */
            void update(const glm::mat4 &projection_matrix)
            {
                glm::mat4 view_matrix = parent.getViewMatrix();
                /* daca nu au avut loc modificari ale ferestrei sau ale camerei, nu e nimic de
                 * actualizat */
                if (projection_matrix == last_projection_matrix && view_matrix == last_view_matrix)
                    return;
                if (projection_matrix != last_projection_matrix)
                    last_projection_matrix = projection_matrix;
                if (view_matrix != last_view_matrix)
                    last_view_matrix = view_matrix;
                glm::mat4 proj_view = projection_matrix * view_matrix;
                /* stanga */
                planes[0].a = -proj_view[0][3] - proj_view[0][0];
                planes[0].b = -proj_view[1][3] - proj_view[1][0];
                planes[0].c = -proj_view[2][3] - proj_view[2][0];
                planes[0].d = -proj_view[3][3] - proj_view[3][0];
                /* dreapta */
                planes[1].a = -proj_view[0][3] + proj_view[0][0];
                planes[1].b = -proj_view[1][3] + proj_view[1][0];
                planes[1].c = -proj_view[2][3] + proj_view[2][0];
                planes[1].d = -proj_view[3][3] + proj_view[3][0];
                /* jos */
                planes[2].a = -proj_view[0][3] - proj_view[0][1];
                planes[2].b = -proj_view[1][3] - proj_view[1][1];
                planes[2].c = -proj_view[2][3] - proj_view[2][1];
                planes[2].d = -proj_view[3][3] - proj_view[3][1];
                /* sus */
                planes[3].a = -proj_view[0][3] + proj_view[0][1];
                planes[3].b = -proj_view[1][3] + proj_view[1][1];
                planes[3].c = -proj_view[2][3] + proj_view[2][1];
                planes[3].d = -proj_view[3][3] + proj_view[3][1];
                /* aproape */
                planes[4].a = -proj_view[0][3] - proj_view[0][2];
                planes[4].b = -proj_view[1][3] - proj_view[1][2];
                planes[4].c = -proj_view[2][3] - proj_view[2][2];
                planes[4].d = -proj_view[3][3] - proj_view[3][2];
                /* departe */
                planes[5].a = -proj_view[0][3] + proj_view[0][2];
                planes[5].b = -proj_view[1][3] + proj_view[1][2];
                planes[5].c = -proj_view[2][3] + proj_view[2][2];
                planes[5].d = -proj_view[3][3] + proj_view[3][2];
            }
            /* verifica daca un obiect (dat prin varfuri in spatiul utilizator) trebuie desenat */
            bool isVisible(const std::vector<glm::vec3> &vertices) const
            {
                for (int i = 0; i < 6; ++i)
                {
                    bool all_points_outside = true;
                    for (int j = 0; j < (int)vertices.size(); ++j)
                        if (planes[i].a * vertices[j][0] + planes[i].b * vertices[j][1] +
                            planes[i].c * vertices[j][2] + planes[i].d < 0)
                        {
                            all_points_outside = false;
                            break;
                        }
                    if (all_points_outside)
                        return false;
                }
                return true;
            }
        } frustum;
    };
}
