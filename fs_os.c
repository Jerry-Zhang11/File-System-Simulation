#include "fs_os.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct VolumeControlBlock fileSystem;

int getFileInformationBlockID();

void initFS(){
    // Initialize logical disk blocks
    for(int i = 0; i < TOTAL_BLOCKS; i++){
        fileSystem.logicalDisk[i].blockNumber = i;
        memset(fileSystem.logicalDisk[i].data, 0, BLOCK_SIZE);
    }

    // Initialize free block list (linked list of all blocks)
    fileSystem.freeBlockListHead = NULL;
    fileSystem.freeBlockListTail = NULL;
    
    for(int i = 0; i < TOTAL_BLOCKS; i++){
        struct FreeBlockNode* newNode = (struct FreeBlockNode*)malloc(sizeof(struct FreeBlockNode));
        if(newNode == NULL){
            printf("Error: Failed to allocate memory for free block list.\n");
            return;
        }
        
        newNode->block = &fileSystem.logicalDisk[i];  // Point to the actual disk block
        newNode->next = NULL;
        
        // Add to tail of list
        if(fileSystem.freeBlockListHead == NULL){
            fileSystem.freeBlockListHead = newNode;
            fileSystem.freeBlockListTail = newNode;
        } else {
            fileSystem.freeBlockListTail->next = newNode;
            fileSystem.freeBlockListTail = newNode;
        }
    }

    // Initialize FCB status array
    for(int i = 0; i < MAX_FILES; i++){
        fileSystem.fcbStatus[i].fcbID = i;
        fileSystem.fcbStatus[i].isUsed = 0;
        
        // Initialize file list entries
        fileSystem.fileList[i].fibId = -1;  // -1 indicates unused
        memset(fileSystem.fileList[i].fileName, 0, 100);
        fileSystem.fileList[i].fileSize = 0;
        fileSystem.fileList[i].blockCount = 0;
        fileSystem.fileList[i].indexBlock = NULL;
    }

    // Initialize file count
    fileSystem.numFilesCreated = 0;

    // Display success message
    printf("Filesystem initialized with %d blocks of %d bytes each.\n", TOTAL_BLOCKS, BLOCK_SIZE);
}

int createFile(const char* fileName, int size){
    // Check that the total number of files does not exceed the maximum allowed
    if(fileSystem.numFilesCreated >= MAX_FILES){
        printf("Error: File system has reached maximum number of files (%d).\n", MAX_FILES);
        return -1;
    }

    // Calculate number of blocks needed (round up)
    int blocksNeeded = (int)ceil((double)size / BLOCK_SIZE);
    
    // Need blocksNeeded + 1 for index block
    int totalBlocksNeeded = blocksNeeded + 1;

    // Count free blocks available
    int numOfFreeBlocks = 0;
    struct FreeBlockNode* current = fileSystem.freeBlockListHead;
    while(current != NULL){
        numOfFreeBlocks++;
        current = current->next;
    }

    if(totalBlocksNeeded > numOfFreeBlocks){
        printf("Error: Not enough free blocks. Need %d blocks, only %d available.\n", 
               totalBlocksNeeded, numOfFreeBlocks);
        return -1;
    }

    // Allocate data blocks and store block pointers in index array
    unsigned char indexArray[blocksNeeded];
    for(int i = 0; i < blocksNeeded; i++){
        struct Block* allocatedBlock = allocateFreeBlock();
        indexArray[i] = allocatedBlock->blockNumber;
    }

    // Get and set up file information block
    int fibID = getFileInformationBlockID();
    if(fibID == -1){
        printf("Error: Could not allocate File Information Block.\n");
        // TODO: Return allocated blocks back to free list
        return -1;
    }

    // Allocate index block
    struct Block* indexBlock = allocateFreeBlock();
    
    // Set up FIB entry
    fileSystem.fileList[fibID].fibId = fibID;
    strncpy(fileSystem.fileList[fibID].fileName, fileName, 
            sizeof(fileSystem.fileList[fibID].fileName) - 1);
    fileSystem.fileList[fibID].fileName[99] = '\0';
    fileSystem.fileList[fibID].fileSize = size;
    fileSystem.fileList[fibID].blockCount = blocksNeeded;
    fileSystem.fileList[fibID].indexBlock = indexBlock;

    // Copy block numbers into index block
    memcpy(indexBlock->data, indexArray, sizeof(unsigned char) * blocksNeeded);

    // Update FCB status
    fileSystem.fcbStatus[fibID].isUsed = 1;

    // Update the number of files in the file system
    fileSystem.numFilesCreated++;

    printf("File '%s' created with %d data blocks + 1 index block.\n", 
           fileName, blocksNeeded);

    return fibID;
}

int deleteFile(const char* fileName){
    int fibID = -1;
    struct Block* indexBlock;

    // Locate the corresponding FIB in file system
    for(int i = 0; i < MAX_FILES; i++){
        if(strcmp(fileSystem.fileList[i].fileName, fileName) == 0){
            fibID = fileSystem.fileList[i].fibId;
            indexBlock = fileSystem.fileList[i].indexBlock;
            break;
        }
    }

    if(fibID == -1){
        printf("Error: File '%s' not found.\n", fileName);
        return -1;
    }


    int length = fileSystem.fileList[fibID].blockCount;
    unsigned char* blockNumbers = (unsigned char*)indexBlock->data;

    for(int i = 0; i < length; i++){
        unsigned char blockNumber = blockNumbers[i];
        struct Block* block = &fileSystem.logicalDisk[blockNumber];
        // Free each data block
        returnFreeBlock(block);
    }

    // Free index block
    returnFreeBlock(indexBlock);


    // Update FIB and FCB status
    fileSystem.fcbStatus[fibID].isUsed = 0;
    fileSystem.fileList[fibID].fibId = -1;
    memset(fileSystem.fileList[fibID].fileName, 0, 100);
    fileSystem.fileList[fibID].fileSize = 0;
    fileSystem.fileList[fibID].blockCount = 0;
    fileSystem.fileList[fibID].indexBlock = NULL;

    // Decrement file count
    fileSystem.numFilesCreated--;

    printf("File %s deleted\n", fileName);
    return fibID;
}


// This function finds the first available FIB ID
int getFileInformationBlockID(){
    for(int i = 0; i < MAX_FILES; i++){
        if(fileSystem.fcbStatus[i].isUsed == 0){
            return i;
        }
    }
    return -1; // No available FIB
}

void listFiles(){
    printf("Root Directory Listing (%d files):\n", fileSystem.numFilesCreated);
    for(int i = 0; i < 10; i++){
        if(fileSystem.fcbStatus[i].isUsed){
            char fileName[100];
            strcpy(fileName, fileSystem.fileList[i].fileName);
            int fileSize = fileSystem.fileList[i].fileSize;
            int blockCount = fileSystem.fileList[i].blockCount;
            printf("%s  |  %d bytes  |  %d data blocks  |  FIBID=%d\n", fileName, fileSize, blockCount, i);
        }
    }
    return;
}

struct Block* allocateFreeBlock(){
    if(fileSystem.freeBlockListHead == NULL){
        printf("Error: No free blocks available.\n");
        return NULL;
    }

    // Get the block
    struct Block* freeBlock = fileSystem.freeBlockListHead->block;

    struct FreeBlockNode* nodeToFree = fileSystem.freeBlockListHead;
   
    // Update head to next node
    fileSystem.freeBlockListHead = fileSystem.freeBlockListHead->next;
    
    // If list is now empty, update tail too
    if(fileSystem.freeBlockListHead == NULL){
        fileSystem.freeBlockListTail = NULL;
    }

    // Free the node memory
    free(nodeToFree);
    
    return freeBlock;
}

void returnFreeBlock(struct Block* block){

    if(fileSystem.freeBlockListTail == NULL){
        fileSystem.freeBlockListTail = (struct FreeBlockNode*)malloc(sizeof(struct FreeBlockNode));

        fileSystem.freeBlockListTail->block = block;
        fileSystem.freeBlockListTail->next = NULL;

        // Set head to tail if tail is NULL
        fileSystem.freeBlockListHead = fileSystem.freeBlockListTail;
    }
    else{
        fileSystem.freeBlockListTail->next = (struct FreeBlockNode*)malloc(sizeof(struct FreeBlockNode));

        fileSystem.freeBlockListTail->next->block = block;
        fileSystem.freeBlockListTail->next->next = NULL;

        // Update free block list tail
        fileSystem.freeBlockListTail = fileSystem.freeBlockListTail->next;
    }
}

void printFreeBlocks(){
    struct FreeBlockNode* head = fileSystem.freeBlockListHead;
    int id;
    int numOfFreeBlocks = TOTAL_BLOCKS;
    
    for(int i = 0; i < 10; i++){
        // Subtract index block as well
        if(fileSystem.fcbStatus[i].isUsed){
            numOfFreeBlocks = numOfFreeBlocks - fileSystem.fileList[i].blockCount - 1;
        }
    }

    printf("Free Blocks (%d): ", numOfFreeBlocks);
    while(head != NULL){
        id = head->block->blockNumber;
        head = head->next;
        printf("[%d] -> ", id);
    }
    printf("NULL\n");
}
