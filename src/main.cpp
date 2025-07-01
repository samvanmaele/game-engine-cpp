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

#include <platform.hpp>
#ifdef TARGET_PLATFORM_WEB
	#include <emscripten.h>
    #include <emscripten/html5.h>
	#include <GLES3/gl3.h>
#else
	#include <glew/glew.h>
#endif

#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_SIMD
#define GLM_FORCE_ALIGNED
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glcontext.hpp>
#include <shader.hpp>
#include <loadGLTF.hpp>

struct alignas(16) viewmat {
    glm::mat4 view;
};
struct alignas(16) projmat {
    glm::mat4 projection;
};

int WIDTH = 2560;
int HEIGHT = 1400;
SDL_Window* window;

GLint modelpos;

GLuint uboProjection;
GLuint uboView;

class Object {
    public:
        Object() {}

        ~Object() {}

        alignas(16) glm::vec3 position = glm::vec3(0,0,0);
        alignas(16) glm::vec3 eulers = glm::vec3(0,0,0);
        alignas(16) glm::mat4 transmat = glm::mat4(1.0f);

        void makeTransmat() {
            transmat = glm::rotate(glm::mat4(1.0f), eulers.x, glm::vec3(1,0,0));
            transmat = glm::rotate(transmat, eulers.y, glm::vec3(0,1,0));
            transmat = glm::rotate(transmat, eulers.z, glm::vec3(0,0,1));
            transmat[3][0] = position.x;
            transmat[3][1] = position.y;
            transmat[3][2] = position.z;
        }
};
class Camera: public Object {
    public:
        alignas(16) glm::vec3 forward;
        alignas(16) glm::vec3 right;
        alignas(16) glm::vec3 up;
        float zoom;

        Camera(glm::vec3 center): Object() {
            forward = glm::vec3(0,0,1);
            right = glm::vec3(1,0,0);
            up = glm::vec3(0,1,0);
            zoom = 10.0f;
        }
        std::pair<float, float> update() {
            float cosX = std::cos(eulers.x);
            float sinX = std::sin(eulers.x);
            float cosY = std::cos(eulers.y);
            float sinY = std::sin(eulers.y);

            forward = glm::vec3(cosX*cosY, sinY, -sinX*cosY);
            right = glm::vec3(sinX, 0, cosX);
            up = glm::vec3(-cosX*sinY, cosY, sinX*sinY);

            return {cosX, sinX};
        }
        void makeView(glm::vec3 center) {
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
        }
        ~Camera() {}

    private:
        viewmat viewData = {};
};
class Player: public Object {
    public:
        Camera cam;
        alignas(16) glm::vec3 forward;
        alignas(16) glm::vec3 right;

        Player(): Object(), cam(position) {
            position = glm::vec3(0,0,0);
            eulers = glm::vec3(0,0,0);
            forward = glm::vec3(0,0,1);
            right = glm::vec3(1,0,0);
        }
        void update(float moveX, float moveY, bool updateCam) {
            if (updateCam) {
                auto [cosX, sinX] = cam.update();
                forward = glm::vec3(cosX, 0, -sinX);
                right = glm::vec3(-sinX, 0, -cosX);
            }
            position -= right * moveX * 0.05f;
            position += forward * moveY * 0.05f;

            cam.makeView(position);
            makeTransmat();
        }
        ~Player() {}

};

struct RenderableObject {
    Object object;
    std::shared_ptr<Model> model;
};
struct PlayerObject {
    Player object;
    std::shared_ptr<Model> model;
};
struct TextureHandle {
    GLuint handle;
};
void useTex(TextureHandle texture) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture.handle);
}
TextureHandle makeTex(const char* filepath) {
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

std::vector<RenderableObject> objectlist;
PlayerObject playobj;

extern "C" void renderLoopWrapper(void* arg);

class Render {
    public:
        Render() {
            chooseLoop();
        }
        void loopOnce() {
            if (!baseloop()) {
                #ifdef TARGET_PLATFORM_WEB
                    emscripten_cancel_main_loop(); // stop loop if baseloop returned false
                #endif
            }
        }
        inline void chooseLoop() {
            #ifdef TARGET_PLATFORM_WEB
                emscripten_set_main_loop_arg(renderLoopWrapper, this, 0, 1);
            #else
                nativeLoop();
            #endif
        }
        ~Render() {}


    private:
        Uint32 previousFrameTime = SDL_GetTicks();
        Uint32 lastTime = SDL_GetTicks();
        int frameCount = 0;
        float deltaTime = 0.0f;

        void nativeLoop() {
            bool running = true;
            while(running) {
                running = baseloop();
            }
        }
        bool baseloop() {
            bool updateCam = false;

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
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

            if (keystate[SDL_SCANCODE_W] || keystate[SDL_SCANCODE_UP])    move.y += 0.3 * deltaTime;
            if (keystate[SDL_SCANCODE_S] || keystate[SDL_SCANCODE_DOWN])  move.y -= 0.3 * deltaTime;
            if (keystate[SDL_SCANCODE_A] || keystate[SDL_SCANCODE_LEFT])  move.x -= 0.3 * deltaTime;
            if (keystate[SDL_SCANCODE_D] || keystate[SDL_SCANCODE_RIGHT]) move.x += 0.3 * deltaTime;

            if (move.x or move.y or updateCam) {playobj.object.update(move.x, move.y, updateCam);}

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glUniformMatrix4fv(modelpos, 1, GL_FALSE, &playobj.object.transmat[0][0]);
            playobj.model->drawModel();

            //std::for_each(std::execution::par, objectlist.begin(), objectlist.end(), [](RenderableObject &rendObj)
            for (RenderableObject &rendObj : objectlist) {
                glUniformMatrix4fv(modelpos, 1, GL_FALSE, &rendObj.object.transmat[0][0]);
                rendObj.model->drawModel();
            }//);

            getFPS();
            SDL_GL_SwapWindow(window);

            return true;
        }
        void getFPS() {
            frameCount++;

            Uint32 currentTime = SDL_GetTicks();
            deltaTime = currentTime - previousFrameTime;
            previousFrameTime = currentTime;

            if (currentTime - lastTime >= 1000) {
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

extern "C" void renderLoopWrapper(void* arg) {
    static_cast<Render*>(arg)->loopOnce();
}

class Scene {
    public:
        Scene(int sceneNr) {
            if (sceneNr == 0) {
                int shaderProgram = makeShader("shaders/shader3D.vs", "shaders/shader3D.fs");
                glUseProgram(shaderProgram);
                setUniformBuffer(shaderProgram);

                Player obj1;
                Object obj2;
                Object obj3;
                Object obj4;
                Object obj5;
                Object obj6;
                Object obj7;
                Object obj8;
                Object obj9;
                Object obj10;
                Object obj11;
                Object obj12;
                Object obj13;

                obj1.position =  glm::vec3(0,      0,    0     );
                obj2.position =  glm::vec3(-8,     5.55, 40    );
                obj3.position =  glm::vec3(43,     2.35, 7.775 );
                obj4.position =  glm::vec3(0,      2.5,  0     );
                obj5.position =  glm::vec3(46,     6.31, -33.75);
                obj6.position =  glm::vec3(42,     0.1,   46    );
                obj7.position =  glm::vec3(6,      0,     0     );
                obj8.position =  glm::vec3(41.9,   3.2,  -10.05);
                obj9.position =  glm::vec3(7.75,   0.6,   -37   );
                obj10.position = glm::vec3(0,      2.5,  0     );
                obj11.position = glm::vec3(2,      51.1,  4     );
                obj12.position = glm::vec3(-36.15, 5.5,  26    );
                obj1.makeTransmat();
                obj2.makeTransmat();
                obj3.makeTransmat();
                obj4.makeTransmat();
                obj5.makeTransmat();
                obj6.makeTransmat();
                obj7.makeTransmat();
                obj8.makeTransmat();
                obj9.makeTransmat();
                obj10.makeTransmat();
                obj11.makeTransmat();
                obj12.makeTransmat();

                using ModelPtr = std::shared_ptr<Model>;
                ModelPtr mesh1 = std::make_shared<Model>("models/vedal987/vedal987.gltf");
                ModelPtr mesh2 = std::make_shared<Model>("models/V-nexus/Camilla's_tent/Camillas_tent.gltf");
                ModelPtr mesh3 = std::make_shared<Model>("models/V-nexus/drone_factory/drone_factory.gltf");
                ModelPtr mesh4 = std::make_shared<Model>("models/V-nexus/floors/floors.gltf");
                ModelPtr mesh5 = std::make_shared<Model>("models/V-nexus/item_factory/item_factory.gltf");
                ModelPtr mesh6 = std::make_shared<Model>("models/V-nexus/item_shop/item_shop.gltf");
                ModelPtr mesh7 = std::make_shared<Model>("models/V-nexus/street/street.gltf");
                ModelPtr mesh8 = std::make_shared<Model>("models/V-nexus/upgrade_smith/upgrade_smith.gltf");
                ModelPtr mesh9 = std::make_shared<Model>("models/V-nexus/utilities/utilities.gltf");
                ModelPtr mesh10 = std::make_shared<Model>("models/V-nexus/walls/walls.gltf");
                ModelPtr mesh11 = std::make_shared<Model>("models/V-nexus/world_center/world_center.gltf");
                ModelPtr mesh12 = std::make_shared<Model>("models/V-nexus/vedal's_house/vedals_house.gltf");

                playobj = PlayerObject{obj1, mesh1};

                objectlist = {
                    {obj2,  mesh2},
                    {obj3,  mesh3},
                    {obj4,  mesh4},
                    {obj5,  mesh5},
                    {obj6,  mesh6},
                    {obj7,  mesh7},
                    {obj8,  mesh8},
                    {obj9,  mesh9},
                    {obj10, mesh10},
                    {obj11, mesh11},
                    {obj12, mesh12},
                };
            }
        }
        ~Scene() {}


    private:
        void setUniformBuffer(int shaderProgram) {
            projmat dataproj = {};
            dataproj.projection = glm::perspective(glm::radians(45.0f), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
            viewmat data = {};
            data.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

            GLuint blockIndex;

            glGenBuffers(1, &uboProjection);
            glBindBuffer(GL_UNIFORM_BUFFER, uboProjection);
            blockIndex = glGetUniformBlockIndex(shaderProgram, "PROJ");
            glUniformBlockBinding(shaderProgram, blockIndex, 0);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(projmat), &dataproj, GL_STATIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, uboProjection);

            glGenBuffers(1, &uboView);
            glBindBuffer(GL_UNIFORM_BUFFER, uboView);
            blockIndex = glGetUniformBlockIndex(shaderProgram, "VIEW");
            glUniformBlockBinding(shaderProgram, blockIndex, 1);
            glBufferData(GL_UNIFORM_BUFFER, sizeof(viewmat), &data, GL_DYNAMIC_DRAW);
            glBindBufferBase(GL_UNIFORM_BUFFER, blockIndex, uboView);

            modelpos = glGetUniformLocation(shaderProgram, "model");
        }

};
int main(int argc, char* argv[]) {
    window = InitGLContext(WIDTH, HEIGHT, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    Scene scene(0);

    Render renderer;
    return 0;
}