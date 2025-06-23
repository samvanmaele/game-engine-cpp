#pragma once

#include <tinygltf/tiny_gltf.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <platform.hpp>

#ifdef TARGET_PLATFORM_WEB
	#include <emscripten.h>
	#include <emscripten/html5.h>
	#include <GLES3/gl3.h>
#else
	#include <glew/glew.h>
#endif

struct VaoData
{
    GLuint vao;
    std::map<int, GLuint> ebos;
};

void drawMesh(const std::map<int, GLuint>& vbos, tinygltf::Model &model, tinygltf::Mesh &mesh);

void drawModelNodes(const VaoData& vaoAndEbos, tinygltf::Model &model, tinygltf::Node &node);

void drawModel(const VaoData& vaoAndEbos, tinygltf::Model &model);

bool loadModel(tinygltf::Model &model, const char *filename);

void bindMesh(std::map<int, GLuint>& vbos, tinygltf::Model &model, tinygltf::Mesh &mesh);

void bindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model &model, tinygltf::Node &node);

VaoData bindModel(tinygltf::Model &model);