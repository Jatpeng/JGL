with open("JGL_Engine/source/engine/render_engine.h", "r") as f:
    text = f.read()

import re

# Remove duplicate shadow_pass
text = re.sub(r'    void shadow_pass\(\);\n    void shadow_pass\(\);\n    void shadow_pass\(\);\n    void shadow_pass\(\);\n    void shadow_pass\(\);\n', '    void shadow_pass();\n', text)

# Remove duplicate shadow variables block
shadow_vars = r"""    unsigned int mShadowMapFBO_id = 0;
    std::unique_ptr<nshaders::Shader> mDepthShader;
    glm::mat4 mLightSpaceMatrix;
    unsigned int mShadowMapTexture = 0;
    const unsigned int SHADOW_WIDTH = 2048;
    const unsigned int SHADOW_HEIGHT = 2048;"""

shadow_vars_alt = r"""    unsigned int mShadowMapFBO_id = 0;
    std::unique_ptr<nshaders::Shader> mDepthShader;
    glm::mat4 mLightSpaceMatrix;
    unsigned int mShadowMapTexture = 0;
    unsigned int SHADOW_WIDTH = 2048;
    unsigned int SHADOW_HEIGHT = 2048;"""

text = re.sub(shadow_vars + r'\n\n' + shadow_vars + r'\n\n' + shadow_vars_alt + r'\n\n' + shadow_vars + r'\n\n' + shadow_vars, shadow_vars, text)
text = re.sub(shadow_vars + r'\n\n' + shadow_vars, shadow_vars, text)
text = re.sub(shadow_vars + r'\n\n' + shadow_vars, shadow_vars, text)


with open("JGL_Engine/source/engine/render_engine.h", "w") as f:
    f.write(text)
