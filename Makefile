src = $(wildcard src/rynx/*.cpp) $(wildcard src/rynx/*/*.cpp) $(wildcard src/rynx/*/*/*.cpp) $(wildcard src/rynx/*/*/*/*.cpp) $(wildcard src/game/*.cpp) $(wildcard src/game/*/*.cpp)
obj = $(src:.c=.o)

myprog: $(obj)
	clang++ -std=c++2a -Wno-assume -march=native -I src/ -I external/catch2/ -I external/stb/ -lGL -lGLEW -lglfw -lpthread -O3 -o ./build/bin/game $^

.PHONY: clean
clean:
	rm -f $(obj) myprog
