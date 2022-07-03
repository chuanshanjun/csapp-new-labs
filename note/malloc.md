* 只能修改mm.c

* 使用make编译

* ./mdriver -V 查看帮助信息

* 动态存储分配器由以由以下四个函数构成
```C
int mm_init (void);
void *mm_malloc (size_t size);
void mm_free (void *ptr);
void *mm_realloc(void *ptr, size_t size);
```

* mm_init 测试程序会调用 mm_init 在 其他三个函数之前. mm_init需要做初始化
例如初始话heap area. 正确返回0,错误返回-1

* mm_malloc 返回有效载荷的指针,8字节对齐

* mm_free 释放 mm_malloc和mm_realloc 函数返回的指针

* mm_realloc 
    * ptr is NULL == mm_malloc(size)
    * size is zero == mm_free(ptr)
    * ptr is not NULL, ptr一定是之前的mm_malloc或mm_realloc返回的指针,对它的调用改变了ptr指向的内存块的大小(旧block)来调整字节大小并返回新地址，返回地址可能与旧的相同可能也不相同，这个取决于自己的实现、旧块中的内部碎片量以及重新分配的大小要求。
    如果老的块是8bytes，新的块是12bytes，那么新的块的前8bytes的内容与老的相同，后4位则为未初始化。如果老的是8bytes新的是4bytes，那么新的4bytes的内容与老的相同

* 上述方法的语义与lib.c中的malloc及realloc及free相同

* heap一致性检查器
    * free列表中的block都被标记了free?
    * 是否有任何连续的空闲块以某种方式逃脱了合并？
    * 每个free block 实际都在free list中?
    * free list中的指针是否指向有效的free blocks?
    * 任何分配的块是否重叠？
    * heap块中的指针是否指向有效的heap地址
* 检查器的内容在 int mm_check(void)中,当错误时可将信息打印出来,mm_check用在自己开发时候的调试中，提交代码的时候删除对mm_check的调用，因为会降低吞吐量

* 支持的例程
* 可以调用memlib.c的以下方法
    * void *mem_sbrk(int incr);
    * void *mem_heap_lo(void) 返回堆中第一个byte的指针
    * void *mem_heap_hi(void) 返回堆中最新的buye的指针
    * size_t mem_heapsize(void) 返回堆中的size
    * size_t mem_pagesize(void) 返回系统的页大小 linux系统 4KB

* The Trace-driven Driver Program
* mdriver.c 用来测试mm.c中的正确性，空间利用率，吞吐量
driver 程序被trace files 控制，每个trace file 包含一个迅速的分配，再分配，以及释放 命令 driver 调用你的mm_malloc mm_realloc 以及 free 例程在一些顺序中。
mdriver.c 接受以下参数
    * -t tracedir: Look for the default trace files in directory tracedir instead of the default directory defined in config.h.
    * -f tracefile: Use one particular tracefile for testing instead of the default set of tracefiles
    * -h: Print a summary of the command line arguments.
    * -l: Run and measure libc malloc in addition to the student’s malloc package
    * -v: Verbose output. Print a performance breakdown for each tracefile in a compact table.
    * -V: More verbose output. Prints additional diagnostic information as each trace file is processed.
Useful during debugging for determining which trace file is causing your malloc package to fail.

* Programming Rules
    * 不能改mm.c中的接口
    * 不能调用相关的包或者系统调用如 malloc,calloc,free,realloc,sbrk,brk等
    * 不能定义全局的活静态的数据结构，如arrays,structs,trees,lists，但是可以定义全局的标量如 int floats pinters 在 mm.c中
    * 8位对齐

* 9 评估

* 10 

* 11 提示
    * mdriver -f 初始化开发 使用小的trace files 会简化debuging and testing, short1,2-bal.rep
    * mdriver -v -V 知道些信息
    * 使用 gcc -g 编译 并使用 debugger
    * 熟悉书中的implicit 代码的每一行
    * 指针操作使用宏
    * 前面9个traces包含malloc与free,后面两个包含realloc,malloc及free，先搞好前面9个
    * 性能测试 使用gprof
    