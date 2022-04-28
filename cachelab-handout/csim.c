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
#include <limits.h>

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

typedef struct {
    int validbit;
    unsigned long tag;
    int stamp;
} cache_line;

cache_line** cache = NULL;

int hitCounts;
int missCounts;
int evictionCounts;

void updateCache(cache_line** cache, int S, int s, int E, int b, unsigned long address, int size) {
    // 因为stamp默认值为-1, 缓存第一次被设置时为0, 为了便于之后比较所以使用INT_MIN
    int maxStamp = INT_MIN;
    int maxStampIndex = -1;

    // 获取S
    // 1 右移b位，因为是无符号数，所以不用担心符号位
    // identifier >> b;
    // 2 1左移s位
    // 1 << s;
    // 3 1 & 2 得到 组索引
    // printf("address = %lu, 0x%lx\n", address, address);
    // printf("address >> %d = %lu, 0x%lx\n", b, address >> b, address >> b);
    // printf("1 << %d = %d, %x\n", s, 1 << s, 1 << s);

    unsigned long mask = 0xffffffffffffffff;

    // printf("make = %lu, 0x%lx\n", mask, mask);

    unsigned long mask2 = mask >> (64-s);

    // printf("makk2 = %lu, 0x%lx\n", mask2, mask2);

    unsigned long sIndex = ((address >> b) & mask2);
    // printf("sIndex = %lx\n", sIndex);
    // 获取tag
    // 1 继续右移S位
    unsigned long tag = ((address >> b) >> s);
    // printf("tag = %lx\n", eIndex);

    // 先判断是否命中
    // 循环 validbit == 1 & tag == tag
    for (int i = 0; i < E; i++) {
        // //如果tag相同，就hit，重置时间戳
        if (cache[sIndex][i].validbit == 1 && cache[sIndex][i].tag == tag) {
            cache[sIndex][i].stamp = 0;
            hitCounts++;
            printf(" hit");
            // printf(" sIndex = %lx, tag=%lx", sIndex, tag);
            return;
        }
    }

    // 看缓存是否已经满
    int totalE = 0;
    for (int j = 0; j < E; j++) {
        if (cache[sIndex][j].validbit == 1) {
            totalE++;
        }
    }

    // 满 -> miss 并替换]
    if (totalE == E) {
        // 备注：错误思路，只是找到最少被使用的那个值，然后替换
        // 但LRU思想是，替换最久未被使用的那个
        // unsigned long minEIndex = 0;
        // 先找到值最小的索引
        // for (int j = 0; j < (E - 1); j++) {
        //     if (cache[sIndex][j+1].stamp < cache[sIndex][j].stamp) {
        //         minEIndex = j+1;
        //     }
        // }

        // 找到max_stamp那个，因为最大的那个是最久未更新过的
        for (int i = 0; i < E; i++) {
            if (cache[sIndex][i].stamp > maxStamp) {
                maxStamp = cache[sIndex][i].stamp;
                maxStampIndex = i;
            }
        }

        // 替换
        cache[sIndex][maxStampIndex].validbit = 1;
        cache[sIndex][maxStampIndex].tag = tag;
        cache[sIndex][maxStampIndex].stamp = 0;  
        missCounts++;
        evictionCounts++;  
        printf(" miss eviction");    
        printf(" sIndex = %lx, tag=%lx", sIndex, tag);
    } else {
        // 缓存未满，放置
        for (int j = 0; j < E; j++) {
            if (cache[sIndex][j].validbit == 0) {
                cache[sIndex][j].tag = tag;
                cache[sIndex][j].stamp = 0;
                cache[sIndex][j].validbit = 1;
                missCounts++;
                printf(" miss");
                // printf(" sIndex = %lx, tag=%lx", sIndex, tag);
                break;
            }
        }
    }
}

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

    int S = 1 << s;
    // printf("totalSSize = %d\n", S);

    // printf("s= %d, E=%d, b=%d, t=%s\n", s, E, b, fileName);
    
    // 根据S和E构造 cache, 先malloc 一级指针数组
    cache = (cache_line**)malloc(sizeof(cache_line*)*S);
    // 每个一级指针数组，分配二级数组的大小
    for (int i = 0; i < S; i++) {
        cache[i] = (cache_line*)malloc(sizeof(cache_line) * E);
    }

    // 初始化数组内容
    for (int i = 0; i < S; i++) {
        // 初始化每个结构的信息
        for (int j = 0; j < E; j++) {
            cache[i][j].validbit = 0;
            cache[i][j].tag = -1;
            // 因为更新缓存时stamp会设置为0, 所以此处还是设置成-1, 用于区别
            cache[i][j].stamp = -1;
        }
    }

    if (fileName == NULL) {
        printf("option requires an argument -- 't'\n");
        printf("%s",tipMsg);
    }

    FILE* pFile; //pointer to FILE object
    pFile = fopen(fileName, "r"); // open file for reading

    char identifier;
    unsigned long address;
    int size;

    // Reading lines like " M 20,1" or "L 19,3"
    while (fscanf(pFile, " %c %lx,%d", &identifier, &address, &size) > 0) {
        // Do stuff
        if (verbose)
            if (identifier != 'I') {
                printf("%c %lx,%d", identifier, address, size);
            }
            
        // Load操作
        if (identifier == 'L' || identifier == 'S') {
            updateCache(cache, S, s, E, b, address, size);
            printf("\n");
        }

        // M操作先读后写，要访问cache两次
        if (identifier == 'M') {
            updateCache(cache, S, s, E, b, address, size);
            updateCache(cache, S, s, E, b, address, size);
            printf("\n");
        }
        
        // I 代表指令,不是数据,所以不解析
        if (identifier == 'I') {
            // printf("identifier == I\n");
            continue;
        }

        // 关键：每次处理完一条数据后，更新stamp的值，所以stamp最大的就是最久未被使用的缓存
        for (int i = 0; i < S; i++) {
            for (int j = 0;j < E; j++) {
                if (cache[i][j].validbit == 1) {
                    cache[i][j].stamp++;
                }
            }
        }
    }

    fclose(pFile); // rember to close file when done
    
    // free
    // 先释放二级指针
    for (int i = 0; i < S; i++) {
        free(cache[i]);
    }
    // 再释放一级指针
    free(cache);

    printSummary(hitCounts, missCounts, evictionCounts);
    return 0;
}
