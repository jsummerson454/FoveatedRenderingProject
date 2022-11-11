#include "shader.h"

#include <glad/glad.h> // include glad to get all the required OpenGL headers

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace std;

enum class check {
	VERTEX,
	FRAGMENT,
	SHADER
};

string readFile(const char* path) {
	ifstream file;
	//allow input filstream to throw exceptions
	file.exceptions(ifstream::failbit | ifstream::badbit);
	try {
		file.open(path);
		stringstream ss;
		ss << file.rdbuf();
		file.close();

		return ss.str();
	}
	catch (...) {
		cout << "Error whilst reading file: " << path << endl;
	}
}

void logSuccess(unsigned int objectID, check type) {
	int success;
	char log[1024];

	if (type == check::VERTEX || type == check::FRAGMENT) {
		glGetShaderiv(objectID, GL_COMPILE_STATUS, &success);
		if (!success) {
			glad_glGetShaderInfoLog(objectID, 1024, NULL, log);
			if (type == check::VERTEX) {
				cout << "Error compiling vertex shader:\n" << log << endl;
			}
			else {
				cout << "Error compiling fragment shader:\n" << log << endl;
			}
		}
	}
	else if (type == check::SHADER) {
		glGetProgramiv(objectID, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(objectID, 1024, NULL, log);
			cout << "Error linking shader program:\n" << log << endl;
		}
	}
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
	string vertexString = readFile(vertexPath);
	string fragmentString = readFile(fragmentPath);
	const char* vertexShaderCode = vertexString.c_str();
	const char* fragmentShaderCode = fragmentString.c_str();

	//Compile shaders
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderCode, NULL);
	glCompileShader(vertexShader);
	logSuccess(vertexShader, check::VERTEX);

	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderCode, NULL);
	glCompileShader(fragmentShader);
	logSuccess(fragmentShader, check::FRAGMENT);

	//link shaders with shader program
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	logSuccess(shaderProgram, check::SHADER);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void Shader::use() const {
	glUseProgram(shaderProgram);
}

int Shader::getAttributeLocation(const char* attribute) const {
	return glGetAttribLocation(shaderProgram, attribute);
}

void Shader::setFloat(const char* name, float value) const {
	glUniform1f(glGetUniformLocation(shaderProgram, name), value);
}

void Shader::setInt(const char* name, int value) const {
	glUniform1i(glGetUniformLocation(shaderProgram, name), value);
}

void Shader::setMat4f(const char* name, const float* value) const {
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, name), 1, GL_FALSE, value);
}

void Shader::setMat3f(const char* name, const float* value) const {
	glUniformMatrix3fv(glGetUniformLocation(shaderProgram, name), 1, GL_FALSE, value);
}

void Shader::setVec2f(const char* name, const glm::vec2& value) const {
	glUniform2fv(glGetUniformLocation(shaderProgram, name), 1, &value[0]);
}

void Shader::setVec3f(const char* name, const glm::vec3& value) const {
	glUniform3fv(glGetUniformLocation(shaderProgram, name), 1, &value[0]);
}

void Shader::setVec4f(const char* name, const glm::vec4& value) const {
	glUniform4fv(glGetUniformLocation(shaderProgram, name), 1, &value[0]);
}

void Shader::setBool(const char* name, bool value) const {
	//no glUniform1b, so set as an int - in this case zeros are converted to false and non-zeroes to true
	//this means using C++'s casting of bools to ints will work fine
	glUniform1i(glGetUniformLocation(shaderProgram, name), value);
}