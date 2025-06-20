//make
//make run
//make emcc
//npx live-server . -o -p 9999

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <ostream>
#include <string>
#include <iostream>
#include <vector>
#include <math.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <platform.hpp>
#include <glcontext.hpp>
#include <shader.hpp>
#include <loadGLTF.hpp>

struct viewmat
{
    glm::mat4 view;
};
struct projmat
{
    glm::mat4 projection;
};

int screenWidth = 1000;
int screenHeight = 800;
SDL_Window* window;

tinygltf::Model modelVedal;
tinygltf::Model modelVedal2;
std::pair<GLuint, std::map<int, GLuint>> vaoAndEbos;
std::pair<GLuint, std::map<int, GLuint>> vaoAndEbos2;

float rot = 0.05f;
glm::mat4 model;
glm::mat4 model2;

GLint modelpos;
GLint projectionpos;

void image(const char* filepath)
{
    SDL_Surface *image = IMG_Load(filepath);

    int bitsPerPixel = image->format->BitsPerPixel;
    printf("Image dimensions %dx%d, %d bits per pixel\n", image->w, image->h, bitsPerPixel);

    GLint format = GL_RGB;
    if (bitsPerPixel == 32)
    {
        format = GL_RGBA;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, image->w, image->h, 0, format, GL_UNSIGNED_BYTE, image->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    SDL_FreeSurface(image);
}

bool loop()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT)
        {
            return false;
        }
    }

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    rot += 0.05f;

    model = glm::translate(glm::mat4(1.0f), glm::vec3(-0.75,-0.5,0));
    model = glm::rotate(model, rot, glm::vec3(0, 1, 0));
    glUniformMatrix4fv(modelpos, 1, GL_FALSE, &model[0][0]);
    drawModel(vaoAndEbos, modelVedal);

    model2 = glm::translate(glm::mat4(1.0f), glm::vec3(1.5,0,0)) * model;
    glUniformMatrix4fv(modelpos, 1, GL_FALSE, &model2[0][0]);
    drawModel(vaoAndEbos2, modelVedal2);

    SDL_GL_SwapWindow(window);

    return true;
}

#ifdef TARGET_PLATFORM_WEB
void chooseLoop()
{
    emscripten_set_main_loop((em_callback_func) loop, 0, 0);
}
#else
void chooseLoop()
{
    bool running = true;
    while(running)
    {
        running = loop();
    }
}
#endif

class objects
{
    public:
    std::pair<GLuint, std::map<int, GLuint>> makeMesh(tinygltf::Model &model, int shaderProgram, const char* filepath)
    {
        if (!loadModel(model, filepath))
        {
            std::cerr << "Failed to load glTF model." << std::endl;
        }

        std::pair<GLuint, std::map<int, GLuint>> vaoAndEboslocal = bindModel(model);

        glm::mat4 modelmat = glm::mat4(1.0);
        glUniformMatrix4fv(modelpos, 1, GL_FALSE, &modelmat[0][0]);

        return vaoAndEboslocal;
    }
    void setUniformBuffer(int shaderProgram)
    {
        GLuint ubo;
        glGenBuffers(1, &ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo);

        projmat dataproj = {};
        dataproj.projection = glm::perspective(glm::radians(45.0f), (float)screenWidth/(float)screenHeight, 0.1f, 100.0f);

        GLuint blockIndex2 = glGetUniformBlockIndex(shaderProgram, "PROJ");

        glUniformBlockBinding(shaderProgram, blockIndex2, 0);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(projmat), &dataproj, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex2, ubo);

        GLuint ubo2;
        glGenBuffers(1, &ubo2);
        glBindBuffer(GL_UNIFORM_BUFFER, ubo2);

        viewmat data = {};
        data.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        GLuint blockIndex = glGetUniformBlockIndex(shaderProgram, "VIEW");

        glUniformBlockBinding(shaderProgram, blockIndex, 1);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(viewmat), &data, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, ubo2);

        modelpos = glGetUniformLocation(shaderProgram, "model");
    }
    void createObjects()
    {
        int shaderProgram = makeShader("shaders/shader3D.vs", "shaders/shader3D.fs");
        glUseProgram(shaderProgram);
        setUniformBuffer(shaderProgram);

        vaoAndEbos = makeMesh(modelVedal, shaderProgram, "models/vedal987/vedal987.gltf");
        vaoAndEbos2 = makeMesh(modelVedal2, shaderProgram, "models/vedal987/vedal987.gltf");
    }
};
int main(int argc, char* argv[])
{
    window = InitGLContext(screenWidth, screenHeight, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    objects obj;
    obj.createObjects();

    chooseLoop();
    return 0;
}