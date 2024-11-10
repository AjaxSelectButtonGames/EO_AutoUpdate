#pragma once
#include "atlas_ui_utilities.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <GL/glew.h>
#include <iostream>

namespace AtlasRendering {

    struct TextureInfo {
        GLuint id;
        int width;
        int height;
    };

    GLuint VAO, VBO, EBO;

    TextureInfo loadTexture(const char* filePath) {
        TextureInfo textureInfo = { 0, 0, 0 };

        // Load the image using SDL2
        SDL_Surface* surface = IMG_Load(filePath);
        if (!surface) {
            std::cerr << "Failed to load texture: " << IMG_GetError() << std::endl;
            return textureInfo;
        }

        // Generate an OpenGL texture ID
        glGenTextures(1, &textureInfo.id);
        glBindTexture(GL_TEXTURE_2D, textureInfo.id);

        // Set texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // Determine the format of the surface
        GLenum format;
        if (surface->format->BytesPerPixel == 4) { // RGBA
            format = (surface->format->Rmask == 0x000000ff) ? GL_RGBA : GL_BGRA;
        }
        else { // RGB
            format = (surface->format->Rmask == 0x000000ff) ? GL_RGB : GL_BGR;
        }

        // Upload the texture data to OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0, format, surface->w, surface->h, 0, format, GL_UNSIGNED_BYTE, surface->pixels);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Store texture dimensions
        textureInfo.width = surface->w;
        textureInfo.height = surface->h;

        // Free the SDL surface
        SDL_FreeSurface(surface);

        // Unbind the texture
        glBindTexture(GL_TEXTURE_2D, 0);

        return textureInfo;
    }

    void setupVBO() {
        float vertices[] = {
            // Positions       // Texture Coords
            0.0f, 1.0f,        0.0f, 1.0f,
            1.0f, 0.0f,        1.0f, 0.0f,
            0.0f, 0.0f,        0.0f, 0.0f,

            0.0f, 1.0f,        0.0f, 1.0f,
            1.0f, 1.0f,        1.0f, 1.0f,
            1.0f, 0.0f,        1.0f, 0.0f
        };

        glGenBuffers(1, &VBO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    }

    void renderTexture(TextureInfo texture, float x, float y, float width, float height) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture.id);

        glPushMatrix();
        glTranslatef(x, y, 0.0f);
        glScalef(width, height, 1.0f);

        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glVertexPointer(2, GL_FLOAT, 4 * sizeof(float), (void*)0);
        glTexCoordPointer(2, GL_FLOAT, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);

        glPopMatrix();

        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);
    }
}
