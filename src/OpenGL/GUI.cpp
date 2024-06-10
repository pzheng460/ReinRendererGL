#include "GUI.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <learnopengl/filesystem.h>
#include <stb_image.h>

GUI::GUI() : pbrActive(true), wireframeMode(false) {
    // 初始化其他参数
}

GUI::~GUI() {
    // 清理 ImGui 资源
}

void GUI::init(GLFWwindow* window) {
    const char* glsl_version = "#version 330";
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark(); // Dark style，暗色风格

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f); // 背景颜色
}

void GUI::render(Camera& camera, std::vector<unique_ptr<Object>>& objects, std::string& modelFilePath, std::vector<Light>& lights) {
    ImGui_ImplOpenGL3_NewFrame(); // OpenGL 渲染 ImGui
    ImGui_ImplGlfw_NewFrame(); // GLFW 渲染 ImGui
    ImGui::NewFrame(); // ImGui 新帧
    {
        // 创建一个新的窗口并显示信息
        ImGui::Begin("Parameters");

        // 显示帧率
        ImGui::Text("Framerate: %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

        // 显示和控制相机参数
        ImGui::Text("Camera Position");
        ImGui::SliderFloat("X", &camera.Position.x, -30.0f, 30.0f);
        ImGui::SliderFloat("Y", &camera.Position.y, -30.0f, 30.0f);
        ImGui::SliderFloat("Z", &camera.Position.z, -30.0f, 30.0f);
        ImGui::Text("Camera Yaw");
        ImGui::SliderFloat("Yaw", &camera.Yaw, -180.0f, 180.0f);
        ImGui::Text("Camera Pitch");
        ImGui::SliderFloat("Pitch", &camera.Pitch, -89.0f, 89.0f);

        // 更新相机的方向
        camera.updateCameraVectors();

        ImGui::Separator(); // 分隔线

        // 显示和控制光源参数
        for (unsigned int i = 0; i < lights.size(); i++)
        {
            ImGui::Text("Light %d Position", i);
            ImGui::SliderFloat(("X##" + std::to_string(i)).c_str(), &lights[i].position.x, -20.0f, 20.0f);
            ImGui::SliderFloat(("Y##" + std::to_string(i)).c_str(), &lights[i].position.y, -20.0f, 20.0f);
            ImGui::SliderFloat(("Z##" + std::to_string(i)).c_str(), &lights[i].position.z, -20.0f, 20.0f);
            ImGui::Text("Light %d Color", i);
            ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(), (float*)&lights[i].color);
        }

        ImGui::Separator(); // 分隔线

        ImGui::Text("EFFECTS");
        // 开关 PBR
        ImGui::Checkbox("PBR", &pbrActive);

        // 开关 SkyBox
        ImGui::Checkbox("SkyBox", &skyBoxActive);

        ImGui::Separator(); // 分隔线

        // 模型导入
        ImGui::Text("Model Import");
        static char filePath[128] = "";
        ImGui::InputText("File Path", filePath, sizeof(filePath));
        if (ImGui::Button("Load Model")) {
            if (strlen(filePath) == 0) {
                strcpy(filePath, modelFilePath.c_str()); // 使用默认的模型文件路径
            } else {
                modelFilePath = std::string(filePath); // 更新模型文件路径
            }

            // load models
            stbi_set_flip_vertically_on_load(false); // tell stb_image.h to flip loaded texture's on the y-axis.
            Model ourModel(FileSystem::getPath(modelFilePath));

            auto object = std::make_unique<Object>(ourModel, RenderMode::NORMAL);
            objects.push_back(std::move(object));
        }

        ImGui::End();
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
