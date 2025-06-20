CXX = g++
CXXFLAGS = -O3 -Wall -Isrc -Isrc/include -Imodels/V-nexus
LDFLAGS = -Lsrc/lib -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lopengl32 -lglew32
OBJS = $(OUTPUT)/main.o $(OUTPUT)/shader.o $(OUTPUT)/loadGLTF.o $(OUTPUT)/glcontext.o
OUTPUT = output

$(OUTPUT)/a.out: $(OBJS)
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)

$(OUTPUT)/main.o: src/main.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT)/shader.o: src/shader.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT)/loadGLTF.o: src/loadGLTF.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT)/glcontext.o: src/glcontext.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT):
	mkdir -p $(OUTPUT)

run:
	$(OUTPUT)/a.out
emcc:
	emcc -O3 \
	src/main.cpp src/shader.cpp src/loadGLTF.cpp src/glcontext.cpp \
	-o output/main.html \
	-Isrc -I src/include -Imodels -Imodels/V-nexus \
	--preload-file shaders \
	--preload-file models \
	-s USE_SDL=2 \
	-s USE_SDL_IMAGE=2 \
	-s USE_SDL_TTF=2 \
	-s MAX_WEBGL_VERSION=2 \
	-s MIN_WEBGL_VERSION=2 \
	--use-preload-plugins