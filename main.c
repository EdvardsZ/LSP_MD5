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
#define DEBUG 0

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
    lastAdress += size;
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


float timedifference_msec(struct timeval t0, struct timeval t1) {
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
}

void * firstFit(struct RequestSizeNode ** temp) {
    struct Block * current = headBlock;
    while(current!=NULL) {
        if(((current->size)-(current->sizeUsed)) >= (*temp)->size) {
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


void * bestFit(struct RequestSizeNode ** temp) {
    struct Block * current = headBlock;
    struct Block * best = CreateBlock(__INT_MAX__);
    while (current != NULL) {
        if (((current->size) - (current->sizeUsed)) == (*temp)->size) { // ja atrodam vienaadu, automatiski pats labaakais variants, uzreiz atgriezam
            current->sizeUsed += (*temp)->size;   // pieskaita atmiņu cik izmanto
            (*temp)->address = current->address;  // iedod attiecīgajam requestam adresi( lai pēc tam varētu piekļūt)
            current->address += (*temp)->size;    // pieskaita blokam adresi (lai tas atkal norādītu uz tukšu vietu)
            (*temp)->successfulAllocation = true; // ieraksta, ka attiecīgais request ir izdevies
            return NULL;
        }
        else if (((current->size) - (current->sizeUsed)) > (*temp)->size) { // atrodam lielaaku ?
            if (best->size > current->size - current->sizeUsed) { // vai tas ir labaaks par sobriid labaako ?
                best = current; // jauns labaakais
            }            
        }        
        current=current->next;
    }
    
    if (best->size != __INT_MAX__) {          // atradam briivo bloku?
        best->sizeUsed += (*temp)->size;      // pieskaita atmiņu cik izmanto
        (*temp)->address = best->address;     // iedod attiecīgajam requestam adresi( lai pēc tam varētu piekļūt)
        best->address += (*temp)->size;       // pieskaita blokam adresi (lai tas atkal norādītu uz tukšu vietu)
        (*temp)->successfulAllocation = true; // ieraksta, ka attiecīgais request ir izdevies                
    }
    return NULL;
}

void * worstFit(struct RequestSizeNode ** temp) {
    struct Block * current = headBlock;
    struct Block * worst = CreateBlock(0);
    while (current != NULL) {        
        if (((current->size) - (current->sizeUsed)) >= (*temp)->size) { // atrodam lielaaku ?
            if (worst->size < current->size - current->sizeUsed) { // vai tas ir labaaks par sobriid labaako ?
                worst = current; // jauns labaakais
            }            
        }        
        current=current->next;
    }
    if (worst->size > 0) {                     // atradam briivo bloku?
        worst->sizeUsed += (*temp)->size;      // pieskaita atmiņu cik izmanto
        (*temp)->address = worst->address;     // iedod attiecīgajam requestam adresi( lai pēc tam varētu piekļūt)
        worst->address += (*temp)->size;       // pieskaita blokam adresi (lai tas atkal norādītu uz tukšu vietu)
        (*temp)->successfulAllocation = true;  // ieraksta, ka attiecīgais request ir izdevies
    }
    return NULL;  
}
//Next fit is a modified version of ‘first fit’. It begins as the first fit to find a free partition but when called next time it starts searching from where it
//left off, not from the beginning. 
// void * nextfit(struct RequestSizeNode ** tempSize){
//     // if(lastAllocBlock != NULL){
//         static struct Block * lastAllocBlock = headBlock;  //initially no block has been the last to have been allocated
//     // }    
//     struct Block * current = lastAllocBlock; //start from the beginning of available chunks
//     bool looped = false; 

//     while(!looped || (current->address != lastAllocBlock->address)) { // while current blocks adress is not the adress
//         printf("%d \n", looped); 
//         if(current == NULL && lastAllocBlock != NULL){ //handles starting from head again
//             current = headBlock;
//             looped = true;
//         }
//         if(((current->size)-(current->sizeUsed))>=(*tempSize)->size){
//             current->sizeUsed += (*tempSize)->size;// pieskaita atmiņu cik izmanto
//             (*tempSize)->address=current->address;//
//             current->address += (*tempSize)->size;//
//             (*tempSize)->successfulAllocation = true;// ieraksta, ka attiecīgais request ir izdevies
            
//             //marks current block as lastAllocatedBlock, moves the list of sizes forward, breaks loop
//             lastAllocBlock = current;
//         break;
//         }
//         current=current->next;
//     }
//     return NULL;
// }

void * nextFit(struct RequestSizeNode ** tempSize){
    static struct Block * current;
    if(current == NULL){
        current = headBlock;
    }
    struct Block * lastAllocBlock = current;
    bool looped = false;
    while(!looped || (current->address != lastAllocBlock->address)) { // while current blocks adress is not the adress 
        if(current == NULL && lastAllocBlock != NULL){ //handles starting from head again
            current = headBlock;
            looped = true;
        }
        if(((current->size)-(current->sizeUsed))>=(*tempSize)->size){
            current->sizeUsed += (*tempSize)->size;// pieskaita atmiņu cik izmanto
            (*tempSize)->address=current->address;//
            current->address += (*tempSize)->size;//
            (*tempSize)->successfulAllocation = true;// ieraksta, ka attiecīgais request ir izdevies
            
            //marks current block as lastAllocatedBlock, moves the list of sizes forward, breaks loop
            lastAllocBlock = current;
            break;

        }
        current=current->next;
    }
    return NULL;
}

unsigned long totalRequestedMemory() {
    struct RequestSizeNode * current = headSize;
    unsigned long total = 0;
    while(current!=NULL){
        total += current->size;
        current = current->next;
    }
    return total;
}

unsigned long totalAllocatedMemory() {
    struct RequestSizeNode * current = headSize;
    unsigned long totalAllocatedMemory = 0;
    while(current!=NULL){
        if(current->successfulAllocation == true) {
            totalAllocatedMemory += current->size;
        }
        current = current->next;
    }
    return totalAllocatedMemory;
}

float allocateAndReturnTime(void * (*f)(struct RequestSizeNode **)) {
    struct timeval t0;
    struct timeval t1;
    float elapsed;
    gettimeofday(&t0, 0);
    //Timer start   /// 
    struct RequestSizeNode * temp= headSize;
    while(temp!=NULL) {
        (*f)(&temp);
        temp = temp->next;
    }

    //Timer end

    gettimeofday(&t1, 0);

    elapsed = timedifference_msec(t0, t1);
    return elapsed;
}

float getFragmentation() {// Vienkāršākais frag. noteikšanas veids - ja pietiks laiks uztaisīšu ko krutāku
    size_t free = 0;
    size_t freeMax = 0;
    struct Block * current = headBlock;

    while(current!=NULL) {
        size_t blockFree = current->size - current->sizeUsed;
        if(blockFree>freeMax){
            freeMax = blockFree;
        }
        free += blockFree;
        current = current -> next;
    }
    size_t freeDif = free - freeMax;
    
    return (float) freeDif / free * 100;
}
void reinitialize(){
    struct Block * current = headBlock;
    lastAdress=buffer;
    while(current != NULL) {
        current->address = lastAdress;
        current->sizeUsed = 0;
        lastAdress += current->size;
        current = current->next;
    }
    struct RequestSizeNode * temp = headSize;
    while(temp != NULL) {
        temp->address = NULL;
        temp->successfulAllocation = false;
        temp = temp->next;
    }
}
void * (*func_ptr[4])(struct RequestSizeNode **) = {bestFit, worstFit, firstFit, nextFit};
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
    readRequestAllocationSizes(sizesFile);// Nolasa sizes un izveido sarakstu
    for(int i=0; i<4; i++) {
        float time = allocateAndReturnTime(func_ptr[i]);// 0 - bestfit 1- worstfit, 2 firstfit, 3 nextfit
            if(func_ptr[i]==bestFit)
            printf("\n-------BestFit----------\n");
            if(func_ptr[i]==worstFit)
            printf("-------WorstFit----------\n");
            if(func_ptr[i]==firstFit)
            printf("-------FirstFit----------\n");
            if(func_ptr[i]==nextFit)
            printf("-------NextFit----------\n");
            printf("Time: %f\n",time);
            printf("Total requested memory:%lu\n",totalRequestedMemory());
            printf("Total aquired memory %lu\n", totalAllocatedMemory());
            printf("Total fragmentation:%f %%\n\n", getFragmentation());
        reinitialize();
    }
    //-----Dati ir nolasīti no faila
    #if DEBUG
    printChunksInfo();
    printRequestSizesInfo();
    #endif
    return 0;
}
