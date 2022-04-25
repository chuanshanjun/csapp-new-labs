/**
 * @file csim.c
 * @author your name (you@domain.com)
 * @brief 忽视I开头的，其他指令开头的都有空格?
 * 可忽视traces中的size
 * @version 0.1
 * @date 2022-04-23
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "cachelab.h"
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <stdio.h>

const char* tipMsg = "Usage: ./csim-ref [-hv] -s <num> -E <num> -b <num> -t <file>\n\
Options:\n\
  -h         Print this help message.\n\
  -v         Optional verbose flag.\n\
  -s <num>   Number of set index bits.\n\
  -E <num>   Number of lines per set.\n\
  -b <num>   Number of block offset bits.\n\
  -t <file>  Trace file.\n\
\n\
Examples:\n\
  linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n\
  linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n";

struct cache_line {
    int validbit;
    unsigned long tag;
};


int main(int argc, char** argv) {
    int opt,s,E,b;
    int verbose;
    char* fileName;
    while (-1 != (opt = getopt(argc, argv, "vhs:E:b:t:"))) {
        switch (opt) {
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                fileName = optarg;
                break;
            case 'v':
                verbose = 1;
                break;
            case 'h':
                printf("%s", tipMsg);
                break;
            default:
                printf("wrong argument\n");
                break;
        }
    }

    int totalSize = 1 << s;
    printf("totalSize = %d\n", totalSize);

    printf("s= %d, E=%d, b=%d, t=%s\n", s, E, b, fileName);
    
    if (fileName == NULL) {
        printf("option requires an argument -- 't'\n");
        printf("%s",tipMsg);
    }

    FILE* pFile; //pointer to FILE object
    pFile = fopen(fileName, "r"); // open file for reading

    char identifier;
    unsigned address;
    int size;
    
    printf("balabala");

    // Reading lines like " M 20,1" or "L 19,3"
    while (fscanf(pFile, " %c %x,%d", &identifier, &address, &size) > 0) {
        // Do stuff
        if (verbose)
            printf("identifier=%c address=%x,size=%d\n", identifier, address, size);

        // I 代表指令,不是数据,所以不解析
        if (identifier == 'I') {
            printf("identifier == I");
            continue;
        }

        // M操作先读后写，要访问cache两次

    }

    fclose(pFile); // rember to close file when done

    

    printSummary(0, 0, 0);
    return 0;
}
