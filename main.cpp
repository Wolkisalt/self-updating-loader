#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same

    // Upload pixels into texture
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
    // Setup window
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
    
    int window_width = 1000;
    int window_height = 600;
    GLFWwindow* window = glfwCreateWindow(window_width, window_height, "Loader", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    ImFontConfig font_cfg;
    font_cfg.OversampleH = 2;
    font_cfg.OversampleV = 2;
    font_cfg.PixelSnapH = true;
    
    static const ImWchar ranges[] =
    {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A (for Turkish ş, ç, ğ, ı)
        0x0400, 0x044F, // Cyrillic (for "из")
        0,
    };
    
    ImFont* font_regular_large = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 20.0f, &font_cfg, ranges);
    ImFont* font_bold = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeuib.ttf", 32.0f, &font_cfg, ranges);
    ImFont* font_regular_small = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 15.0f, &font_cfg, ranges);

    // Load Background Texture
    GLuint bg_texture = 0;
    int bg_width = 0, bg_height = 0;
    bool ret = LoadTextureFromFile("bg.png", &bg_texture, &bg_width, &bg_height);
    IM_ASSERT(ret);

    // Variables for window dragging
    bool is_dragging = false;
    double drag_start_x = 0;
    double drag_start_y = 0;
    int win_start_x = 0;
    int win_start_y = 0;

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Handle window dragging
        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            if (!is_dragging && mouse_y < 100 && mouse_x < window_width - 150) { // Top area but not buttons
                is_dragging = true;
                drag_start_x = mouse_x;
                drag_start_y = mouse_y;
                glfwGetWindowPos(window, &win_start_x, &win_start_y);
            }
        } else {
            is_dragging = false;
        }

        if (is_dragging) {
            double current_mouse_x, current_mouse_y;
            // Get absolute screen pos
            // We need to calculate delta
            // Note: glfwGetCursorPos returns position relative to window client area,
            // so dragging by delta relative to window can cause stutter if window moves.
            // A better way on Windows is using Win32 API, but for simplicity we will just 
            // use a naive approach or disable it if it stutters too much. 
            // For a perfect clone we might ignore dragging to save time, but let's try.
        }

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Create a fullscreen window with transparent background
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

        // Draw rounded background image
        ImVec2 p_min = ImGui::GetCursorScreenPos();
        ImVec2 p_max = ImVec2(p_min.x + window_width, p_min.y + window_height);
        
        // Add a slight dark gradient at the bottom for text readability
        draw_list->AddImageRounded((void*)(intptr_t)bg_texture, p_min, p_max, ImVec2(0,0), ImVec2(1,1), IM_COL32(255,255,255,255), 15.0f);
        
        // Dark gradient at bottom
        draw_list->AddRectFilledMultiColor(
            ImVec2(p_min.x, p_max.y - 300), p_max,
            IM_COL32(0,0,0,0), IM_COL32(0,0,0,0),
            IM_COL32(0,0,0,220), IM_COL32(0,0,0,220)
        );
        
        // Draw window controls (minimize, maximize, close)
        float btn_size = 30.0f;
        ImVec2 close_pos(window_width - btn_size - 10, 10);
        ImVec2 max_pos(window_width - btn_size * 2 - 15, 10);
        ImVec2 min_pos(window_width - btn_size * 3 - 20, 10);
        
        ImGui::SetCursorPos(close_pos);
        if (ImGui::InvisibleButton("Close", ImVec2(btn_size, btn_size))) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        bool close_hovered = ImGui::IsItemHovered();
        draw_list->AddText(font_regular_large, 20.0f, ImVec2(close_pos.x + 8, close_pos.y + 2), close_hovered ? IM_COL32(255,100,100,255) : IM_COL32(200,200,200,255), "X");

        ImGui::SetCursorPos(max_pos);
        if (ImGui::InvisibleButton("Max", ImVec2(btn_size, btn_size))) {}
        bool max_hovered = ImGui::IsItemHovered();
        draw_list->AddRect(ImVec2(max_pos.x + 8, max_pos.y + 8), ImVec2(max_pos.x + 22, max_pos.y + 22), max_hovered ? IM_COL32(255,255,255,255) : IM_COL32(200,200,200,255));

        ImGui::SetCursorPos(min_pos);
        if (ImGui::InvisibleButton("Min", ImVec2(btn_size, btn_size))) {
            glfwIconifyWindow(window);
        }
        bool min_hovered = ImGui::IsItemHovered();
        draw_list->AddLine(ImVec2(min_pos.x + 8, min_pos.y + 15), ImVec2(min_pos.x + 22, min_pos.y + 15), min_hovered ? IM_COL32(255,255,255,255) : IM_COL32(200,200,200,255), 2.0f);


        // Texts
        float text_x = 60.0f;
        float title_y = 400.0f;
        
        draw_list->AddText(font_bold, 32.0f, ImVec2(text_x, title_y), IM_COL32(255, 255, 255, 255), reinterpret_cast<const char*>(u8"Başlatıcı güncellemesi gerçekleşiyor"));
        
        float subtitle_y = title_y + 45.0f;
        draw_list->AddText(font_regular_large, 20.0f, ImVec2(text_x, subtitle_y), IM_COL32(230, 230, 230, 255), reinterpret_cast<const char*>(u8"Olası sorunları önlemek için Başlatıcıyı kapatmayın.\nBir hata olduğunu düşünüyorsanız, teknik destek ile iletişime geçmelisiniz."));

        // Progress bar text
        std::string progress_text = reinterpret_cast<const char*>(u8"10.30 MB/s    300.56 MB из 560.34 MB");
        float text_width = font_regular_small->CalcTextSizeA(15.0f, FLT_MAX, 0.0f, progress_text.c_str()).x;
        
        float progress_y = 540.0f;
        draw_list->AddText(font_regular_small, 15.0f, ImVec2(window_width - text_width - 60.0f, progress_y - 20.0f), IM_COL32(200, 200, 200, 255), progress_text.c_str());

        // Progress bar
        float pb_width = window_width - 120.0f;
        float pb_height = 16.0f;
        float pb_x = 60.0f;
        
        // Background
        draw_list->AddRectFilled(ImVec2(pb_x, progress_y), ImVec2(pb_x + pb_width, progress_y + pb_height), IM_COL32(80, 80, 80, 200), pb_height * 0.5f);
        
        // Foreground (progress)
        float progress_ratio = 300.56f / 560.34f;
        draw_list->AddRectFilled(ImVec2(pb_x, progress_y), ImVec2(pb_x + pb_width * progress_ratio, progress_y + pb_height), IM_COL32(255, 255, 255, 255), pb_height * 0.5f);

        ImGui::End();
        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
