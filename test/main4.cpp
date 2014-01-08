#include <stdlib.h>
#include <iostream>

int main4() {
    void* ptr;
    int testCount = 10;
    size_t test = 100000;
    while(testCount--) {
        for(size_t align = 8; align < test;align*=2) {
            for(size_t size = 1;size< test;size+=10) {
                posix_memalign(&ptr, align, size);
                if ((size_t) ptr % align != 0) {
                    std::cout << align << " FAIL " << size << std::endl;
                    return 0;
                }
                free(ptr);
            }
            std::cout << align << std::endl;
        }
    }
}
