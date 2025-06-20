all:
	g++ -O3 -Isrc -Isrc/include -I models/V-nexus -Lsrc/lib -o main src/main.cpp -lmingw32 -lSDL2main -lSDL2 -lSDL2_image -lSDL2_ttf -lopengl32 -lglew32 -Wall
run:
	./main
emcc:
	emcc -O3 src/main.cpp -o output/main.html -I src -I src/include -I models -I models/V-nexus --preload-file shaders --preload-file models -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s USE_SDL_TTF=2 -s MAX_WEBGL_VERSION=2 --use-preload-plugins