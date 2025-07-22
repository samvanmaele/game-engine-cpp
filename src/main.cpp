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
#include <numbers>
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
#include <glm/gtx/euler_angles.hpp>

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

GLint modelpos;
GLint timepos;
GLint forwardpos;
GLint densitypos;
GLint billboardpos;

GLuint uboProjection;
GLuint uboView;
GLuint uboLight;
GLuint uboCampos;

GLuint vao;
GLint loc;

int shaderProgram;
int shaderGrass;
int shaderSkybox;
int shaderBoundingbox;

class Object
{
    public:
        glm::vec3 position;
        glm::vec3 eulers;
        glm::mat4 transmat = glm::mat4(1.0f);
        boundingbox aabb;
        std::vector<boundingbox> boundingboxes;

        Object(glm::vec3 pos = glm::vec3(0,0,0), glm::vec3 eul = glm::vec3(0,0,0))
        {
            position = pos;
            eulers = eul;
            makeTransmat();
        }
        void makeTransmat()
        {
            transmat = glm::eulerAngleXYZ(eulers.x, eulers.y, eulers.z);
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

        Player(glm::vec3 pos = glm::vec3(0,0,0)): Object(pos), cam(pos)
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
                boundingbox aabb = rendObj.object.aabb;
                if
                (
                    aabb.max.x >= pathMin.x &&
                    aabb.min.x <= pathMax.x &&
                    aabb.max.z >= pathMin.z &&
                    aabb.min.z <= pathMax.z
                )
                {
                    for (boundingbox box : rendObj.object.boundingboxes)
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
                glUseProgram(shaderBoundingbox);
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
    TextureHandle texture;
    glGenTextures(1, &texture.handle);
    glBindTexture(GL_TEXTURE_2D, texture.handle);

    SDL_Surface *image = IMG_Load(filepath);
    int bitsPerPixel = image->format->BitsPerPixel;
    GLint format = (bitsPerPixel == 32) ? GL_RGBA : GL_RGB;

    glTexImage2D(GL_TEXTURE_2D, 0, format, image->w, image->h, 0, format, GL_UNSIGNED_BYTE, image->pixels);
    SDL_FreeSurface(image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);

    return texture;
}
TextureHandle makeTex3D(const std::array<const char*, 6>& filepath)
{
    TextureHandle texture;
    glGenTextures(1, &texture.handle);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture.handle);

    for (unsigned int i = 0; i < 6; ++i)
    {
        SDL_Surface *image = IMG_Load(filepath[i]);
        int bitsPerPixel = image->format->BitsPerPixel;
        GLint format = (bitsPerPixel == 32) ? GL_RGBA : GL_RGB;

        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, image->w, image->h, 0, format, GL_UNSIGNED_BYTE, image->pixels);
        SDL_FreeSurface(image);
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}

extern "C" void renderLoopWrapper(void* arg);

class Render
{
    public:
        Render()
        {
            playobj.object.cam.makeView(playobj.object.position);
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
                    playobj.object.cam.eulers.y = std::min(0.9, std::max(-0.9, playobj.object.cam.eulers.y - event.motion.yrel * 0.001));
                    playobj.object.cam.eulers.x -= event.motion.xrel * 0.001;
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

            glDisable(GL_CULL_FACE);

            glUseProgram(shaderSkybox);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glUseProgram(shaderGrass);
            float time = previousFrameTime * 0.002;
            glUniform1fv(timepos, 1, &time);
            glm::vec2 forward = glm::normalize(glm::vec2(playobj.object.cam.forward.x, playobj.object.cam.forward.z));
            glUniform2fv(forwardpos, 1, &forward[0]);

            float density = 1.0/6.0;
            glUniform1fv(densitypos, 1, &density);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 5, 16384);

            density = 1.0/3.0;
            glUniform1fv(densitypos, 1, &density);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 5, 16384);

            density = 1.0/2.0;
            glUniform1fv(densitypos, 1, &density);
            glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 5, 16384);

            glUseProgram(shaderBoundingbox);
            glDrawArrays(GL_TRIANGLES, 0, 36);

            glEnable(GL_CULL_FACE);

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
                shaderProgram = makeShader("shaders/shader3D.vs", "shaders/shader3D.fs");
                shaderGrass = makeShader("shaders/shaderGrass.vs", "shaders/shaderGrass.fs");
                shaderSkybox = makeShader("shaders/shaderSkybox.vs", "shaders/shaderSkybox.fs");
                shaderBoundingbox = makeShader("shaders/shaderBoundingbox.vs", "shaders/shaderBoundingbox.fs");

                setUniformBuffer();
                setBoundingboxShader();
                setSkyboxShader();

                glUseProgram(shaderProgram);

                playobj = PlayerObject{Player(glm::vec3(200.56, 0, -170.47)), std::make_shared<Model>("models/vedal987/vedal987.gltf")};
                auto houseModel = std::make_shared<Model>("models/house/house.gltf");

                objectlist =
                {
                    {Object(glm::vec3(0, 0, -9.9f), glm::vec3(0, std::numbers::pi, 0)), std::make_shared<Model>("models/ground/ground.gltf")},
                    {Object(glm::vec3(232.73, 4, -254.95)), houseModel},
                    {Object(glm::vec3(232.73, 4, -222.77)), houseModel},
                    {Object(glm::vec3(232.73, 4, -190.60)), houseModel},

                    {Object(glm::vec3(226.54, 4, -167.09), glm::vec3(0, (330.0f/180.0f) * std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(200.56, 4, -153.47), glm::vec3(0, (3.0f/2.0f) * std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(174.75, 4, -167.09), glm::vec3(0, (210.0f/180.0f) * std::numbers::pi, 0)), houseModel},

                    {Object(glm::vec3(168.32, 4, -254.95), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(168.32, 4, -222.77), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(168.32, 4, -190.60), glm::vec3(0, std::numbers::pi, 0)), houseModel},

                    {Object(glm::vec3(133.73, 4, -254.95)), houseModel},
                    {Object(glm::vec3(133.73, 4, -222.77)), houseModel},
                    {Object(glm::vec3(133.73, 4, -190.60)), houseModel},
                    {Object(glm::vec3(133.73, 4, -158.42)), houseModel},
                    {Object(glm::vec3(133.73, 4, -126.25)), houseModel},
                    {Object(glm::vec3(133.73, 4, -94.075)), houseModel},
                    {Object(glm::vec3(133.73, 4, -61.900)), houseModel},
                    {Object(glm::vec3(133.73, 4, -29.725)), houseModel},

                    {Object(glm::vec3(79.217, 4, -254.95), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(79.217, 4, -222.77), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(79.217, 4, -190.60), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(79.217, 4, -158.42), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(79.217, 4, -126.25), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(79.217, 4, -94.075), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(79.217, 4, -61.900), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                    {Object(glm::vec3(79.217, 4, -29.725), glm::vec3(0, std::numbers::pi, 0)), houseModel},
                };

                auto computeTransformedAABB = [&](const glm::mat4& transmat, const glm::vec3& bbmin, const glm::vec3& bbmax) -> boundingbox
                {
                    std::array<glm::vec3, 4> transformedPoints =
                    {
                        glm::vec3(transmat * glm::vec4(bbmin, 1.0f)),
                        glm::vec3(transmat * glm::vec4(bbmin.x, bbmin.y, bbmax.z, 1.0f)),
                        glm::vec3(transmat * glm::vec4(bbmax, 1.0f)),
                        glm::vec3(transmat * glm::vec4(bbmax.x, bbmax.y, bbmin.z, 1.0f)),
                    };

                    glm::vec3 min = transformedPoints[0];
                    glm::vec3 max = transformedPoints[0];

                    for (const glm::vec3& v : transformedPoints)
                    {
                        min = glm::min(min, v);
                        max = glm::max(max, v);
                    }

                    boundingbox box = {};
                    box.min = min - glm::vec3(0.4f, 0.1f, 0.4f);
                    box.max = max + glm::vec3(0.4f, 0.1f, 0.4f);

                    return box;
                };
                for (RenderableObject &rendObj : objectlist)
                {
                    glm::mat4 transmat = rendObj.object.transmat;
                    rendObj.object.aabb = computeTransformedAABB(transmat, rendObj.model->aabb.min, rendObj.model->aabb.max);

                    for (const boundingbox &box : rendObj.model->boundingboxes)
                    {
                        rendObj.object.boundingboxes.push_back(computeTransformedAABB(transmat, box.min, box.max));
                    }
                }
            }
        }
        ~Scene()
        {
        }

    private:
        float FOV = glm::radians(45.0f);
        float nearplane = 0.1f;
        float farplane = 500.0f;
        glm::vec3 lightpos = glm::vec3(0.0f, 1000.0f, 0.0f);
        glm::vec3 lightcolour = glm::vec3(244.0f, 233.0f, 155.0f);
        float lightstrenght = 1000.0f;

        void setUniformBuffer()
        {
            projmat dataproj = {};
            dataproj.projection = glm::perspective(FOV, (float)WIDTH/(float)HEIGHT, nearplane, farplane);
            viewmat data = {};
            data.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            lighting datalight = {};
            datalight.lightcolor = glm::vec4(lightcolour, lightstrenght);
            datalight.lightposition = lightpos;
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

            for (int shader : std::initializer_list<int>{shaderProgram, shaderGrass, shaderSkybox, shaderBoundingbox})
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

            for (int shader : std::initializer_list<int>{shaderProgram, shaderGrass})
            {
                blockIndex = glGetUniformBlockIndex(shader, "cam");
                glUniformBlockBinding(shader, blockIndex, 3);
                glBindBufferBase(GL_UNIFORM_BUFFER, 3, uboCampos);
            }
            modelpos = glGetUniformLocation(shaderProgram, "model");
            timepos = glGetUniformLocation(shaderGrass, "time");
            forwardpos = glGetUniformLocation(shaderGrass, "forward");
            densitypos = glGetUniformLocation(shaderGrass, "density");
            billboardpos = glGetUniformLocation(shaderGrass, "billboard");
        }
        void setBoundingboxShader()
        {
            glUseProgram(shaderBoundingbox);
            loc = glGetUniformLocation(shaderBoundingbox, "boundingBox");
            glm::vec3 boundingBox[2] = {glm::vec3(0,0,0), glm::vec3(1,1,1)};
            glUniform3fv(loc, 2, &boundingBox[0][0]);
        }
        void setSkyboxShader()
        {
            glUseProgram(shaderSkybox);
            std::array<const char*, 6> faces =
            {
                "gfx/skybox/skybox_right.png",
                "gfx/skybox/skybox_left.png",
                "gfx/skybox/skybox_top.png",
                "gfx/skybox/skybox_bottom.png",
                "gfx/skybox/skybox_front.png",
                "gfx/skybox/skybox_back.png"
            };
            makeTex3D(faces);
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