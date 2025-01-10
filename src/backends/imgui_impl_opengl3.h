#pragma once
#include "imgui.h"      // IMGUI_IMPL_API
#ifndef IMGUI_DISABLE

bool     ImGuiGLInit();
bool     ImGuiGLInitContext();
void     ImGuiGLShutdown();
void     ImGuiGLShutdownContext();
void     ImGuiGLNewFrame();
void     ImGuiGLRenderDrawData(ImDrawData* drawData);

// (Optional) Called by Init/NewFrame/Shutdown
bool     ImGuiGLCreateShader();
void     ImGuiGLDestroyShader();
bool     ImGuiGLCreateFontsTexture();
void     ImGuiGLDestroyFontsTexture();
bool     ImGuiGLCreateDeviceObjects();
void     ImGuiGLDestroyDeviceObjects();

#endif // #ifndef IMGUI_DISABLE
