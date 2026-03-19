#include "pch.h"
#include "post_process_stack.h"

namespace nrender
{
  PostProcessStack::PostProcessStack()
  {
  }

  PostProcessStack::~PostProcessStack()
  {
    if (mQuadVAO != 0)
    {
      glDeleteVertexArrays(1, &mQuadVAO);
      glDeleteBuffers(1, &mQuadVBO);
    }
  }

  bool PostProcessStack::init()
  {
    mShader = std::make_unique<nshaders::Shader>("JGL_Engine/shaders/post_process_vs.shader", "JGL_Engine/shaders/post_process_fs.shader");
    if (mShader->get_program_id() == 0)
    {
      std::cerr << "Failed to load post-processing shaders" << std::endl;
      return false;
    }

    float quadVertices[] = {
      // positions        // texture Coords
      -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
      -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
       1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
       1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    glGenVertexArrays(1, &mQuadVAO);
    glGenBuffers(1, &mQuadVBO);
    glBindVertexArray(mQuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mQuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindVertexArray(0);

    return true;
  }

  void PostProcessStack::render(uint32_t hdr_texture_id, int32_t width, int32_t height)
  {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    mShader->use();
    mShader->set_i1(0, "hdrBuffer");

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdr_texture_id);

    glBindVertexArray(mQuadVAO);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
  }
}
