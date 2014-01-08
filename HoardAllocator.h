#pragma once

#include <cstdlib>
#include <mutex>

namespace hoard {
    
    bool isTraceEnabled();
    
    void printMessage(size_t size);
    void printMessage(void* pointer);
    void printMessage(const char* message);
    void printMessageln(const char* message);
    void printMessageln(const char* message, size_t size);
    void printMessageln(const char* message, void* pointer);
    void printMessageln(const char* message, size_t size, void* pointer);
    void printMessageln(const char* message, size_t size, size_t size2, void* pointer);
    void printMessageln(const char* message, size_t size, void* pointer, void* pointer2);
    
    struct SuperBlock;
    struct Heap;
    struct SingleBlock;
    
    class Lockable;
    struct SuperBlock;
    struct Heap;

    struct SingleBlock {
        size_t sizeOfData;
        size_t alignedSize;
        void* magicBytes;
        SingleBlock* linkToThis;
        
        SingleBlock(size_t _sizeOfData, size_t _alignedSize);
    };

    struct Block {
        SuperBlock* superBlock;
        
        size_t sizeOfData;
        size_t alignedSize;
        
        void* magicBytes;
        Block* linkToThis;
        
        Block(SuperBlock* _superBlock, size_t _sizeOfData, size_t _alignedSize);
        void freeBlock();
    };

    struct Lockable {
    private:
        pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
        
    public:
        void lock();
        void unlock();
    };

    struct SuperBlock : Lockable { 
    private: 
        const size_t sizeClass;
        const size_t maxSizeOfDataPerBlock;
        const size_t maxCountOfBlocks;

        const size_t offsetOfFirstBlock;
        size_t nextOffsetIndex;
        
        size_t countOfFreeBlocksInStack;

        size_t calcMaxCountOfBlocks();
        
    public:
        Heap* heap;
        
        size_t usedMemory;
        
        SuperBlock* previous;
        SuperBlock* next;
        
        void* magicBytes;
        
        SuperBlock(size_t sizePerBlock, size_t sizeClazz);
        Block* allocateBlock(size_t size, size_t sizeOfData);
        void addFreeBlock(Block* block);
        void deattachFromHeap();
        void attachToHeap(Heap* heap);
    };

    struct Heap : Lockable {
        SuperBlock** firstSBbySizeclass;
        SuperBlock** lastSBbySizeclass;
        
        size_t usedMemory;
        size_t allocatedMemory;
        
        Heap();
        Block* allocateBlock(size_t size, size_t sizeOfData);
    };

    
    void* hoardMalloc(size_t size, size_t alignment = 1);
    void* hoardCalloc(size_t n, size_t size);
    void hoardFree(void* pointer);
    void* hoardRealloc(void* pointer, size_t size);
    int hoardPosixMemalign(void** memoryPointer, size_t alignment, size_t size);
    
}