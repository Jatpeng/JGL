#include "material.h"
#include "tinyxml2.h"

#include <unordered_set>

namespace
{
	bool is_engine_managed_param_name(const std::string& param_name)
	{
		static const std::unordered_set<std::string> kEngineManagedParams = {
			"camPos",
			"opacity",
			"time",
			"lightCount"
		};

		return kEngineManagedParams.find(param_name) != kEngineManagedParams.end();
	}
}

Material::Material() : name("default")
{
}

void Material::load(const char* materialPath, const MaterialLoadContext* context)
{
	params.clear();
	mEngineParams.clear();
	mTexture_map.clear();
	mFloat_map.clear();
	mFloat2_map.clear();
	mFloat3_map.clear();
	mEngineTexture_map.clear();
	mEngineFloat_map.clear();
	mEngineFloat2_map.clear();
	mEngineFloat3_map.clear();
	mshader_path.clear();
	name = "default";
	mMultipass = false;
	mPassCount = 0;

	auto resolve_path = [&](const std::string& path_value)
	{
		if (context && context->resolve_path)
			return context->resolve_path(path_value);
		return FileSystem::getPath(path_value);
	};

	auto load_texture = [&](const std::string& path_value, GLint tex_wrapping)
	{
		if (context && context->load_texture_2d)
			return context->load_texture_2d(path_value, tex_wrapping);
		return TextureSystem::getTextureId(path_value.c_str(), tex_wrapping);
	};

	tinyxml2::XMLDocument doc;
	const auto result = doc.LoadFile(materialPath);
	if (result != tinyxml2::XML_SUCCESS)
	{
		std::cout << "Failed to load material file: " << materialPath
		          << " error: " << doc.ErrorStr() << std::endl;
		return;
	}

	tinyxml2::XMLElement* root = doc.FirstChildElement("Material");
	if (!root)
	{
		std::cout << "Material file has no <Material> root: " << materialPath << std::endl;
		return;
	}

	const char* shader_attr = root->Attribute("shader");
	const char* name_attr = root->Attribute("Name");
	const char* multipass_attr = root->Attribute("multipass");

	if (!shader_attr || !name_attr || !multipass_attr)
	{
		std::cout << "Material file missing required attributes (shader/Name/multipass): "
		          << materialPath << std::endl;
		return;
	}

	mshader_path = resolve_path(shader_attr);
	name = name_attr;
	istringstream(multipass_attr) >> boolalpha >> mMultipass;

	for (tinyxml2::XMLElement* child = root->FirstChildElement("Param");
	     child != nullptr;
	     child = child->NextSiblingElement("Param"))
	{
		const char* param_name_attr = child->Attribute("Name");
		const char* param_type_attr = child->Attribute("Type");
		const char* param_default_attr = child->Attribute("Default");

		if (!param_name_attr || !param_type_attr || !param_default_attr)
			continue;

		Param param;
		param.name = param_name_attr;
		param.type = param_type_attr;
		param.defaultValue = param_default_attr;

		const bool engine_managed = is_engine_managed_param_name(param.name);
		auto& target_params = engine_managed ? mEngineParams : params;
		auto& target_texture_map = engine_managed ? mEngineTexture_map : mTexture_map;
		auto& target_float_map = engine_managed ? mEngineFloat_map : mFloat_map;
		auto& target_float2_map = engine_managed ? mEngineFloat2_map : mFloat2_map;
		auto& target_float3_map = engine_managed ? mEngineFloat3_map : mFloat3_map;

		target_params.push_back(param);

		if (param.type == "Texture")
		{
			std::string tex_path = resolve_path(param.defaultValue);
			unsigned int tex_id = load_texture(tex_path, GL_REPEAT);
			target_texture_map[param.name] = std::make_pair(tex_id, tex_path);
		}
		else if (param.type == "float")
		{
			target_float_map.insert(std::make_pair(param.name, std::stof(param.defaultValue)));
		}
		else if (param.type == "float2")
		{
			glm::vec2 value = StringtoFloat2(param.defaultValue);
			target_float2_map.insert(std::make_pair(param.name, value));
		}
		else if (param.type == "float3")
		{
			glm::vec3 value = StringtoFloat3(param.defaultValue);
			target_float3_map.insert(std::make_pair(param.name, value));
		}
	}
}

bool Material::update_shader_params(nshaders::Shader* shader)
{
	return update_shader_params(shader, 0);
}

bool Material::update_shader_params(nshaders::Shader* shader, int texture_unit_offset)
{
	for (const auto& it : mFloat_map)
		shader->set_f1(it.second, it.first);

	for (const auto& it : mFloat2_map)
		shader->set_vec2(it.second, it.first);

	for (const auto& it : mFloat3_map)
		shader->set_vec3(it.second, it.first);

	int tex_index = texture_unit_offset;
	for (const auto& it : mTexture_map)
	{
		shader->set_i1(tex_index, it.first);
		shader->set_texture(GL_TEXTURE0 + tex_index, GL_TEXTURE_2D, it.second.first);
		++tex_index;
	}
	return true;
}

bool Material::set_param(string param_name, string type, string value)
{
	auto& target_float_map = is_engine_managed_param_name(param_name) ? mEngineFloat_map : mFloat_map;
	auto& target_float2_map = is_engine_managed_param_name(param_name) ? mEngineFloat2_map : mFloat2_map;
	auto& target_float3_map = is_engine_managed_param_name(param_name) ? mEngineFloat3_map : mFloat3_map;

	if (type == "float")
		target_float_map[param_name] = stof(value);
	if (type == "float2")
		target_float2_map[param_name] = StringtoFloat2(value);
	if (type == "float3")
		target_float3_map[param_name] = StringtoFloat3(value);
	return true;
}

glm::vec2 Material::StringtoFloat2(std::string str)
{
	istringstream iss(str);
	string token;
	vector<string> tmp;
	while (getline(iss, token, ','))
		tmp.push_back(token);
	glm::vec2 res{ stof(tmp[0]), stof(tmp[1]) };
	return res;
}

glm::vec3 Material::StringtoFloat3(std::string str)
{
	istringstream iss(str);
	string token;
	vector<string> tmp;
	while (getline(iss, token, ','))
		tmp.push_back(token);
	glm::vec3 res{ stof(tmp[0]), stof(tmp[1]), stof(tmp[2]) };
	return res;
}

Material::~Material()
{
	params.clear();
	mEngineParams.clear();
	mTexture_map.clear();
	mFloat_map.clear();
	mFloat2_map.clear();
	mFloat3_map.clear();
	mEngineTexture_map.clear();
	mEngineFloat_map.clear();
	mEngineFloat2_map.clear();
	mEngineFloat3_map.clear();
}

void Material::set_textures(const map<string, pair<unsigned int, string>>& textures)
{
	mTexture_map = textures;
}
