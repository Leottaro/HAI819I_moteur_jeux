#pragma once

#include <vector>
#include "Mesh.hpp"
#include "Transformation.hpp"

using namespace std;

class SceneNode {
private:
    Transformation m_transfo;

    // Soit l'un soit l'autre
    vector<SceneNode *> m_children;
    int m_mesh_i;

public:
    SceneNode(const vector<SceneNode *> &_children) : m_transfo(), m_children(_children), m_mesh_i(-1) {}
    SceneNode(const vector<SceneNode *> &_children, Transformation _transfo) : m_transfo(_transfo), m_children(_children), m_mesh_i(-1) {}

    SceneNode(int _mesh_i) : m_transfo(), m_children({}), m_mesh_i(_mesh_i) {}
    SceneNode(int _mesh_i, Transformation _transfo) : m_transfo(_transfo), m_children({}), m_mesh_i(_mesh_i) {}

    void render(GLuint programID, const vector<Mesh *> &_meshes, const glm::mat4 &_transfo = glm::mat4()) const {
        glm::mat4 render_transfo = _transfo * m_transfo.computeTransformationMatrix();
        if (m_mesh_i >= 0) {
            _meshes[m_mesh_i]->render(programID, render_transfo);
        }
        for (const SceneNode *child : m_children) {
            child->render(programID, _meshes, render_transfo);
        }
    }

    // TRANSFORMATION API

    // GETTERS
    inline const glm::vec3 getTranslation() const { return m_transfo.getTranslation(); }
    inline const glm::vec3 getEulerAngles() const { return m_transfo.getEulerAngles(); }
    inline const glm::quat getRotation() const { return m_transfo.getRotation(); }
    inline glm::vec3 getScale() const { return m_transfo.getScale(); }
    inline glm::vec3 getFrontVector() const { return m_transfo.getFrontVector(); }

    // SETTERS
    inline void setTranslation(const glm::vec3 &t) { m_transfo.setTranslation(t); }
    inline void setEulerAngles(const glm::vec3 &r) { m_transfo.setEulerAngles(r); }
    inline void addEulerAngles(const glm::vec3 &r) { m_transfo.addEulerAngles(r); }
    inline void setEulerAnglesFromFront(const glm::vec3 &_front) { m_transfo.setEulerAnglesFromFront(_front); }
    inline void setRotation(const glm::quat &q) { m_transfo.setRotation(q); }
    inline void setScale(glm::vec3 s) { m_transfo.setScale(s); }
    inline void setScale(float s) { m_transfo.setScale(s); }
    inline void setScaleX(float sx) { m_transfo.setScaleX(sx); }
    inline void setScaleY(float sy) { m_transfo.setScaleY(sy); }
    inline void setScaleZ(float sz) { m_transfo.setScaleZ(sz); }
    inline void setScaleXY(float s) { m_transfo.setScaleXY(s); }
    inline void setScaleXZ(float s) { m_transfo.setScaleXZ(s); }
    inline void setScaleYZ(float s) { m_transfo.setScaleYZ(s); }

    // UPDATES
    inline void updateRotation() { m_transfo.updateRotation(); }
    inline void updateEulerAngles() { m_transfo.updateEulerAngles(); }
};

class Scene {
private:
    vector<Mesh *> m_meshes;
    SceneNode m_root;

public:
    Scene(vector<Mesh *> _meshes, SceneNode _root) : m_meshes(_meshes), m_root(_root) {}
    ~Scene() {
        clear();
    }

    void render(GLuint programID) const {
        m_root.render(programID, m_meshes);
    }

    void clear() {
        for (Mesh *mesh : m_meshes) {
            mesh->clear();
        }
    }
};
