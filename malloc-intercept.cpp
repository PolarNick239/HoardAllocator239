// compile:            g++ --shared -std=c++11 -fPIC -g -o libHoardAllocator239.so malloc-intercept.cpp HoardAllocator.cpp
// run with no trace:  LD_PRELOAD=./libHoardAllocator239.so HOARD_NO_TRACE=1 krusader
// run with trace:     LD_PRELOAD=./libHoardAllocator239.so HOARD_NO_TRACE=0 krusader
// run with trace:     LD_PRELOAD=./libHoardAllocator239.so krusader

#include <cerrno>

#include "HoardAllocator.h"

using namespace hoard;

namespace
{
    __thread bool inside_malloc = false;

    struct recuirsion_guard
    {
        recuirsion_guard()
        {
            if (inside_malloc)
            {
                printMessage("recursive call\n");
                std::abort();
            }
            
            inside_malloc = true;
        }

        ~recuirsion_guard()
        {
            inside_malloc = false;
        }

    private:
        recuirsion_guard(recuirsion_guard const&);
        recuirsion_guard& operator=(recuirsion_guard const&);
    };
}

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;//delete

extern "C"
void* malloc(size_t size)
{
    recuirsion_guard rg;
    
    void *p = hoardMalloc(size);
    
    if (isTraceEnabled()) {
        printMessageln("malloc", size, p);
    }

    return p;
}

extern "C"
void* calloc(size_t n, size_t size)
{
    recuirsion_guard rg;

    void* p = hoardCalloc(n, size);
    
    if (isTraceEnabled()) {
        printMessageln("calloc", n, size, p);
    }

    return p;
}

extern "C"
void free(void *ptr)
{
    recuirsion_guard rg;

    hoardFree(ptr);
    
    if (isTraceEnabled()) {
        printMessageln("free", ptr);
    }
}

extern "C"
void* realloc(void *ptr, size_t size)
{
    recuirsion_guard rg;

    void* p = hoardRealloc(ptr, size);

    if (isTraceEnabled()) {
        printMessageln("realloc", size, ptr, p);
    }
    return p;
}

extern "C"
int posix_memalign(void** memptr, size_t alignment, size_t size)
{
    recuirsion_guard rg;

    int result = hoardPosixMemalign(memptr, alignment, size);
    
    if (isTraceEnabled()) {
        printMessageln("posix_memalign", alignment, size, *memptr);
    }
    return result;
}

extern "C"
void *valloc(size_t size)
{
    recuirsion_guard rg;

    printMessage("deprecated function valloc is not supported\n");
    std::abort();
}

extern "C"
void *memalign(size_t boundary, size_t size)
{
    recuirsion_guard rg;

    printMessage("deprecated function memalign is not supported\n");
    std::abort();
}