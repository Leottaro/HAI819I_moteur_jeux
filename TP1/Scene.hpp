#pragma once

#include <vector>
#include "Mesh.hpp"

using namespace std;

class SceneNode {
private:
    Transformation m_transfo;
    
    // Soit l'un soit l'autre
    vector<SceneNode*> m_children;
    int m_mesh_i;

public:
    SceneNode(const vector<SceneNode*> &_children) : m_transfo(), m_children(_children), m_mesh_i(-1) {}
    SceneNode(const vector<SceneNode*> &_children, Transformation _transfo) : m_transfo(_transfo), m_children(_children), m_mesh_i(-1) {}
    
    SceneNode(int _mesh_i) : m_transfo(), m_children({}), m_mesh_i(_mesh_i) {}
    SceneNode(int _mesh_i, Transformation _transfo) : m_transfo(_transfo), m_children({}), m_mesh_i(_mesh_i) {}

    void render(GLuint programID, const vector<Mesh*> & _meshes, const glm::mat4 &_transfo = glm::mat4()) const {
        glm::mat4 render_transfo = m_transfo.computeTransformationMatrix() * _transfo; // TODO: inverser si problème
        if (m_mesh_i >= 0) {
            _meshes[m_mesh_i]->render(programID, render_transfo);
        } 
        for (const SceneNode* child: m_children) {
            child->render(programID, _meshes, render_transfo);
        }
    }

    // TRANSFORMATION API

    inline const glm::vec3 getTranslation() const { return m_transfo.getTranslation(); }
    inline void setTranslation(const glm::vec3 &t) { m_translation = t; }
    inline const glm::vec3 getRotation() const { return m_transfo.getRotation(); }
    inline void setRotation(const glm::vec3 &r) { m_rotation = r; }
    inline glm::vec3 getScale() const { return m_transfo.getScale(); }
    inline void setScale(glm::vec3 s) { m_scale = s; }
    inline void setScale(float s) { m_scale = glm::vec3(s); }
    inline void setScaleX(float sx) { m_scale.x = sx; }
    inline void setScaleY(float sy) { m_scale.y = sy; }
    inline void setScaleZ(float sz) { m_scale.z = sz; }
    inline void setScaleXY(float s) { m_scale.x = m_scale.y = s; }
    inline void setScaleXZ(float s) { m_scale.x = m_scale.z = s; }
    inline void setScaleYZ(float s) { m_scale.y = m_scale.z = s; }
};

class Scene {
private:
    vector<Mesh*> m_meshes;
    SceneNode m_root;

public:
    Scene(vector<Mesh*> _meshes, SceneNode _root) : m_meshes(_meshes), m_root(_root) {}
    ~Scene() {
        clear();
    }

    void render(GLuint programID) const {
        m_root.render(programID, m_meshes);
    }

    void clear() {
        for (Mesh* mesh: m_meshes) {
            mesh->clear();
        }
    }
};
