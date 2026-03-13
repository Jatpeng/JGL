#include "pch.h"
#include "scene_view.h"
#include "imgui.h"
#include<filesystem>
namespace nui
{
	void SceneView::loadSkyBox()
	{
		std::string vsname = FileSystem::getPath("JGL_MeshLoader/shaders/buit_in/skybox_vs.shader");
		std::string fsname = FileSystem::getPath("JGL_MeshLoader/shaders/buit_in/skybox_fs.shader");
		mSkyShader = std::make_unique<nshaders::Shader>(vsname.c_str(), fsname.c_str());
		if (mSkyShader->get_program_id() == 0)
			std::cout << "[SceneView] Skybox shader failed: " << vsname << " / " << fsname << std::endl;
		mSkyBox = std::make_shared<nelems::Model>(FileSystem::getPath("JGL_MeshLoader/resource/defaultcube.fbx"));

		vector<std::string> faces
		{
			FileSystem::getPath("JGL_MeshLoader/resource/built_in/textures/skybox/right.jpg"),
			FileSystem::getPath("JGL_MeshLoader/resource/built_in/textures/skybox/left.jpg"),
			FileSystem::getPath("JGL_MeshLoader/resource/built_in/textures/skybox/top.jpg"),
			FileSystem::getPath("JGL_MeshLoader/resource/built_in/textures/skybox/bottom.jpg"),
			FileSystem::getPath("JGL_MeshLoader/resource/built_in/textures/skybox/front.jpg"),
			FileSystem::getPath("JGL_MeshLoader/resource/built_in/textures/skybox/back.jpg")
		};
		mCubemapTexture = TextureSystem::loadCubemap(faces,false);

	}
	void SceneView::loadPlane()
	{
		std::string vsname = FileSystem::getPath("JGL_MeshLoader/shaders/buit_in/base_vs.shader");
		std::string fsname = FileSystem::getPath("JGL_MeshLoader/shaders/buit_in/base_fs.shader");
		mPlaneShader = std::make_unique<nshaders::Shader>(vsname.c_str(), fsname.c_str());
		mPlane = std::make_shared<nelems::Model>(FileSystem::getPath("JGL_MeshLoader/resource/built_in/plane.fbx"));
		mPlaneTexture = TextureSystem::getTextureId(FileSystem::getPath("JGL_MeshLoader/resource/built_in/textures/wood.png").c_str());

	}
	void SceneView::resize(int32_t width, int32_t height)
	{
		mSize.x = width;
		mSize.y = height;

		mFrameBuffer->create_buffers((int32_t)mSize.x, (int32_t)mSize.y);
	}

	void SceneView::on_mouse_move(double x, double y, nelems::EInputButton button)
	{
		mCamera->on_mouse_move(x, y, button);
	}

	void SceneView::on_mouse_wheel(double delta)
	{
		mCamera->on_mouse_wheel(delta);
	}

	void SceneView::load_mesh(const std::string& filepath)
	{
		mModel = std::make_shared<nelems::Model>(filepath);
		mIsSkin = mModel->GetIsSkinInModel();
		if(mIsSkin) {
			//???????
			load_shader(FileSystem::getPath("JGL_MeshLoader/resource/Anim.xml"));
			mAnimation = Animation(filepath, mModel.get());
			mAnimator = Animator(&mAnimation);
			mMaterial->set_textures(mModel->GetTexturesMap());
		}
	}

	void SceneView::load_shader(const std::string& filepath)
	{
		if (!mMaterial)
			mMaterial = std::make_shared<Material>();
		mMaterial->load(filepath.c_str());
		std::string shadername = mMaterial->getshaderPath();
		if (shadername.empty())
		{
			std::cout << "load_shader: material did not load, skip creating shader for " << filepath << std::endl;
			return;
		}
		size_t found = shadername.find_last_of('_');
		if (found != std::string::npos)
			shadername = shadername.substr(0, found);
		std::string vsname = shadername + "_vs.shader";
		std::string fsname = shadername + "_fs.shader";
		std::filesystem::path pathpath(shadername);
		m_shadername = pathpath.stem().string();
		mShader = std::make_unique<nshaders::Shader>(vsname.c_str(), fsname.c_str());
		if (mShader->get_program_id() == 0)
			std::cout << "[SceneView] Main shader failed: " << vsname << " / " << fsname << std::endl;
	}
	void SceneView::renderSkyBox()
	{
		// draw skybox as last
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
		glDepthMask(GL_FALSE);
		mSkyShader->use();
		mCamera->setcam(mSkyShader.get());
		mSkyShader.get()->set_i1(0, "skybox");
		mSkyShader.get()->set_texture(GL_TEXTURE0, GL_TEXTURE_CUBE_MAP, mCubemapTexture);

		mSkyBox->Draw();
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS); // set depth function back to default
	}

	void SceneView::renderPlane()
	{
		mPlaneShader->use();
		mCamera->update(mPlaneShader.get());
		mPlaneShader.get()->set_i1(GL_TEXTURE0, "baseMap");
		mPlaneShader.get()->set_texture(GL_TEXTURE0, GL_TEXTURE_2D, mPlaneTexture);
		mPlane->Draw();
	}

	void SceneView::render()
	{
		mFrameBuffer->bind();

		if (!mModel)
			load_mesh(FileSystem::getPath("JGL_MeshLoader/resource/cube.fbx"));

		if (mShader && mShader->get_program_id() != 0)
		{
			mShader->use();
			mCamera->update(mShader.get());
		}

		float currentFrame = glfwGetTime();
		mdeltaTime = currentFrame - mlastFrame;
		mlastFrame = currentFrame;
		mAnimator.UpdateAnimation(mdeltaTime);
		//????????????
		if (mIsSkin && mShader && mShader->get_program_id() != 0) {
			auto transforms = mAnimator.GetFinalBoneMatrices();
			for (size_t i = 0; i < transforms.size(); ++i)
				mShader->set_mat4(transforms[i], "finalBonesMatrices[" + std::to_string(i) + "]");
		}

		if (mModel && mShader && mShader->get_program_id() != 0)
		{
			mShader->set_f1((float)glfwGetTime(), "time");
			if (m_shadername == "fur" && mMaterial && mMaterial->isMultyPass()) {
				int passcount = int(mMaterial->getFloatMap()["Pass"]);
				for (int i = 0; i < passcount; i += 1) {
					mShader->set_f1(i, "PassIndex");
					mMaterial->update_shader_params(mShader.get());
					mModel->Draw();
				}
			}
			else
			{
				if (mMaterial)
					mMaterial->update_shader_params(mShader.get());
				mModel->Draw();
			}

		}
		// ?? Scene ??????γ?????????????????????? DockSpace ????
		ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size, ImGuiCond_FirstUseEver);
		ImGui::Begin("Scene");
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();
		mSize = { viewportPanelSize.x, viewportPanelSize.y };
		if (mSize.x > 0 && mSize.y > 0)
			mCamera->set_aspect(mSize.x / mSize.y);
		if (m_showPlane && mPlaneShader && mPlaneShader->get_program_id() != 0 && mPlane)
			renderPlane();
		if (mSkyShader && mSkyShader->get_program_id() != 0 && mSkyBox)
			renderSkyBox();

		mFrameBuffer->unbind();

		ImVec2 displaySize(mSize.x, mSize.y);
		if (displaySize.x < 1.0f) displaySize.x = 1.0f;
		if (displaySize.y < 1.0f) displaySize.y = 1.0f;
		ImVec2 p0 = ImGui::GetCursorScreenPos();
		ImVec2 p1(p0.x + displaySize.x, p0.y + displaySize.y);
		// 先画一块明显绿色，确认 Scene 窗口和绘制区域可见
		ImGui::GetWindowDrawList()->AddRectFilled(p0, p1, IM_COL32(0, 180, 0, 255));
		uint32_t texId = mFrameBuffer->get_texture();
		if (texId != 0)
			ImGui::Image((void*)(intptr_t)texId, displaySize, ImVec2(0, 1), ImVec2(1, 0));
		else
			ImGui::TextColored(ImVec4(1,1,0,1), "FBO texture invalid");
		ImGui::End();
	}
}
