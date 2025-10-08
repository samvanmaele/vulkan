#include "gl_loadGLTF.hpp"
#include <iostream>
#include <ostream>

#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
std::unordered_map<int, GLuint> textureMap;

GlModel::GlModel(const char* filename)
{
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) std::cout << "Warning: " << warn << std::endl;
    if (!err.empty()) std::cerr << "Error: " << err << std::endl;
    if (!res) std::cerr << "Failed to load glTF: " << filename << std::endl;

    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    //boundingbox box = {};
    //box.min = glm::vec3(std::numeric_limits<float>::max());
    //box.max = glm::vec3(std::numeric_limits<float>::lowest());
    //aabb = box;

    for (int nodeIndex : scene.nodes)
    {
        const tinygltf::Node& node = model.nodes[nodeIndex];
        bindNode(model, node);
    }
}
void GlModel::drawModel()
{
    for (const PrimitiveData primitiveData : primitiveDataList)
    {
        glBindVertexArray(primitiveData.vao);
        if (primitiveData.textureIndex >= 0)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureMap[primitiveData.textureIndex]);
        }

        glDrawElements(primitiveData.mode, primitiveData.count, primitiveData.indexType, (void*)0);
    }
}
GlModel::~GlModel()
{
}
void GlModel::bindNode(tinygltf::Model& model, const tinygltf::Node& node)
{
    if (node.mesh >= 0) bindMesh(model, model.meshes[node.mesh]);
    for (int child : node.children)
    {
        if (child >= 0) bindNode(model, model.nodes[child]);
    }
}
void GlModel::bindMesh(tinygltf::Model& model, tinygltf::Mesh& mesh)
{
    for (const auto& primitive : mesh.primitives)
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        for (const auto& attrib : primitive.attributes)
        {
            if (attrib.first == "POSITION") bindAttrib(model, 0, 3, attrib.second, true);
            else if (attrib.first == "NORMAL") bindAttrib(model, 1, 3, attrib.second, false);
            else if (attrib.first == "TEXCOORD_0") bindAttrib(model, 2, 2, attrib.second, false);
            else if (attrib.first == "JOINTS_0") bindAttrib(model, 3, 4, attrib.second, false);
            else if (attrib.first == "WEIGHTS_0") bindAttrib(model, 4, 4, attrib.second, false);
        }
        const auto& accessor = model.accessors[primitive.indices];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];

        GLuint indexVbo;
        glGenBuffers(1, &indexVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferView.byteLength, buffer.data.data() + bufferView.byteOffset, GL_STATIC_DRAW);

        PrimitiveData primitiveData;
        primitiveData.vao = vao;
        primitiveData.count = accessor.count;
        primitiveData.indexType = accessor.componentType;
        primitiveData.mode = primitive.mode;

        int materialIndex = primitive.material;
        if (materialIndex >= 0)
        {
            const auto& material = model.materials[materialIndex];
            if (material.values.find("baseColorTexture") != material.values.end())
            {
                int texIndex = material.values.at("baseColorTexture").TextureIndex();
                primitiveData.textureIndex = texIndex;
                createTexture(model.images[texIndex], texIndex);
            }
        }

        primitiveDataList.push_back(std::move(primitiveData));
    }
}
void GlModel::bindAttrib(tinygltf::Model& model, int binding, int vecSize, int attribPos, bool collision)
{
    const auto& accessor = model.accessors[attribPos];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, buffer.data.data() + bufferView.byteOffset, GL_STATIC_DRAW);

    glEnableVertexAttribArray(binding);
    glVertexAttribPointer(binding, vecSize, accessor.componentType, GL_FALSE, vecSize*4, (void*)0);

    /*
    if (collision)
    {
        boundingbox box = {};
        box.min = glm::make_vec3(accessor.minValues.data());
        box.max = glm::make_vec3(accessor.maxValues.data());

        for (int i = 0; i < 3; i++)
        {
            aabb.min[i] = std::min(aabb.min[i], box.min[i]);
            aabb.max[i] = std::max(aabb.max[i], box.max[i]);
        }

        boundingboxes.push_back(box);
    }
    */
}
void GlModel::createTexture(const tinygltf::Image& image, int index)
{
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.image.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    textureMap[index] = tex;
}