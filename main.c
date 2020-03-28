#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "BestFit/bestfit.h"
#include "FirstFit/firstfit.h"
#include "NextFit/nextfit.h"
#include "WorstFit/worstfit.h"

int main(int argc, char *argv[]) {
    int option;
    char chunksFile[256];
    char sizesFile[256];
    while((option = getopt(argc, argv, "c:s:")) != -1){ //get option from the getopt() method
        switch(option){
            case 'c': 
                strcpy(chunksFile,optarg);
                printf("Given File: %s\n", optarg);
                break;
            case 's':
                strcpy(sizesFile,optarg);
                printf("Given File: %s\n", optarg);
                break;   
        }
    }
    //-----Nolasīti komandrindas argumenti
}