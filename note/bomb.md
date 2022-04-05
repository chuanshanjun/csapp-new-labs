### phase_1

查看源码进入phase_1发现 **$0x402400,%esi**之后进入一个函数strings_not_equal
猜测应该是将输入值和固定值做比较，使用**x/s 0x402400** 从地址0x402400输出该字符串的内容。
**Border relations with Canada have never been better.**

### phase_2

查看源码发现首先读6个数，读完六个数之后关键是**cmpl   $0x1,(%rsp)**，说明rsp上的第一个数是1，
之后再根据下面的代码(**add    %eax,%eax**)可以推出6个数字是**1,2,4,8,16,32**。
记录下之前自己搞错的地方：
* mov -0x4(%rbx),%eax 是将内存地址(%rbx-0x4)上的值给到%eax
* lea    0x4(%rsp),%rbx 是将地址(%rsp+4)加载到%rbx上

### phase_3

该阶段相当于switch case，输入一个数满足则跳转到相应条件。

查看源码stepi进入到sscanf函数发现，这个函数是将两个字符串格式化为数字，
```asm
0x0000000000400bf0 in __isoc99_sscanf@plt ()
(gdb) 
__GI___isoc99_sscanf (s=0x603820 <input_strings+160> "3 256", 
    format=0x4025cf "%d %d") at isoc99_sscanf.c:24
24      isoc99_sscanf.c: No such file or directory.
(gdb) finish
Run till exit from #0  __GI___isoc99_sscanf (
    s=0x603820 <input_strings+160> "3 256", format=0x4025cf "%d %d")
    at isoc99_sscanf.c:24
0x0000000000400f60 in phase_3 ()
Value returned is $1 = 2
```

并且其返回值为2，正好呼应了下面的代码，如果返回值不大于1的话则会触碰炸弹
```asm
cmp    $0x1,%eax
jg     0x400f6a <phase_3+39>
callq  0x40143a <explode_bomb>
```

继续往下，发现我们第一个输入的参数不能大于7
```asm
cmpl   $0x7,0x8(%rsp)
ja     0x400fad <phase_3+106>
```

将第一个输入的参数，放到%rax，返现下面那个跳转不理解了，但可继续stepi
```asm
mov    0x8(%rsp),%eax
jmpq   *0x402470(,%rax,8) # 跳转至%rax*8+0x402470的内存保存的地址上
```

上面的代码直接跳转到，相应地址，发现将0x100给到%eax
```asm
mov    $0x100,%eax
```

拿0x100和第二个参数做比较，只有相等才不会处罚炸弹
```asm
cmp    0xc(%rsp),%eax
```

