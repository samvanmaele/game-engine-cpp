#pragma once

#include <tinygltf/tiny_gltf.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <platform.hpp>
#include <unordered_map>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <optional>

#pragma once

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#ifdef TARGET_PLATFORM_WEB
	#include <emscripten.h>
	#include <emscripten/html5.h>
	#include <GLES3/gl3.h>
#else
	#include <glew/glew.h>
#endif

#define BUFFER_OFFSET(i) ((char *)nullptr + (i))

struct MeshDraw
{
    GLuint vao;
    GLsizei count;
    GLenum indexType;
    GLenum mode;
	int textureIndex = -1;
};
struct boundingbox
{
	glm::vec3 min;
	glm::vec3 max;

	std::optional<glm::vec3> intersectRay(const glm::vec3& ray, const glm::vec3& origin);
};

bool loadModel(tinygltf::Model& model, const std::string& filename);

GLuint createTexture(const tinygltf::Model& model, int index);

class Model
{
	public:
		std::vector<MeshDraw> meshdata;
		std::vector<boundingbox> boundingboxes;
		boundingbox aabb;

		Model(const char* filename);
		void drawModel();
		void drawDepth();
		~Model();

	private:
		std::unordered_map<int, GLuint> textureMap;

		void bindNode(tinygltf::Model& model, const tinygltf::Node& node);
		void bindMesh(tinygltf::Model& model, tinygltf::Mesh& mesh);
		void bindAttrib(tinygltf::Model& model, int binding, int vecSize, int attribPos, bool collision);
		void createTexture(const tinygltf::Model& model, int index);
};