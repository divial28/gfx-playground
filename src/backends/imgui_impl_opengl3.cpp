//----------------------------------------
// OpenGL    GLSL      GLSL
// version   version   string
//----------------------------------------
//  2.0       110       "#version 110"
//  2.1       120       "#version 120"
//  3.0       130       "#version 130"
//  3.1       140       "#version 140"
//  3.2       150       "#version 150"
//  3.3       330       "#version 330 core"
//  4.0       400       "#version 400 core"
//  4.1       410       "#version 410 core"
//  4.2       420       "#version 410 core"
//  4.3       430       "#version 430 core"
//  ES 2.0    100       "#version 100"      = WebGL 1.0
//  ES 3.0    300       "#version 300 es"   = WebGL 2.0
//----------------------------------------

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_opengl3.h"
#include <glad/glad.h>
#include <stdint.h> // intptr_t
#include <stdio.h>

// Clang/GCC warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored                                               \
    "-Wunknown-warning-option" // warning: ignore unknown flags
#pragma clang diagnostic ignored                                               \
    "-Wold-style-cast" // warning: use of old-style cast
#pragma clang diagnostic ignored                                               \
    "-Wsign-conversion" // warning: implicit conversion changes signedness
#pragma clang diagnostic ignored "-Wunused-macros" // warning: macro is not used
#pragma clang diagnostic ignored "-Wnonportable-system-include-path"
#pragma clang diagnostic ignored                                               \
    "-Wcast-function-type" // warning: cast between incompatible function types
                           // (for loader)
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored                                                 \
    "-Wpragmas" // warning: unknown option after '#pragma GCC diagnostic' kind
#pragma GCC diagnostic ignored                                                 \
    "-Wunknown-warning-option" // warning: unknown warning group 'xxx'
#pragma GCC diagnostic ignored                                                 \
    "-Wcast-function-type" // warning: cast between incompatible function types
                           // (for loader)
#pragma GCC diagnostic ignored                                                 \
    "-Wstrict-overflow" // warning: assuming signed overflow does not occur when
                        // simplifying division / ..when changing X +- C1 cmp C2
                        // to X cmp C2 -+ C1
#endif

// [Debugging]
// #define IMGUI_IMPL_OPENGL_DEBUG
#ifdef IMGUI_IMPL_OPENGL_DEBUG
#include <stdio.h>
#define GL_CALL(_CALL)                                                         \
    do {                                                                       \
        _CALL;                                                                 \
        GLenum gl_err = glGetError();                                          \
        if (gl_err != 0)                                                       \
            fprintf(stderr, "GL error 0x%x returned from '%s'.\n", gl_err,     \
                    #_CALL);                                                   \
    } while (0) // Call with error check
#else
#define GL_CALL(_CALL) _CALL // Call without error check
#endif

// OpenGL Data
struct ImGuiGLData
{
    GLint        attribLocationTex; // Uniforms location
    GLint        attribLocationProjMtx;
    GLuint       attribLocationVtxPos; // Vertex attributes location
    GLuint       attribLocationVtxUv;
    GLuint       attribLocationVtxColor;
    unsigned int vboHandle, elementsHandle;
    GLsizeiptr   vertexBufferSize;
    GLsizeiptr   indexBufferSize;

    ImGuiGLData() { memset((void*)this, 0, sizeof(*this)); }
};

namespace {
GLuint g_shaderHandle;
GLuint g_fontTexture;
} // namespace

// Backend data stored in io.BackendRendererUserData to allow support for
// multiple Dear ImGui contexts It is STRONGLY preferred that you use docking
// branch with multi-viewports (== single Dear ImGui context + multiple windows)
// instead of multiple Dear ImGui contexts.
static ImGuiGLData* ImGuiGLGetBackendData()
{
    return ImGui::GetCurrentContext()
        ? (ImGuiGLData*)ImGui::GetIO().BackendRendererUserData
        : nullptr;
}

// Functions

bool ImGuiGLInit() { return ImGuiGLCreateShader(); }

bool ImGuiGLInitContext()
{
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr
              && "Already initialized a renderer backend!");

    // Setup backend capabilities flags
    ImGuiGLData* bd = IM_NEW(ImGuiGLData)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_opengl3";

    GLint glProfileMask = 0;
    glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &glProfileMask);
    GLint glProfileIsCompat
        = (glProfileMask & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT) != 0;

#ifdef IMGUI_IMPL_OPENGL_DEBUG
    printf("GL_VERSION = \"%s\"\nCompatibility profile = %d\nProfile mask = "
           "0x%X\nGL_VENDOR = '%s'\nGL_RENDERER = '%s'\n",
           (const char*)glGetString(GL_VERSION), glProfileIsCompat,
           glProfileMask, (const char*)glGetString(GL_VENDOR),
           (const char*)glGetString(GL_RENDERER)); // [DEBUG]
#endif

    io.BackendFlags
        |= ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the
                                                   // ImDrawCmd::VtxOffset
                                                   // field, allowing for large
                                                   // meshes.
    return true;
}

void ImGuiGLShutdown()
{
    ImGuiGLDestroyShader();
    ImGuiGLDestroyFontsTexture();
}

void ImGuiGLShutdownContext()
{
    ImGuiGLData* bd = ImGuiGLGetBackendData();
    IM_ASSERT(bd != nullptr
              && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGuiGLDestroyDeviceObjects();
    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &= ~(ImGuiBackendFlags_RendererHasVtxOffset
                         | ImGuiBackendFlags_RendererHasViewports);
    IM_DELETE(bd);
}

void ImGuiGLNewFrame()
{
    ImGuiGLData* bd = ImGuiGLGetBackendData();
    IM_ASSERT(bd != nullptr
              && "Context or backend not initialized! Did you call "
                 "ImGui_ImplOpenGL3_Init()?");

    if (!g_fontTexture) {
        ImGuiGLCreateFontsTexture();
    }
    if (!bd->vboHandle) {
        ImGuiGLCreateDeviceObjects();
    }

    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->SetTexID((ImTextureID)(intptr_t)g_fontTexture);
}

static void ImGuiGLSetupRenderState(ImDrawData* drawData, int fbWidth,
                                    int fbHeight, GLuint vertexArrayObject)
{
    ImGuiGLData* bd = ImGuiGLGetBackendData();

    // Setup render state: alpha-blending enabled, no face culling, no depth
    // testing, scissor enabled, polygon fill
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE,
                        GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_SCISSOR_TEST);
    glDisable(GL_PRIMITIVE_RESTART);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Support for GL 4.5 rarely used glClipControl(GL_UPPER_LEFT)
    bool   clipOriginLowerLeft = true;
    GLenum currentClipOrigin = 0;
    glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&currentClipOrigin);
    if (currentClipOrigin == GL_UPPER_LEFT)
        clipOriginLowerLeft = false;

    // Setup viewport, orthographic projection matrix
    // Our visible imgui space lies from draw_data->DisplayPos (top left) to
    // draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos
    // is (0,0) for single viewport apps.
    GL_CALL(glViewport(0, 0, (GLsizei)fbWidth, (GLsizei)fbHeight));
    float l = drawData->DisplayPos.x;
    float r = drawData->DisplayPos.x + drawData->DisplaySize.x;
    float t = drawData->DisplayPos.y;
    float b = drawData->DisplayPos.y + drawData->DisplaySize.y;
    if (!clipOriginLowerLeft) {
        float tmp = t;
        t = b;
        b = tmp;
    } // Swap top and bottom if origin is upper left
    const float orthoProjection[4][4] = {
        { 2.0f / (r - l), 0.0f, 0.0f, 0.0f },
        { 0.0f, 2.0f / (t - b), 0.0f, 0.0f },
        { 0.0f, 0.0f, -1.0f, 0.0f },
        { (r + l) / (l - r), (t + b) / (b - t), 0.0f, 1.0f },
    };
    glUseProgram(g_shaderHandle);
    glUniform1i(bd->attribLocationTex, 0);
    glUniformMatrix4fv(bd->attribLocationProjMtx, 1, GL_FALSE,
                       &orthoProjection[0][0]);
    glBindSampler(0, 0); // We use combined texture/sampler state. Applications
                         // using GL 3.3 and GL ES 3.0 may set that otherwise.
    glBindVertexArray(vertexArrayObject);

    // Bind vertex/index buffers and setup attributes for ImDrawVert
    GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, bd->vboHandle));
    GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bd->elementsHandle));
    GL_CALL(glEnableVertexAttribArray(bd->attribLocationVtxPos));
    GL_CALL(glEnableVertexAttribArray(bd->attribLocationVtxUv));
    GL_CALL(glEnableVertexAttribArray(bd->attribLocationVtxColor));
    GL_CALL(glVertexAttribPointer(bd->attribLocationVtxPos, 2, GL_FLOAT,
                                  GL_FALSE, sizeof(ImDrawVert),
                                  (GLvoid*)offsetof(ImDrawVert, pos)));
    GL_CALL(glVertexAttribPointer(bd->attribLocationVtxUv, 2, GL_FLOAT,
                                  GL_FALSE, sizeof(ImDrawVert),
                                  (GLvoid*)offsetof(ImDrawVert, uv)));
    GL_CALL(glVertexAttribPointer(bd->attribLocationVtxColor, 4,
                                  GL_UNSIGNED_BYTE, GL_TRUE, sizeof(ImDrawVert),
                                  (GLvoid*)offsetof(ImDrawVert, col)));
}

// OpenGL3 Render function.
// Note that this implementation is little overcomplicated because we are
// saving/setting up/restoring every OpenGL state explicitly. This is in order
// to be able to run within an OpenGL engine that doesn't do so.
void ImGuiGLRenderDrawData(ImDrawData* drawData)
{
    // Avoid rendering when minimized, scale coordinates for retina displays
    // (screen coordinates != framebuffer coordinates)
    int fbWidth = (int)(drawData->DisplaySize.x * drawData->FramebufferScale.x);
    int fbHeight
        = (int)(drawData->DisplaySize.y * drawData->FramebufferScale.y);
    if (fbWidth <= 0 || fbHeight <= 0)
        return;

    ImGuiGLData* bd = ImGuiGLGetBackendData();

    // Backup GL state
    GLenum lastActiveTexture;
    glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&lastActiveTexture);
    glActiveTexture(GL_TEXTURE0);
    GLuint lastProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, (GLint*)&lastProgram);
    GLuint lastTexture;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&lastTexture);
    GLuint lastSampler;
    glGetIntegerv(GL_SAMPLER_BINDING, (GLint*)&lastSampler);
    GLuint lastArrayBuffer;
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&lastArrayBuffer);
    GLuint lastVertexArrayObject;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&lastVertexArrayObject);
    GLint lastPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, lastPolygonMode);
    GLint lastViewport[4];
    glGetIntegerv(GL_VIEWPORT, lastViewport);
    GLint lastScissorBox[4];
    glGetIntegerv(GL_SCISSOR_BOX, lastScissorBox);
    GLenum lastBlendSrcRgb;
    glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&lastBlendSrcRgb);
    GLenum lastBlendDstRgb;
    glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&lastBlendDstRgb);
    GLenum lastBlendSrcAlpha;
    glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&lastBlendSrcAlpha);
    GLenum lastBlendDstAlpha;
    glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&lastBlendDstAlpha);
    GLenum lastBlendEquationRgb;
    glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&lastBlendEquationRgb);
    GLenum lastBlendEquationAlpha;
    glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&lastBlendEquationAlpha);
    GLboolean lastEnableBlend = glIsEnabled(GL_BLEND);
    GLboolean lastEnableCullFace = glIsEnabled(GL_CULL_FACE);
    GLboolean lastEnableDepthTest = glIsEnabled(GL_DEPTH_TEST);
    GLboolean lastEnableStencilTest = glIsEnabled(GL_STENCIL_TEST);
    GLboolean lastEnableScissorTest = glIsEnabled(GL_SCISSOR_TEST);
    GLboolean lastEnablePrimitiveRestart = glIsEnabled(GL_PRIMITIVE_RESTART);

    // Setup desired GL state
    // Recreate the VAO every time (this is to easily allow multiple GL contexts
    // to be rendered to. VAO are not shared among GL contexts) The renderer
    // would actually work without any VAO bound, but then our VertexAttrib
    // calls would overwrite the default one currently bound.
    GLuint vertexArrayObject = 0;
    GL_CALL(glGenVertexArrays(1, &vertexArrayObject));
    ImGuiGLSetupRenderState(drawData, fbWidth, fbHeight, vertexArrayObject);

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clipOff = drawData->DisplayPos; // (0,0) unless using multi-viewports
    ImVec2 clipScale
        = drawData->FramebufferScale; // (1,1) unless using retina display which
                                      // are often (2,2)

    // Render command lists
    for (int n = 0; n < drawData->CmdListsCount; n++) {
        const ImDrawList* drawList = drawData->CmdLists[n];
        const GLsizeiptr  vtxBufferSize
            = (GLsizeiptr)drawList->VtxBuffer.Size * (int)sizeof(ImDrawVert);
        const GLsizeiptr idxBufferSize
            = (GLsizeiptr)drawList->IdxBuffer.Size * (int)sizeof(ImDrawIdx);
        if (bd->vertexBufferSize < vtxBufferSize) {
            bd->vertexBufferSize = vtxBufferSize;
            GL_CALL(glBufferData(GL_ARRAY_BUFFER, bd->vertexBufferSize, nullptr,
                                 GL_STREAM_DRAW));
        }
        if (bd->indexBufferSize < idxBufferSize) {
            bd->indexBufferSize = idxBufferSize;
            GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, bd->indexBufferSize,
                                 nullptr, GL_STREAM_DRAW));
        }
        GL_CALL(glBufferSubData(GL_ARRAY_BUFFER, 0, vtxBufferSize,
                                (const GLvoid*)drawList->VtxBuffer.Data));
        GL_CALL(glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, idxBufferSize,
                                (const GLvoid*)drawList->IdxBuffer.Data));

        for (int cmdI = 0; cmdI < drawList->CmdBuffer.Size; cmdI++) {
            const ImDrawCmd* pcmd = &drawList->CmdBuffer[cmdI];
            if (pcmd->UserCallback != nullptr) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value
                // used by the user to request the renderer to reset render
                // state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGuiGLSetupRenderState(drawData, fbWidth, fbHeight,
                                            vertexArrayObject);
                else
                    pcmd->UserCallback(drawList, pcmd);
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clipMin((pcmd->ClipRect.x - clipOff.x) * clipScale.x,
                               (pcmd->ClipRect.y - clipOff.y) * clipScale.y);
                ImVec2 clipMax((pcmd->ClipRect.z - clipOff.x) * clipScale.x,
                               (pcmd->ClipRect.w - clipOff.y) * clipScale.y);
                if (clipMax.x <= clipMin.x || clipMax.y <= clipMin.y)
                    continue;

                // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
                GL_CALL(glScissor((int)clipMin.x,
                                  (int)((float)fbHeight - clipMax.y),
                                  (int)(clipMax.x - clipMin.x),
                                  (int)(clipMax.y - clipMin.y)));

                // Bind texture, Draw
                GL_CALL(glBindTexture(GL_TEXTURE_2D,
                                      (GLuint)(intptr_t)pcmd->GetTexID()));
                GL_CALL(glDrawElementsBaseVertex(
                    GL_TRIANGLES, (GLsizei)pcmd->ElemCount,
                    sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT
                                           : GL_UNSIGNED_INT,
                    (void*)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)),
                    (GLint)pcmd->VtxOffset));
            }
        }
    }

    // Destroy the temporary VAO
    GL_CALL(glDeleteVertexArrays(1, &vertexArrayObject));

    // Restore modified GL state
    // This "glIsProgram()" check is required because if the program is "pending
    // deletion" at the time of binding backup, it will have been deleted by now
    // and will cause an OpenGL error. See #6220.
    if (lastProgram == 0 || glIsProgram(lastProgram))
        glUseProgram(lastProgram);
    glBindTexture(GL_TEXTURE_2D, lastTexture);
    glBindSampler(0, lastSampler);
    glActiveTexture(lastActiveTexture);
    glBindVertexArray(lastVertexArrayObject);
    glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    glBlendEquationSeparate(lastBlendEquationRgb, lastBlendEquationAlpha);
    glBlendFuncSeparate(lastBlendSrcRgb, lastBlendDstRgb, lastBlendSrcAlpha,
                        lastBlendDstAlpha);
    if (lastEnableBlend)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
    if (lastEnableCullFace)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
    if (lastEnableDepthTest)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
    if (lastEnableStencilTest)
        glEnable(GL_STENCIL_TEST);
    else
        glDisable(GL_STENCIL_TEST);
    if (lastEnableScissorTest)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
    if (lastEnablePrimitiveRestart)
        glEnable(GL_PRIMITIVE_RESTART);
    else
        glDisable(GL_PRIMITIVE_RESTART);

    // Desktop OpenGL 3.0 and OpenGL 3.1 had separate polygon draw modes for
    // front-facing and back-facing faces of polygons
    glPolygonMode(GL_FRONT_AND_BACK, (GLenum)lastPolygonMode[0]);

    glViewport(lastViewport[0], lastViewport[1], (GLsizei)lastViewport[2],
               (GLsizei)lastViewport[3]);
    glScissor(lastScissorBox[0], lastScissorBox[1], (GLsizei)lastScissorBox[2],
              (GLsizei)lastScissorBox[3]);
}

bool ImGuiGLCreateFontsTexture()
{
    ImGuiIO&     io = ImGui::GetIO();
    ImGuiGLData* bd = ImGuiGLGetBackendData();

    // Build texture atlas
    unsigned char* pixels;
    int            width, height;
    io.Fonts->GetTexDataAsRGBA32(
        &pixels, &width,
        &height); // Load as RGBA 32-bit (75% of the memory is wasted, but
                  // default font is so small) because it is more likely to be
                  // compatible with user's existing shaders. If your
                  // ImTextureId represent a higher-level concept than just a GL
                  // texture id, consider calling GetTexDataAsAlpha8() instead
                  // to save on GPU memory.

    // Upload texture to graphics system
    // (Bilinear sampling is required by default. Set 'io.Fonts->Flags |=
    // ImFontAtlasFlags_NoBakedLines' or 'style.AntiAliasedLinesUseTex = false'
    // to allow point/nearest sampling)
    GLint lastTexture;
    GL_CALL(glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture));
    GL_CALL(glGenTextures(1, &g_fontTexture));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, g_fontTexture));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    GL_CALL(
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CALL(glPixelStorei(GL_UNPACK_ROW_LENGTH, 0));
    GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, pixels));

    // Restore state
    GL_CALL(glBindTexture(GL_TEXTURE_2D, lastTexture));

    return true;
}

void ImGuiGLDestroyFontsTexture()
{
    if (g_fontTexture) {
        glDeleteTextures(1, &g_fontTexture);
        g_fontTexture = 0;
    }
}

// If you get an error please report on github. You may try different GL context
// version or GLSL version. See GL<>GLSL version table at the top of this file.
static bool CheckShader(GLuint handle, const char* desc)
{
    ImGuiGLData* bd = ImGuiGLGetBackendData();
    GLint        status = 0, logLength = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &logLength);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr,
                "ERROR: ImGui_ImplOpenGL3_CreateDeviceObjects: failed to "
                "compile %s!\n",
                desc);
    if (logLength > 1) {
        ImVector<char> buf;
        buf.resize((int)(logLength + 1));
        glGetShaderInfoLog(handle, logLength, nullptr, (GLchar*)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

// If you get an error please report on GitHub. You may try different GL context
// version or GLSL version.
static bool CheckProgram(GLuint handle, const char* desc)
{
    ImGuiGLData* bd = ImGuiGLGetBackendData();
    GLint        status = 0, logLength = 0;
    glGetProgramiv(handle, GL_LINK_STATUS, &status);
    glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &logLength);
    if ((GLboolean)status == GL_FALSE)
        fprintf(stderr,
                "ERROR: ImGui_ImplOpenGL3_CreateDeviceObjects: failed to link "
                "%s!\n",
                desc);
    if (logLength > 1) {
        ImVector<char> buf;
        buf.resize((int)(logLength + 1));
        glGetProgramInfoLog(handle, logLength, nullptr, (GLchar*)buf.begin());
        fprintf(stderr, "%s\n", buf.begin());
    }
    return (GLboolean)status == GL_TRUE;
}

bool ImGuiGLCreateShader()
{
    if (g_shaderHandle) {
        return true;
    }

    static constexpr const GLchar* vertexShader
        = "#version 460\n"
          "layout (location = 0) in vec2 Position;\n"
          "layout (location = 1) in vec2 UV;\n"
          "layout (location = 2) in vec4 Color;\n"
          "uniform mat4 ProjMtx;\n"
          "out vec2 Frag_UV;\n"
          "out vec4 Frag_Color;\n"
          "void main()\n"
          "{\n"
          "    Frag_UV = UV;\n"
          "    Frag_Color = Color;\n"
          "    gl_Position = ProjMtx * vec4(Position.xy,0,1);\n"
          "}\n";

    static constexpr const GLchar* fragmentShader
        = "#version 460\n"
          "in vec2 Frag_UV;\n"
          "in vec4 Frag_Color;\n"
          "uniform sampler2D Texture;\n"
          "layout (location = 0) out vec4 Out_Color;\n"
          "void main()\n"
          "{\n"
          "    Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
          "}\n";

    // Create shaders
    GLuint vertHandle;
    GL_CALL(vertHandle = glCreateShader(GL_VERTEX_SHADER));
    glShaderSource(vertHandle, 1, &vertexShader, nullptr);
    glCompileShader(vertHandle);
    CheckShader(vertHandle, "vertex shader");

    GLuint fragHandle;
    GL_CALL(fragHandle = glCreateShader(GL_FRAGMENT_SHADER));
    glShaderSource(fragHandle, 1, &fragmentShader, nullptr);
    glCompileShader(fragHandle);
    CheckShader(fragHandle, "fragment shader");

    // Link
    g_shaderHandle = glCreateProgram();
    glAttachShader(g_shaderHandle, vertHandle);
    glAttachShader(g_shaderHandle, fragHandle);
    glLinkProgram(g_shaderHandle);
    CheckProgram(g_shaderHandle, "shader program");

    glDetachShader(g_shaderHandle, vertHandle);
    glDetachShader(g_shaderHandle, fragHandle);
    glDeleteShader(vertHandle);
    glDeleteShader(fragHandle);

    return true;
}

void ImGuiGLDestroyShader()
{
    if (g_shaderHandle) {
        glDeleteProgram(g_shaderHandle);
        g_shaderHandle = 0;
    }
}

bool ImGuiGLCreateDeviceObjects()
{
    ImGuiGLData* bd = ImGuiGLGetBackendData();

    // Backup GL state
    GLint lastTexture, lastArrayBuffer;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &lastTexture);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &lastArrayBuffer);
    GLint lastPixelUnpackBuffer = 0;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &lastPixelUnpackBuffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    GLint lastVertexArray;
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &lastVertexArray);

    bd->attribLocationTex = glGetUniformLocation(g_shaderHandle, "Texture");
    bd->attribLocationProjMtx = glGetUniformLocation(g_shaderHandle, "ProjMtx");
    bd->attribLocationVtxPos
        = (GLuint)glGetAttribLocation(g_shaderHandle, "Position");
    bd->attribLocationVtxUv = (GLuint)glGetAttribLocation(g_shaderHandle, "UV");
    bd->attribLocationVtxColor
        = (GLuint)glGetAttribLocation(g_shaderHandle, "Color");

    // Create buffers
    glGenBuffers(1, &bd->vboHandle);
    glGenBuffers(1, &bd->elementsHandle);

    // Restore modified GL state
    glBindTexture(GL_TEXTURE_2D, lastTexture);
    glBindBuffer(GL_ARRAY_BUFFER, lastArrayBuffer);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, lastPixelUnpackBuffer);
    glBindVertexArray(lastVertexArray);

    return true;
}

void ImGuiGLDestroyDeviceObjects()
{
    ImGuiGLData* bd = ImGuiGLGetBackendData();
    if (bd->vboHandle) {
        glDeleteBuffers(1, &bd->vboHandle);
        bd->vboHandle = 0;
    }
    if (bd->elementsHandle) {
        glDeleteBuffers(1, &bd->elementsHandle);
        bd->elementsHandle = 0;
    }
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->SetTexID(0);
}

#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif // #ifndef IMGUI_DISABLE
