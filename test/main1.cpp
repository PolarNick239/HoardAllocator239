#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <string.h>
#include "HoardAllocator.h"

using namespace std;

void test1() {
    const int count = 10000;
    const int maxSize = 10;
    
    vector<void*> ptrs;
    vector<int> sizes;
    map<void*, int> idByPtr;
    int nextID = 1;
    bool flag = true;
    for (int i = 0; i < count && flag; i++) {
        for (int j = 0; j < count && flag; j++) {
            int size = rand() % maxSize + 1;
            void* ptr = malloc(size);
            memset(ptr, 239, size);
            if (idByPtr.find(ptr) == idByPtr.end()) {
                idByPtr[ptr] = nextID;
                nextID++;
            } else {
                //cout << "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << endl;
                flag = false;
            }
            //cout << size << " alloc " << ptr << " " << idByPtr[ptr] << endl;
            ptrs.push_back(ptr);
            sizes.push_back(size);
        }
        for (int j = 0; j < count / 2; j++) {
            int randI = rand() % ptrs.size();
            int size = sizes[randI];
            void* ptr = ptrs[randI];
            //cout << size << " deall " << ptr << " " << idByPtr[ptr] << endl;
            sizes.erase(sizes.begin() + randI);
            ptrs.erase(ptrs.begin() + randI);
            hoard::hoardFree(ptr);
        }
    }
    cout.flush();
}

void test2() {
    int count = 493;
    int size = 100;
    vector<void*> ptrs;
    for (int i = 0; i < count; i++) {
        void* ptr = malloc(size);
        memset(ptr, 239, size);
        cout << i << " alloc " << ptr << endl;
        ptrs.push_back(ptr);
    }
    for (int i = 0; i < count; i++) {
        cout << i << " free  " << ptrs[i] << endl;
        hoard::hoardFree(ptrs[i]);
    }
    cout << "alloc " << malloc(size) << endl;
    cout.flush();
}

void test3() {
    int size = 10;
    char* ptr = (char*) malloc(size);
    for (int i = 0; i < size; i++) {
        ptr[i] = 127;
    }
    free(ptr);
}

int main1(int argc, char** argv) {
    test1();
    return 0;
}

