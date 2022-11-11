#pragma once

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "shader.h"
#include "Mesh.h"

#include <vector>

struct Texture {
	unsigned int id;
	const char* path;
};

class Scene {
public:
	Scene(const char* path);

	void draw(Shader &shader, int instances);
	void drawFoveated(
		Shader& renderingShader,
		Shader& blendingShader,
		unsigned int* framebufferIDs,
		unsigned int* framebufferTextureIDs,
		int* resolutions,
		int* sizes,
		int numLayers,
		unsigned int quadVAO,
		int instances);

	void drawFoveatedMultisample(
		Shader& renderingShader,
		Shader& blendingShader,
		unsigned int* multisampleFBs,
		unsigned int* intermediateFBs,
		unsigned int* intermediateFBtextures,
		int* resolutions,
		int* sizes,
		int numLayers,
		unsigned int quadVAO,
		int instances);

	//loads the texture from the path and returns the id of the openGL texture object created for it, will check though the
	//loaded textures beforehand to avoid reloading the same texture multiple times
	static unsigned int loadTexture(const char* path, std::string directory);
private:
	std::vector<Mesh> meshes;
	Mesh processMesh(aiMesh* mesh, const aiScene* scene);

	static std::vector<Texture> loadedTextures;
	std::string directory;
};