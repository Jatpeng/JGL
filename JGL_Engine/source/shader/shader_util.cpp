#include "pch.h"
#include "shader_util.h"

#include <filesystem>

#include "utils/filesystem.h"

namespace nshaders
{
	namespace
	{
		std::string resolve_shader_path(const char* path)
		{
			if (path == nullptr || path[0] == '\0')
				return {};

			const std::filesystem::path shader_path(path);
			if (shader_path.is_absolute())
				return shader_path.lexically_normal().string();

			return FileSystem::getPath(path);
		}

		bool read_shader_file(const std::string& path, std::string* out_code)
		{
			if (!out_code)
				return false;

			std::ifstream shader_file;
			shader_file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
			try
			{
				shader_file.open(path);
				std::stringstream shader_stream;
				shader_stream << shader_file.rdbuf();
				*out_code = shader_stream.str();
				return true;
			}
			catch (const std::ifstream::failure&)
			{
				std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << path << std::endl;
				return false;
			}
		}

		GLuint compile_shader_stage(const std::string& source_code, GLenum shader_type, const char* debug_name)
		{
			const char* shader_code = source_code.c_str();
			const GLuint shader = glCreateShader(shader_type);
			glShaderSource(shader, 1, &shader_code, nullptr);
			glCompileShader(shader);

			GLint success = GL_FALSE;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if (success == GL_TRUE)
				return shader;

			GLchar info_log[1024] = {};
			glGetShaderInfoLog(shader, 1024, nullptr, info_log);
			std::cout << "ERROR::SHADER_COMPILATION_ERROR of type: " << debug_name
				<< "\n" << info_log << "\n -- --------------------------------------------------- -- " << std::endl;
			glDeleteShader(shader);
			return 0;
		}

		GLuint build_program_from_sources(
			const std::string& vertex_code,
			const std::string& fragment_code,
			const std::string& geometry_code)
		{
			const GLuint vertex = compile_shader_stage(vertex_code, GL_VERTEX_SHADER, "VERTEX");
			if (vertex == 0)
				return 0;

			const GLuint fragment = compile_shader_stage(fragment_code, GL_FRAGMENT_SHADER, "FRAGMENT");
			if (fragment == 0)
			{
				glDeleteShader(vertex);
				return 0;
			}

			GLuint geometry = 0;
			if (!geometry_code.empty())
			{
				geometry = compile_shader_stage(geometry_code, GL_GEOMETRY_SHADER, "GEOMETRY");
				if (geometry == 0)
				{
					glDeleteShader(vertex);
					glDeleteShader(fragment);
					return 0;
				}
			}

			const GLuint program = glCreateProgram();
			glAttachShader(program, vertex);
			glAttachShader(program, fragment);
			if (geometry != 0)
				glAttachShader(program, geometry);
			glLinkProgram(program);

			GLint success = GL_FALSE;
			glGetProgramiv(program, GL_LINK_STATUS, &success);
			if (success != GL_TRUE)
			{
				GLchar info_log[1024] = {};
				glGetProgramInfoLog(program, 1024, nullptr, info_log);
				std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: PROGRAM"
					<< "\n" << info_log << "\n -- --------------------------------------------------- -- " << std::endl;
				glDeleteProgram(program);
				glDeleteShader(vertex);
				glDeleteShader(fragment);
				if (geometry != 0)
					glDeleteShader(geometry);
				return 0;
			}

			glDeleteShader(vertex);
			glDeleteShader(fragment);
			if (geometry != 0)
				glDeleteShader(geometry);

			return program;
		}
	}

	Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath)
		: mVertexPath(resolve_shader_path(vertexPath)),
		  mFragmentPath(resolve_shader_path(fragmentPath)),
		  mGeometryPath(resolve_shader_path(geometryPath))
	{
		reload();
	}

	void Shader::checkCompileErrors(GLuint shader, std::string type)
	{
		(void)shader;
		(void)type;
	}

	void Shader::use()
	{
		glUseProgram(mProgramId);
	}

	bool Shader::reload()
	{
		if (mVertexPath.empty() || mFragmentPath.empty())
			return false;

		std::string vertex_code;
		std::string fragment_code;
		std::string geometry_code;
		if (!read_shader_file(mVertexPath, &vertex_code) ||
			!read_shader_file(mFragmentPath, &fragment_code))
		{
			return false;
		}

		if (!mGeometryPath.empty() && !read_shader_file(mGeometryPath, &geometry_code))
			return false;

		const GLuint new_program = build_program_from_sources(vertex_code, fragment_code, geometry_code);
		if (new_program == 0)
			return false;

		if (mProgramId != 0)
			glDeleteProgram(mProgramId);

		mProgramId = new_program;
		return true;
	}

	void Shader::unload()
	{
		if (mProgramId != 0)
		{
			glDeleteProgram(mProgramId);
			mProgramId = 0;
		}
	}

	void Shader::set_mat4(const glm::mat4& mat4, const std::string& name)
	{
		GLint myLoc = glGetUniformLocation(get_program_id(), name.c_str());
		glUniformMatrix4fv(myLoc, 1, GL_FALSE, glm::value_ptr(mat4));
	}

	void Shader::set_i1(int v, const std::string& name)
	{
		GLint myLoc = glGetUniformLocation(get_program_id(), name.c_str());
		glUniform1i(myLoc, v);
	}

	void Shader::set_f1(float v, const std::string& name)
	{
		GLint myLoc = glGetUniformLocation(get_program_id(), name.c_str());
		glUniform1f(myLoc, v);
	}

	void Shader::set_f2(float a, float b, const std::string& name)
	{
		GLint myLoc = glGetUniformLocation(get_program_id(), name.c_str());
		glUniform2f(myLoc, a, b);
	}

	void Shader::set_f3(float a, float b, float c, const std::string& name)
	{
		GLint myLoc = glGetUniformLocation(get_program_id(), name.c_str());
		glUniform3f(myLoc, a, b, c);
	}

	void Shader::set_vec2(const glm::vec2& vec2, const std::string& name)
	{
		GLint myLoc = glGetUniformLocation(get_program_id(), name.c_str());
		glProgramUniform2fv(get_program_id(), myLoc, 1, glm::value_ptr(vec2));
	}

	void Shader::set_vec3(const glm::vec3& vec3, const std::string& name)
	{
		GLint myLoc = glGetUniformLocation(get_program_id(), name.c_str());
		glProgramUniform3fv(get_program_id(), myLoc, 1, glm::value_ptr(vec3));
	}

	void Shader::set_vec4(const glm::vec4& vec4, const std::string& name)
	{
		GLint myLoc = glGetUniformLocation(get_program_id(), name.c_str());
		glProgramUniform4fv(get_program_id(), myLoc, 1, glm::value_ptr(vec4));
	}

	void Shader::set_texture(int shader_param_id, int tex_type, unsigned int tex_id)
	{
		glActiveTexture(shader_param_id);
		glBindTexture(tex_type, tex_id);
	}
}
