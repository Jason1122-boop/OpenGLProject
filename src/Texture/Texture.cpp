#include "Texture.h"
#include <glad/glad.h>
#include <stb_image.h>
#include <iostream>

Texture::Texture(const std::string& path)
{
    glGenTextures(1, &ID);
    glBindTexture(GL_TEXTURE_2D, ID);

    int w, h, c;
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 0);

    if (data)
    {
        GLenum format = (c == 4) ? GL_RGBA : GL_RGB;

        glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Texture load fail: " << path << std::endl;
    }

    stbi_image_free(data);
}

void Texture::bind()
{
    glBindTexture(GL_TEXTURE_2D, ID);
}