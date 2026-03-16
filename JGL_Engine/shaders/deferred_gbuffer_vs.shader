#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 5) in ivec4 aBoneIds;
layout(location = 6) in vec4 aBoneWeights;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform bool useSkinning;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoords;

void main()
{
    vec4 local_position = vec4(aPosition, 1.0);
    vec3 local_normal = aNormal;

    if (useSkinning)
    {
        mat4 blended_bones = mat4(0.0);
        vec3 skinned_normal = vec3(0.0);
        bool applied = false;

        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            int bone_id = aBoneIds[i];
            float weight = aBoneWeights[i];

            if (bone_id < 0 || bone_id >= MAX_BONES || weight <= 0.0)
                continue;

            blended_bones += finalBonesMatrices[bone_id] * weight;
            skinned_normal += mat3(finalBonesMatrices[bone_id]) * aNormal * weight;
            applied = true;
        }

        if (applied)
        {
            local_position = blended_bones * vec4(aPosition, 1.0);
            local_normal = normalize(skinned_normal);
        }
    }

    vec4 world_position = model * local_position;
    WorldPos = world_position.xyz;
    Normal = mat3(transpose(inverse(model))) * local_normal;
    TexCoords = aTexCoords;

    gl_Position = projection * view * world_position;
}
