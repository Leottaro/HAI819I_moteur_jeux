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
    int m_texture_i;

public:
    SceneNode(const vector<SceneNode *> &_children) : m_transfo(), m_children(_children), m_mesh_i(-1), m_texture_i(-1) {}
    SceneNode(const vector<SceneNode *> &_children, Transformation _transfo) : m_transfo(_transfo), m_children(_children), m_mesh_i(-1), m_texture_i(-1) {}

    SceneNode(int _mesh_i, int _texture_i) : m_transfo(), m_children({}), m_mesh_i(_mesh_i), m_texture_i(_texture_i) {}
    SceneNode(int _mesh_i, int _texture_i, Transformation _transfo) : m_transfo(_transfo), m_children({}), m_mesh_i(_mesh_i), m_texture_i(_texture_i) {}

    void render(GLuint programID, const vector<Mesh *> &_meshes, const glm::mat4 &_transfo = glm::mat4()) const {
        glm::mat4 render_transfo = _transfo * m_transfo.computeTransformationMatrix();
        if (m_mesh_i >= 0) {
            glUniform1i(glGetUniformLocation(programID, "texture_sampler"), m_texture_i);
            glUniformMatrix4fv(glGetUniformLocation(programID, "model"), 1, false, glm::value_ptr(render_transfo));
            _meshes[m_mesh_i]->render();
        } else {
            for (const SceneNode *child : m_children) {
                child->render(programID, _meshes, render_transfo);
            }
        }
    }

    // TRANSFORMATION API

    // GETTERS
    inline const glm::vec3 getTranslation() const { return m_transfo.getTranslation(); }
    inline const glm::vec3 getEulerAngles() const { return m_transfo.getEulerAngles(); }
    inline glm::vec3 getScale() const { return m_transfo.getScale(); }
    inline glm::vec3 getFrontVector() const { return m_transfo.getFrontVector(); }

    // SETTERS
    inline void setTranslation(const glm::vec3 &t) { m_transfo.setTranslation(t); }
    inline void setTranslationX(float tx) { m_transfo.setTranslationX(tx); }
    inline void setTranslationY(float ty) { m_transfo.setTranslationY(ty); }
    inline void setTranslationZ(float tz) { m_transfo.setTranslationZ(tz); }
    inline void setEulerAngles(const glm::vec3 &r) { m_transfo.setEulerAngles(r); }
    inline void setEulerAnglesFromFront(const glm::vec3 &_front) { m_transfo.setEulerAnglesFromFront(_front); }
    inline void setPitch(float p) { m_transfo.setPitch(p); }
    inline void setYaw(float y) { m_transfo.setYaw(y); }
    inline void setRoll(float r) { m_transfo.setRoll(r); }
    inline void addEulerAngles(const glm::vec3 &r) { m_transfo.addEulerAngles(r); }
    inline void addPitch(float p) { m_transfo.addPitch(p); }
    inline void addYaw(float y) { m_transfo.addYaw(y); }
    inline void addRoll(float r) { m_transfo.addRoll(r); }
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
};

class Scene {
private:
    vector<ImageBase *> m_textures;
    vector<Mesh *> m_meshes;
    SceneNode *m_root;

public:
    Scene(vector<Mesh *> _meshes, vector<ImageBase *> _textures, SceneNode *_root) : m_meshes(_meshes), m_textures(_textures), m_root(_root) {}
    ~Scene() {
        clear();
    }

    void initShaderData() {
        for (Mesh *mesh : m_meshes) {
            mesh->initShaderData();
        }

        for (uint i = 0; i < m_textures.size(); i++) {
            m_textures[i]->initShaderData(i);
        }
    }

    void render(GLuint programID) const {
        m_root->render(programID, m_meshes);
    }

    void clear() {
        for (Mesh *mesh : m_meshes) {
            mesh->clear();
        }

        for (ImageBase *texture : m_textures) {
            texture->clearShaderData();
        }
    }
};
