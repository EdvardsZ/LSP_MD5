#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include "BestFit/bestfit.h"
#include "FirstFit/firstfit.h"
#include "NextFit/nextfit.h"
#include "WorstFit/worstfit.h"

#define BUFFER_SIZE 1024
#define DEBUG 1

unsigned char buffer[BUFFER_SIZE];

void * lastAdress = buffer;

struct Block {
    struct Block *prev;
    struct Block *next;
    void * address;
    size_t size;
    bool used;
} Block;

struct RequestSizeNode {
    struct RequestSizeNode *next;
    struct RequestSizeNode *prev;
    size_t size;
    bool successfulAllocation; // Kad meģināsim iegrūst atmiņā nevienmēr izdosies to iegrūst...
} requestSizeNode;

struct Block *headBlock = NULL;
struct Block *tailBlock = NULL;

struct RequestSizeNode *headSize = NULL;
struct RequestSizeNode *tailSize = NULL;

struct Block *CreateBlock(const size_t size) {
    struct Block *block = malloc(sizeof(Block));
    block->prev = NULL;
    block->next = NULL;
    block->size = size;
    block->used = false;
    block->address = lastAdress;
    lastAdress+=size;
    return block;
}

struct Block *addBlock(const size_t size) {
    struct Block *temp = tailBlock;
    if(temp == NULL) {
        tailBlock = CreateBlock(size);
        headBlock = tailBlock;
        return tailBlock;
    }
    temp->next = CreateBlock(size);
    temp->next->prev = temp;
    tailBlock = temp->next;
    return tailBlock;
}

void readChunks(const char *chunks) {
    FILE *chunksFile = fopen(chunks, "r");
    size_t  currentSize;
    fscanf(chunksFile, "%zu", &currentSize);

    while (!feof(chunksFile)) {
        addBlock(currentSize);
        fscanf(chunksFile, "%zu", &currentSize);
    }

    fclose(chunksFile);
}

//Sizes saraksta realizācija
struct RequestSizeNode * CreateRequestSizeNode(const size_t size) {
    struct RequestSizeNode * sizeNode = malloc(sizeof(requestSizeNode));
    sizeNode->prev = NULL;
    sizeNode->next = NULL;
    sizeNode->size = size;
    sizeNode->successfulAllocation = false;
    return sizeNode;
}

struct RequestSizeNode * addSizesNode(const size_t size) {
    struct RequestSizeNode * temp = tailSize;
    if(temp == NULL) {
        tailSize = CreateRequestSizeNode(size);
        headSize = tailSize;
        return tailSize;
    }
    temp->next = CreateRequestSizeNode(size);
    temp->next->prev = temp;
    tailSize = temp->next;
    return tailSize;
}

void readRequestAllocationSizes(const char *sizes) {
    FILE *sizesFile = fopen(sizes, "r");
    size_t  requestSize;
    fscanf(sizesFile, "%zu", &requestSize);

    while (!feof(sizesFile)) {
        addSizesNode(requestSize);
        fscanf(sizesFile, "%zu", &requestSize);
    }

    fclose(sizesFile);
}

void printChunksInfo() {
    struct Block * temp = headBlock;
    while(temp != NULL) {
        printf("Block: {size:%zu adress:%zu used:%s}\n",temp->size, (temp->address)-(void *)buffer,  temp->used ? "true":"false");
        temp = temp->next;
    }

}

void printRequestSizesInfo() {
    struct RequestSizeNode * temp = headSize;
    while(temp != NULL){
        printf("RequestSizeNode: {size:%zu successfulAllocation:%s}\n",temp->size, temp->successfulAllocation ? "true":"false");
        temp = temp->next;
    }
}

int main(int argc, char *argv[]) {
    int option;
    char chunksFile[256];
    char sizesFile[256];
    bool chunks = false;
    bool sizes = false;
    while((option = getopt(argc, argv, "c:s:")) != -1){ //get option from the getopt() method
        switch(option){
            case 'c': 
                strcpy(chunksFile,optarg);
                chunks = true;
                break;
            case 's':
                strcpy(sizesFile,optarg);
                sizes= true;
                break;   
        }
    }
    if(!chunks || !sizes){
        printf("You did not specify chunks or sizes file (-c -s)");
        return -1;
    }
    //-----Nolasīti komandrindas argumenti
    readChunks(chunksFile);// Nolasa chunks un izveido sarakstu
    readRequestAllocationSizes(sizesFile);// Nolasa sizes un izveido sarakstu, jo pēc tam būs vajadzīgs saglabāt informāciju par katru pieprasījumu
    #if DEBUG
    printChunksInfo();
    printRequestSizesInfo();
    #endif
}