#include "fs_os.h"
#include <stdio.h>

int main(){
    // Initialize the file system
    initFS();

    createFile("alpha.txt", 3072);
    createFile("beta.txt", 5120);

    listFiles();
    printFreeBlocks();

    deleteFile("alpha.txt");

    listFiles();
    printFreeBlocks();

    createFile("gamma.txt", 4096);
    createFile("delta.txt", 8192);

    listFiles();
    printFreeBlocks();

    deleteFile("beta.txt");
    deleteFile("gamma.txt");
    deleteFile("delta.txt");

    listFiles();
    printFreeBlocks();

    return 1;
}