#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

bool     ImGui_ImplOpenGL3_Init();
void     ImGui_ImplOpenGL3_Shutdown();
void     ImGui_ImplOpenGL3_NewFrame();
void     ImGui_ImplOpenGL3_RenderDrawData(ImDrawData* draw_data);

// (Optional) Called by Init/NewFrame/Shutdown
bool     ImGui_ImplOpenGL3_CreateFontsTexture();
void     ImGui_ImplOpenGL3_DestroyFontsTexture();
bool     ImGui_ImplOpenGL3_CreateDeviceObjects();
void     ImGui_ImplOpenGL3_DestroyDeviceObjects();

#endif // #ifndef IMGUI_DISABLE
