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
#include <list>
#include <memory>

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

int WIDTH = 1080;
int HEIGHT = 720;
SDL_Window* window;

GLint modelpos;
GLint projectionpos;

class Object
{
    public:
        Object()
        {
        }
        ~Object()
        {
        }
        glm::vec3 position = glm::vec3(0,0,0);
        glm::vec3 eulers = glm::vec3(0,0,0);
        glm::mat4 transmat = glm::mat4(1.0f);

        void makeTransmat()
        {
            transmat = glm::translate(glm::mat4(1.0f), position);
            transmat = glm::rotate(transmat, eulers.x, glm::vec3(1,0,0));
            transmat = glm::rotate(transmat, eulers.y, glm::vec3(0,1,0));
            transmat = glm::rotate(transmat, eulers.z, glm::vec3(0,0,1));
        }
};
class Camera: public Object
{
    public:
        glm::vec3 forwards;
        float zoom;

        Camera(glm::vec3 center): Object()
        {
            forwards = glm::vec3(0,0,1);
            zoom = 3.0f;
            position = center + zoom * forwards;
        }

        ~Camera()
        {
        }
};
class Player: public Object
{
    public:
        Camera cam;

        Player(): Object(), cam(position)
        {
            position = glm::vec3(0,0,0);
            eulers = glm::vec3(0,0,0);
            glm::vec3 forwards = glm::vec3(0,0,1);
        }

        ~Player()
        {
        }
};

struct RenderableObject
{
    Object object;
    std::shared_ptr<tinygltf::Model> mesh;
    VaoData vaoAndEbos;
};
struct TextureHandle
{
    GLuint handle;
};
void useTex(TextureHandle texture)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.handle);
}
TextureHandle makeTex(const char* filepath)
{
    SDL_Surface *image = IMG_Load(filepath);

    int bitsPerPixel = image->format->BitsPerPixel;
    printf("Image dimensions %dx%d, %d bits per pixel\n", image->w, image->h, bitsPerPixel);

    GLint format = GL_RGB;
    if (bitsPerPixel == 32) {format = GL_RGBA;}

    TextureHandle texture;
    glGenTextures(1, &texture.handle);
    glBindTexture(GL_TEXTURE_2D, texture.handle);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, format, image->w, image->h, 0, format, GL_UNSIGNED_BYTE, image->pixels);
    glGenerateMipmap(GL_TEXTURE_2D);

    SDL_FreeSurface(image);
    return texture;
}

std::list<RenderableObject> objectlist;

class Render
{
    public:
        Render()
        {
        }
        inline void chooseLoop()
        {
            #ifdef TARGET_PLATFORM_WEB
                static Render* instance;
                emscripten_set_main_loop([](){instance->baseloop();}, 0, 0);
            #else
                nativeLoop();
            #endif
        }

    private:
        void nativeLoop()
        {
            bool running = true;
            while(running)
            {
                running = baseloop();
            }
        }
        bool baseloop()
        {
            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_QUIT:
                    return false;
                    break;
                }
            }

            const Uint8* keystate = SDL_GetKeyboardState(NULL);
            glm::vec2 move = glm::vec2(0,0);

            if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP])    move.y -= 1.0f;
            if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN])  move.y += 1.0f;
            if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT])  move.x -= 1.0f;
            if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) move.x += 1.0f;

            if (move.x or move.y)
            {
                objectlist.front().object.position.x += move.x * 0.02;
                objectlist.front().object.position.z += move.y * 0.02;
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            for (RenderableObject &rendObj : objectlist)
            {
                rendObj.object.eulers.y += 0.05;
                rendObj.object.makeTransmat();

                glUniformMatrix4fv(modelpos, 1, GL_FALSE, &rendObj.object.transmat[0][0]);
                drawModel(rendObj.vaoAndEbos, *rendObj.mesh);
            }

            SDL_GL_SwapWindow(window);

            return true;
        }
};

class Scene
{
    public:
        Scene(int sceneNr)
        {
            if (sceneNr == 0)
            {
                int shaderProgram = makeShader("shaders/shader3D.vs", "shaders/shader3D.fs");
                glUseProgram(shaderProgram);
                setUniformBuffer(shaderProgram);

                std::shared_ptr<tinygltf::Model> model1 = std::make_shared<tinygltf::Model>();
                //std::shared_ptr<tinygltf::Model> model2 = std::make_shared<tinygltf::Model>();

                Player obj1;
                //Object obj2;

                obj1.position = glm::vec3(-1,-0.5, 0);
                //obj2.position = glm::vec3(1,-0.5, 0);

                obj1.makeTransmat();
                //obj2.makeTransmat();

                VaoData mesh1 = makeMesh(*model1, shaderProgram, "models/vedal987/vedal987.gltf");
                //VaoData mesh2 = makeMesh(*model2, shaderProgram, "models/vedal987/vedal987.gltf");

                objectlist =
                {
                    {obj1, model1, mesh1},
                    //{obj2, model2, mesh2}
                };
            }
        }

    private:
        VaoData makeMesh(tinygltf::Model &model, int shaderProgram, const char* filepath)
        {
            if (!loadModel(model, filepath))
            {
                std::cerr << "Failed to load glTF model." << std::endl;
            }

            VaoData vaoAndEboslocal = bindModel(model);

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
            dataproj.projection = glm::perspective(glm::radians(45.0f), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);

            GLuint blockIndex;
            blockIndex = glGetUniformBlockIndex(shaderProgram, "PROJ");
            glUniformBlockBinding(shaderProgram, blockIndex, 0);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(projmat), &dataproj, GL_STATIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, ubo);

            GLuint ubo2;
            glGenBuffers(1, &ubo2);
            glBindBuffer(GL_UNIFORM_BUFFER, ubo2);

            viewmat data = {};
            data.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            blockIndex = glGetUniformBlockIndex(shaderProgram, "VIEW");
            glUniformBlockBinding(shaderProgram, blockIndex, 1);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(viewmat), &data, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, ubo2);

            modelpos = glGetUniformLocation(shaderProgram, "model");
        }

};
int main(int argc, char* argv[])
{
    window = InitGLContext(WIDTH, HEIGHT, 1);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Scene scene(0);

    Render renderer;
    renderer.chooseLoop();
    return 0;
}