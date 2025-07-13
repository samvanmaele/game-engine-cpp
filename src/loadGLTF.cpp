#include <loadGLTF.hpp>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <tinygltf/tiny_gltf.h>

#ifdef TARGET_PLATFORM_WEB
    #include <emscripten.h>
    #include <emscripten/html5.h>
    #include <GLES3/gl3.h>
#else
    #include <glew/glew.h>
#endif

#include <unordered_map>
#include <vector>
#include <iostream>

#define BUFFER_OFFSET(i) ((char *)nullptr + (i))
std::unordered_map<int, GLuint> textureMap;

std::optional<glm::vec3> boundingbox::intersectRay(const glm::vec3& ray, const glm::vec3& origin)
{
    glm::vec3 dirFrac;
    for (int i = 0; i < 3; ++i)
    {
        dirFrac[i] = (ray[i] != 0.0f) ? 1.0f / ray[i] : std::numeric_limits<float>::infinity();
    }

    float t1 = (min.x - origin.x) * dirFrac.x;
    float t2 = (max.x - origin.x) * dirFrac.x;
    float t3 = (min.y - origin.y) * dirFrac.y;
    float t4 = (max.y - origin.y) * dirFrac.y;
    float t5 = (min.z - origin.z) * dirFrac.z;
    float t6 = (max.z - origin.z) * dirFrac.z;

    float tmin = std::max({std::min(t1, t2), std::min(t3, t4), std::min(t5, t6)});
    float tmax = std::min({std::max(t1, t2), std::max(t3, t4), std::max(t5, t6)});

    if (tmax < 0 || tmin > tmax) {return std::nullopt;}
    float t = (tmin >= 0) ? tmin : tmax;
    return ray * t;
}

bool loadModel(tinygltf::Model& model, const std::string& filename)
{
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    bool res = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    if (!warn.empty()) {std::cout << "Warning: " << warn << std::endl;}
    if (!err.empty()) {std::cerr << "Error: " << err << std::endl;}
    if (!res) {std::cerr << "Failed to load glTF: " << filename << std::endl;}

    return res;
}

Model::Model(const char* filename)
{
    tinygltf::Model model;
    loadModel(model, filename);
    const tinygltf::Scene& scene = model.scenes[model.defaultScene];

    boundingbox box = {};
    box.min = glm::vec3(std::numeric_limits<float>::max());
    box.max = glm::vec3(std::numeric_limits<float>::lowest());
    aabb = box;

    for (int nodeIndex : scene.nodes)
    {
        const tinygltf::Node& node = model.nodes[nodeIndex];
        bindNode(model, node);
    }
}
void Model::drawModel()
{
    for (const auto& mesh : meshdata)
    {
        glBindVertexArray(mesh.vao);
        if (mesh.textureIndex >= 0)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureMap[mesh.textureIndex]);
        }

        glDrawElements(mesh.mode, mesh.count, mesh.indexType, (void*)0);
    }
}
Model::~Model()
{
}
void Model::bindNode(tinygltf::Model& model, const tinygltf::Node& node)
{
    if (node.mesh >= 0) {bindMesh(model, model.meshes[node.mesh]);}
    for (int child : node.children)
    {
        if (child >= 0) {bindNode(model, model.nodes[child]);}
    }
}
void Model::bindMesh(tinygltf::Model& model, tinygltf::Mesh& mesh)
{
    for (const auto& primitive : mesh.primitives)
    {
        GLuint vao;
        glGenVertexArrays(1, &vao);
        glBindVertexArray(vao);

        for (const auto& attrib : primitive.attributes)
        {
            if (attrib.first == "POSITION") {bindAttrib(model, 0, 3, attrib.second, true);}
            else if (attrib.first == "NORMAL") {bindAttrib(model, 1, 3, attrib.second, false);}
            else if (attrib.first == "TEXCOORD_0") {bindAttrib(model, 2, 2, attrib.second, false);}
            else if (attrib.first == "JOINTS_0") {bindAttrib(model, 3, 4, attrib.second, false);}
            else if (attrib.first == "WEIGHTS_0") {bindAttrib(model, 4, 4, attrib.second, false);}
        }
        const auto& accessor = model.accessors[primitive.indices];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];

        GLuint indexVbo;
        glGenBuffers(1, &indexVbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexVbo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, bufferView.byteLength, buffer.data.data() + bufferView.byteOffset, GL_STATIC_DRAW);

        MeshDraw draw;
        draw.vao = vao;
        draw.count = accessor.count;
        draw.indexType = accessor.componentType;
        draw.mode = primitive.mode;

        int materialIndex = primitive.material;
        if (materialIndex >= 0)
        {
            const auto& material = model.materials[materialIndex];
            if (material.values.find("baseColorTexture") != material.values.end())
            {
                int texIndex = material.values.at("baseColorTexture").TextureIndex();
                draw.textureIndex = texIndex;
                createTexture(model, texIndex);
            }
        }

        meshdata.push_back(std::move(draw));
    }
}
void Model::bindAttrib(tinygltf::Model& model, int binding, int vecSize, int attribPos, bool collision)
{
    GLuint vbo;

    const auto& accessor = model.accessors[attribPos];
    const auto& bufferView = model.bufferViews[accessor.bufferView];
    const auto& buffer = model.buffers[bufferView.buffer];

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferView.byteLength, buffer.data.data() + bufferView.byteOffset, GL_STATIC_DRAW);

    glEnableVertexAttribArray(binding);
    glVertexAttribPointer(binding, vecSize, accessor.componentType, GL_FALSE, vecSize*4, (void*)0);

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
}
void Model::createTexture(const tinygltf::Model& model, int index)
{
    const tinygltf::Image& image = model.images[index];

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.image.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    textureMap[index] = tex;
}