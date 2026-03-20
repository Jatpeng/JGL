#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 5) in ivec4 aBoneIds;
layout(location = 6) in vec4 aBoneWeights;

uniform mat4 model;
uniform mat4 lightSpaceMatrix;
uniform bool useSkinning;

const int MAX_BONES = 100;
const int MAX_BONE_INFLUENCE = 4;
uniform mat4 finalBonesMatrices[MAX_BONES];

void main()
{
    vec4 local_position = vec4(aPosition, 1.0);

    if (useSkinning)
    {
        vec4 blended_position = vec4(0.0);
        bool applied = false;

        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i)
        {
            int bone_id = aBoneIds[i];
            float weight = aBoneWeights[i];

            if (bone_id < 0 || bone_id >= MAX_BONES || weight <= 0.0)
                continue;

            blended_position += finalBonesMatrices[bone_id] * vec4(aPosition, 1.0) * weight;
            applied = true;
        }

        if (applied)
            local_position = blended_position;
    }

    gl_Position = lightSpaceMatrix * model * local_position;
}
