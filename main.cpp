#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <cmath>
#include "crypto.h"

bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) return false;

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;
    return true;
}

// State
bool is_activated = false;
bool show_activation_modal = false;
bool show_toast = false;
double toast_time = 0.0;
char license_key_input[512] = "";
std::string user_hwid = "";
std::string toast_message = "";

void CheckLicense() {
    std::string saved_sig = LoadLicense();
    if (!saved_sig.empty() && VerifyLicense(user_hwid, saved_sig)) {
        is_activated = true;
    }
}

int main(int, char**)
{
    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    
    int window_width = 1000;
    int window_height = 600;
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Advanced Loader", NULL, NULL);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImGui::StyleColorsDark();

    // Style adjustments
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 12.0f;
    style.FrameRounding = 8.0f;
    style.PopupRounding = 12.0f;
    style.ItemSpacing = ImVec2(10, 10);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
    style.Colors[ImGuiCol_PopupBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.98f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.20f, 0.20f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.25f, 0.25f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2; font_cfg.OversampleV = 2; font_cfg.PixelSnapH = true;
    static const ImWchar ranges[] = { 0x0020, 0x00FF, 0x0100, 0x017F, 0x0400, 0x044F, 0 };
    
    ImFont* font_regular = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 16.0f, &font_cfg, ranges);
    ImFont* font_bold_large = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 48.0f, &font_cfg, ranges);
    ImFont* font_bold = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 16.0f, &font_cfg, ranges);
    ImFont* font_small = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 13.0f, &font_cfg, ranges);

    GLuint bg_texture = 0;
    int bg_width = 0, bg_height = 0;
    LoadTextureFromFile("game_bg.png", &bg_texture, &bg_width, &bg_height);

    // Get HWID and check local license
    user_hwid = GetHWID();
    CopyToClipboard(user_hwid);
    CheckLicense();

    bool is_dragging = false;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (!is_dragging && mouse_y < 50 && mouse_x < window_width - 150) is_dragging = true;
        } else is_dragging = false;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2((float)window_width, (float)window_height));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
        ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // 1. Sidebar (Dark vertical strip on the left)
        float sidebar_width = 80.0f;
        draw_list->AddRectFilled(ImVec2(0,0), ImVec2(sidebar_width, window_height), IM_COL32(20,20,20,255), 12.0f, ImDrawFlags_RoundCornersLeft);
        
        // 2. Main Content Area Background
        if (bg_texture) {
            draw_list->AddImageRounded((void*)(intptr_t)bg_texture, ImVec2(sidebar_width, 0), ImVec2(window_width, window_height), ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,255), 12.0f, ImDrawFlags_RoundCornersRight);
            // Dark gradient over image
            draw_list->AddRectFilledMultiColor(ImVec2(sidebar_width, 0), ImVec2(window_width, window_height), IM_COL32(0,0,0,150), IM_COL32(0,0,0,50), IM_COL32(0,0,0,200), IM_COL32(0,0,0,240));
        } else {
            draw_list->AddRectFilled(ImVec2(sidebar_width, 0), ImVec2(window_width, window_height), IM_COL32(30,30,30,255), 12.0f, ImDrawFlags_RoundCornersRight);
        }

        // Window Controls
        ImGui::SetCursorPos(ImVec2(window_width - 40, 10));
        if (ImGui::Button("X", ImVec2(30,30))) glfwSetWindowShouldClose(window, GLFW_TRUE);
        ImGui::SetCursorPos(ImVec2(window_width - 80, 10));
        if (ImGui::Button("_", ImVec2(30,30))) glfwIconifyWindow(window);

        // Sidebar Icons (Simulated)
        for(int i=0; i<5; i++) {
            ImGui::SetCursorPos(ImVec2(20, 150 + i * 60));
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, i==1 ? ImVec4(0.9f, 0.8f, 0.5f, 0.2f) : ImVec4(0.1f, 0.1f, 0.1f, 0.5f));
            if (ImGui::Button("O", ImVec2(40,40))) {} // Placeholder for icon
            ImGui::PopStyleColor();
            ImGui::PopID();
        }

        // Content
        float content_x = sidebar_width + 40.0f;
        ImGui::SetCursorPos(ImVec2(content_x, 250));
        ImGui::PushFont(font_bold_large);
        ImGui::Text("Warface - Lite");
        ImGui::PopFont();

        ImGui::SetCursorPos(ImVec2(content_x, 320));
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.8f, 0.5f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1.0f, 0.9f, 0.6f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.8f, 0.7f, 0.4f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::PushFont(font_bold);

        if (is_activated) {
            if (ImGui::Button(u8"  Oyun  ", ImVec2(200, 45))) {
                // Play game action
            }
        } else {
            if (ImGui::Button(u8"Anahtar\u0131 etkinle\u015ftir", ImVec2(250, 45))) {
                show_activation_modal = true;
            }
        }
        
        ImGui::PopFont();
        ImGui::PopStyleColor(4);

        ImGui::SameLine();
        ImGui::Button(" i ", ImVec2(45, 45));

        // System Requirements Panel
        float panel_y = window_height - 200.0f;
        draw_list->AddRectFilled(ImVec2(content_x, panel_y), ImVec2(window_width - 40, window_height - 20), IM_COL32(20,20,20,200), 16.0f);
        
        ImGui::SetCursorPos(ImVec2(content_x + 20, panel_y - 30));
        ImGui::PushFont(font_bold);
        ImGui::Text("Sistem Gereksinimleri");
        ImGui::PopFont();

        ImGui::SetCursorPos(ImVec2(content_x + 20, panel_y + 20));
        ImGui::PushFont(font_small);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Tavsiye edilir");
        ImGui::SetCursorPos(ImVec2(content_x + 20, panel_y + 50));
        ImGui::Text("Islemci\t\t\t\tCore i3-530 / AMD Athlon II X3");
        ImGui::SetCursorPos(ImVec2(content_x + 20, panel_y + 80));
        ImGui::Text("Hafiza \t\t\t\t4 GB");
        ImGui::SetCursorPos(ImVec2(content_x + 20, panel_y + 110));
        ImGui::Text("Ekran karti\t\t\tGeForce 250 GTS / Radeon HD 4850");
        ImGui::PopFont();

        // Activation Modal
        if (show_activation_modal) {
            ImGui::OpenPopup(u8"Ürün aktivasyonu");
        }

        ImGui::SetNextWindowSize(ImVec2(400, 250));
        if (ImGui::BeginPopupModal(u8"Ürün aktivasyonu", &show_activation_modal, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 20);
            ImGui::Text(u8"Anahtarı girin (HWID'niz panoya kopyalandı):");
            ImGui::SetNextItemWidth(-1);
            ImGui::InputText("##key", license_key_input, IM_ARRAYSIZE(license_key_input));

            ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 30);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.9f, 0.8f, 0.5f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            
            if (ImGui::Button(u8"ETKİNLEŞTİR", ImVec2(-1, 40))) {
                if (VerifyLicense(user_hwid, license_key_input)) {
                    is_activated = true;
                    SaveLicense(license_key_input);
                    show_activation_modal = false;
                    show_toast = true;
                    toast_time = glfwGetTime();
                    toast_message = u8"Aboneliğinizi başarıyla etkinleştirdiniz";
                } else {
                    show_toast = true;
                    toast_time = glfwGetTime();
                    toast_message = u8"Geçersiz Anahtar!";
                }
            }
            ImGui::PopStyleColor(2);
            ImGui::EndPopup();
        }

        // Toast Notification
        if (show_toast) {
            double current_time = glfwGetTime();
            if (current_time - toast_time < 3.0) {
                float alpha = 1.0f;
                if (current_time - toast_time > 2.5) alpha = (3.0f - (current_time - toast_time)) / 0.5f;
                
                ImGui::SetNextWindowPos(ImVec2(window_width / 2.0f, window_height - 60.0f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
                ImGui::SetNextWindowBgAlpha(alpha * 0.9f);
                ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 20.0f);
                ImGui::Begin("Toast", NULL, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, alpha));
                ImGui::Text("%s", toast_message.c_str());
                ImGui::PopStyleColor();
                ImGui::End();
                ImGui::PopStyleVar();
            } else {
                show_toast = false;
            }
        }

        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
