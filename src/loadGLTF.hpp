#pragma once

#include <tinygltf/tiny_gltf.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <platform.hpp>
#include <unordered_map>
#include <vector>
#include <string>

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

struct MeshDraw {
    GLuint vao;
    GLsizei count;
    GLenum indexType;
    GLenum mode;
	int textureIndex = -1;
};

bool loadModel(tinygltf::Model& model, const std::string& filename);

GLuint createTexture(const tinygltf::Model& model, int index);

class Model {
	public:
        std::vector<MeshDraw> meshdata;

		Model(const char* filename);
		void drawModel();
		~Model();

	private:
		std::unordered_map<int, GLuint> textureMap;

		void bindNode(tinygltf::Model& model, const tinygltf::Node& node);
		void bindMesh(tinygltf::Model& model, tinygltf::Mesh& mesh);
		void bindAttrib(tinygltf::Model& model, int binding, int vecSize, int attribPos);
		void createTexture(const tinygltf::Model& model, int index);
};