#include <cstdlib>
#include <iostream>
#include <thread>

template <typename T>
struct vector {
    T* data;
    size_t capacity;
    size_t size;

    vector() {
        void* ptr;
        posix_memalign(&ptr, 1 << (3 + rand() % 14), sizeof(T) *
(capacity = 64));
        data = (T*) ptr;
        size = 0;
    }

    ~vector() {
        free(data);
    }

    void push_back(T const& value) {
        if (size == capacity) {
            data = (T*) realloc(data, (capacity *= 2) * sizeof(T));
        }
        data[size++] = value;
    }
};

const size_t ITERATIONS = 1000000;

void vector_test() {
    for (int i = 0; i < 100; ++i) {
        vector<size_t> v;
        for (int j = 0; j < ITERATIONS; ++j) {
            v.push_back(j);
        }
        for (int k = 0; k < ITERATIONS; ++k) {
            if (v.data[k] != k) {
                std::cout << "=*=*=FAIL=*=*= i=" << i
                        << " j=" << k << "!=" << v.data[k] << std::endl;
                std::abort();
            }
        }
    }
}

const size_t THREAD_COUNT = 20;

int main0() {
    std::thread* threads[THREAD_COUNT];
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads[i] = new std::thread(vector_test);
    }
    for (int i = 0; i < THREAD_COUNT; ++i) {
        threads[i]->join();
        delete threads[i];
    }

    return 0;
}
