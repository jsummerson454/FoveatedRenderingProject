#include <glad/glad.h>

#include "Mesh.h"
#include "shader.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, Material material) {
	this->vertices = vertices;
	this->indices = indices;
	this->material = material;

	// ----- BUFFERS -------

	//create the openGL objects (vertex buffer, vertex array and element buffer)
	glGenBuffers(1, &vbo);
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &ebo);

	//bind the vertex array first
	glBindVertexArray(vao);
	//then bind vertex buffer object (vbo) to vertex buffer type target (GL_ARRAY_BUFFER), so now any calls on that configures the currently bound buffer
	glBindBuffer(GL_ARRAY_BUFFER, vbo);

	//copy vertex data into the buffer
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
	//GL_STATIC_DRAW as the data is set only once

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

	//tell OpenGL how to interpret the vertex data (per vertex attribute) and enable each attribute
	//arguments to glVertexAttribPointer are (index, size, type, normalised, stride, offset)
	//indexes defined using layouts in shaders, position is 0, normal is 1, texcoords is 2
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(1);

	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
	glEnableVertexAttribArray(2);
}

void Mesh::draw(Shader &shader, int instances) {
	//convention here is to always bind diffuse texture to GL_TEXTURE0 and specular to GL_TEXTURE1
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, material.diffuseMapID);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, material.specularMapID);

	shader.setBool("diffuseEnabled", material.diffuseEnabled);
	shader.setBool("specularEnabled", material.specularEnabled);
	shader.setFloat("shininess", material.shininess);
	shader.setVec3f("objectColour", material.colour);

	glBindVertexArray(vao);
	glDrawElementsInstanced(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0, instances);
}

int Mesh::getNumVertices() {
	return vertices.size();
}

