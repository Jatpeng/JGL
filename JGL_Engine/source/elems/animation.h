#pragma once

#include <algorithm>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "bone.h"
#include "model.h"
#include "animdata.h"

struct AssimpNodeData
{
	glm::mat4 transformation;
	std::string name;
	int childrenCount;
	std::vector<AssimpNodeData> children;
};

class Animation
{
public:
	Animation() = default;

	Animation(const std::string& animationPath, nelems::Model* model, const std::string& clipName = "")
	{
		load(animationPath, model, clipName);
	}

	~Animation()
	{
	}

	bool load(const std::string& animationPath, nelems::Model* model, const std::string& clipName = "")
	{
		m_Duration = 0.0f;
		m_TicksPerSecond = 25.0f;
		m_Bones.clear();
		m_RootNode = AssimpNodeData {};
		m_BoneInfoMap.clear();
		m_ClipName.clear();
		m_IsValid = false;

		if (!model)
			return false;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(animationPath, aiProcess_Triangulate);
		if (!scene || !scene->mRootNode || scene->mNumAnimations == 0)
			return false;

		const aiAnimation* selectedAnimation = scene->mAnimations[0];
		if (!clipName.empty())
		{
			selectedAnimation = nullptr;
			for (unsigned int index = 0; index < scene->mNumAnimations; ++index)
			{
				const aiAnimation* candidate = scene->mAnimations[index];
				if (candidate && std::string(candidate->mName.C_Str()) == clipName)
				{
					selectedAnimation = candidate;
					break;
				}
			}
		}

		if (!selectedAnimation)
			return false;

		m_Duration = static_cast<float>(selectedAnimation->mDuration);
		m_TicksPerSecond = selectedAnimation->mTicksPerSecond > 0.0
			? static_cast<float>(selectedAnimation->mTicksPerSecond)
			: 25.0f;
		m_ClipName = selectedAnimation->mName.length > 0 ? selectedAnimation->mName.C_Str() : clipName;
		ReadHeirarchyData(m_RootNode, scene->mRootNode);
		ReadAnimationChannels(selectedAnimation, *model);
		m_IsValid = m_Duration > 0.0f && !m_Bones.empty();
		return m_IsValid;
	}

	Bone* FindBone(const std::string& name)
	{
		auto iter = std::find_if(m_Bones.begin(), m_Bones.end(),
			[&](const Bone& Bone)
			{
				return Bone.GetBoneName() == name;
			}
		);
		if (iter == m_Bones.end()) return nullptr;
		else return &(*iter);
	}

	
	inline float GetTicksPerSecond() const { return m_TicksPerSecond; }
	inline float GetDuration() const { return m_Duration;}
	inline const AssimpNodeData& GetRootNode() const { return m_RootNode; }
	inline const std::map<std::string,BoneInfo>& GetBoneIDMap() const
	{ 
		return m_BoneInfoMap;
	}
	inline const std::string& GetClipName() const { return m_ClipName; }
	inline bool IsValid() const { return m_IsValid; }

private:
	void ReadAnimationChannels(const aiAnimation* animation, nelems::Model& model)
	{
		int size = animation->mNumChannels;

		auto& boneInfoMap = model.GetBoneInfoMap();

		for (int i = 0; i < size; i++)
		{
			auto channel = animation->mChannels[i];
			std::string boneName = channel->mNodeName.data;
			int boneId = -1;
			if (boneInfoMap.find(boneName) != boneInfoMap.end())
				boneId = boneInfoMap[boneName].id;
			m_Bones.push_back(Bone(channel->mNodeName.data,
				boneId, channel));
		}

		m_BoneInfoMap = boneInfoMap;
	}

	void ReadHeirarchyData(AssimpNodeData& dest, const aiNode* src)
	{
		assert(src);

		dest.name = src->mName.data;
		dest.transformation = AssimpGLMHelpers::ConvertMatrixToGLMFormat(src->mTransformation);
		dest.childrenCount = src->mNumChildren;

		for (int i = 0; i < src->mNumChildren; i++)
		{
			AssimpNodeData newData;
			ReadHeirarchyData(newData, src->mChildren[i]);
			dest.children.push_back(newData);
		}
	}
	float m_Duration = 0.0f;
	float m_TicksPerSecond = 25.0f;
	std::vector<Bone> m_Bones;
	AssimpNodeData m_RootNode;
	std::map<std::string, BoneInfo> m_BoneInfoMap;
	std::string m_ClipName;
	bool m_IsValid = false;
};

