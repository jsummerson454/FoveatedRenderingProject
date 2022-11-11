#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "shader.h"

struct Vertex {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
};

struct Material {
	unsigned int diffuseMapID;
	bool diffuseEnabled;

	unsigned int specularMapID;
	bool specularEnabled;

	//If material doesn't have a diffuse texture map then instead use this
	glm::vec3 colour;
	float shininess;
};

class Mesh {
public:
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Material material);

	void draw(Shader& shader, int instances);

	int getNumVertices();

private:
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	Material material;

	//OpenGL buffer object ids
	unsigned int vao, vbo, ebo;

};