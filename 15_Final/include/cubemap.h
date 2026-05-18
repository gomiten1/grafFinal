#ifndef CUBEMAP_H
#define CUBEMAP_H

#include <glad/glad.h> 

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
#include <stdlib.h>
#include <shader_m.h>

using namespace std;

#include <glm/gtx/string_cast.hpp>

class CubeMap {

public:
	CubeMap():textureID(0), VAO(0), VBO(0), EBO(0){
        float size = 500.0f;
        float skyboxVertices[] = {
            // positions          
            -size,  size, -size,
            -size, -size, -size,
             size, -size, -size,
             size, -size, -size,
             size,  size, -size,
            -size,  size, -size,

            -size, -size,  size,
            -size, -size, -size,
            -size,  size, -size,
            -size,  size, -size,
            -size,  size,  size,
            -size, -size,  size,

             size, -size, -size,
             size, -size,  size,
             size,  size,  size,
             size,  size,  size,
             size,  size, -size,
             size, -size, -size,

            -size, -size,  size,
            -size,  size,  size,
             size,  size,  size,
             size,  size,  size,
             size, -size,  size,
            -size, -size,  size,

            -size,  size, -size,
             size,  size, -size,
             size,  size,  size,
             size,  size,  size,
            -size,  size,  size,
            -size,  size, -size,

            -size, -size, -size,
            -size, -size,  size,
             size, -size, -size,
             size, -size, -size,
            -size, -size,  size,
             size, -size,  size
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, 36 * 3 * sizeof(float), skyboxVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glBindVertexArray(0);

	}

	~CubeMap() {
	
	}

    void loadCubemap(vector<std::string> faces)
    {
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        int width, height, nrChannels;
        for (unsigned int i = 0; i < faces.size(); i++)
        {
            unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
                );
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    }

    // Cielo azul → grisáceo según pollutionGray (0 = limpio, 1 = máxima contaminación)
    void loadProceduralSkyCubemap(int faceSize = 128, float pollutionGray = 0.0f)
    {
        pollutionGray = glm::clamp(pollutionGray, 0.0f, 1.0f);

        const int w = faceSize;
        const int h = faceSize;
        std::vector<unsigned char> faceData((size_t)w * (size_t)h * 3);

        auto skyRgb = [pollutionGray](const glm::vec3& dir) -> glm::vec3 {
            const glm::vec3 d = glm::normalize(dir);
            const glm::vec3 zenith(0.52f, 0.78f, 0.98f);
            const glm::vec3 horizon(0.38f, 0.62f, 0.88f);
            const glm::vec3 nadir(0.06f, 0.12f, 0.26f);
            glm::vec3 col;
            if (d.y >= 0.0f) {
                col = glm::mix(horizon, zenith, powf(d.y, 0.55f));
            }
            else {
                col = glm::mix(horizon, nadir, powf(-d.y, 0.75f));
            }

            const glm::vec3 grayZenith(0.58f, 0.60f, 0.63f);
            const glm::vec3 grayHorizon(0.45f, 0.47f, 0.49f);
            const glm::vec3 grayNadir(0.24f, 0.25f, 0.27f);
            glm::vec3 smog;
            if (d.y >= 0.0f) {
                smog = glm::mix(grayHorizon, grayZenith, powf(d.y, 0.55f));
            }
            else {
                smog = glm::mix(grayHorizon, grayNadir, powf(-d.y, 0.75f));
            }
            return glm::mix(col, smog, pollutionGray);
        };

        auto dirForFace = [](unsigned int face, float u, float v) -> glm::vec3 {
            switch (face) {
            case 0: return glm::vec3( 1.0f, -v, -u);
            case 1: return glm::vec3(-1.0f, -v,  u);
            case 2: return glm::vec3( u,  1.0f,  v);
            case 3: return glm::vec3( u, -1.0f, -v);
            case 4: return glm::vec3( u, -v,  1.0f);
            default: return glm::vec3(-u, -v, -1.0f);
            }
        };

        if (textureID == 0) {
            glGenTextures(1, &textureID);
        }
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        for (unsigned int face = 0; face < 6; ++face) {
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    const float u = ((x + 0.5f) / (float)w) * 2.0f - 1.0f;
                    const float v = ((y + 0.5f) / (float)h) * 2.0f - 1.0f;
                    const glm::vec3 rgbF = skyRgb(dirForFace(face, u, v)) * 255.0f;
                    const size_t i = ((size_t)y * (size_t)w + (size_t)x) * 3;
                    faceData[i + 0] = (unsigned char)glm::clamp(rgbF.r, 0.0f, 255.0f);
                    faceData[i + 1] = (unsigned char)glm::clamp(rgbF.g, 0.0f, 255.0f);
                    faceData[i + 2] = (unsigned char)glm::clamp(rgbF.b, 0.0f, 255.0f);
                }
            }
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, faceData.data());
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }

    void drawCubeMap(Shader &shad, glm::mat4 &projection, glm::mat4 &view) {
        
        glUseProgram(0);
        glDepthMask(GL_FALSE);
        shad.use();
        
        shad.setMat4("projection", projection);
        shad.setMat4("view", view);

        glBindVertexArray(VAO);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
        glDrawArrays(GL_TRIANGLES, 0, 36*3);
        glDepthMask(GL_TRUE);
        glUseProgram(0);
    }

    unsigned int VAO;
    unsigned int textureID; // Cubemap texture id

private:

    unsigned int VBO, EBO;

};

#endif