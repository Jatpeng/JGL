#include "material.h"
#include "tinyxml2.h"

Material::Material():name("default")
{
}



void Material::load(const char* xmlPath)
{
    tinyxml2::XMLDocument doc;
    auto result = doc.LoadFile(xmlPath);
    if (result != tinyxml2::XML_SUCCESS)
    {
        std::cout << "Failed to load material xml: " << xmlPath
                  << " error: " << doc.ErrorStr() << std::endl;
        return;
    }

    tinyxml2::XMLElement* root = doc.FirstChildElement("Material");
    if (!root)
    {
        std::cout << "Material xml has no <Material> root: " << xmlPath << std::endl;
        return;
    }

    const char* shaderAttr = root->Attribute("shader");
    const char* nameAttr   = root->Attribute("Name");
    const char* multipassAttr = root->Attribute("multipass");

    if (!shaderAttr || !nameAttr || !multipassAttr)
    {
        std::cout << "Material xml missing required attributes (shader/Name/multipass): "
                  << xmlPath << std::endl;
        return;
    }

    mshader_path = FileSystem::getPath(shaderAttr);
    name = nameAttr;
    istringstream(multipassAttr) >> boolalpha >> mMultipass;

    for (tinyxml2::XMLElement* child = root->FirstChildElement("Param");
         child != nullptr;
         child = child->NextSiblingElement("Param"))
    {
        const char* paramNameAttr    = child->Attribute("Name");
        const char* paramTypeAttr    = child->Attribute("Type");
        const char* paramDefaultAttr = child->Attribute("Default");

        if (!paramNameAttr || !paramTypeAttr || !paramDefaultAttr)
            continue;

        Param param;
        param.name         = paramNameAttr;
        param.type         = paramTypeAttr;
        param.defaultValue = paramDefaultAttr;
        params.push_back(param);

        if (param.type == "Texture")
        {
            std::string texPath = FileSystem::getPath(param.defaultValue);
            unsigned int tex_id = TextureSystem::getTextureId(texPath.c_str());
            mTexture_map[param.name] = std::make_pair(tex_id, texPath);
        }
        else if (param.type == "float")
        {
            mFloat_map.insert(std::make_pair(param.name, std::stof(param.defaultValue)));
        }
        else if (param.type == "float3")
        {
            glm::vec3 value = StringtoFloat3(param.defaultValue);
            mFloat3_map.insert(std::make_pair(param.name, value));
        }
    }
}

bool Material::update_shader_params(nshaders::Shader* shader)
{
	for (auto it : mFloat_map) {
		shader->set_f1(it.second, it.first);
	}
	for (auto it : mFloat3_map) {
		shader->set_vec3(it.second, it.first);
	}
	int tex_index = 0;
	for (auto it : mTexture_map) {
		//给纹理采样器分配一个位置值
		shader->set_i1(tex_index, it.first);
		//激活纹理单元，绑定纹理到激活的纹理单元
		shader->set_texture(GL_TEXTURE0 + tex_index, GL_TEXTURE_2D,it.second.first);
		tex_index++;
	}
	return true;
}

bool Material::set_param(string name, string type, string value)
{
	if (type == "float") {
		mFloat_map[name] = stof(value);
	}
	if (type == "float3") {
		mFloat3_map[name] = StringtoFloat3(value);
	}
	return true;
}

glm::vec3 Material::StringtoFloat3(std::string str)
{
	istringstream iss(str);
	string token;
	vector<string> tmp;
	while (getline(iss, token, ','))
	{
		tmp.push_back(token);
	}
	glm::vec3 res{ stof(tmp[0]),stof(tmp[1]),stof(tmp[2]) };
	return res;
}

Material::~Material() {
	// 清除参数列表
	params.clear();
	// 清除纹理映射
	mTexture_map.clear();
	// 清除浮点数映射
	mFloat_map.clear();
	// 清除浮点向量映射
	mFloat3_map.clear();
}

void Material::set_textures(map<string, pair<unsigned int, string>> textures)
{
	mTexture_map = textures;
}
