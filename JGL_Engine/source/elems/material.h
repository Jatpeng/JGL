#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <fstream>
#include <functional>
#include "pch.h"
#include "tinyxml2.h"
#include "utils/texturessystem.h"
#include "shader/shader_util.h"

using namespace std;

class Attribute {
public:
	string name;
	string value;
};

struct Param {
	string name;
	string type;
	string defaultValue;
	bool multipass;
};

struct MaterialLoadContext
{
	std::function<std::string(const std::string&)> resolve_path;
	std::function<unsigned int(const std::string&, GLint)> load_texture_2d;
};


class Material {
public:
	Material();
    ~Material();
	void set_textures(map<string, pair<unsigned int, string>> textures);
	void load(const char* materialPath, const MaterialLoadContext* context = nullptr);
	void print() {
		cout << "Name: " << name << endl;
		cout << "Material Params:" << endl;
		for (const auto& param : params) {
			cout << "Param Name: " << param.name << endl;
			cout << "Param Type: " << param.type << endl;
			cout << "Param Default Value: " << param.defaultValue << endl;
		}
		if (!mEngineParams.empty()) {
			cout << "Engine Params:" << endl;
			for (const auto& param : mEngineParams) {
				cout << "Param Name: " << param.name << endl;
				cout << "Param Type: " << param.type << endl;
				cout << "Param Default Value: " << param.defaultValue << endl;
			}
		}
	}
	bool update_shader_params(nshaders::Shader* shader);
	bool set_param(string name,string type,string value);
	glm::vec3 StringtoFloat3(std::string str);
	string getshaderPath() { return mshader_path; }
	inline bool isMultyPass() { return mMultipass; }
	inline map<string, float>& getFloatMap() { return mFloat_map; }
	inline map<string, glm::vec3>& getFloat3Map() { return mFloat3_map; }
	inline map<string, pair<unsigned int, string>>& getTextureMap(){ return mTexture_map; }
	inline vector<Param>& getEngineParams() { return mEngineParams; }
	inline map<string, float>& getEngineFloatMap() { return mEngineFloat_map; }
	inline map<string, glm::vec3>& getEngineFloat3Map() { return mEngineFloat3_map; }
	inline map<string, pair<unsigned int, string>>& getEngineTextureMap() { return mEngineTexture_map; }
	inline int passcount() { return mPassCount; }
public:
	vector<Param> params;
	vector<Param> mEngineParams;
	string name;
	map<string,pair<unsigned int,string>> mTexture_map;
	map<string, float> mFloat_map;
	map<string, glm::vec3> mFloat3_map;
	map<string,pair<unsigned int,string>> mEngineTexture_map;
	map<string, float> mEngineFloat_map;
	map<string, glm::vec3> mEngineFloat3_map;
	bool mMultipass;
	string mshader_path;
	int mPassCount;
};

inline vector<string> split(const string& str, char delimiter) {
	vector<string> tokens;
	stringstream ss(str);
	string token;
	while (getline(ss, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

