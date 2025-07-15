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
#include <execution>
#include <optional>

#include <platform.hpp>
#ifdef TARGET_PLATFORM_WEB
	#include <emscripten.h>
    #include <emscripten/html5.h>
	#include <GLES3/gl3.h>
#else
	#include <glew/glew.h>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_INTRINSICS
//#define GLM_FORCE_ALIGNED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/type_aligned.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include <glcontext.hpp>
#include <shader.hpp>
#include <loadGLTF.hpp>

struct alignas(16) viewmat
{
    glm::mat4 view;
};
struct alignas(16) projmat
{
    glm::mat4 projection;
};
struct alignas(16) lighting
{
    glm::vec4 lightcolor;
    glm::vec3 lightposition;
};
struct alignas(16) campos
{
    glm::vec3 camPos;
};

int WIDTH = 1920;
int HEIGHT = 1080;
SDL_Window* window;
float farplane = 500.0f;

GLint modelpos;

GLuint uboProjection;
GLuint uboView;
GLuint uboLight;
GLuint uboCampos;

GLuint vao;
GLint loc;

int shaderBoundingbox;
int shaderProgram;

class Object
{
    public:
        glm::vec3 position;
        glm::vec3 eulers = glm::vec3(0,0,0);
        glm::mat4 transmat = glm::mat4(1.0f);

        Object(glm::vec3 pos = glm::vec3(0,0,0))
        {
            position = pos;
            makeTransmat();
        }
        void makeTransmat()
        {
            transmat = glm::rotate(glm::mat4(1.0f), eulers.x, glm::vec3(1,0,0));
            transmat = glm::rotate(transmat, eulers.y, glm::vec3(0,1,0));
            transmat = glm::rotate(transmat, eulers.z, glm::vec3(0,0,1));
            transmat[3][0] = position.x;
            transmat[3][1] = position.y;
            transmat[3][2] = position.z;
        }
        ~Object()
        {
        }
};
struct RenderableObject
{
    Object object;
    std::shared_ptr<Model> model;
};
std::vector<RenderableObject> objectlist;

class Camera: public Object
{
    public:
        alignas(16) glm::vec3 forward;
        alignas(16) glm::vec3 right;
        alignas(16) glm::vec3 up;
        float zoom;

        Camera(glm::vec3 center): Object()
        {
            forward = glm::vec3(0,0,1);
            right = glm::vec3(1,0,0);
            up = glm::vec3(0,1,0);
            zoom = 10.0f;
        }
        std::pair<float, float> update()
        {
            float cosX = std::cos(eulers.x);
            float sinX = std::sin(eulers.x);
            float cosY = std::cos(eulers.y);
            float sinY = std::sin(eulers.y);

            forward = glm::vec3(cosX*cosY, sinY, -sinX*cosY);
            right = glm::vec3(sinX, 0, cosX);
            up = glm::vec3(-cosX*sinY, cosY, sinX*sinY);

            return {cosX, sinX};
        }
        void makeView(glm::vec3 center)
        {
            position = center - forward * zoom;

            viewData.view[0][0] = right.x;
            viewData.view[1][0] = right.y;
            viewData.view[2][0] = right.z;
            viewData.view[3][0] = -glm::dot(right, position);

            viewData.view[0][1] = up.x;
            viewData.view[1][1] = up.y;
            viewData.view[2][1] = up.z;
            viewData.view[3][1] = -glm::dot(up, position);

            viewData.view[0][2] = -forward.x;
            viewData.view[1][2] = -forward.y;
            viewData.view[2][2] = -forward.z;
            viewData.view[3][2] = glm::dot(forward, position);

            viewData.view[0][3] = 0.0f;
            viewData.view[1][3] = 0.0f;
            viewData.view[2][3] = 0.0f;
            viewData.view[3][3] = 1.0f;

            glBindBuffer(GL_UNIFORM_BUFFER, uboView);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(viewData), &viewData);

            camData.camPos = position;
            glBindBuffer(GL_UNIFORM_BUFFER, uboCampos);
            glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camData), &camData);
        }
        ~Camera()
        {
        }
    private:
        viewmat viewData = {};
        campos camData = {};
};
class Player: public Object
{
    public:
        Camera cam;
        glm::vec3 forward;
        glm::vec3 right;

        Player(glm::vec3 pos = glm::vec3(-55,0,0)): Object(pos), cam(position)
        {
            eulers = glm::vec3(0,0,0);
            forward = glm::vec3(0,0,1);
            right = glm::vec3(1,0,0);
        }
        void update(glm::vec2 move, float deltaTime)
        {
            glm::vec3 movement = glm::normalize(forward * move.y - right * move.x);
            movement = checkCollision(movement) * deltaTime * 0.02f;

            position += movement;
            cam.makeView(position);
            makeTransmat();
        }
        void updateCam()
        {
            auto [cosX, sinX] = cam.update();
            forward = glm::vec3(cosX, 0, -sinX);
            right = glm::vec3(-sinX, 0, -cosX);
            cam.makeView(position);
        }
        ~Player()
        {
        }

    private:
        glm::vec3 checkCollision(glm::vec3 movement, glm::vec3 offset = glm::vec3(0.0f), bool recursion = false)
        {
            glm::vec3 startpos = position + offset;
            glm::vec3 endpos = startpos + movement;
            float distance = 1.0f;
            float height = 0.0f;
            glm::vec3 normal(0.0f);
            glm::vec3 boundingBox[2];

            glm::vec3 pathMin = glm::min(startpos, endpos) - glm::vec3(1);
            glm::vec3 pathMax = glm::max(startpos, endpos) + glm::vec3(1);

            for (const RenderableObject &rendObj : objectlist)
            {
                boundingbox aabb = rendObj.model->aabb;

                if
                (
                    aabb.max.x >= pathMin.x &&
                    aabb.min.x <= pathMax.x &&
                    aabb.max.z >= pathMin.z &&
                    aabb.min.z <= pathMax.z
                )
                {
                    for (boundingbox box : rendObj.model->boundingboxes)
                    {
                        std::optional<rayHit> collision = box.intersectRay(movement, startpos);
                        if (collision)
                        {
                            if (box.max.y < 1 + startpos.y)
                            {
                                if (box.min.x < startpos.x && startpos.x < box.max.x && box.min.z < startpos.z && startpos.z < box.max.z)
                                {
                                    height = std::max(height, box.max.y);
                                }
                            }
                            else
                            {
                                rayHit hit = *collision;
                                if (distance > hit.distance)
                                {
                                    distance = hit.distance;
                                    normal = hit.normal;
                                    boundingBox[0] = box.min;
                                    boundingBox[1] = box.max;
                                }
                            }
                        }
                    }
                }
            }

            glm::vec3 closestCollision = movement * distance + 0.01f * normal;

            if (distance != 1.0f)
            {
                if (!recursion)
                {
                    glm::vec3 remainingMovement = movement * (1.0f - distance);
                    remainingMovement -= glm::dot(remainingMovement, normal) * normal;
                    closestCollision += checkCollision(remainingMovement, closestCollision, true);
                }
                glUniform3fv(loc, 2, &boundingBox[0][0]);
            }

            position.y += height;
            return closestCollision;
        }
};
struct PlayerObject
{
    Player object;
    std::shared_ptr<Model> model;
};
PlayerObject playobj;

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

extern "C" void renderLoopWrapper(void* arg);

class Render
{
    public:
        Render()
        {
            chooseLoop();
        }
        void loopOnce()
        {
            if (!baseloop())
            {
                #ifdef TARGET_PLATFORM_WEB
                    emscripten_cancel_main_loop(); // stop loop if baseloop returned false
                #endif
            }
        }
        inline void chooseLoop()
        {
            #ifdef TARGET_PLATFORM_WEB
                emscripten_set_main_loop_arg(renderLoopWrapper, this, 0, 1);
            #else
                nativeLoop();
            #endif
        }
        ~Render()
        {
        }

    private:
        Uint32 previousFrameTime = SDL_GetTicks();
        Uint32 lastTime = SDL_GetTicks();
        int frameCount = 0;
        float deltaTime = 0.0f;

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
            bool updateCam = false;

            SDL_Event event;
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                case SDL_QUIT:
                    return false;
                    break;
                case SDL_MOUSEMOTION:
                    playobj.object.cam.eulers.x -= event.motion.xrel * 0.001;
                    playobj.object.cam.eulers.y -= event.motion.yrel * 0.001;
                    updateCam = true;
                    break;
                }
            }

            const Uint8* keystate = SDL_GetKeyboardState(NULL);
            glm::vec2 move = glm::vec2(0,0);

            if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP])    move.y += 1;
            if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN])  move.y -= 1;
            if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT])  move.x -= 1;
            if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) move.x += 1;

            if (move.x or move.y)
            {
                playobj.object.update(move, deltaTime);
                playobj.object.updateCam();
            }
            if (updateCam)
            {
                playobj.object.updateCam();
            }

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUseProgram(shaderProgram);

            glUniformMatrix4fv(modelpos, 1, GL_FALSE, &playobj.object.transmat[0][0]);
            playobj.model->drawModel();

            for (RenderableObject &rendObj : objectlist)
            {
                glUniformMatrix4fv(modelpos, 1, GL_FALSE, &rendObj.object.transmat[0][0]);
                rendObj.model->drawModel();
            }

            glUseProgram(shaderBoundingbox);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            getFPS();
            SDL_GL_SwapWindow(window);

            return true;
        }
        void getFPS()
        {
            frameCount++;

            Uint32 currentTime = SDL_GetTicks();
            deltaTime = currentTime - previousFrameTime;
            previousFrameTime = currentTime;

            if (currentTime - lastTime >= 1000)
            {
                float delta = currentTime - lastTime;
                float fps = frameCount * 1000.0f / delta;
                float avgFrameTime = delta / frameCount;

                std::string title = "FPS: " + std::to_string((int)fps) + " | Frame Time: " + std::to_string(avgFrameTime).substr(0, 5) + " ms";
                SDL_SetWindowTitle(window, title.c_str());
                frameCount = 0;
                lastTime = currentTime;
            }
        }
};
extern "C" void renderLoopWrapper(void* arg)
{
    static_cast<Render*>(arg)->loopOnce();
}

class Scene
{
    public:
        Scene(int sceneNr)
        {
            if (sceneNr == 0)
            {
                shaderBoundingbox = makeShader("shaders/shaderBoundingbox.vs", "shaders/shaderBoundingbox.fs");
                glUseProgram(shaderBoundingbox);

                glGenVertexArrays(1, &vao);
                glBindVertexArray(vao);

                GLuint data[] =
                {
                    0, 2, 1, 3, 1, 2,
                    4, 5, 6, 7, 6, 5,
                    0, 1, 4, 5, 4, 1,
                    2, 6, 3, 7, 3, 6,
                    0, 4, 2, 6, 2, 4,
                    1, 3, 5, 7, 5, 3
                };

                GLuint vbo;
                glGenBuffers(1, &vbo);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

                glEnableVertexAttribArray(0);
                glVertexAttribIPointer(0, 1, GL_UNSIGNED_INT, sizeof(GLuint), (void*)0);

                glm::vec3 boundingBox[2] =
                {
                    glm::vec3(-50,0,-50),
                    glm::vec3(50,50,50)
                };

                loc = glGetUniformLocation(shaderBoundingbox, "boundingBox");
                glUniform3fv(loc, 2, &boundingBox[0][0]);



                shaderProgram = makeShader("shaders/shader3D.vs", "shaders/shader3D.fs");
                glUseProgram(shaderProgram);

                playobj = PlayerObject{Player(), std::make_shared<Model>("models/vedal987/vedal987.gltf")};

                objectlist =
                {
                    {Object(glm::vec3(-8,     5.55, 40    )), std::make_shared<Model>("models/V-nexus/Camilla's_tent/Camillas_tent.gltf")},
                    {Object(glm::vec3(43,     2.35, 7.775 )), std::make_shared<Model>("models/V-nexus/drone_factory/drone_factory.gltf")},
                    {Object(glm::vec3(0,      2.5,  0     )), std::make_shared<Model>("models/V-nexus/floors/floors.gltf")},
                    {Object(glm::vec3(46,     6.31, -33.75)), std::make_shared<Model>("models/V-nexus/item_factory/item_factory.gltf")},
                    {Object(glm::vec3(42,     0.1,  46    )), std::make_shared<Model>("models/V-nexus/item_shop/item_shop.gltf")},
                    {Object(glm::vec3(6,      0,    0     )), std::make_shared<Model>("models/V-nexus/street/street.gltf")},
                    {Object(glm::vec3(41.9,   3.2,  -10.05)), std::make_shared<Model>("models/V-nexus/upgrade_smith/upgrade_smith.gltf")},
                    {Object(glm::vec3(7.75,   0.6,  -37   )), std::make_shared<Model>("models/V-nexus/utilities/utilities.gltf")},
                    {Object(glm::vec3(0,      2.5,  0     )), std::make_shared<Model>("models/V-nexus/walls/walls.gltf")},
                    {Object(glm::vec3(2,      51.1, 4     )), std::make_shared<Model>("models/V-nexus/world_center/world_center.gltf")},
                    {Object(glm::vec3(-36.15, 5.5,  26    )), std::make_shared<Model>("models/V-nexus/vedal's_house/vedals_house.gltf")},
                };

                for (RenderableObject &rendObj : objectlist)
                {
                    glm::vec3 position = rendObj.object.position;
                    rendObj.model->aabb.min += position;
                    rendObj.model->aabb.max += position;

                    for (boundingbox &box : rendObj.model->boundingboxes)
                    {
                        box.min += position - glm::vec3(0.7, 0.1, 0.7);
                        box.max += position + glm::vec3(0.7, 0.1, 0.7);
                    }
                }
                setUniformBuffer();
            }
        }
        ~Scene()
        {
        }

    private:
        void setUniformBuffer()
        {
            projmat dataproj = {};
            dataproj.projection = glm::perspective(glm::radians(45.0f), (float)WIDTH/(float)HEIGHT, 0.1f, farplane);
            viewmat data = {};
            data.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            lighting datalight = {};
            datalight.lightcolor = glm::vec4(244.0f, 233.0f, 155.0f, 50.0f);
            datalight.lightposition = glm::vec3(0.0f, 100.0f, 0.0f);
            campos datacam = {};
            datacam.camPos = glm::vec3(0.0f,10.0f,0.0f);

            GLuint blockIndex;

            glGenBuffers(1, &uboProjection);
            glBindBuffer(GL_UNIFORM_BUFFER, uboProjection);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(projmat), &dataproj, GL_STATIC_DRAW);
            glGenBuffers(1, &uboView);
            glBindBuffer(GL_UNIFORM_BUFFER, uboView);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(viewmat), &data, GL_DYNAMIC_DRAW);
            glGenBuffers(1, &uboLight);
            glBindBuffer(GL_UNIFORM_BUFFER, uboLight);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(lighting), &datalight, GL_STATIC_DRAW);
            glGenBuffers(1, &uboCampos);
            glBindBuffer(GL_UNIFORM_BUFFER, uboCampos);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(campos), &datacam, GL_DYNAMIC_DRAW);

            for (int shader : std::initializer_list<int>{shaderProgram, shaderBoundingbox})
            {
                blockIndex = glGetUniformBlockIndex(shader, "PROJ");
                glUniformBlockBinding(shader, blockIndex, 0);
                glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, uboProjection);

                blockIndex = glGetUniformBlockIndex(shader, "VIEW");
                glUniformBlockBinding(shader, blockIndex, 1);
                glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, uboView);
            }

            blockIndex = glGetUniformBlockIndex(shaderProgram, "lighting");
            glUniformBlockBinding(shaderProgram, blockIndex, 2);
            glBindBufferBase(GL_UNIFORM_BUFFER, 2, uboLight);

            blockIndex = glGetUniformBlockIndex(shaderProgram, "cam");
            glUniformBlockBinding(shaderProgram, blockIndex, 3);
            glBindBufferBase(GL_UNIFORM_BUFFER, 3, uboCampos);

            modelpos = glGetUniformLocation(shaderProgram, "model");
        }

};
int main(int argc, char* argv[])
{
    window = InitGLContext(WIDTH, HEIGHT, 0);
    SDL_GL_SetSwapInterval(0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Scene scene(0);

    Render renderer;
    return 0;
}