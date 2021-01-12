// TODO: Make some of these defines global so we don't
// accidentally call a function that we disabled

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_PNG
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_THREAD_LOCALS
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ASSERT(x)
#define STBI_MALLOC(sz)        mj::malloc(sz)
#define STBI_REALLOC(p, newsz) mj::realloc(p, newsz)
#define STBI_FREE(p)           mj::free(p)

#include "ncrt_memory.h"
#include "stb_image.h"
