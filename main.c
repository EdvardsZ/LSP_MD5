#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <sys/time.h>
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
    size_t sizeUsed;
} Block;
struct RequestSizeNode {
    struct RequestSizeNode *next;
    struct RequestSizeNode *prev;
    size_t size;
    void * address;// adrese uz buferi, kur šis apgabals ticis iedots
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
    block->sizeUsed = 0;
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
    sizeNode->address= NULL;
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
        printf("Block: {size:%zu adress:%zu sizeUsed:%zu}\n",temp->size, (temp->address)-(void *)buffer,temp->sizeUsed);
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


float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

void * firstFit(struct RequestSizeNode ** temp){
    struct Block * current = headBlock;
    while(current!=NULL) {
        if(((current->size)-(current->sizeUsed))>=(*temp)->size){
            current->sizeUsed += (*temp)->size;// pieskaita atmiņu cik izmanto
            (*temp)->address=current->address;// iedod attiecīgajam requestam adresi( lai pēc tam varētu piekļūt)
            current->address += (*temp)->size;// pieskaita blokam adresi (lai tas atkal norādītu uz tukšu vietu)
            (*temp)->successfulAllocation = true;// ieraksta, ka attiecīgais request ir izdevies
            return (*temp)->address;
        }
        current=current->next;
    }
    return NULL;
}

float allocateAndReturnTime() {
    struct timeval t0;
    struct timeval t1;
    float elapsed;

    gettimeofday(&t0, 0);
    //Timer start   /// 
    struct RequestSizeNode * temp= headSize;
    while(temp!=NULL) {
        firstFit(&temp);// here willl go code for each fit
        temp = temp->next;
    }
    //Timer end

    gettimeofday(&t1, 0);

    elapsed = timedifference_msec(t0, t1);
    return elapsed;
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
    //-----Dati ir nolasīti no faila
    float time = allocateAndReturnTime();
    #if DEBUG
    printChunksInfo();
    printRequestSizesInfo();
    #endif
    return 0;
}
