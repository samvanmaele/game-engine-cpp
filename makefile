CXX = g++
CXXFLAGS = -O3 -Wall -Isrc -Isrc/include -Imodels/V-nexus
LDFLAGS = -Lsrc/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lopengl32 -lglew32
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
	--use-preload-plugins