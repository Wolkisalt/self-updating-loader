#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include <cmath>
#include "updater.h"

// Simple helper function to load an image into a OpenGL texture with common settings
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

#if defined(GL_UNPACK_ROW_LENGTH) && !defined(__EMSCRIPTEN__)
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
#endif
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;
    return true;
}

int main(int, char**)
{
    if (!glfwInit()) return 1;

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    
    int window_width = 1000;
    int window_height = 600;
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Loader", NULL, NULL);
    if (window == NULL) return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); 

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 2;
    font_cfg.PixelSnapH = true;
    
    static const ImWchar ranges[] = {
        0x0020, 0x00FF, // Basic Latin
        0x0100, 0x017F, // Latin Extended-A
        0x0400, 0x044F, // Cyrillic
        0,
    };
    
    ImFont* font_regular_large = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 20.0f, &font_cfg, ranges);
    ImFont* font_bold = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 32.0f, &font_cfg, ranges);
    ImFont* font_regular_small = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f, &font_cfg, ranges);

    GLuint bg_texture = 0;
    int bg_width = 0, bg_height = 0;
    LoadTextureFromFile("bg.png", &bg_texture, &bg_width, &bg_height);

    bool is_dragging = false;
    int win_start_x = 0, win_start_y = 0;

    // DOWNLOAD STATE
    DownloadProgress dl_progress;
    // We will download a 10MB test file from tele2 for demonstration of the real progress
    StartDownloadAsync("http://speedtest.tele2.net/10MB.zip", "test_file.zip", &dl_progress);

    float visual_progress_ratio = 0.0f;
    double start_time = glfwGetTime();

    // FPS / Speed calculation
    double last_time = glfwGetTime();
    uint64_t last_bytes = 0;
    float current_speed_mb = 0.0f;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Drag window logic (simplified)
        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (!is_dragging && mouse_y < 100 && mouse_x < window_width - 150) {
                is_dragging = true;
            }
        } else {
            is_dragging = false;
        }
        if (is_dragging) {
            int xpos, ypos;
            glfwGetWindowPos(window, &xpos, &ypos);
            // simple drag - requires better win32 integration for smoothness, skipping for UI focus
        }

        // Calculate download speed and progress
        double current_time = glfwGetTime();
        if (current_time - last_time >= 0.5) { // Update speed every 0.5s
            uint64_t current_bytes = dl_progress.bytes_downloaded;
            float bytes_diff = (float)(current_bytes - last_bytes);
            current_speed_mb = (bytes_diff / 1024.0f / 1024.0f) / (current_time - last_time);
            last_time = current_time;
            last_bytes = current_bytes;
        }

        float actual_progress_ratio = (float)dl_progress.bytes_downloaded / (float)dl_progress.total_bytes;
        if (actual_progress_ratio > 1.0f) actual_progress_ratio = 1.0f;
        if (dl_progress.is_finished) actual_progress_ratio = 1.0f;

        // ANIMATIONS
        float dt = io.DeltaTime;
        
        // 1. Fade in globally over 1.5 seconds
        float global_alpha = (float)(current_time - start_time) / 1.5f;
        if (global_alpha > 1.0f) global_alpha = 1.0f;

        // 2. Smooth progress bar lerp
        visual_progress_ratio += (actual_progress_ratio - visual_progress_ratio) * 5.0f * dt;
        
        // 3. Pulsing effect
        float pulse = (sin(current_time * 4.0f) + 1.0f) * 0.5f; // 0 to 1

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2((float)window_width, (float)window_height));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | 
                                        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;
        
        ImGui::Begin("Main", nullptr, window_flags);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + window_width, p_min.y + window_height);
        
        // Alpha color helper
        auto applyAlpha = [&](ImU32 col, float alpha) -> ImU32 {
            int a = (int)(((col >> 24) & 0xFF) * alpha);
            return (col & 0x00FFFFFF) | (a << 24);
        };

        // Draw background
        draw_list->AddImageRounded((void*)(intptr_t)bg_texture, p_min, p_max, ImVec2(0,0), ImVec2(1,1), applyAlpha(IM_COL32(255,255,255,255), global_alpha), 15.0f);
        
        draw_list->AddRectFilledMultiColor(
            ImVec2(p_min.x, p_max.y - 300), p_max,
            applyAlpha(IM_COL32(0,0,0,0), global_alpha), applyAlpha(IM_COL32(0,0,0,0), global_alpha),
            applyAlpha(IM_COL32(0,0,0,220), global_alpha), applyAlpha(IM_COL32(0,0,0,220), global_alpha)
        );
        
        // Controls
        float btn_size = 30.0f;
        ImVec2 close_pos(window_width - btn_size - 10, 10);
        ImGui::SetCursorPos(close_pos);
        if (ImGui::InvisibleButton("Close", ImVec2(btn_size, btn_size))) glfwSetWindowShouldClose(window, GLFW_TRUE);
        bool close_hovered = ImGui::IsItemHovered();
        draw_list->AddText(font_regular_large, 20.0f, ImVec2(close_pos.x + 8, close_pos.y + 2), applyAlpha(close_hovered ? IM_COL32(255,100,100,255) : IM_COL32(200,200,200,255), global_alpha), "X");

        // Texts
        float text_x = 60.0f;
        float title_y = 400.0f;
        
        std::string title_text = dl_progress.is_finished ? (dl_progress.is_error ? u8"Güncelleme Hatası" : u8"Güncelleme Tamamlandı") : u8"Başlatıcı güncellemesi gerçekleşiyor";
        
        // Add subtle pulse to title if downloading
        float title_alpha = dl_progress.is_finished ? 1.0f : (0.8f + 0.2f * pulse);
        
        draw_list->AddText(font_bold, 32.0f, ImVec2(text_x, title_y), applyAlpha(IM_COL32(255, 255, 255, 255), global_alpha * title_alpha), title_text.c_str());
        
        float subtitle_y = title_y + 45.0f;
        std::string subtitle = dl_progress.is_finished ? (dl_progress.is_error ? dl_progress.error_message : u8"Oyuna giriş yapabilirsiniz.") : u8"Olası sorunları önlemek için Başlatıcıyı kapatmayın.\nBir hata olduğunu düşünüyorsanız, teknik destek ile iletişime geçmelisiniz.";
        draw_list->AddText(font_regular_large, 20.0f, ImVec2(text_x, subtitle_y), applyAlpha(IM_COL32(230, 230, 230, 255), global_alpha), subtitle.c_str());

        // Progress text
        char progress_buf[256];
        if (dl_progress.is_finished) {
            snprintf(progress_buf, sizeof(progress_buf), "100%%");
        } else {
            float mb_down = dl_progress.bytes_downloaded / 1024.0f / 1024.0f;
            float mb_total = dl_progress.total_bytes / 1024.0f / 1024.0f;
            snprintf(progress_buf, sizeof(progress_buf), "%.2f MB/s    %.2f MB из %.2f MB", current_speed_mb, mb_down, mb_total);
        }
        
        float text_width = font_regular_small->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, progress_buf).x;
        float progress_y = 540.0f;
        draw_list->AddText(font_regular_small, 15.0f, ImVec2(window_width - text_width - 60.0f, progress_y - 20.0f), applyAlpha(IM_COL32(200, 200, 200, 255), global_alpha), progress_buf);

        // Progress bar
        float pb_width = window_width - 120.0f;
        float pb_height = 16.0f;
        float pb_x = 60.0f;
        
        // Background track
        draw_list->AddRectFilled(ImVec2(pb_x, progress_y), ImVec2(pb_x + pb_width, progress_y + pb_height), applyAlpha(IM_COL32(80, 80, 80, 200), global_alpha), pb_height * 0.5f);
        
        // Foreground fill with glow/pulse effect on the color
        ImU32 fill_color = dl_progress.is_finished ? IM_COL32(100, 255, 100, 255) : IM_COL32(255 - (int)(50*pulse), 255 - (int)(50*pulse), 255, 255);
        if (visual_progress_ratio > 0.01f) {
            draw_list->AddRectFilled(ImVec2(pb_x, progress_y), ImVec2(pb_x + pb_width * visual_progress_ratio, progress_y + pb_height), applyAlpha(fill_color, global_alpha), pb_height * 0.5f);
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
