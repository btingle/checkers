#pragma once

#pragma once
#include "includes.h"

/*
 * Class for any shaders in use, upon initialization the class loads and compiles shaders from the given file paths, and stores the openGL shader ID
 * Class member functions allow for manipulation of uniform variables in the shaders, most importantly the model, view, and projection matrices.
 */

using namespace std;

class Shader
{
public:
	unsigned int shader_id;
	Shader() {}

	Shader(const char * vertex_path, const char * fragment_path)
	{
		string vertex_code;
		string fragment_code;
		ifstream vertex_file;
		ifstream fragment_file;

		vertex_file.exceptions(ifstream::failbit | ifstream::badbit);
		fragment_file.exceptions(ifstream::failbit | ifstream::badbit);
		try
		{
			vertex_file.open(vertex_path);
			fragment_file.open(fragment_path);
			stringstream vert_stream, frag_stream;

			vert_stream << vertex_file.rdbuf();
			frag_stream << fragment_file.rdbuf();

			vertex_code = vert_stream.str();
			fragment_code = frag_stream.str();
		}
		catch (ifstream::failure e)
		{
			cout << "Failed to open shader files";
		}
		const char * vert_code = vertex_code.c_str();
		const char * frag_code = fragment_code.c_str();
		unsigned int vert, frag;

		vert = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vert, 1, &vert_code, NULL);
		glCompileShader(vert);
		checkCompileErrors(vert, "VERTEX");

		frag = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(frag, 1, &frag_code, NULL);
		glCompileShader(frag);
		checkCompileErrors(frag, "FRAGMENT");

		shader_id = glCreateProgram();
		glAttachShader(shader_id, vert);
		glAttachShader(shader_id, frag);
		glLinkProgram(shader_id);
		checkCompileErrors(shader_id, "PROGRAM");

		glDeleteShader(vert);
		glDeleteShader(frag);
	}

	void use()
	{
		glUseProgram(shader_id);
	}

	int getUniformID(const char * uniform_string)
	{
		return glGetUniformLocation(shader_id, uniform_string);
	}

	void setInt(const char * uniform_string, int arg)
	{
		glUniform1i(glGetUniformLocation(shader_id, uniform_string), arg);
	}

	void setInt(int uniform_id, float arg)
	{
		glUniform1i(uniform_id, arg);
	}

	void setFloat(const char * uniform_string, float arg)
	{
		glUniform1f(glGetUniformLocation(shader_id, uniform_string), arg);
	}

	void setFloat(int uniform_id, float arg)
	{
		glUniform1f(uniform_id, arg);
	}

	void setMat4(const char * uniform_string, glm::mat4 arg)
	{
		glUniformMatrix4fv(glGetUniformLocation(shader_id, uniform_string), 1, GL_FALSE, glm::value_ptr(arg));
	}

	void setMat4(int uniform_id, glm::mat4 arg)
	{
		glUniformMatrix4fv(uniform_id, 1, GL_FALSE, glm::value_ptr(arg));
	}

	void setVec4(const char * uniform_string, glm::vec4 arg)
	{
		glUniform4fv(glGetUniformLocation(shader_id, uniform_string), 1, glm::value_ptr(arg));
	}

	void setVec4(int uniform_id, glm::vec4 arg)
	{
		glUniform4fv(uniform_id, 1, glm::value_ptr(arg));
	}

	void setVec3(const char * uniform_string, glm::vec3 arg)
	{
		glUniform3fv(glGetUniformLocation(shader_id, uniform_string), 1, glm::value_ptr(arg));
	}

	void setVec3(int uniform_id, glm::vec3 arg)
	{
		glUniform3fv(uniform_id, 1, glm::value_ptr(arg));
	}

private:
	void checkCompileErrors(GLuint shader, std::string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if (type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if (!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
			}
		}
	}
};
