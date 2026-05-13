#pragma once
#include <GLES2/gl2.h>
#include <string>
#include <android/log.h>

#define STB_IMAGE_IMPLEMENTATION
#include "wLibs/stb_image.h"

#define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, "REE_ImageLoader", __VA_ARGS__))

struct TextureInfo {
    GLuint id = 0;
    int width = 0;
    int height = 0;
    bool loaded = false;
};

namespace ImageLoader {

    // Загрузка текстуры из файла
    inline TextureInfo LoadTexture(const char* path) {
        TextureInfo tex;

        int channels;
        unsigned char* data = stbi_load(path, &tex.width, &tex.height, &channels, 4);

        if (!data) {
            LOGD("Failed to load image: %s", path);
            return tex;
        }

        glGenTextures(1, &tex.id);
        glBindTexture(GL_TEXTURE_2D, tex.id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);

        tex.loaded = true;
        LOGD("Loaded image: %s (%dx%d)", path, tex.width, tex.height);

        return tex;
    }

    // Загрузка текстуры из памяти
    inline TextureInfo LoadTextureFromMemory(const unsigned char* buffer, int size) {
        TextureInfo tex;

        int channels;
        unsigned char* data = stbi_load_from_memory(buffer, size, &tex.width, &tex.height, &channels, 4);

        if (!data) {
            LOGD("Failed to load image from memory");
            return tex;
        }

        glGenTextures(1, &tex.id);
        glBindTexture(GL_TEXTURE_2D, tex.id);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex.width, tex.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

        stbi_image_free(data);

        tex.loaded = true;
        LOGD("Loaded image from memory (%dx%d)", tex.width, tex.height);

        return tex;
    }

    // Выгрузка текстуры
    inline void UnloadTexture(TextureInfo& tex) {
        if (tex.loaded && tex.id != 0) {
            glDeleteTextures(1, &tex.id);
            tex.id = 0;
            tex.loaded = false;
            LOGD("Unloaded texture");
        }
    }

    // Перезагрузка текстуры
    inline void ReloadTexture(TextureInfo& tex, const char* path) {
        UnloadTexture(tex);
        tex = LoadTexture(path);
    }
}