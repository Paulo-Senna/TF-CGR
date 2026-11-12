# Makefile simples (alternativa ao CMake).
#   Linux:          make
#   Windows/MinGW:  mingw32-make OS=Windows_NT   (GLFW instalado no MinGW)
CXX      ?= g++
CC       ?= gcc
CXXFLAGS := -std=c++17 -O2 -Wall -Isrc -Iexternal/glad/include
CFLAGS   := -O2 -Iexternal/glad/include

ifeq ($(OS),Windows_NT)
    TARGET := morph3d.exe
    LIBS   := -lglfw3 -lopengl32 -lgdi32
else
    TARGET := morph3d
    LIBS   := -lglfw -lGL -ldl -lpthread
endif

OBJS := src/main.o src/shapes.o src/shader.o external/glad/src/gl.o

$(TARGET): $(OBJS)
	$(CXX) -o $@ $^ $(LIBS)

src/%.o: src/%.cpp src/*.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

external/glad/src/gl.o: external/glad/src/gl.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: clean
