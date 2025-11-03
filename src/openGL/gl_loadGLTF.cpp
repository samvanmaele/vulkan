#include "gl_loadGLTF.hpp"
#include <iostream>
#include <ostream>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>

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
    if (node.mesh >= 0) bindMesh(model, node);
    for (int child : node.children)
    {
        if (child >= 0) bindNode(model, model.nodes[child]);
    }
}
void GlModel::bindMesh(tinygltf::Model& model, const tinygltf::Node& node)
{
    glm::mat4 nodeMatrix = glm::mat4(1.0);
    if (node.scale.size() == 3)
    {
        glm::vec3 S = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
        nodeMatrix = glm::scale(nodeMatrix, S);
    }
    if (node.rotation.size() == 4)
    {
        glm::quat R = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
        nodeMatrix *= glm::mat4_cast(R);
    }
    if (node.translation.size() == 3)
    {
        glm::vec3 T = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
        nodeMatrix = glm::translate(nodeMatrix, T);
    }

    const tinygltf::Mesh mesh = model.meshes[node.mesh];
    for (const auto& primitive : mesh.primitives)
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        for (const auto& attrib : primitive.attributes)
        {
            if (attrib.first == "POSITION") bindPos(model, 0, 3, attrib.second, nodeMatrix);
            else if (attrib.first == "NORMAL") bindAttrib(model, 1, 3, attrib.second);
            else if (attrib.first == "TEXCOORD_0") bindAttrib(model, 2, 2, attrib.second);
            else if (attrib.first == "JOINTS_0") bindAttrib(model, 3, 4, attrib.second);
            else if (attrib.first == "WEIGHTS_0") bindAttrib(model, 4, 4, attrib.second);
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
void GlModel::bindPos(tinygltf::Model& model, int binding, int vecSize, int attribPos, glm::mat4 nodeMatrix)
{
    const auto& accessor = model.accessors[attribPos];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];
    const float* rawPosData = reinterpret_cast<const float*>(buffer.data.data() + bufferView.byteOffset);

    std::vector<float> transformedPositions(accessor.count * 3);
    for (size_t i = 0; i < accessor.count; ++i)
    {
        glm::vec4 rawPos(rawPosData[i * 3 + 0], rawPosData[i * 3 + 1], rawPosData[i * 3 + 2], 1.0f);
        glm::vec4 finalPos = nodeMatrix * rawPos;

        transformedPositions[i * 3 + 0] = finalPos.x;
        transformedPositions[i * 3 + 1] = finalPos.y;
        transformedPositions[i * 3 + 2] = finalPos.z;
    }

    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, transformedPositions.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(binding);
    glVertexAttribPointer(binding, vecSize, accessor.componentType, GL_FALSE, vecSize*4, (void*)0);

    /*
    boundingbox box = {};
    box.min = glm::make_vec3(accessor.minValues.data());
    box.max = glm::make_vec3(accessor.maxValues.data());

    for (int i = 0; i < 3; i++)
    {
        aabb.min[i] = std::min(aabb.min[i], box.min[i]);
        aabb.max[i] = std::max(aabb.max[i], box.max[i]);
    }

    boundingboxes.push_back(box);
    */
}
void GlModel::bindAttrib(tinygltf::Model& model, int binding, int vecSize, int attribPos)
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