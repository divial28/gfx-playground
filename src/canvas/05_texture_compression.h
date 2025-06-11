#pragma once

#include "canvas.h"
#include "utils.h"

#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include <string>

class TextureCompressionCanvas : public Canvas
{
public:
    TextureCompressionCanvas();
    ~TextureCompressionCanvas();

    void BuildUI() override;
    void Render() override;

private:
    void FetchSupportedCompressions();
    GLuint LoadTexture(const std::string& path, GLenum compression = GL_NONE);

    void DeleteTexture(GLuint texture);
    void UpdateAllTextures();
    static const std::string& GetCompressionName(GLenum compression);

private:

    std::string imagePath_ = "./assets/Lenna_512x512.png";
    ImVec2 imageSize_ = {};
    std::vector<GLenum> supportedCompressions_;
    GLuint shader_;
    std::array<GLuint, 2> textures_ = {0, 0};
    std::array<uint32_t, 2> compressions_ = {0, 0};
    Color bgColor_ = Utils::GetNextColorFromPalette();
    glm::mat4 proj_;
    float edge_ = 0.5;
    float zoom_ = 1.0f;
};
