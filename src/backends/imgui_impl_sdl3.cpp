// dear imgui: Platform Backend for SDL3 (*EXPERIMENTAL*)
// This needs to be used along with a Renderer (e.g. DirectX11, OpenGL3,
// Vulkan..) (Info: SDL3 is a cross-platform general purpose library for
// handling windows, inputs, graphics context creation, etc.)

// (**IMPORTANT: SDL 3.0.0 is NOT YET RELEASED AND CURRENTLY HAS A FAST CHANGING
// API. THIS CODE BREAKS OFTEN AS SDL3 CHANGES.**)

// Implemented features:
//  [X] Platform: Clipboard support.
//  [X] Platform: Mouse support. Can discriminate Mouse/TouchScreen.
//  [X] Platform: Keyboard support. Since 1.87 we are using the io.AddKeyEvent()
//  function. Pass ImGuiKey values to all key functions e.g.
//  ImGui::IsKeyPressed(ImGuiKey_Space). [Legacy SDL_SCANCODE_* values are
//  obsolete since 1.87 and not supported since 1.91.5] [X] Platform: Gamepad
//  support. Enabled with 'io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad'.
//  [X] Platform: Mouse cursor shape and visibility
//  (ImGuiBackendFlags_HasMouseCursors). Disable with 'io.ConfigFlags |=
//  ImGuiConfigFlags_NoMouseCursorChange'. [x] Platform: Multi-viewport support
//  (multiple windows). Enable with 'io.ConfigFlags |=
//  ImGuiConfigFlags_ViewportsEnable' -> the OS animation effect when window
//  gets created/destroyed is problematic. SDL2 backend doesn't have issue.
// Missing features or Issues:
//  [ ] Platform: Multi-viewport: Minimized windows seems to break mouse wheel
//  events (at least under Windows). [x] Platform: IME support. Position somehow
//  broken in SDL3 + app needs to call 'SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");'
//  before SDL_CreateWindow()!.

// You can use unmodified imgui_impl_* files in your project. See examples/
// folder for examples of using this. Prefer including the entire imgui/
// repository into your project (either as a copy or as a submodule), and only
// build the backends you need. Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/
// folder).
// - Introduction, links and more at the top of imgui.cpp

// CHANGELOG
// (minor and older changes stripped away, please see git history for details)
//  2024-XX-XX: Platform: Added support for multiple windows via the
//  ImGuiPlatformIO interface. 2024-09-11: (Docking) Added support for
//  viewport->ParentViewportId field to support parenting at OS level. (#7973)
//  2024-10-24: Emscripten: SDL_EVENT_MOUSE_WHEEL event doesn't require dividing
//  by 100.0f on Emscripten. 2024-09-03: Update for SDL3 api changes:
//  SDL_GetGamepads() memory ownership revert. (#7918, #7898, #7807) 2024-08-22:
//  moved some OS/backend related function pointers from ImGuiIO to
//  ImGuiPlatformIO:
//               - io.GetClipboardTextFn    ->
//               platform_io.Platform_GetClipboardTextFn
//               - io.SetClipboardTextFn    ->
//               platform_io.Platform_SetClipboardTextFn
//               - io.PlatformSetImeDataFn  -> platform_io.Platform_SetImeDataFn
//  2024-08-19: Storing SDL_WindowID inside ImGuiViewport::PlatformHandle
//  instead of SDL_Window*. 2024-08-19: ImGui_ImplSDL3_ProcessEvent() now
//  ignores events intended for other SDL windows. (#7853) 2024-07-22: Update
//  for SDL3 api changes: SDL_GetGamepads() memory ownership change. (#7807)
//  2024-07-18: Update for SDL3 api changes: SDL_GetClipboardText() memory
//  ownership change. (#7801) 2024-07-15: Update for SDL3 api changes:
//  SDL_GetProperty() change to SDL_GetPointerProperty(). (#7794) 2024-07-02:
//  Update for SDL3 api changes: SDLK_x renames and SDLK_KP_x removals (#7761,
//  #7762). 2024-07-01: Update for SDL3 api changes: SDL_SetTextInputRect()
//  changed to SDL_SetTextInputArea(). 2024-06-26: Update for SDL3 api changes:
//  SDL_StartTextInput()/SDL_StopTextInput()/SDL_SetTextInputRect() functions
//  signatures. 2024-06-24: Update for SDL3 api changes:
//  SDL_EVENT_KEY_DOWN/SDL_EVENT_KEY_UP contents. 2024-06-03; Update for SDL3
//  api changes: SDL_SYSTEM_CURSOR_ renames. 2024-05-15: Update for SDL3 api
//  changes: SDLK_ renames. 2024-04-15: Inputs: Re-enable calling
//  SDL_StartTextInput()/SDL_StopTextInput() as SDL3 no longer enables it by
//  default and should play nicer with IME. 2024-02-13: Inputs: Fixed gamepad
//  support. Handle gamepad disconnection. Added
//  ImGui_ImplSDL3_SetGamepadMode(). 2023-11-13: Updated for recent SDL3 API
//  changes. 2023-10-05: Inputs: Added support for extra ImGuiKey values: F13 to
//  F24 function keys, app back/forward keys. 2023-05-04: Fixed build on
//  Emscripten/iOS/Android. (#6391) 2023-04-06: Inputs: Avoid calling
//  SDL_StartTextInput()/SDL_StopTextInput() as they don't only pertain to IME.
//  It's unclear exactly what their relation is to IME. (#6306) 2023-04-04:
//  Inputs: Added support for io.AddMouseSourceEvent() to discriminate
//  ImGuiMouseSource_Mouse/ImGuiMouseSource_TouchScreen. (#2702) 2023-02-23:
//  Accept SDL_GetPerformanceCounter() not returning a monotonically increasing
//  value. (#6189, #6114, #3644) 2023-02-07: Forked "imgui_impl_sdl2" into
//  "imgui_impl_sdl3". Removed version checks for old feature. Refer to
//  imgui_impl_sdl2.cpp for older changelog.

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_sdl3.h"

// Clang warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored                                               \
    "-Wimplicit-int-float-conversion" // warning: implicit conversion from 'xxx'
                                      // to 'float' may lose precision
#endif

// SDL
#include <SDL3/SDL.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if !defined(__EMSCRIPTEN__) && !defined(__ANDROID__)                          \
    && !(defined(__APPLE__) && TARGET_OS_IOS) && !defined(__amigaos4__)
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE 1
#else
#define SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE 0
#endif

// FIXME-LEGACY: remove when SDL 3.1.3 preview is released.
#ifndef SDLK_APOSTROPHE
#define SDLK_APOSTROPHE SDLK_QUOTE
#endif
#ifndef SDLK_GRAVE
#define SDLK_GRAVE SDLK_BACKQUOTE
#endif

// SDL Data
struct ImGuiSDL3Data
{
    SDL_Window*   window;
    SDL_WindowID  windowId;
    SDL_Renderer* renderer;
    Uint64        time;
    char*         clipboardTextData;
    bool          useVulkan;
    bool          wantUpdateMonitors;

    // IME handling
    SDL_Window* imeWindow;

    // Mouse handling
    Uint32      mouseWindowId;
    int         mouseButtonsDown;
    SDL_Cursor* mouseCursors[ImGuiMouseCursor_COUNT];
    SDL_Cursor* mouseLastCursor;
    int         mousePendingLeaveFrame;
    bool        mouseCanUseGlobalState;
    bool
        mouseCanReportHoveredViewport; // This is hard to use/unreliable on SDL
                                       // so we'll set
                                       // ImGuiBackendFlags_HasMouseHoveredViewport
                                       // dynamically based on state.

    // Gamepad handling
    ImVector<SDL_Gamepad*> gamepads;
    ImGuiSDL3GamepadMode   gamepadMode;
    bool                   wantUpdateGamepadsList;

    ImGuiSDL3Data() { memset((void*)this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for
// multiple Dear ImGui contexts It is STRONGLY preferred that you use docking
// branch with multi-viewports (== single Dear ImGui context + multiple windows)
// instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in
// this backend.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled
// when using multi-context.
static ImGuiSDL3Data* ImGuiSDL3GetBackendData()
{
    return ImGui::GetCurrentContext()
        ? (ImGuiSDL3Data*)ImGui::GetIO().BackendPlatformUserData
        : nullptr;
}

// Forward Declarations
static void ImGuiSDL3UpdateMonitors();
static void ImGuiSDL3InitMultiViewportSupport(SDL_Window* window,
                                              void*       sdlGlContext);
static void ImGuiSDL3ShutdownMultiViewportSupport();

// Functions
static const char* ImGuiSDL3GetClipboardText(ImGuiContext*)
{
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();
    if (bd->clipboardTextData)
        SDL_free(bd->clipboardTextData);
    const char* sdlClipboardText = SDL_GetClipboardText();
    bd->clipboardTextData
        = sdlClipboardText ? SDL_strdup(sdlClipboardText) : nullptr;
    return bd->clipboardTextData;
}

static void ImGuiSDL3SetClipboardText(ImGuiContext*, const char* text)
{
    SDL_SetClipboardText(text);
}

static void ImGuiSDL3PlatformSetImeData(ImGuiContext*, ImGuiViewport* viewport,
                                        ImGuiPlatformImeData* data)
{
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();
    SDL_WindowID   windowId = (SDL_WindowID)(intptr_t)viewport->PlatformHandle;
    SDL_Window*    window = SDL_GetWindowFromID(windowId);
    if ((data->WantVisible == false || bd->imeWindow != window)
        && bd->imeWindow != nullptr) {
        SDL_StopTextInput(bd->imeWindow);
        bd->imeWindow = nullptr;
    }
    if (data->WantVisible) {
        SDL_Rect r;
        r.x = (int)(data->InputPos.x - viewport->Pos.x);
        r.y = (int)(data->InputPos.y - viewport->Pos.y + data->InputLineHeight);
        r.w = 1;
        r.h = (int)data->InputLineHeight;
        SDL_SetTextInputArea(window, &r, 0);
        SDL_StartTextInput(window);
        bd->imeWindow = window;
    }
}

// Not static to allow third-party code to use that if they want to (but
// undocumented)
ImGuiKey ImGuiSDL3KeyEventToImGuiKey(SDL_Keycode  keycode,
                                     SDL_Scancode scancode);
ImGuiKey ImGuiSDL3KeyEventToImGuiKey(SDL_Keycode keycode, SDL_Scancode scancode)
{
    // Keypad doesn't have individual key values in SDL3
    switch (scancode) {
    case SDL_SCANCODE_KP_0:
        return ImGuiKey_Keypad0;
    case SDL_SCANCODE_KP_1:
        return ImGuiKey_Keypad1;
    case SDL_SCANCODE_KP_2:
        return ImGuiKey_Keypad2;
    case SDL_SCANCODE_KP_3:
        return ImGuiKey_Keypad3;
    case SDL_SCANCODE_KP_4:
        return ImGuiKey_Keypad4;
    case SDL_SCANCODE_KP_5:
        return ImGuiKey_Keypad5;
    case SDL_SCANCODE_KP_6:
        return ImGuiKey_Keypad6;
    case SDL_SCANCODE_KP_7:
        return ImGuiKey_Keypad7;
    case SDL_SCANCODE_KP_8:
        return ImGuiKey_Keypad8;
    case SDL_SCANCODE_KP_9:
        return ImGuiKey_Keypad9;
    case SDL_SCANCODE_KP_PERIOD:
        return ImGuiKey_KeypadDecimal;
    case SDL_SCANCODE_KP_DIVIDE:
        return ImGuiKey_KeypadDivide;
    case SDL_SCANCODE_KP_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
    case SDL_SCANCODE_KP_MINUS:
        return ImGuiKey_KeypadSubtract;
    case SDL_SCANCODE_KP_PLUS:
        return ImGuiKey_KeypadAdd;
    case SDL_SCANCODE_KP_ENTER:
        return ImGuiKey_KeypadEnter;
    case SDL_SCANCODE_KP_EQUALS:
        return ImGuiKey_KeypadEqual;
    default:
        break;
    }
    switch (keycode) {
    case SDLK_TAB:
        return ImGuiKey_Tab;
    case SDLK_LEFT:
        return ImGuiKey_LeftArrow;
    case SDLK_RIGHT:
        return ImGuiKey_RightArrow;
    case SDLK_UP:
        return ImGuiKey_UpArrow;
    case SDLK_DOWN:
        return ImGuiKey_DownArrow;
    case SDLK_PAGEUP:
        return ImGuiKey_PageUp;
    case SDLK_PAGEDOWN:
        return ImGuiKey_PageDown;
    case SDLK_HOME:
        return ImGuiKey_Home;
    case SDLK_END:
        return ImGuiKey_End;
    case SDLK_INSERT:
        return ImGuiKey_Insert;
    case SDLK_DELETE:
        return ImGuiKey_Delete;
    case SDLK_BACKSPACE:
        return ImGuiKey_Backspace;
    case SDLK_SPACE:
        return ImGuiKey_Space;
    case SDLK_RETURN:
        return ImGuiKey_Enter;
    case SDLK_ESCAPE:
        return ImGuiKey_Escape;
    case SDLK_APOSTROPHE:
        return ImGuiKey_Apostrophe;
    case SDLK_COMMA:
        return ImGuiKey_Comma;
    case SDLK_MINUS:
        return ImGuiKey_Minus;
    case SDLK_PERIOD:
        return ImGuiKey_Period;
    case SDLK_SLASH:
        return ImGuiKey_Slash;
    case SDLK_SEMICOLON:
        return ImGuiKey_Semicolon;
    case SDLK_EQUALS:
        return ImGuiKey_Equal;
    case SDLK_LEFTBRACKET:
        return ImGuiKey_LeftBracket;
    case SDLK_BACKSLASH:
        return ImGuiKey_Backslash;
    case SDLK_RIGHTBRACKET:
        return ImGuiKey_RightBracket;
    case SDLK_GRAVE:
        return ImGuiKey_GraveAccent;
    case SDLK_CAPSLOCK:
        return ImGuiKey_CapsLock;
    case SDLK_SCROLLLOCK:
        return ImGuiKey_ScrollLock;
    case SDLK_NUMLOCKCLEAR:
        return ImGuiKey_NumLock;
    case SDLK_PRINTSCREEN:
        return ImGuiKey_PrintScreen;
    case SDLK_PAUSE:
        return ImGuiKey_Pause;
    case SDLK_LCTRL:
        return ImGuiKey_LeftCtrl;
    case SDLK_LSHIFT:
        return ImGuiKey_LeftShift;
    case SDLK_LALT:
        return ImGuiKey_LeftAlt;
    case SDLK_LGUI:
        return ImGuiKey_LeftSuper;
    case SDLK_RCTRL:
        return ImGuiKey_RightCtrl;
    case SDLK_RSHIFT:
        return ImGuiKey_RightShift;
    case SDLK_RALT:
        return ImGuiKey_RightAlt;
    case SDLK_RGUI:
        return ImGuiKey_RightSuper;
    case SDLK_APPLICATION:
        return ImGuiKey_Menu;
    case SDLK_0:
        return ImGuiKey_0;
    case SDLK_1:
        return ImGuiKey_1;
    case SDLK_2:
        return ImGuiKey_2;
    case SDLK_3:
        return ImGuiKey_3;
    case SDLK_4:
        return ImGuiKey_4;
    case SDLK_5:
        return ImGuiKey_5;
    case SDLK_6:
        return ImGuiKey_6;
    case SDLK_7:
        return ImGuiKey_7;
    case SDLK_8:
        return ImGuiKey_8;
    case SDLK_9:
        return ImGuiKey_9;
    case SDLK_A:
        return ImGuiKey_A;
    case SDLK_B:
        return ImGuiKey_B;
    case SDLK_C:
        return ImGuiKey_C;
    case SDLK_D:
        return ImGuiKey_D;
    case SDLK_E:
        return ImGuiKey_E;
    case SDLK_F:
        return ImGuiKey_F;
    case SDLK_G:
        return ImGuiKey_G;
    case SDLK_H:
        return ImGuiKey_H;
    case SDLK_I:
        return ImGuiKey_I;
    case SDLK_J:
        return ImGuiKey_J;
    case SDLK_K:
        return ImGuiKey_K;
    case SDLK_L:
        return ImGuiKey_L;
    case SDLK_M:
        return ImGuiKey_M;
    case SDLK_N:
        return ImGuiKey_N;
    case SDLK_O:
        return ImGuiKey_O;
    case SDLK_P:
        return ImGuiKey_P;
    case SDLK_Q:
        return ImGuiKey_Q;
    case SDLK_R:
        return ImGuiKey_R;
    case SDLK_S:
        return ImGuiKey_S;
    case SDLK_T:
        return ImGuiKey_T;
    case SDLK_U:
        return ImGuiKey_U;
    case SDLK_V:
        return ImGuiKey_V;
    case SDLK_W:
        return ImGuiKey_W;
    case SDLK_X:
        return ImGuiKey_X;
    case SDLK_Y:
        return ImGuiKey_Y;
    case SDLK_Z:
        return ImGuiKey_Z;
    case SDLK_F1:
        return ImGuiKey_F1;
    case SDLK_F2:
        return ImGuiKey_F2;
    case SDLK_F3:
        return ImGuiKey_F3;
    case SDLK_F4:
        return ImGuiKey_F4;
    case SDLK_F5:
        return ImGuiKey_F5;
    case SDLK_F6:
        return ImGuiKey_F6;
    case SDLK_F7:
        return ImGuiKey_F7;
    case SDLK_F8:
        return ImGuiKey_F8;
    case SDLK_F9:
        return ImGuiKey_F9;
    case SDLK_F10:
        return ImGuiKey_F10;
    case SDLK_F11:
        return ImGuiKey_F11;
    case SDLK_F12:
        return ImGuiKey_F12;
    case SDLK_F13:
        return ImGuiKey_F13;
    case SDLK_F14:
        return ImGuiKey_F14;
    case SDLK_F15:
        return ImGuiKey_F15;
    case SDLK_F16:
        return ImGuiKey_F16;
    case SDLK_F17:
        return ImGuiKey_F17;
    case SDLK_F18:
        return ImGuiKey_F18;
    case SDLK_F19:
        return ImGuiKey_F19;
    case SDLK_F20:
        return ImGuiKey_F20;
    case SDLK_F21:
        return ImGuiKey_F21;
    case SDLK_F22:
        return ImGuiKey_F22;
    case SDLK_F23:
        return ImGuiKey_F23;
    case SDLK_F24:
        return ImGuiKey_F24;
    case SDLK_AC_BACK:
        return ImGuiKey_AppBack;
    case SDLK_AC_FORWARD:
        return ImGuiKey_AppForward;
    default:
        break;
    }
    return ImGuiKey_None;
}

static void ImGuiSDL3UpdateKeyModifiers(SDL_Keymod sdlKeyMods)
{
    ImGuiIO& io = ImGui::GetIO();
    io.AddKeyEvent(ImGuiMod_Ctrl, (sdlKeyMods & SDL_KMOD_CTRL) != 0);
    io.AddKeyEvent(ImGuiMod_Shift, (sdlKeyMods & SDL_KMOD_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (sdlKeyMods & SDL_KMOD_ALT) != 0);
    io.AddKeyEvent(ImGuiMod_Super, (sdlKeyMods & SDL_KMOD_GUI) != 0);
}

static ImGuiViewport* ImGuiSDL3GetViewportForWindowID(SDL_WindowID windowId)
{
    return ImGui::FindViewportByPlatformHandle((void*)(intptr_t)windowId);
}

// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if
// dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your
// main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to
// your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from
// your application based on those two flags.
bool ImGuiSDL3ProcessEvent(const SDL_Event* event)
{
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();
    IM_ASSERT(bd != nullptr
              && "Context or backend not initialized! Did you call "
                 "ImGui_ImplSDL3_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    switch (event->type) {
    case SDL_EVENT_MOUSE_MOTION: {
        if (ImGuiSDL3GetViewportForWindowID(event->motion.windowID) == nullptr)
            return false;
        ImVec2 mousePos((float)event->motion.x, (float)event->motion.y);
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            int windowX, windowY;
            SDL_GetWindowPosition(SDL_GetWindowFromID(event->motion.windowID),
                                  &windowX, &windowY);
            mousePos.x += windowX;
            mousePos.y += windowY;
        }
        io.AddMouseSourceEvent(event->motion.which == SDL_TOUCH_MOUSEID
                                   ? ImGuiMouseSource_TouchScreen
                                   : ImGuiMouseSource_Mouse);
        io.AddMousePosEvent(mousePos.x, mousePos.y);
        return true;
    }
    case SDL_EVENT_MOUSE_WHEEL: {
        if (ImGuiSDL3GetViewportForWindowID(event->wheel.windowID) == nullptr)
            return false;
        // IMGUI_DEBUG_LOG("wheel %.2f %.2f, precise %.2f %.2f\n",
        // (float)event->wheel.x, (float)event->wheel.y, event->wheel.preciseX,
        // event->wheel.preciseY);
        float wheelX = -event->wheel.x;
        float wheelY = event->wheel.y;
        io.AddMouseSourceEvent(event->wheel.which == SDL_TOUCH_MOUSEID
                                   ? ImGuiMouseSource_TouchScreen
                                   : ImGuiMouseSource_Mouse);
        io.AddMouseWheelEvent(wheelX, wheelY);
        return true;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP: {
        if (ImGuiSDL3GetViewportForWindowID(event->button.windowID) == nullptr)
            return false;
        int mouseButton = -1;
        if (event->button.button == SDL_BUTTON_LEFT) {
            mouseButton = 0;
        }
        if (event->button.button == SDL_BUTTON_RIGHT) {
            mouseButton = 1;
        }
        if (event->button.button == SDL_BUTTON_MIDDLE) {
            mouseButton = 2;
        }
        if (event->button.button == SDL_BUTTON_X1) {
            mouseButton = 3;
        }
        if (event->button.button == SDL_BUTTON_X2) {
            mouseButton = 4;
        }
        if (mouseButton == -1)
            break;
        io.AddMouseSourceEvent(event->button.which == SDL_TOUCH_MOUSEID
                                   ? ImGuiMouseSource_TouchScreen
                                   : ImGuiMouseSource_Mouse);
        io.AddMouseButtonEvent(mouseButton,
                               (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN));
        bd->mouseButtonsDown = (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN)
            ? (bd->mouseButtonsDown | (1 << mouseButton))
            : (bd->mouseButtonsDown & ~(1 << mouseButton));
        return true;
    }
    case SDL_EVENT_TEXT_INPUT: {
        if (ImGuiSDL3GetViewportForWindowID(event->text.windowID) == nullptr)
            return false;
        io.AddInputCharactersUTF8(event->text.text);
        return true;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
        if (ImGuiSDL3GetViewportForWindowID(event->key.windowID) == nullptr)
            return false;
        // IMGUI_DEBUG_LOG("SDL_EVENT_KEY_%d: key=%d, scancode=%d, mod=%X\n",
        // (event->type == SDL_EVENT_KEY_DOWN) ? "DOWN" : "UP", event->key.key,
        // event->key.scancode, event->key.mod);
        ImGuiSDL3UpdateKeyModifiers((SDL_Keymod)event->key.mod);
        ImGuiKey key
            = ImGuiSDL3KeyEventToImGuiKey(event->key.key, event->key.scancode);
        io.AddKeyEvent(key, (event->type == SDL_EVENT_KEY_DOWN));
        io.SetKeyEventNativeData(
            key, event->key.key, event->key.scancode,
            event->key.scancode); // To support legacy indexing (<1.87 user
                                  // code). Legacy backend uses SDLK_*** as
                                  // indices to IsKeyXXX() functions.
        return true;
    }
    case SDL_EVENT_DISPLAY_ORIENTATION:
    case SDL_EVENT_DISPLAY_ADDED:
    case SDL_EVENT_DISPLAY_REMOVED:
    case SDL_EVENT_DISPLAY_MOVED:
    case SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED: {
        bd->wantUpdateMonitors = true;
        return true;
    }
    case SDL_EVENT_WINDOW_MOUSE_ENTER: {
        if (ImGuiSDL3GetViewportForWindowID(event->window.windowID) == nullptr)
            return false;
        bd->mouseWindowId = event->window.windowID;
        bd->mousePendingLeaveFrame = 0;
        return true;
    }
    // - In some cases, when detaching a window from main viewport SDL may send
    // SDL_WINDOWEVENT_ENTER one frame too late,
    //   causing SDL_WINDOWEVENT_LEAVE on previous frame to interrupt drag
    //   operation by clear mouse position. This is why we delay process the
    //   SDL_WINDOWEVENT_LEAVE events by one frame. See issue #5012 for details.
    // FIXME: Unconfirmed whether this is still needed with SDL3.
    case SDL_EVENT_WINDOW_MOUSE_LEAVE: {
        if (ImGuiSDL3GetViewportForWindowID(event->window.windowID) == nullptr)
            return false;
        bd->mousePendingLeaveFrame = ImGui::GetFrameCount() + 1;
        return true;
    }
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    case SDL_EVENT_WINDOW_FOCUS_LOST: {
        if (ImGuiSDL3GetViewportForWindowID(event->window.windowID) == nullptr)
            return false;
        io.AddFocusEvent(event->type == SDL_EVENT_WINDOW_FOCUS_GAINED);
        return true;
    }
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_WINDOW_MOVED:
    case SDL_EVENT_WINDOW_RESIZED: {
        ImGuiViewport* viewport
            = ImGuiSDL3GetViewportForWindowID(event->window.windowID);
        if (viewport == NULL)
            return false;
        if (event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
            viewport->PlatformRequestClose = true;
        if (event->type == SDL_EVENT_WINDOW_MOVED)
            viewport->PlatformRequestMove = true;
        if (event->type == SDL_EVENT_WINDOW_RESIZED)
            viewport->PlatformRequestResize = true;
        return true;
    }
    case SDL_EVENT_GAMEPAD_ADDED:
    case SDL_EVENT_GAMEPAD_REMOVED: {
        bd->wantUpdateGamepadsList = true;
        return true;
    }
    }
    return false;
}

static void ImGuiSDL3SetupPlatformHandles(ImGuiViewport* viewport,
                                          SDL_Window*    window)
{
    viewport->PlatformHandle = (void*)(intptr_t)SDL_GetWindowID(window);
    viewport->PlatformHandleRaw = nullptr;
#if defined(_WIN32) && !defined(__WINRT__)
    viewport->PlatformHandleRaw = (HWND)SDL_GetPointerProperty(
        SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER,
        nullptr);
#elif defined(__APPLE__) && defined(SDL_VIDEO_DRIVER_COCOA)
    viewport->PlatformHandleRaw
        = SDL_GetPointerProperty(SDL_GetWindowProperties(window),
                                 SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
#endif
}

static bool ImGuiSDL3Init(SDL_Window* window, SDL_Renderer* renderer,
                          void* sdlGlContext)
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendPlatformUserData == nullptr
              && "Already initialized a platform backend!");
    IM_UNUSED(sdlGlContext); // Unused in this branch

    // Check and store if we are on a SDL backend that supports global mouse
    // position
    // ("wayland" and "rpi" don't support it, but we chose to use a white-list
    // instead of a black-list)
    bool mouseCanUseGlobalState = false;
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    const char* sdlBackend = SDL_GetCurrentVideoDriver();
    const char* globalMouseWhitelist[]
        = { "windows", "cocoa", "x11", "DIVE", "VMAN" };
    for (int n = 0; n < IM_ARRAYSIZE(globalMouseWhitelist); n++)
        if (strncmp(sdlBackend, globalMouseWhitelist[n],
                    strlen(globalMouseWhitelist[n]))
            == 0)
            mouseCanUseGlobalState = true;
#endif

    // Setup backend capabilities flags
    ImGuiSDL3Data* bd = IM_NEW(ImGuiSDL3Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_sdl3";
    io.BackendFlags
        |= ImGuiBackendFlags_HasMouseCursors; // We can honor GetMouseCursor()
                                              // values (optional)
    io.BackendFlags
        |= ImGuiBackendFlags_HasSetMousePos; // We can honor io.WantSetMousePos
                                             // requests (optional, rarely used)
    if (mouseCanUseGlobalState)
        io.BackendFlags
            |= ImGuiBackendFlags_PlatformHasViewports; // We can create
                                                       // multi-viewports on the
                                                       // Platform side
                                                       // (optional)

    bd->window = window;
    bd->windowId = SDL_GetWindowID(window);
    bd->renderer = renderer;

    // SDL on Linux/OSX doesn't report events for unfocused windows (see
    // https://github.com/ocornut/imgui/issues/4960) We will use
    // 'MouseCanReportHoveredViewport' to set
    // 'ImGuiBackendFlags_HasMouseHoveredViewport' dynamically each frame.
    bd->mouseCanUseGlobalState = mouseCanUseGlobalState;
#ifndef __APPLE__
    bd->mouseCanReportHoveredViewport = bd->mouseCanUseGlobalState;
#else
    bd->MouseCanReportHoveredViewport = false;
#endif

    ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    platformIo.Platform_SetClipboardTextFn = ImGuiSDL3SetClipboardText;
    platformIo.Platform_GetClipboardTextFn = ImGuiSDL3GetClipboardText;
    platformIo.Platform_SetImeDataFn = ImGuiSDL3PlatformSetImeData;

    // Update monitor a first time during init
    ImGuiSDL3UpdateMonitors();

    // Gamepad handling
    bd->gamepadMode = ImGuiSDL3_GamepadMode_AutoFirst;
    bd->wantUpdateGamepadsList = true;

    // Load mouse cursors
    bd->mouseCursors[ImGuiMouseCursor_Arrow]
        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_DEFAULT);
    bd->mouseCursors[ImGuiMouseCursor_TextInput]
        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_TEXT);
    bd->mouseCursors[ImGuiMouseCursor_ResizeAll]
        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    bd->mouseCursors[ImGuiMouseCursor_ResizeNS]
        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NS_RESIZE);
    bd->mouseCursors[ImGuiMouseCursor_ResizeEW]
        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_EW_RESIZE);
    bd->mouseCursors[ImGuiMouseCursor_ResizeNESW]
        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NESW_RESIZE);
    bd->mouseCursors[ImGuiMouseCursor_ResizeNWSE]
        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NWSE_RESIZE);
    bd->mouseCursors[ImGuiMouseCursor_Hand]
        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_POINTER);
    bd->mouseCursors[ImGuiMouseCursor_NotAllowed]
        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_NOT_ALLOWED);

    // Set platform dependent data in viewport
    // Our mouse update function expect PlatformHandle to be filled for the main
    // viewport
    ImGuiViewport* mainViewport = ImGui::GetMainViewport();
    ImGuiSDL3SetupPlatformHandles(mainViewport, window);

    // From 2.0.5: Set SDL hint to receive mouse click events on window focus,
    // otherwise SDL doesn't emit the event. Without this, when clicking to gain
    // focus, our widgets wouldn't activate even though they showed as hovered.
    // (This is unfortunately a global SDL setting, so enabling it might have a
    // side-effect on your application. It is unlikely to make a difference, but
    // if your app absolutely needs to ignore the initial on-focus click: you
    // can ignore SDL_EVENT_MOUSE_BUTTON_DOWN events coming right after a
    // SDL_WINDOWEVENT_FOCUS_GAINED)
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");

    // From 2.0.22: Disable auto-capture, this is preventing drag and drop
    // across multiple windows (see #5710)
    SDL_SetHint(SDL_HINT_MOUSE_AUTO_CAPTURE, "0");

    // SDL 3.x : see https://github.com/libsdl-org/SDL/issues/6659
    SDL_SetHint("SDL_BORDERLESS_WINDOWED_STYLE", "0");

    // We need SDL_CaptureMouse(), SDL_GetGlobalMouseState() from SDL 2.0.4+ to
    // support multiple viewports. We left the call to
    // ImGui_ImplSDL3_InitPlatformInterface() outside of #ifdef to avoid
    // unused-function warnings.
    if (io.BackendFlags & ImGuiBackendFlags_PlatformHasViewports)
        ImGuiSDL3InitMultiViewportSupport(window, sdlGlContext);

    return true;
}

// Should technically be a SDL_GLContext but due to typedef it is sane to keep
// it void* in public interface.
bool ImGuiSDL3InitForOpenGL(SDL_Window* window, void* sdlGlContext)
{
    return ImGuiSDL3Init(window, nullptr, sdlGlContext);
}

bool ImGuiSDL3InitForVulkan(SDL_Window* window)
{
    if (!ImGuiSDL3Init(window, nullptr, nullptr))
        return false;
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();
    bd->useVulkan = true;
    return true;
}

bool ImGuiImplSDL3InitForD3D(SDL_Window* window)
{
#if !defined(_WIN32)
    IM_ASSERT(0 && "Unsupported");
#endif
    return ImGuiSDL3Init(window, nullptr, nullptr);
}

bool ImGuiSDL3InitForMetal(SDL_Window* window)
{
    return ImGuiSDL3Init(window, nullptr, nullptr);
}

bool ImGuiSDL3InitForSDLRenderer(SDL_Window* window, SDL_Renderer* renderer)
{
    return ImGuiSDL3Init(window, renderer, nullptr);
}

bool ImGuiSDL3InitForOther(SDL_Window* window)
{
    return ImGuiSDL3Init(window, nullptr, nullptr);
}

static void ImGuiSDL3CloseGamepads();

void ImGuiSDL3Shutdown()
{
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();
    IM_ASSERT(bd != nullptr
              && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGuiSDL3ShutdownMultiViewportSupport();

    if (bd->clipboardTextData)
        SDL_free(bd->clipboardTextData);
    for (ImGuiMouseCursor cursorN = 0; cursorN < ImGuiMouseCursor_COUNT;
         cursorN++)
        SDL_DestroyCursor(bd->mouseCursors[cursorN]);
    ImGuiSDL3CloseGamepads();

    io.BackendPlatformName = nullptr;
    io.BackendPlatformUserData = nullptr;
    io.BackendFlags &= ~(
        ImGuiBackendFlags_HasMouseCursors | ImGuiBackendFlags_HasSetMousePos
        | ImGuiBackendFlags_HasGamepad | ImGuiBackendFlags_PlatformHasViewports
        | ImGuiBackendFlags_HasMouseHoveredViewport);
    IM_DELETE(bd);
}

// This code is incredibly messy because some of the functions we need for full
// viewport support are not available in SDL < 2.0.4.
static void ImGuiSDL3UpdateMouseData()
{
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();
    ImGuiIO&       io = ImGui::GetIO();

    // We forward mouse input when hovered or captured (via
    // SDL_EVENT_MOUSE_MOTION) or when focused (below)
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
    // SDL_CaptureMouse() let the OS know e.g. that our imgui drag outside the
    // SDL window boundaries shouldn't e.g. trigger other operations outside
    SDL_CaptureMouse(bd->mouseButtonsDown != 0);
    SDL_Window* focusedWindow = SDL_GetKeyboardFocus();
    const bool  isAppFocused = (focusedWindow
                               && (bd->window == focusedWindow
                                   || ImGuiSDL3GetViewportForWindowID(
                                          SDL_GetWindowID(focusedWindow))
                                       != NULL));
#else
    SDL_Window* focused_window = bd->Window;
    const bool  is_app_focused
        = (SDL_GetWindowFlags(bd->Window) & SDL_WINDOW_INPUT_FOCUS)
        != 0; // SDL 2.0.3 and non-windowed systems: single-viewport only
#endif
    if (isAppFocused) {
        // (Optional) Set OS mouse position from Dear ImGui if requested (rarely
        // used, only when io.ConfigNavMoveSetMousePos is enabled by user)
        if (io.WantSetMousePos) {
#if SDL_HAS_CAPTURE_AND_GLOBAL_MOUSE
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
                SDL_WarpMouseGlobal(io.MousePos.x, io.MousePos.y);
            else
#endif
                SDL_WarpMouseInWindow(bd->window, io.MousePos.x, io.MousePos.y);
        }

        // (Optional) Fallback to provide mouse position when focused
        // (SDL_EVENT_MOUSE_MOTION already provides this when hovered or
        // captured)
        if (bd->mouseCanUseGlobalState && bd->mouseButtonsDown == 0) {
            // Single-viewport mode: mouse position in client window coordinates
            // (io.MousePos is (0,0) when the mouse is on the upper-left corner
            // of the app window) Multi-viewport mode: mouse position in OS
            // absolute coordinates (io.MousePos is (0,0) when the mouse is on
            // the upper-left of the primary monitor)
            float mouseX, mouseY;
            int   windowX, windowY;
            SDL_GetGlobalMouseState(&mouseX, &mouseY);
            if (!(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)) {
                SDL_GetWindowPosition(focusedWindow, &windowX, &windowY);
                mouseX -= windowX;
                mouseY -= windowY;
            }
            io.AddMousePosEvent((float)mouseX, (float)mouseY);
        }
    }

    // (Optional) When using multiple viewports: call io.AddMouseViewportEvent()
    // with the viewport the OS mouse cursor is hovering. If
    // ImGuiBackendFlags_HasMouseHoveredViewport is not set by the backend, Dear
    // imGui will ignore this field and infer the information using its flawed
    // heuristic.
    // - [!] SDL backend does NOT correctly ignore viewports with the _NoInputs
    // flag.
    //       Some backend are not able to handle that correctly. If a backend
    //       report an hovered viewport that has the _NoInputs flag (e.g. when
    //       dragging a window for docking, the viewport has the _NoInputs flag
    //       in order to allow us to find the viewport under), then Dear ImGui
    //       is forced to ignore the value reported by the backend, and use its
    //       flawed heuristic to guess the viewport behind.
    // - [X] SDL backend correctly reports this regardless of another viewport
    // behind focused and dragged from (we need this to find a useful drag and
    // drop target).
    if (io.BackendFlags & ImGuiBackendFlags_HasMouseHoveredViewport) {
        ImGuiID mouseViewportId = 0;
        if (ImGuiViewport* mouseViewport
            = ImGuiSDL3GetViewportForWindowID(bd->mouseWindowId))
            mouseViewportId = mouseViewport->ID;
        io.AddMouseViewportEvent(mouseViewportId);
    }
}

static void ImGuiSDL3UpdateMouseCursor()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
        return;
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();

    ImGuiMouseCursor imguiCursor = ImGui::GetMouseCursor();
    if (io.MouseDrawCursor || imguiCursor == ImGuiMouseCursor_None) {
        // Hide OS mouse cursor if imgui is drawing it or if it wants no cursor
        SDL_HideCursor();
    } else {
        // Show OS mouse cursor
        SDL_Cursor* expectedCursor = bd->mouseCursors[imguiCursor]
            ? bd->mouseCursors[imguiCursor]
            : bd->mouseCursors[ImGuiMouseCursor_Arrow];
        if (bd->mouseLastCursor != expectedCursor) {
            SDL_SetCursor(expectedCursor); // SDL function doesn't have an early
                                           // out (see #6113)
            bd->mouseLastCursor = expectedCursor;
        }
        SDL_ShowCursor();
    }
}

static void ImGuiSDL3CloseGamepads()
{
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();
    if (bd->gamepadMode != ImGui_ImplSDL3_GamepadMode_Manual)
        for (SDL_Gamepad* gamepad : bd->gamepads)
            SDL_CloseGamepad(gamepad);
    bd->gamepads.resize(0);
}

void ImGuiSDL3SetGamepadMode(ImGuiSDL3GamepadMode mode,
                             SDL_Gamepad**        manualGamepadsArray,
                             int                  manualGamepadsCount)
{
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();
    ImGuiSDL3CloseGamepads();
    if (mode == ImGui_ImplSDL3_GamepadMode_Manual) {
        IM_ASSERT(manualGamepadsArray != nullptr && manualGamepadsCount > 0);
        for (int n = 0; n < manualGamepadsCount; n++)
            bd->gamepads.push_back(manualGamepadsArray[n]);
    } else {
        IM_ASSERT(manualGamepadsArray == nullptr && manualGamepadsCount <= 0);
        bd->wantUpdateGamepadsList = true;
    }
    bd->gamepadMode = mode;
}

static void ImGuiSDL3UpdateGamepadButton(ImGuiSDL3Data* bd, ImGuiIO& io,
                                         ImGuiKey          key,
                                         SDL_GamepadButton buttonNo)
{
    bool mergedValue = false;
    for (SDL_Gamepad* gamepad : bd->gamepads)
        mergedValue |= SDL_GetGamepadButton(gamepad, buttonNo) != 0;
    io.AddKeyEvent(key, mergedValue);
}

static inline float Saturate(float v)
{
    return v < 0.0f ? 0.0f : v > 1.0f ? 1.0f : v;
}
static void ImGuiSDL3UpdateGamepadAnalog(ImGuiSDL3Data* bd, ImGuiIO& io,
                                         ImGuiKey key, SDL_GamepadAxis axisNo,
                                         float v0, float v1)
{
    float mergedValue = 0.0f;
    for (SDL_Gamepad* gamepad : bd->gamepads) {
        float vn = Saturate((float)(SDL_GetGamepadAxis(gamepad, axisNo) - v0)
                            / (float)(v1 - v0));
        if (mergedValue < vn)
            mergedValue = vn;
    }
    io.AddKeyAnalogEvent(key, mergedValue > 0.1f, mergedValue);
}

static void ImGuiSDL3UpdateGamepads()
{
    ImGuiIO&       io = ImGui::GetIO();
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();

    // Update list of gamepads to use
    if (bd->wantUpdateGamepadsList
        && bd->gamepadMode != ImGui_ImplSDL3_GamepadMode_Manual) {
        ImGuiSDL3CloseGamepads();
        int             sdlGamepadsCount = 0;
        SDL_JoystickID* sdlGamepads = SDL_GetGamepads(&sdlGamepadsCount);
        for (int n = 0; n < sdlGamepadsCount; n++)
            if (SDL_Gamepad* gamepad = SDL_OpenGamepad(sdlGamepads[n])) {
                bd->gamepads.push_back(gamepad);
                if (bd->gamepadMode == ImGuiSDL3_GamepadMode_AutoFirst)
                    break;
            }
        bd->wantUpdateGamepadsList = false;
        SDL_free(sdlGamepads);
    }

    // FIXME: Technically feeding gamepad shouldn't depend on this now that they
    // are regular inputs.
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;
    io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
    if (bd->gamepads.Size == 0)
        return;
    io.BackendFlags |= ImGuiBackendFlags_HasGamepad;

    // Update gamepad inputs
    const int thumbDeadZone = 8000; // SDL_gamepad.h suggests using this value.
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadStart,
                                 SDL_GAMEPAD_BUTTON_START);
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadBack,
                                 SDL_GAMEPAD_BUTTON_BACK);
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadFaceLeft,
                                 SDL_GAMEPAD_BUTTON_WEST); // Xbox X, PS Square
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadFaceRight,
                                 SDL_GAMEPAD_BUTTON_EAST); // Xbox B, PS Circle
    ImGuiSDL3UpdateGamepadButton(
        bd, io, ImGuiKey_GamepadFaceUp,
        SDL_GAMEPAD_BUTTON_NORTH); // Xbox Y, PS Triangle
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadFaceDown,
                                 SDL_GAMEPAD_BUTTON_SOUTH); // Xbox A, PS Cross
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadDpadLeft,
                                 SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadDpadRight,
                                 SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadDpadUp,
                                 SDL_GAMEPAD_BUTTON_DPAD_UP);
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadDpadDown,
                                 SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadL1,
                                 SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadR1,
                                 SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadL2,
                                 SDL_GAMEPAD_AXIS_LEFT_TRIGGER, 0.0f, 32767);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadR2,
                                 SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, 0.0f, 32767);
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadL3,
                                 SDL_GAMEPAD_BUTTON_LEFT_STICK);
    ImGuiSDL3UpdateGamepadButton(bd, io, ImGuiKey_GamepadR3,
                                 SDL_GAMEPAD_BUTTON_RIGHT_STICK);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadLStickLeft,
                                 SDL_GAMEPAD_AXIS_LEFTX, -thumbDeadZone,
                                 -32768);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadLStickRight,
                                 SDL_GAMEPAD_AXIS_LEFTX, +thumbDeadZone,
                                 +32767);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadLStickUp,
                                 SDL_GAMEPAD_AXIS_LEFTY, -thumbDeadZone,
                                 -32768);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadLStickDown,
                                 SDL_GAMEPAD_AXIS_LEFTY, +thumbDeadZone,
                                 +32767);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadRStickLeft,
                                 SDL_GAMEPAD_AXIS_RIGHTX, -thumbDeadZone,
                                 -32768);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadRStickRight,
                                 SDL_GAMEPAD_AXIS_RIGHTX, +thumbDeadZone,
                                 +32767);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadRStickUp,
                                 SDL_GAMEPAD_AXIS_RIGHTY, -thumbDeadZone,
                                 -32768);
    ImGuiSDL3UpdateGamepadAnalog(bd, io, ImGuiKey_GamepadRStickDown,
                                 SDL_GAMEPAD_AXIS_RIGHTY, +thumbDeadZone,
                                 +32767);
}

static void ImGuiSDL3UpdateMonitors()
{
    ImGuiSDL3Data*   bd = ImGuiSDL3GetBackendData();
    ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    platformIo.Monitors.resize(0);
    bd->wantUpdateMonitors = false;

    int            displayCount;
    SDL_DisplayID* displays = SDL_GetDisplays(&displayCount);
    for (int n = 0; n < displayCount; n++) {
        // Warning: the validity of monitor DPI information on Windows depends
        // on the application DPI awareness settings, which generally needs to
        // be set in the manifest or at runtime.
        SDL_DisplayID        displayId = displays[n];
        ImGuiPlatformMonitor monitor;
        SDL_Rect             r;
        SDL_GetDisplayBounds(displayId, &r);
        monitor.MainPos = monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
        monitor.MainSize = monitor.WorkSize = ImVec2((float)r.w, (float)r.h);
        SDL_GetDisplayUsableBounds(displayId, &r);
        monitor.WorkPos = ImVec2((float)r.x, (float)r.y);
        monitor.WorkSize = ImVec2((float)r.w, (float)r.h);
        // FIXME-VIEWPORT: On MacOS SDL reports actual monitor DPI scale,
        // ignoring OS configuration. We may want to set
        //  DpiScale to cocoa_window.backingScaleFactor here.
        monitor.DpiScale = SDL_GetDisplayContentScale(displayId);
        monitor.PlatformHandle = (void*)(intptr_t)n;
        if (monitor.DpiScale <= 0.0f)
            continue; // Some accessibility applications are declaring virtual
                      // monitors with a DPI of 0, see #7902.
        platformIo.Monitors.push_back(monitor);
    }
    SDL_free(displays);
}

void ImGuiSDL3NewFrame()
{
    ImGuiSDL3Data* bd = ImGuiSDL3GetBackendData();
    IM_ASSERT(bd != nullptr
              && "Context or backend not initialized! Did you call "
                 "ImGui_ImplSDL3_Init()?");
    ImGuiIO& io = ImGui::GetIO();

    // Setup display size (every frame to accommodate for window resizing)
    int w, h;
    int displayW, displayH;
    SDL_GetWindowSize(bd->window, &w, &h);
    if (SDL_GetWindowFlags(bd->window) & SDL_WINDOW_MINIMIZED)
        w = h = 0;
    SDL_GetWindowSizeInPixels(bd->window, &displayW, &displayH);
    io.DisplaySize = ImVec2((float)w, (float)h);
    if (w > 0 && h > 0)
        io.DisplayFramebufferScale
            = ImVec2((float)displayW / w, (float)displayH / h);

    // Update monitors
    if (bd->wantUpdateMonitors)
        ImGuiSDL3UpdateMonitors();

    // Setup time step (we don't use SDL_GetTicks() because it is using
    // millisecond resolution) (Accept SDL_GetPerformanceCounter() not returning
    // a monotonically increasing value. Happens in VMs and Emscripten, see
    // #6189, #6114, #3644)
    static Uint64 frequency = SDL_GetPerformanceFrequency();
    Uint64        currentTime = SDL_GetPerformanceCounter();
    if (currentTime <= bd->time)
        currentTime = bd->time + 1;
    io.DeltaTime = bd->time > 0
        ? (float)((double)(currentTime - bd->time) / frequency)
        : (float)(1.0f / 60.0f);
    bd->time = currentTime;

    if (bd->mousePendingLeaveFrame
        && bd->mousePendingLeaveFrame >= ImGui::GetFrameCount()
        && bd->mouseButtonsDown == 0) {
        bd->mouseWindowId = 0;
        bd->mousePendingLeaveFrame = 0;
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
    }

    // Our io.AddMouseViewportEvent() calls will only be valid when not
    // capturing. Technically speaking testing for 'bd->MouseButtonsDown == 0'
    // would be more rigorous, but testing for payload reduces noise and
    // potential side-effects.
    if (bd->mouseCanReportHoveredViewport
        && ImGui::GetDragDropPayload() == nullptr)
        io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
    else
        io.BackendFlags &= ~ImGuiBackendFlags_HasMouseHoveredViewport;

    ImGuiSDL3UpdateMouseData();
    ImGuiSDL3UpdateMouseCursor();

    // Update game controllers (if enabled and available)
    ImGuiSDL3UpdateGamepads();
}

//--------------------------------------------------------------------------------------------------------
// MULTI-VIEWPORT / PLATFORM INTERFACE SUPPORT
// This is an _advanced_ and _optional_ feature, allowing the backend to create
// and handle multiple viewports simultaneously. If you are new to dear imgui or
// creating a new binding for dear imgui, it is recommended that you completely
// ignore this section first..
//--------------------------------------------------------------------------------------------------------

// Helper structure we store in the void* RendererUserData field of each
// ImGuiViewport to easily retrieve our backend data.
struct ImGuiSDL3ViewportData
{
    SDL_Window*   window;
    SDL_Window*   parentWindow;
    Uint32        windowId;
    bool          windowOwned;
    SDL_GLContext glContext;

    ImGuiSDL3ViewportData()
    {
        window = parentWindow = nullptr;
        windowId = 0;
        windowOwned = false;
        glContext = nullptr;
    }
    ~ImGuiSDL3ViewportData()
    {
        IM_ASSERT(window == nullptr && glContext == nullptr);
    }
};

static SDL_Window* ImGuiSDL3GetSDLWindowFromViewportID(ImGuiID viewportId)
{
    if (viewportId != 0)
        if (ImGuiViewport* viewport = ImGui::FindViewportByID(viewportId)) {
            SDL_WindowID windowId
                = (SDL_WindowID)(intptr_t)viewport->PlatformHandle;
            return SDL_GetWindowFromID(windowId);
        }
    return nullptr;
}

static void ImGuiSDL3CreateWindow(ImGuiViewport* viewport)
{
    ImGuiSDL3Data*         bd = ImGuiSDL3GetBackendData();
    ImGuiSDL3ViewportData* vd = IM_NEW(ImGuiSDL3ViewportData)();
    viewport->PlatformUserData = vd;

    vd->parentWindow
        = ImGuiSDL3GetSDLWindowFromViewportID(viewport->ParentViewportId);

    ImGuiViewport*         mainViewport = ImGui::GetMainViewport();
    ImGuiSDL3ViewportData* mainViewportData
        = (ImGuiSDL3ViewportData*)mainViewport->PlatformUserData;

    // Share GL resources with main context
    bool          useOpengl = (mainViewportData->glContext != nullptr);
    SDL_GLContext backupContext = nullptr;
    if (useOpengl) {
        backupContext = SDL_GL_GetCurrentContext();
        SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
        SDL_GL_MakeCurrent(mainViewportData->window,
                           mainViewportData->glContext);
    }

    SDL_WindowFlags sdlFlags = 0;
    sdlFlags |= useOpengl ? SDL_WINDOW_OPENGL
                          : (bd->useVulkan ? SDL_WINDOW_VULKAN : 0);
    sdlFlags |= SDL_GetWindowFlags(bd->window) & SDL_WINDOW_HIGH_PIXEL_DENSITY;
    sdlFlags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration)
        ? SDL_WINDOW_BORDERLESS
        : 0;
    sdlFlags |= (viewport->Flags & ImGuiViewportFlags_NoDecoration)
        ? 0
        : SDL_WINDOW_RESIZABLE;
    sdlFlags |= (viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)
        ? SDL_WINDOW_UTILITY
        : 0;
    sdlFlags |= (viewport->Flags & ImGuiViewportFlags_TopMost)
        ? SDL_WINDOW_ALWAYS_ON_TOP
        : 0;
    vd->window = SDL_CreateWindow("No Title Yet", (int)viewport->Size.x,
                                  (int)viewport->Size.y, sdlFlags);
    SDL_SetWindowParent(vd->window, vd->parentWindow);
    SDL_SetWindowPosition(vd->window, (int)viewport->Pos.x,
                          (int)viewport->Pos.y);
    vd->windowOwned = true;
    if (useOpengl) {
        vd->glContext = SDL_GL_CreateContext(vd->window);
        SDL_GL_SetSwapInterval(0);
    }
    if (useOpengl && backupContext)
        SDL_GL_MakeCurrent(vd->window, backupContext);

    ImGuiSDL3SetupPlatformHandles(viewport, vd->window);
}

static void ImGuiSDL3DestroyWindow(ImGuiViewport* viewport)
{
    if (ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData) {
        if (vd->glContext && vd->windowOwned)
            SDL_GL_DestroyContext(vd->glContext);
        if (vd->window && vd->windowOwned)
            SDL_DestroyWindow(vd->window);
        vd->glContext = nullptr;
        vd->window = nullptr;
        IM_DELETE(vd);
    }
    viewport->PlatformUserData = viewport->PlatformHandle = nullptr;
}

static void ImGuiSDL3ShowWindow(ImGuiViewport* viewport)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
#if defined(_WIN32)                                                            \
    && !(defined(WINAPI_FAMILY)                                                \
         && (WINAPI_FAMILY == WINAPI_FAMILY_APP                                \
             || WINAPI_FAMILY == WINAPI_FAMILY_GAMES))
    HWND hwnd = (HWND)viewport->PlatformHandleRaw;

    // SDL hack: Show icon in task bar (#7989)
    // Note: SDL_WINDOW_UTILITY can be used to control task bar visibility, but
    // on Windows, it does not affect child windows.
    if (!(viewport->Flags & ImGuiViewportFlags_NoTaskBarIcon)) {
        LONG ex_style = ::GetWindowLong(hwnd, GWL_EXSTYLE);
        ex_style |= WS_EX_APPWINDOW;
        ex_style &= ~WS_EX_TOOLWINDOW;
        ::ShowWindow(hwnd, SW_HIDE);
        ::SetWindowLong(hwnd, GWL_EXSTYLE, ex_style);
    }
#endif

    SDL_SetHint(
        SDL_HINT_WINDOW_ACTIVATE_WHEN_SHOWN,
        (viewport->Flags & ImGuiViewportFlags_NoFocusOnAppearing) ? "0" : "1");
    SDL_ShowWindow(vd->window);
}

static void ImGuiSDL3UpdateWindow(ImGuiViewport* viewport)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;

    // Update SDL3 parent if it changed _after_ creation.
    // This is for advanced apps that are manipulating ParentViewportID
    // manually.
    SDL_Window* newParent
        = ImGuiSDL3GetSDLWindowFromViewportID(viewport->ParentViewportId);
    if (newParent != vd->parentWindow) {
        vd->parentWindow = newParent;
        SDL_SetWindowParent(vd->window, vd->parentWindow);
    }
}

static ImVec2 ImGuiSDL3GetWindowPos(ImGuiViewport* viewport)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    int x = 0, y = 0;
    SDL_GetWindowPosition(vd->window, &x, &y);
    return ImVec2((float)x, (float)y);
}

static void ImGuiSDL3SetWindowPos(ImGuiViewport* viewport, ImVec2 pos)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    SDL_SetWindowPosition(vd->window, (int)pos.x, (int)pos.y);
}

static ImVec2 ImGuiSDL3GetWindowSize(ImGuiViewport* viewport)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    int w = 0, h = 0;
    SDL_GetWindowSize(vd->window, &w, &h);
    return ImVec2((float)w, (float)h);
}

static void ImGuiSDL3SetWindowSize(ImGuiViewport* viewport, ImVec2 size)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    SDL_SetWindowSize(vd->window, (int)size.x, (int)size.y);
}

static void ImGuiSDL3SetWindowTitle(ImGuiViewport* viewport, const char* title)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    SDL_SetWindowTitle(vd->window, title);
}

static void ImGuiSDL3SetWindowAlpha(ImGuiViewport* viewport, float alpha)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    SDL_SetWindowOpacity(vd->window, alpha);
}

static void ImGuiSDL3SetWindowFocus(ImGuiViewport* viewport)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    SDL_RaiseWindow(vd->window);
}

static bool ImGuiSDL3GetWindowFocus(ImGuiViewport* viewport)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    return (SDL_GetWindowFlags(vd->window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

static bool ImGuiSDL3GetWindowMinimized(ImGuiViewport* viewport)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    return (SDL_GetWindowFlags(vd->window) & SDL_WINDOW_MINIMIZED) != 0;
}

static void ImGuiSDL3RenderWindow(ImGuiViewport* viewport, void*)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    if (vd->glContext)
        SDL_GL_MakeCurrent(vd->window, vd->glContext);
}

static void ImGuiSDL3SwapBuffers(ImGuiViewport* viewport, void*)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    if (vd->glContext) {
        SDL_GL_MakeCurrent(vd->window, vd->glContext);
        SDL_GL_SwapWindow(vd->window);
    }
}

// Vulkan support (the Vulkan renderer needs to call a platform-side support
// function to create the surface) SDL is graceful enough to _not_ need
// <vulkan/vulkan.h> so we can safely include this.
#include <SDL3/SDL_vulkan.h>
static int ImGuiSDL3CreateVkSurface(ImGuiViewport* viewport, ImU64 vkInstance,
                                    const void* vkAllocator,
                                    ImU64*      outVkSurface)
{
    ImGuiSDL3ViewportData* vd
        = (ImGuiSDL3ViewportData*)viewport->PlatformUserData;
    (void)vkAllocator;
    bool ret = SDL_Vulkan_CreateSurface(vd->window, (VkInstance)vkInstance,
                                        (VkAllocationCallbacks*)vkAllocator,
                                        (VkSurfaceKHR*)outVkSurface);
    return ret ? 0 : 1; // ret ? VK_SUCCESS : VK_NOT_READY
}

static void ImGuiSDL3InitMultiViewportSupport(SDL_Window* window,
                                              void*       sdlGlContext)
{
    // Register platform interface (will be coupled with a renderer interface)
    ImGuiPlatformIO& platformIo = ImGui::GetPlatformIO();
    platformIo.Platform_CreateWindow = ImGuiSDL3CreateWindow;
    platformIo.Platform_DestroyWindow = ImGuiSDL3DestroyWindow;
    platformIo.Platform_ShowWindow = ImGuiSDL3ShowWindow;
    platformIo.Platform_UpdateWindow = ImGuiSDL3UpdateWindow;
    platformIo.Platform_SetWindowPos = ImGuiSDL3SetWindowPos;
    platformIo.Platform_GetWindowPos = ImGuiSDL3GetWindowPos;
    platformIo.Platform_SetWindowSize = ImGuiSDL3SetWindowSize;
    platformIo.Platform_GetWindowSize = ImGuiSDL3GetWindowSize;
    platformIo.Platform_SetWindowFocus = ImGuiSDL3SetWindowFocus;
    platformIo.Platform_GetWindowFocus = ImGuiSDL3GetWindowFocus;
    platformIo.Platform_GetWindowMinimized = ImGuiSDL3GetWindowMinimized;
    platformIo.Platform_SetWindowTitle = ImGuiSDL3SetWindowTitle;
    platformIo.Platform_RenderWindow = ImGuiSDL3RenderWindow;
    platformIo.Platform_SwapBuffers = ImGuiSDL3SwapBuffers;
    platformIo.Platform_SetWindowAlpha = ImGuiSDL3SetWindowAlpha;
    platformIo.Platform_CreateVkSurface = ImGuiSDL3CreateVkSurface;

    // Register main window handle (which is owned by the main application, not
    // by us) This is mostly for simplicity and consistency, so that our code
    // (e.g. mouse handling etc.) can use same logic for main and secondary
    // viewports.
    ImGuiViewport*         mainViewport = ImGui::GetMainViewport();
    ImGuiSDL3ViewportData* vd = IM_NEW(ImGuiSDL3ViewportData)();
    vd->window = window;
    vd->windowId = SDL_GetWindowID(window);
    vd->windowOwned = false;
    vd->glContext = (SDL_GLContext)sdlGlContext;
    mainViewport->PlatformUserData = vd;
    mainViewport->PlatformHandle = (void*)(intptr_t)vd->windowId;
}

static void ImGuiSDL3ShutdownMultiViewportSupport()
{
    ImGui::DestroyPlatformWindows();
}

//-----------------------------------------------------------------------------

#if defined(__clang__)
#pragma clang diagnostic pop // NOLINT
#endif

#endif // #ifndef IMGUI_DISABLE
