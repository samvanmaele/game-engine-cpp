CXX = clang++
CXXFLAGS =                                         \
	-fsanitize=address                             \
	-O3 -Wall                                      \
	-I./src -I./src/include -I./models/V-nexus     \
	-I./vcpkg_installed/x64-windows-static/include \
	-DSDL_MAIN_HANDLED                             \
	-DSDL_STATIC                                   \
	-DGLEW_STATIC                                  \
	-std=c++23

LDFLAGS =                                          \
	-fsanitize=address                             \
	-fuse-ld=lld                                   \
	-L./vcpkg_installed/x64-windows-static/lib     \
	-lSDL2-static                                  \
	-lSDL2_image-static                            \
	-lSDL2_ttf                                     \
	-lfreetype                                     \
	-lopengl32                                     \
	-lglew32                                       \
	-lkernel32 -luser32 -lgdi32 -lwinmm -limm32    \
	-lole32 -loleaut32 -lversion -luuid -ladvapi32 \
	-lsetupapi -lshell32 -ldinput8                 \
	-llibpng16                                     \
	-lbz2                                          \
	-lzlib                                         \
	-lbrotlidec -lbrotlienc -lbrotlicommon


OBJS = $(OUTPUT)/main.o $(OUTPUT)/shader.o $(OUTPUT)/loadGLTF.o $(OUTPUT)/glcontext.o
OUTPUT = output

$(OUTPUT)/a.exe: $(OBJS)
	$(CXX) $(OBJS) -g -o $@ $(LDFLAGS)

$(OUTPUT)/main.o: src/main.cpp src/shader.hpp src/loadGLTF.hpp src/glcontext.hpp
	$(CXX) $(CXXFLAGS) -c $< -g -o $@

$(OUTPUT)/shader.o: src/shader.cpp src/shader.hpp
	$(CXX) $(CXXFLAGS) -c $< -g -o $@

$(OUTPUT)/loadGLTF.o: src/loadGLTF.cpp src/loadGLTF.hpp
	$(CXX) $(CXXFLAGS) -c $< -g -o $@

$(OUTPUT)/glcontext.o: src/glcontext.cpp src/glcontext.hpp
	$(CXX) $(CXXFLAGS) -c $< -g -o $@

run:
	$(OUTPUT)/a.exe
server:
	cd $(OUTPUT)
	npx live-server . -o -p 9999
emcc:
	emcc -O3 \
	src/main.cpp src/shader.cpp src/loadGLTF.cpp src/glcontext.cpp \
	-o $(OUTPUT)/main.html \
	-Isrc -Isrc/include -Imodels -Imodels/V-nexus \
	--preload-file shaders \
	--preload-file models \
	--preload-file gfx \
	-s FORCE_FILESYSTEM=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s USE_SDL=2 \
	-s USE_SDL_IMAGE=2 \
	-s USE_SDL_TTF=2 \
	-s MAX_WEBGL_VERSION=2 \
	-s MIN_WEBGL_VERSION=2 \
	-s ASSERTIONS \
	--use-preload-plugins \
	-std=c++23

clean:
	rm -f $(OUTPUT)/*.o $(OUTPUT)/*.d $(OUTPUT)/a.exe