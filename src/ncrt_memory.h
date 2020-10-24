extern "C" void* __cdecl memset(void* dest, int c, size_t num);
#pragma intrinsic(memset)

extern "C" void* __cdecl memcpy(void* dest, const void* src, size_t num);
#pragma intrinsic(memcpy)

extern "C" int memcmp(const void* buf1, const void* buf2, size_t count);
#pragma intrinsic(memcmp)

extern "C" void* memmove(void* dest, const void* src, size_t n);
#pragma intrinsic(memmove)
