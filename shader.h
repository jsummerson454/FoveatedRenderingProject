#pragma once

#include <glm/glm.hpp>

class Shader
{
private:
	// the opengl shader program ID
	unsigned int shaderProgram;
public:
	// constructor reads, compiles and links shaders
	Shader(const char* vertexPath, const char* fragmentPath);
	// use the shader (glUseProgram(ShaderProgram))
	void use() const;
	// get attribute location
	int getAttributeLocation(const char* attribute) const;
	// utility uniform functions - implement as needed REMEBER TO CALL .USE() BEFORE CALLING THESE
	void setFloat(const char* name, float value) const;
	void setInt(const char* name, int value) const;
	void setMat3f(const char* name, const float* value) const;
	void setMat4f(const char* name, const float* value) const;
	void setVec2f(const char* name, const glm::vec2& value) const;
	void setVec3f(const char* name, const glm::vec3& value) const;
	void setVec4f(const char* name, const glm::vec4& value) const;
	void setBool(const char* name, bool value) const;
};