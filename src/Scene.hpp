#pragma once

#include <vector>
#include "Mesh.hpp"
#include "Transformation.hpp"
#include "ShaderProgram.hpp"

using namespace std;

class SceneNode {
public:
    Transformation m_transfo;

    // Soit l'un soit l'autre
    vector<SceneNode *> m_children;

    // TODO: une liste de niveau de détails ?
    int m_mesh_i;
    int m_texture_i;

    SceneNode(const vector<SceneNode *> &_children) : m_transfo(), m_children(_children), m_mesh_i(-1), m_texture_i(-1) {}
    SceneNode(const vector<SceneNode *> &_children, Transformation _transfo) : m_transfo(_transfo), m_children(_children), m_mesh_i(-1), m_texture_i(-1) {}

    SceneNode(int _mesh_i) : m_transfo(), m_children({}), m_mesh_i(_mesh_i), m_texture_i(-1) {}
    SceneNode(int _mesh_i, int _texture_i) : m_transfo(), m_children({}), m_mesh_i(_mesh_i), m_texture_i(_texture_i) {}
    SceneNode(int _mesh_i, int _texture_i, Transformation _transfo) : m_transfo(_transfo), m_children({}), m_mesh_i(_mesh_i), m_texture_i(_texture_i) {}

    void render(ShaderProgram &_shader, const vector<Mesh *> &_meshes, bool _render_octree, const glm::mat4 &_transfo = glm::mat4(1.f)) const {
        glm::mat4 render_transfo = _transfo * m_transfo.computeTransformationMatrix();
        if (m_mesh_i >= 0) {
            _shader.set("texture_i", m_texture_i);
            _shader.set("texture_sampler", m_texture_i);

            _shader.set("model", render_transfo);
            _shader.set("has_normals", 1);
            _meshes[m_mesh_i]->render();
            if (_render_octree) {
                _shader.set("has_normals", 0);
                _meshes[m_mesh_i]->renderOctree();
            }
        } else {
            for (const SceneNode *child : m_children) {
                child->render(_shader, _meshes, _render_octree, render_transfo);
            }
        }
    }
};

class Scene {
private:
    vector<Mesh *> m_meshes;
    vector<ImageBase *> m_textures;
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

    void render(ShaderProgram &_shader, bool _render_octree = false) const {
        m_root->render(_shader, m_meshes, _render_octree);
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
