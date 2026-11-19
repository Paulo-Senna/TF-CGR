# Makefile simples (alternativa ao CMake).
#   Linux:          make            (ou: make DEBUG=1 para build com sanitizers)
#   Windows/MinGW:  mingw32-make OS=Windows_NT   (GLFW instalado no MinGW)
# O padrão do make é 'cc', que não existe no MinGW: usa gcc (a menos que CC=... seja passado).
ifeq ($(origin CC),default)
    CC := gcc
endif

# Nosso código: C99, todos os avisos ligados.
# Os headers de external/ entram com -isystem: avisos deles não aparecem.
EXT_INC := -isystem external/glad/include -isystem external/nuklear -isystem external/stb
CFLAGS := -std=c99 -O2 -Wall -Wextra -Isrc $(EXT_INC)
# Bibliotecas de terceiros (external/): compiladas sem avisos.
EXT_CFLAGS := -std=c99 -O2 -w -Isrc $(EXT_INC)

ifeq ($(DEBUG),1)
    CFLAGS     := $(filter-out -O2,$(CFLAGS)) -O0 -g -fsanitize=address,undefined
    EXT_CFLAGS := $(filter-out -O2,$(EXT_CFLAGS)) -O0 -g
    LDFLAGS    += -fsanitize=address,undefined
endif

ifeq ($(OS),Windows_NT)
    TARGET := morph3d.exe
    LIBS   := -lglfw3 -lopengl32 -lgdi32 -lcomdlg32 -lm
else
    TARGET := morph3d
    LIBS   := -lglfw -lGL -ldl -lpthread -lm
endif

# Todo .c dentro de src/ e das subpastas (app, geometry, render, io, ui).
SRCS     := $(wildcard src/*.c src/*/*.c)
OBJS     := $(SRCS:.c=.o)
HEADERS  := $(wildcard src/*/*.h)
EXT_OBJS := external/glad/src/gl.o external/nuklear/nuklear_impl.o external/stb/stb_image_impl.o

$(TARGET): $(OBJS) $(EXT_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

src/%.o: src/%.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

external/%.o: external/%.c src/ui/nuklear_setup.h
	$(CC) $(EXT_CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(EXT_OBJS) $(TARGET)

.PHONY: clean
