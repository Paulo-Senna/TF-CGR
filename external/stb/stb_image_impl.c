/* stb_image_impl.c - Gera a implementação do stb_image (só PNG e JPEG).
 * Código de terceiros: compilado sem avisos (ver Makefile/CMakeLists.txt). */
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_FAILURE_USERMSG
#include "stb_image.h"
