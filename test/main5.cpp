
#include <cstdlib>
#include <iostream>

const size_t TESTS = 100000;
const size_t SIZE = 100000;
const long long BASE = 27239LL;

long long hash[SIZE];
int* a[SIZE];
size_t size[SIZE];
size_t alloced = 0;

void check_hash(size_t index) {
    long long h = 0;
    for (int i = 0; i < size[index]; ++i) {
        h *= BASE;
        h += a[index][i];
    }
    if (h != hash[index]) {
        std::cout << "=*=*=FAIL=*=*=" << std::endl;
        std::abort();
    }
}

void upd_hash(size_t index) {
    hash[index] = 0;
    for (size_t j = 0; j < size[index]; ++j) {
        hash[index] *= BASE;
        hash[index] += a[index][j];
    }
}

void fill_array(size_t index) {
    for (size_t i = 0; i < size[index]; ++i) {
        a[index][i] = rand();
    }
}

size_t rand_size() {
    return 100 + rand() % 100000;
}

void case_alloc() {
    void* ptr;
    size[alloced] = rand_size();
    posix_memalign(&ptr, 1 << (3 + rand() % 14), sizeof(int) * size[alloced]);
    a[alloced] = (int*) ptr;
    fill_array(alloced);
    upd_hash(alloced);
    alloced++;
}

void case_realloc() {
    size_t index = rand() % alloced;
    check_hash(index);
    size[index] = rand_size();
    a[index] = (int*) realloc(a[index], sizeof(int) * size[index]);
    fill_array(index);
    upd_hash(index);
}

void case_free() {
    --alloced;
    check_hash(alloced);
    free(a[alloced]);
}

int main2() {
    for (int i = 0; i < TESTS; ++i) {
        int t = rand() % 3;
        if (!alloced || t == 0) {
            case_alloc();
        } else if (t == 1) {
            case_realloc();
        } else { // free
            case_free();
        }
    }

    while (alloced) {
        case_free();
    }

    return 0;
}