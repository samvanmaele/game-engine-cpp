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

void drawMesh(const std::map<int, GLuint>& vbos, tinygltf::Model &model, tinygltf::Mesh &mesh);

void drawModelNodes(const std::pair<GLuint, std::map<int, GLuint>>& vaoAndEbos, tinygltf::Model &model, tinygltf::Node &node);

void drawModel(const std::pair<GLuint, std::map<int, GLuint>>& vaoAndEbos, tinygltf::Model &model);

bool loadModel(tinygltf::Model &model, const char *filename);

void bindMesh(std::map<int, GLuint>& vbos, tinygltf::Model &model, tinygltf::Mesh &mesh);

void bindModelNodes(std::map<int, GLuint>& vbos, tinygltf::Model &model, tinygltf::Node &node);

std::pair<GLuint, std::map<int, GLuint>> bindModel(tinygltf::Model &model);