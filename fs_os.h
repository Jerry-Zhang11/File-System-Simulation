#ifndef FS_OS_H

#define BLOCK_SIZE 1024     // 1 KB
#define TOTAL_BLOCKS 64
#define MAX_FILES 10

// Simulating a block
typedef struct Block{
    unsigned char data[BLOCK_SIZE];
    int blockNumber;
}Block;

typedef struct FIB{
    int fibId;
    char fileName[100];
    int fileSize;
    int blockCount;
    struct Block* indexBlock; // Points to the index block
}FIB;

typedef struct FreeBlockNode{
    struct Block* block;
    struct FreeBlockNode* next;
}FreeBlockNode;

typedef struct FCBStatus {
    int fcbID;                         
    int isUsed;                        
} FCBStatus;

typedef struct VolumeControlBlock{
    struct FreeBlockNode* freeBlockListHead;
    struct FreeBlockNode* freeBlockListTail;
    struct Block logicalDisk[TOTAL_BLOCKS];
    struct FCBStatus fcbStatus[MAX_FILES];
    struct FIB fileList[MAX_FILES];
    int numFilesCreated;
}VolumeControlBlock;

void initFS();
int createFile(const char* fileName, int size);
int deleteFile(const char* fileName);
void listFiles();
Block* allocateFreeBlock();
void returnFreeBlock(struct Block* block);
void printFreeBlocks();


#endif 