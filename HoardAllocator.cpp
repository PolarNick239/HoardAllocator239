#include "HoardAllocator.h"
#include <sys/mman.h>
#include <stack>
#include <pthread.h>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <algorithm>
#include <iostream>

namespace hoard {
    
    bool isTraceEnabled() {
        char* envValue = getenv("HOARD_NO_TRACE");
        return envValue == NULL || envValue[0] == '0';
    }
    
    void printMessage(size_t size) {
        if (!isTraceEnabled()) {
            return;
        }
        int pow = 1;
        size_t pow10 = 10;
        while(pow10 <= size) {
            pow++;
            pow10 *= 10;
        }
        char* message = (char*) mmap(NULL, pow, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (size == 0) {
            *message = '0';
        } else {
            int i = pow - 1;
            while (size > 0) {
                message[i] = '0' + size % 10;
                i--;
                size /= 10;
            }
        }
        write(2, message, pow);
    }
    
    void printMessage(void* pointer) {
        printMessage((size_t) pointer);
    }
    
    void printMessage(const char* message) {
        if (!isTraceEnabled()) {
            return;
        }
        write(2, message, strlen(message));
    }
    
    void printMessageln(const char* message) {
        printMessage(message);
        printMessage("\n");
    }
    
    void printMessageln(const char* message, size_t size) {
        printMessage(message);
        printMessage(" ");
        printMessage(size);
        printMessage("\n");
    }
    
    void printMessageln(const char* message, void* pointer) {
        printMessage(message);
        printMessage(" ");
        printMessage(pointer);
        printMessage("\n");
    }
    
    void printMessageln(const char* message, size_t size, void* pointer) {
        printMessage(message);
        printMessage(" ");
        printMessage(size);
        printMessage(" ");
        printMessage(pointer);
        printMessage("\n");
    }
    
    void printMessageln(const char* message, size_t size, size_t size2, void* pointer) {
        printMessage(message);
        printMessage(" ");
        printMessage(size);
        printMessage(" ");
        printMessage(size2);
        printMessage(" ");
        printMessage(pointer);
        printMessage("\n");
    }
    
    void printMessageln(const char* message, size_t size, void* pointer, void* pointer2) {
        printMessage(message);
        printMessage(" ");
        printMessage(size);
        printMessage(" ");
        printMessage(pointer);
        printMessage(" ");
        printMessage(pointer2);
        printMessage("\n");
    }
    
    bool abortIfFalse(bool condition, const char* message) {
        if (!condition) {
            printMessageln(message);
            std::abort();
        }
        return condition;
    }
    
    const int MAX_THREAD_COUNT = 239;
    const int K = 1.2;
    const double BASE = 4;
    const double FRACTION = 1/4;

    void* const MAGIC_BYTES_FOR_SINGLEBLOCK = (void*) 0xBBC;
    void* const MAGIC_BYTES_FOR_BLOCK = (void*) 0x239C0DE;
    void* const MAGIC_BYTES_FOR_FREE_BLOCK = (void*) 0x2391C0DE;
    void* const MAGIC_BYTES_FOR_SUPERBLOCK = (void*) 0xC0DE239;

    size_t getThreadIdHash() {
        size_t tidHash =  std::hash<std::thread::id>()(std::this_thread::get_id());
        return tidHash;
    }
    
    bool isValidAlignment(size_t alignment) {
        size_t tmp = alignment;
        while (tmp > sizeof(void*)) {
            if (tmp % 2 != 0) {
                return false;
            }
            tmp /= 2;
        }
        return tmp % sizeof(void*) == 0;
    }
    
    void* roundUpToAlignment(void* pointer, size_t alignment) {
        size_t mod = (size_t) pointer % alignment;
        if (mod == 0) {
            return pointer;
        } else {
            return (void*) ((char*) pointer + alignment - mod);
        }
    }
    
    void setLinkTo(void* from, void* to) {
        //In case of aligned data we want to be able to jump to Block info
        //so we will write pointer to Block info to void* before "from" pointer
        //(i.e. before data). It is like writing to fantom
        //SingleBlock(Block)->linkToThis variable (this is its purpose).
        //IMPORTANT!!!
        //This method belive, that linkToThis in SingleBlock and Block is a last
        //variable of void* type!
        *(void**)((char*) from - sizeof(void*)) = to;
    }
    
    size_t roundUpSize(size_t size) {
        double curSize = BASE;
        size_t classSize = 0;
        while (curSize < size) {
            curSize *= BASE;
            classSize++;
        }
        return (size_t) curSize;
    }
    
    size_t getClassSize(size_t size) {
        double curSize = BASE;
        size_t classSize = 0;
        while (curSize < size) {
            curSize *= BASE;
            classSize++;
        }
        return classSize;
    }
    
    size_t SIZE_OF_SUPERBLOCK;
    int COUNT_OF_CLASSSIZE;
    size_t HEAPS_COUNT;
    
    Heap* commonHeap;
    Heap* heaps;
    
    bool initialized;
    Lockable initializationLock;
    
    void init() {
        if (!initialized) {
            initializationLock.lock();
            if (!initialized) {
                SIZE_OF_SUPERBLOCK = 4 * sysconf(_SC_PAGE_SIZE);
                COUNT_OF_CLASSSIZE = getClassSize(SIZE_OF_SUPERBLOCK) + 1;
                HEAPS_COUNT = 2 * MAX_THREAD_COUNT;
                
                heaps = (Heap*) mmap(NULL, sizeof(Heap) * (HEAPS_COUNT + 1), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                commonHeap = heaps + HEAPS_COUNT;
                for (Heap* heap = heaps; heap < heaps + HEAPS_COUNT + 1; heap++) {
                    new (heap) Heap();
                }
                
                initialized = true;
            }
            initializationLock.unlock();
        }
    }
    
    SingleBlock::SingleBlock(size_t _sizeOfData, size_t _alignedSize)
            : sizeOfData(_sizeOfData),
            alignedSize(_alignedSize),
            magicBytes(MAGIC_BYTES_FOR_SINGLEBLOCK),
            linkToThis(this) {
    }
    
    Block::Block(SuperBlock* _superBlock, size_t _sizeOfData, size_t _alignedSize)
            : superBlock(_superBlock),
            sizeOfData(_sizeOfData),
            alignedSize(_alignedSize),
            magicBytes(MAGIC_BYTES_FOR_BLOCK),
            linkToThis(this) {
    }
    
    void Block::freeBlock() {
        abortIfFalse(magicBytes == MAGIC_BYTES_FOR_BLOCK,
                "Given pointer does not follow after a correct Block structure!(or block is free)");
        abortIfFalse(superBlock->magicBytes == MAGIC_BYTES_FOR_SUPERBLOCK,
                "Given pointer does not follow correct Superblock structure!");
        magicBytes = MAGIC_BYTES_FOR_FREE_BLOCK;
        superBlock->addFreeBlock(this);
    }
    
    void Lockable::lock() {
        pthread_mutex_lock(&mutex);
    }

    void Lockable::unlock() {
        pthread_mutex_unlock(&mutex);
    }
        
    size_t SuperBlock::calcMaxCountOfBlocks() {
        return (SIZE_OF_SUPERBLOCK - sizeof(SuperBlock)) /
                (sizeof(void*) + sizeof(Block) + this->maxSizeOfDataPerBlock);
    }
    
    SuperBlock::SuperBlock(size_t sizePerBlock, size_t sizeClazz) 
            : heap(NULL),
            usedMemory(0),
            previous(NULL),
            next(NULL),
            sizeClass(sizeClazz),
            maxSizeOfDataPerBlock(sizePerBlock),
            maxCountOfBlocks(calcMaxCountOfBlocks()),
            offsetOfFirstBlock((size_t) (void*) ((char*) this + sizeof(SuperBlock) + sizeof(Block*) * maxCountOfBlocks)),
            nextOffsetIndex(0),
            countOfFreeBlocksInStack(0),
            magicBytes(MAGIC_BYTES_FOR_SUPERBLOCK) {
    }
    
    Block* SuperBlock::allocateBlock(size_t size, size_t sizeOfData) {
        abortIfFalse(size <= maxSizeOfDataPerBlock,
                "Size is to big for allocation in this SuperBlock!");
        
        Block* result = NULL;
        if (nextOffsetIndex != maxCountOfBlocks) {
            result = (Block*) ((char*) offsetOfFirstBlock + nextOffsetIndex * (sizeof(Block) + maxSizeOfDataPerBlock));
            nextOffsetIndex++;
        } else if (countOfFreeBlocksInStack != 0) {
            countOfFreeBlocksInStack--;
            result = *((Block**) ((char*) this + sizeof(SuperBlock) + countOfFreeBlocksInStack * sizeof(Block*)));
        }
        
        if (result != NULL) {
            new (result) Block(this, sizeOfData, size);
            usedMemory += sizeOfData;
            heap->usedMemory += sizeOfData;
        }
        return result;
    }
    
    void SuperBlock::deattachFromHeap() {
        heap->usedMemory -= usedMemory;
        heap->allocatedMemory -= SIZE_OF_SUPERBLOCK;

        if (previous == NULL) {
            heap->firstSBbySizeclass[sizeClass] = next;
        } else {
            previous->next = next;
        }
        if (next == NULL) {
            heap->lastSBbySizeclass[sizeClass] = previous;
        } else {
            next->previous = previous;
        }
        previous = NULL;
        next = NULL;
        heap = NULL;
    }
    
    void SuperBlock::attachToHeap(Heap* heap) {
        this->heap = heap;
        heap->usedMemory += usedMemory;
        heap->allocatedMemory += SIZE_OF_SUPERBLOCK;
        SuperBlock* insertAfter = heap->lastSBbySizeclass[sizeClass];
        while (insertAfter != NULL && insertAfter->usedMemory < usedMemory) {
            insertAfter = insertAfter->previous;
        }
        if (insertAfter == NULL) {
            previous = NULL;
            next = heap->firstSBbySizeclass[sizeClass];
            if (heap->firstSBbySizeclass[sizeClass] != NULL) {
                heap->firstSBbySizeclass[sizeClass]->previous = this;
            }
            heap->firstSBbySizeclass[sizeClass] = this;
            if (heap->lastSBbySizeclass[sizeClass] == NULL) {
                heap->lastSBbySizeclass[sizeClass] = this;
            }
        } else {
            next = insertAfter->next;
            previous = insertAfter;
            insertAfter->next = this;
            if (next == NULL) {
                heap->lastSBbySizeclass[sizeClass] = this;
            } else {
                next->previous = this;
            }
        }
    }
    
    void SuperBlock::addFreeBlock(Block* block) {
        Block** offsetForNewFreeBlock = (Block**) ((char*) this + sizeof(SuperBlock) + countOfFreeBlocksInStack * sizeof(Block*));
        *offsetForNewFreeBlock = block;
        countOfFreeBlocksInStack++;
        usedMemory -= block->sizeOfData;
        heap->usedMemory -= block->sizeOfData;
    }
    
    Heap::Heap() {
        firstSBbySizeclass = (SuperBlock**) mmap(NULL, sizeof(SuperBlock*) * COUNT_OF_CLASSSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        lastSBbySizeclass = (SuperBlock**) mmap(NULL, sizeof(SuperBlock*) * COUNT_OF_CLASSSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        usedMemory = 0;
        allocatedMemory = sizeof(Heap) + 2 * sizeof(SuperBlock*) * COUNT_OF_CLASSSIZE;
        for (int i = 0; i < COUNT_OF_CLASSSIZE; i++) {
            firstSBbySizeclass[i] = NULL;
            lastSBbySizeclass[i] = NULL;
        }
    }
    
    Block* Heap::allocateBlock(size_t size, size_t sizeOfData) {
        size_t classSize = getClassSize(size);
        SuperBlock* cur = firstSBbySizeclass[classSize];
        while (cur != NULL) {
            Block* result = cur->allocateBlock(size, sizeOfData);
            if (result != NULL) {
                return result;
            }
            cur = cur->next;
        }
        return NULL;
    }
    
    void* hoardMalloc(size_t size, size_t alignment) {
        init();
        void* result;
        if (size == 0) {
            result = NULL;
        } else {
            size_t alignedSize = roundUpSize(size + alignment - 1);
            if (alignedSize > SIZE_OF_SUPERBLOCK / 2) {
                SingleBlock* singleBlock = (SingleBlock*) mmap(NULL, sizeof(SingleBlock) + alignedSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                if (singleBlock != NULL && singleBlock != MAP_FAILED) {
                    new (singleBlock) SingleBlock(size, alignedSize);
                    result = roundUpToAlignment((char*) singleBlock + sizeof(SingleBlock), alignment);
                    setLinkTo(result, singleBlock);
                }
            } else {
                size_t heapId = getThreadIdHash() % HEAPS_COUNT;
                Heap* heap = &heaps[heapId];
                heap->lock();
                Block* allocatedBlock = heap->allocateBlock(alignedSize, size);
                heap->unlock();
                if (allocatedBlock == NULL) {
                    commonHeap->lock();
                    allocatedBlock = commonHeap->allocateBlock(alignedSize, size);
                    commonHeap->unlock();
                    if (allocatedBlock == NULL) {
                        SuperBlock* superBlock = (SuperBlock*) mmap(NULL, SIZE_OF_SUPERBLOCK, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
                        if (superBlock != NULL && superBlock != MAP_FAILED) {
                            new (superBlock) SuperBlock(roundUpSize(alignedSize), getClassSize(alignedSize));
                            heap->lock();
                            superBlock->attachToHeap(heap);
                            allocatedBlock = superBlock->allocateBlock(alignedSize, size);
                            heap->unlock();
                        }
                    } else {
                        SuperBlock* superBlock = allocatedBlock->superBlock;
                        superBlock->lock();
                        Heap* oldHeap = superBlock->heap;
                        if (oldHeap == commonHeap) {
                            heap->lock();
                            commonHeap->lock();
                            superBlock->deattachFromHeap();
                            superBlock->attachToHeap(heap);
                            commonHeap->unlock();
                            heap->unlock();
                        }
                        superBlock->unlock();
                    }
                }
                if (allocatedBlock != NULL) {
                    result = roundUpToAlignment((char*) allocatedBlock + sizeof(Block), alignment);
                    setLinkTo(result, allocatedBlock);
                }
            }
        }
        return result;
    }
    
    void* hoardCalloc(size_t n, size_t size) {
        init();
        void* result;
        if (size == 0) {
            result = NULL;
        } else {
            result = hoardMalloc(n * size);
            if (result != NULL) {
                memset(result, 0, n * size);
            }
        }
        return result;
    }
    
    void hoardFree(void* pointer) {
        init();
        if (pointer == NULL) {
            return;
        }
        Block* block = (Block*) ((char*) pointer - sizeof(Block));
        block = block->linkToThis;
        if (block->magicBytes == MAGIC_BYTES_FOR_BLOCK) {
            SuperBlock* superBlock = block->superBlock;
            superBlock->lock();
            Heap* heap = block->superBlock->heap;
            heap->lock();
            block->freeBlock();
            if (heap != commonHeap) {
                if (heap->usedMemory + K * SIZE_OF_SUPERBLOCK < heap->allocatedMemory
                        && heap->usedMemory < (1- FRACTION) * heap->allocatedMemory) {
                    commonHeap->lock();
                    SuperBlock* minUsedBlock = NULL;
                    for(int i = 0; i < COUNT_OF_CLASSSIZE; i++) {
                        if (heap->lastSBbySizeclass[i] != NULL) {
                            if (minUsedBlock == NULL
                                    || minUsedBlock->usedMemory > heap->lastSBbySizeclass[i]->usedMemory) {
                                minUsedBlock = heap->lastSBbySizeclass[i];
                            }
                        }
                    }
                    minUsedBlock->deattachFromHeap();
                    minUsedBlock->attachToHeap(commonHeap);
                    commonHeap->unlock();
                }
            }
            heap->unlock();
            superBlock->unlock();
        } else {
            SingleBlock* singleBlock = (SingleBlock*) ((char*) pointer - sizeof(SingleBlock));
            singleBlock = singleBlock->linkToThis;
            abortIfFalse(singleBlock->magicBytes == MAGIC_BYTES_FOR_SINGLEBLOCK
                    && singleBlock->alignedSize > SIZE_OF_SUPERBLOCK / 2,
                    "Given pointer does not point either not to Block or SingleBlock!");
            singleBlock->magicBytes = MAGIC_BYTES_FOR_FREE_BLOCK;
            munmap(singleBlock, sizeof(SingleBlock) + singleBlock->alignedSize);
        }
    }
    
    void* hoardRealloc(void* pointer, size_t size) {
        init();
        void* result = hoardMalloc(size);
        if (pointer != NULL && result != NULL) {
            Block* block = (Block*) ((char*) pointer - sizeof(Block));
            block = block->linkToThis;
            if (block->magicBytes == MAGIC_BYTES_FOR_BLOCK) {
                memcpy(result, pointer, std::min(size, block->sizeOfData));
            } else {
                SingleBlock* singleBlock = (SingleBlock*) ((char*) pointer - sizeof(SingleBlock));
                singleBlock = singleBlock->linkToThis;
                abortIfFalse(singleBlock->magicBytes == MAGIC_BYTES_FOR_SINGLEBLOCK
                        && singleBlock->alignedSize > SIZE_OF_SUPERBLOCK / 2,
                        "Given pointer does not point either not to Block or SingleBlock!");
                memcpy(result, pointer, std::min(size, singleBlock->sizeOfData));
            }
            hoardFree(pointer);
        }
        return result;
    }
    
    int hoardPosixMemalign(void** memoryPointer, size_t alignment, size_t size) {
        init();
        void* result = NULL;
        *memoryPointer = NULL;
        if (!isValidAlignment(alignment)) {
            return EINVAL;
        }
        result = hoardMalloc(size, alignment);
        if (result == NULL) {
            return ENOMEM;
        }
        *memoryPointer = result;
        return 0;
    }
    
}
