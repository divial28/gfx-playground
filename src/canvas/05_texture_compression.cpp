#include "05_texture_compression.h"
#include "gl/framework.h"
#include "utils.h"

#include <imgui.h>
#include <imgui_stdlib.h>
#include <nfd.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


TextureCompressionCanvas::TextureCompressionCanvas() 
{
    shader_ = GL::CreateShader(
        R"(#version 460 core
        out vec2 texcoord;
        uniform mat4 proj;
        void main()
        {
            vec2 points[4] = vec2[](
                vec2(-1.0f, -1.0f),
                vec2(-1.0f,  1.0f),
                vec2( 1.0f,  1.0f),
                vec2( 1.0f, -1.0f)
            );
            gl_Position = proj * vec4(points[gl_VertexID], 0.0, 1.0);
            texcoord = (points[gl_VertexID] + 1.0) / 2;
        })",
        R"(#version 460 core
        in vec2 texcoord;
        out vec4 color;
        uniform float edge;
        uniform sampler2D tex0;
        uniform sampler2D tex1;
        void main()
        {
            float fragX = gl_FragCoord.x;
            float c = step(edge, gl_FragCoord.x);
            vec4 color0 = texture(tex0, texcoord);
            vec4 color1 = texture(tex1, texcoord);
            float inRange = step(edge - 0.5, fragX) * step(fragX, edge + 0.49);
            color = mix(color0 * (1 - c) + color1 * c, vec4(0.0, 0.0, 0.0, 1.0), inRange);
        })"
    );

    FetchSupportedCompressions();
    UpdateAllTextures();
}

TextureCompressionCanvas::~TextureCompressionCanvas() 
{
    glDeleteProgram(shader_);
}

void TextureCompressionCanvas::BuildUI()
{
    ImGui::Begin("Texture options");
    ImGui::InputText("##image", &imagePath_, ImGuiInputTextFlags_ReadOnly);
    ImGui::SameLine();
    if (ImGui::SmallButton("open")) {
        nfdu8char_t *outPath;
        nfdu8filteritem_t filters[1] = { { "Image", "png,jpg,jpeg,tiff" } };
        nfdopendialogu8args_t args = {0};
        args.filterCount = 1;
        args.filterList = filters;
        args.defaultPath = "./assets";
        nfdresult_t result = NFD_OpenDialogU8_With(&outPath, &args);
        if (result == NFD_OKAY)
        {
            imagePath_ = outPath;
            NFD_FreePathU8(outPath);
            UpdateAllTextures();
        }
        else if (result != NFD_CANCEL)
        {
            SPDLOG_ERROR("Failed to open file: {}", NFD_GetError());
        }
    }

    for (int i = 0; i < 2; ++i) {
        int value = compressions_[i];
        ImGui::Text("texture %d compression", i);
        ImGui::Combo(
            std::format("##texture {} compression", i).c_str(), &value,
            [](void* data, int n) {
                return GetCompressionName(*((GLenum*)data + n)).c_str();
            },
            supportedCompressions_.data(), supportedCompressions_.size());
        if (value != compressions_[i]) {
            compressions_[i] = value;
            DeleteTexture(textures_[i]);
            textures_[i] = LoadTexture(imagePath_, supportedCompressions_[compressions_[i]]);
        }
    }
    ImGui::SliderFloat("edge", &edge_, 0.001f, 1.0f);
    ImGui::SliderFloat("zoom", &zoom_, 0.001f, 10.0f, "%.3f", ImGuiSliderFlags_Logarithmic);
    ImGui::End();
}

void TextureCompressionCanvas::Render()
{
    const auto size = ImGui::GetMainViewport()->Size;
    const auto aspect = size.x / size.y;
    proj_ = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f);

    glViewport(250 + (size.x - imageSize_.x * zoom_) / 2,
               (size.y - imageSize_.y * zoom_) / 2, imageSize_.x * zoom_,
               imageSize_.y * zoom_);
    glClearColor(bgColor_.r, bgColor_.g, bgColor_.b, bgColor_.a);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(shader_);
    for (int i = 0; i < 2; ++i) {
        glActiveTexture(GL_TEXTURE0 + i);
        glBindTexture(GL_TEXTURE_2D, textures_[i]); 
        const auto loc = glGetUniformLocation(shader_, std::format("tex{}", i).c_str());
        glUniform1i(loc, i);
    }

    glUniform1f(glGetUniformLocation(shader_, "edge"), 250 + (size.x - imageSize_.x * zoom_) / 2 + edge_ * imageSize_.x * zoom_);
    glUniformMatrix4fv(glGetUniformLocation(shader_, "proj"), 1, GL_FALSE, glm::value_ptr(proj_));

    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void TextureCompressionCanvas::FetchSupportedCompressions()
{
    // FIXME: DEPRECATED METHOD! need to check compression formats in different way
    supportedCompressions_ = {GL_NONE};

    GLint numCompressedFormats;
    glGetIntegerv(GL_NUM_COMPRESSED_TEXTURE_FORMATS, &numCompressedFormats);

    if (numCompressedFormats <= 0) {
        SPDLOG_ERROR("No compressed formats supported!");
        return;
    }

    std::vector<GLint> compressedFormats;
    compressedFormats.resize(numCompressedFormats);
    glGetIntegerv(GL_COMPRESSED_TEXTURE_FORMATS, compressedFormats.data());
    for (GLenum f : compressedFormats) {
        supportedCompressions_.push_back(f);
    }
}

GLuint TextureCompressionCanvas::LoadTexture(const std::string& path, GLenum compression)
{
    // Generate a texture ID
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // Wrap mode for S axis
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // Wrap mode for T axis
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Minification filter
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Magnification filter

    // Load the image using STB Image
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // Flip the image vertically (OpenGL expects 0,0 at the bottom-left)
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);

    if (data) {
        // Determine the format based on the number of channels
        GLenum format = GL_RGB;
        if (nrChannels == 1)
            format = GL_RED;
        else if (nrChannels == 3)
            format = GL_RGB;
        else if (nrChannels == 4)
            format = GL_RGBA;

        // Upload the image data to the GPU
        glTexImage2D(GL_TEXTURE_2D, 0,
                     compression == GL_NONE ? format : compression, width,
                     height, 0, format, GL_UNSIGNED_BYTE, data);
        GLuint size = width * height * 4;
        GLint compressedSize = 0;
        GL_CALL(glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &compressedSize));
        SPDLOG_DEBUG(
            "loaded texture {}x{}, compression={} size={} compressed={}", width,
            height, GetCompressionName(compression), size, compressedSize);
        // glGenerateMipmap(GL_TEXTURE_2D); // Generate mipmaps
        imageSize_ = ImVec2{(float)width, (float)height};
    } else {
        SPDLOG_ERROR("Failed to load texture: {}", path);
        imageSize_ = ImVec2{0.0f, 0.0f};
    }

    // Free the image data
    stbi_image_free(data);

    return textureID;
}

void TextureCompressionCanvas::UpdateAllTextures()
{
    for (int i = 0; i < 2; ++i) {
        compressions_[i] = 0;
        DeleteTexture(textures_[i]);
        textures_[i] = LoadTexture(imagePath_, supportedCompressions_[compressions_[i]]);
    }
}

void TextureCompressionCanvas::DeleteTexture(GLuint texture)
{
    glDeleteTextures(1, &texture);
}

const std::string& TextureCompressionCanvas::GetCompressionName(GLenum id)
{
    #define element(x) {x, std::format("{:x} "#x, x)}
    static std::unordered_map<GLenum, std::string> names = {
        element(GL_NONE),
        element(GL_COMPRESSED_RGB),
        element(GL_COMPRESSED_RGBA),
        element(GL_COMPRESSED_ALPHA),
        element(GL_COMPRESSED_LUMINANCE),
        element(GL_COMPRESSED_LUMINANCE_ALPHA),
        element(GL_COMPRESSED_INTENSITY),
        element(GL_COMPRESSED_SRGB),
        element(GL_COMPRESSED_SRGB_ALPHA),
        element(GL_COMPRESSED_SLUMINANCE),
        element(GL_COMPRESSED_SLUMINANCE_ALPHA),
        element(GL_COMPRESSED_RED),
        element(GL_COMPRESSED_RG),
        element(GL_COMPRESSED_RED_RGTC1),
        element(GL_COMPRESSED_SIGNED_RED_RGTC1),
        element(GL_COMPRESSED_RG_RGTC2),
        element(GL_COMPRESSED_SIGNED_RG_RGTC2),
        element(GL_COMPRESSED_RGBA_BPTC_UNORM),
        element(GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM),
        element(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT),
        element(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT),
        element(GL_COMPRESSED_RGB8_ETC2),
        element(GL_COMPRESSED_SRGB8_ETC2),
        element(GL_COMPRESSED_RGB8_PUNCHTHROUGH_ALPHA1_ETC2),
        element(GL_COMPRESSED_SRGB8_PUNCHTHROUGH_ALPHA1_ETC2),
        element(GL_COMPRESSED_RGBA8_ETC2_EAC),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ETC2_EAC),
        element(GL_COMPRESSED_R11_EAC),
        element(GL_COMPRESSED_SIGNED_R11_EAC),
        element(GL_COMPRESSED_RG11_EAC),
        element(GL_COMPRESSED_SIGNED_RG11_EAC),
        element(GL_COMPRESSED_RGB_FXT1_3DFX),
        element(GL_COMPRESSED_RGBA_FXT1_3DFX),
        element(GL_COMPRESSED_ALPHA_ARB),
        element(GL_COMPRESSED_LUMINANCE_ARB),
        element(GL_COMPRESSED_LUMINANCE_ALPHA_ARB),
        element(GL_COMPRESSED_INTENSITY_ARB),
        element(GL_COMPRESSED_RGB_ARB),
        element(GL_COMPRESSED_RGBA_ARB),
        element(GL_COMPRESSED_TEXTURE_FORMATS_ARB),
        element(GL_COMPRESSED_RGBA_BPTC_UNORM_ARB),
        element(GL_COMPRESSED_SRGB_ALPHA_BPTC_UNORM_ARB),
        element(GL_COMPRESSED_RGB_BPTC_SIGNED_FLOAT_ARB),
        element(GL_COMPRESSED_RGB_BPTC_UNSIGNED_FLOAT_ARB),
        element(GL_COMPRESSED_LUMINANCE_LATC1_EXT),
        element(GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT),
        element(GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT),
        element(GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT),
        element(GL_COMPRESSED_RED_RGTC1_EXT),
        element(GL_COMPRESSED_SIGNED_RED_RGTC1_EXT),
        element(GL_COMPRESSED_RED_GREEN_RGTC2_EXT),
        element(GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT),
        element(GL_COMPRESSED_RGB_S3TC_DXT1_EXT),
        element(GL_COMPRESSED_RGBA_S3TC_DXT1_EXT),
        element(GL_COMPRESSED_RGBA_S3TC_DXT3_EXT),
        element(GL_COMPRESSED_RGBA_S3TC_DXT5_EXT),
        element(GL_COMPRESSED_SRGB_EXT),
        element(GL_COMPRESSED_SRGB_ALPHA_EXT),
        element(GL_COMPRESSED_SLUMINANCE_EXT),
        element(GL_COMPRESSED_SLUMINANCE_ALPHA_EXT),
        element(GL_COMPRESSED_SRGB_S3TC_DXT1_EXT),
        element(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT),
        element(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT),
        element(GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT),
        element(GL_COMPRESSED_RGBA_ASTC_4x4_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_5x4_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_5x5_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_6x5_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_6x6_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_8x5_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_8x6_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_8x8_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_10x5_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_10x6_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_10x8_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_10x10_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_12x10_KHR),
        element(GL_COMPRESSED_RGBA_ASTC_12x12_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_4x4_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x4_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_5x5_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x5_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_6x6_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x5_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x6_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_8x8_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x5_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x6_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x8_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_10x10_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x10_KHR),
        element(GL_COMPRESSED_SRGB8_ALPHA8_ASTC_12x12_KHR),
    };
    auto it = names.find(id);
    if (it != names.end()) {
        return it->second;
    }

    ;
    auto res = names.emplace(id, std::format("{:x} UNKNOWN COMPRESSION", (int)id));
    return res.first->second;
}