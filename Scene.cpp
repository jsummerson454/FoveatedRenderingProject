#include <glad/glad.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "stb_image.h"

#include "Scene.h"
#include "Mesh.h"
#include <iostream>

extern int WIDTH, HEIGHT;

Scene::Scene(const char* path) {
	//directory needed for texture loading, assumes texture image files are stored in the same directory as the obj (as well at mtl files)
	std::string pathString(path);
	directory = pathString.substr(0, pathString.find_last_of('\\'));

	//load .obj model file into Assimp's scene object, from which we then extract the necessary data we need
	Assimp::Importer importer;
	const aiScene* aScene = importer.ReadFile(path,
		//post processing options
		aiProcess_Triangulate | // transform all model primitives into traingles if they aren't already
		aiProcess_FlipUVs | // flip texture coordinates on y-axis (openGL is funny)
		aiProcess_GenNormals | // creates normal vectors for each vertex if the model does not already have them
		aiProcess_OptimizeMeshes); // attempts to join multiple meshes into larger meshes to reduce number of drawing calls
	if (!aScene || aScene->mFlags && AI_SCENE_FLAGS_INCOMPLETE || !aScene->mRootNode) {
		std::cout << "Error loading scene: " << importer.GetErrorString() << std::endl;
		return;
	}

	//Usually model loader would retain the parent-child relationship between meshes, but since we are rendering the objects statically
	//this is not required, so we can just iterate over the scene's meshes and load them directly
	int numVertices = 0;
	int numVerticesAlt = 0;
	for (int i = 0; i < aScene->mNumMeshes; i++) {
		aiMesh* aMesh = aScene->mMeshes[i];
		meshes.push_back(processMesh(aMesh, aScene));
		numVertices += meshes.back().getNumVertices();
		numVerticesAlt += aMesh->mNumVertices;
	}

	std::cout << "Vertices: " << numVertices << std::endl;
	std::cout << "Vertices: " << numVertices << std::endl;

}

void Scene::draw(Shader& shader, int instances) {
	shader.use();
	for (int i = 0; i < meshes.size(); i++) {
		meshes[i].draw(shader, instances);
	}
}

void Scene::drawFoveated(
	Shader& renderingShader,
	Shader& blendingShader,
	unsigned int* framebufferIDs,
	unsigned int* framebufferTextureIDs,
	int* resolutions,
	int* sizes,
	int numLayers,
	unsigned int quadVAO,
	int instances)
{
	//render various size and resolution images to the framebuffers
	for (int i = 0; i < numLayers; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, framebufferIDs[i]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(
			-((WIDTH - sizes[2 * i]) * resolutions[2 * i] / (2 * sizes[2 * i])),
			-((HEIGHT - sizes[2 * i + 1]) * resolutions[2 * i + 1] / (2 * sizes[2 * i + 1])),
			(WIDTH * resolutions[2 * i]) / sizes[2 * i],
			(HEIGHT * resolutions[2 * i + 1]) / sizes[2 * i + 1]
		);
		this->draw(renderingShader, instances);
	}
	
	//now render to default (window's) framebuffer by rebinding and using the blending shader that uses the newly drawn texture
	blendingShader.use();
	glViewport(0, 0, WIDTH, HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindVertexArray(quadVAO);
	for (int i = 0; i < numLayers; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, framebufferTextureIDs[i]);
	}
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

//more or less the same as drawFoveated, but blits textures from multisample eccentricity framebuffers into intermediate framebuffers
// to read from
void Scene::drawFoveatedMultisample(
	Shader& renderingShader,
	Shader& blendingShader,
	unsigned int* multisampleFBs,
	unsigned int* intermediateFBs,
	unsigned int* intermediateFBtextures,
	int* resolutions,
	int* sizes,
	int numLayers,
	unsigned int quadVAO,
	int instances)
{
	//render various size and resolution images to the framebuffers
	for (int i = 0; i < numLayers; i++) {
		glBindFramebuffer(GL_FRAMEBUFFER, multisampleFBs[i]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(
			-((WIDTH - sizes[2 * i]) * resolutions[2 * i] / (2 * sizes[2 * i])),
			-((HEIGHT - sizes[2 * i + 1]) * resolutions[2 * i + 1] / (2 * sizes[2 * i + 1])),
			(WIDTH * resolutions[2 * i]) / sizes[2 * i],
			(HEIGHT * resolutions[2 * i + 1]) / sizes[2 * i + 1]
		);
		this->draw(renderingShader, instances);
	}

	//blit multisample FB textures to intermediate (non-multisample) FBO which are then fed into blending shader
	for (int i = 0; i < numLayers; i++) {
		glBindFramebuffer(GL_READ_FRAMEBUFFER, multisampleFBs[i]);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, intermediateFBs[i]);
		glBlitFramebuffer(0, 0, resolutions[2 * i], resolutions[2 * i + 1], 0, 0, resolutions[2 * i], resolutions[2 * i + 1], GL_COLOR_BUFFER_BIT, GL_NEAREST);
	}

	//now render to default (window's) framebuffer by rebinding and using the blending shader that uses the newly drawn texture
	blendingShader.use();
	glViewport(0, 0, WIDTH, HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindVertexArray(quadVAO);
	for (int i = 0; i < numLayers; i++) {
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, intermediateFBtextures[i]);
	}
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

Mesh Scene::processMesh(aiMesh* mesh, const aiScene* scene) {
	//need to extract from the assimp mesh everything we need for our Mesh object
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	Material mat;

	bool texCoordsDefined = false;
	
	//Vertices
	for (int i = 0; i < mesh->mNumVertices; i++) {
		Vertex v;

		//position
		aiVector3D assimpVec = mesh->mVertices[i];
		v.position = glm::vec3(assimpVec.x, assimpVec.y, assimpVec.z);

		//normal
		assimpVec = mesh->mNormals[i];
		v.normal = glm::vec3(assimpVec.x, assimpVec.y, assimpVec.z);

		//texCoords
		if (mesh->mTextureCoords[0]) //Only care about fist set of texture coordinates (if they even exist)
		{
			assimpVec = mesh->mTextureCoords[0][i];
			v.texCoords = glm::vec2(assimpVec.x, assimpVec.y);
			texCoordsDefined = true;
		}

		vertices.push_back(v);			
	}

	//Indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++) {
			indices.push_back(face.mIndices[j]);
		}
	}

	//Material properties (diffuse and specular maps, and plain colour if diffuse is missing)
	aiMaterial* aMat = scene->mMaterials[mesh->mMaterialIndex];

	//Only care about textures if the material defines a texture, and the vertices actually contain TexCoords
	if (texCoordsDefined && aMat->GetTextureCount(aiTextureType_DIFFUSE) > 0) {
		aiString path;
		aMat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
		mat.diffuseMapID = loadTexture(path.C_Str(), directory);
	}
	else {
		mat.diffuseEnabled = false;
	}

	if (texCoordsDefined && aMat->GetTextureCount(aiTextureType_SPECULAR) > 0) {
		aiString path;
		aMat->GetTexture(aiTextureType_SPECULAR, 0, &path);
		mat.specularMapID = loadTexture(path.C_Str(), directory);
	}
	else {
		mat.specularEnabled = false;
	}


	float shininess;
	if (AI_SUCCESS != aiGetMaterialFloat(aMat, AI_MATKEY_SHININESS, &shininess) || shininess == 0) {
		// if unsuccessful set a default
		shininess = 32.0f;
	}
	mat.shininess = shininess;
	

	//Colour is just used as a backup for if the model has no diffuse map
	aiColor4D aCol;
	if (AI_SUCCESS != aiGetMaterialColor(aMat, AI_MATKEY_COLOR_DIFFUSE, &aCol)) {
		//if no diffuse color then set to white
		aCol.r = 1.0f;
		aCol.g = 1.0f;
		aCol.b = 1.0f;
	}
	mat.colour = glm::vec3(aCol.r, aCol.g, aCol.b);

	//Forcibly ignore material properties and define manually, used for rendering specific scenes
	//mat.shininess = 32.0f;
	//mat.colour = glm::vec3(0.75f, 0.1f, 0.75f);

	return Mesh(vertices, indices, mat);
}

unsigned int Scene::loadTexture(const char* path, std::string directory) {
	//First check texture hasn't already been loaded - if so just return the openGL texture ID
	for (int i = 0; i < loadedTextures.size(); i++) {
		if (std::strcmp(loadedTextures[i].path, path) == 0) {
			return loadedTextures[i].id;
		}
	}

	//otherwise texture is being loaded for the first time
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrChannels;
	std::string fullPath = directory + "\\" + path;
	unsigned char* data = stbi_load(fullPath.c_str(), &width, &height, &nrChannels, 0);
	if (!data) {
		std::cout << "Failed to load texture: " << path << std::endl;
	}
	else {
		GLenum format;
		if (nrChannels == 3) {
			format = GL_RGB;
		}
		else if (nrChannels == 4) {
			format = GL_RGBA;
		}
		else {
			std::cout << "Unsupported number of channels (" << nrChannels << ") in texture: " << path << std::endl;
			return textureID;
		}

		//bind texture
		glBindTexture(GL_TEXTURE_2D, textureID);
		//set texture wrapping and filtering options
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		//TODO: should this be GL_LINEAR_MIPMAP_LINEAR? - check docs
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		//generate texture using glTexImage2D
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		//and automatically generate the mipmaps for the currently bound texture
		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(data);
		
		Texture t;
		t.id = textureID;
		t.path = path;
		loadedTextures.push_back(t);
	}

	return textureID;
}

//C++ shouts at me if I don't define the static member here
std::vector<Texture> Scene::loadedTextures;