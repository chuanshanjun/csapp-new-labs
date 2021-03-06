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

### phase_4

打印$0x4025cf,发现值是"%d %d",调用用sscanf后，输入的参数一和参数二分别放到0x8(%rsp)及0xc(%rsp)  
关键在三个参数%edi,%edx,%esi,%edi是我们自己输入的第一个参数，结合下面test %eax,%eax及jne 到bomb  
其实就是拿输入值和%edx,%esi中的一个值做比较，然后将比较结果放到%eax，如果不想等就炸  
最后再比较下第二个值要是2
```
mov    $0xe,%edx // e
mov    $0x0,%esi // 0
mov    0x8(%rsp),%edi // 输入的第一个参数
callq  0x400fce <func4>
test   %eax,%eax // 判断%eax 是否等于0
jne    0x401058 <phase_4+76>
```

```asm
    0x000000000040100c <+0>:     sub    $0x18,%rsp
    0x0000000000401010 <+4>:     lea    0xc(%rsp),%rcx // 将%rsp+0xc的地址给到%rcx 
    0x0000000000401015 <+9>:     lea    0x8(%rsp),%rdx // 将%rsp+0x8的地址给到%rdx 
    0x000000000040101a <+14>:    mov    $0x4025cf,%esi
    0x000000000040101f <+19>:    mov    $0x0,%eax // 0
    0x0000000000401024 <+24>:    callq  0x400bf0 <__isoc99_sscanf@plt>  // 入参一 %rdi 需要两个参数； 入参二 %rsi 格式 "%d %d"
    0x0000000000401029 <+29>:    cmp    $0x2,%eax
    0x000000000040102c <+32>:    jne    0x401035 <phase_4+41>
    0x000000000040102e <+34>:    cmpl   $0xe,0x8(%rsp) // 输入的参数要<=0x8
    0x0000000000401033 <+39>:    jbe    0x40103a <phase_4+46>
    0x0000000000401035 <+41>:    callq  0x40143a <explode_bomb>
    0x000000000040103a <+46>:    mov    $0xe,%edx // e
    0x000000000040103f <+51>:    mov    $0x0,%esi // 0
    0x0000000000401044 <+56>:    mov    0x8(%rsp),%edi // 4
    0x0000000000401048 <+60>:    callq  0x400fce <func4>
    0x000000000040104d <+65>:    test   %eax,%eax // 判断%eax 是否等于0, 如果为0 则设置 ZF标志寄存器
    0x000000000040104f <+67>:    jne    0x401058 <phase_4+76>
    0x0000000000401051 <+69>:    cmpl   $0x0,0xc(%rsp)
    0x0000000000401056 <+74>:    je     0x40105d <phase_4+81>
    0x0000000000401058 <+76>:    callq  0x40143a <explode_bomb>
    0x000000000040105d <+81>:    add    $0x18,%rsp
    0x0000000000401061 <+85>:    retq   
```

# phase_5
输入字符串，长度为6
```asm
0x000000000040107a <+24>:    callq  0x40131b <string_length>
0x000000000040107f <+29>:    cmp    $0x6,%eax
0x0000000000401082 <+32>:    je     0x4010d2 <phase_5+112>
0x0000000000401084 <+34>:    callq  0x40143a <explode_bomb>
```

遍历输入的6个字符，并且取每个字符的后4位，以后四位为偏移值，从以下字符串中取出对应的字符
"maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"
```asm
0x000000000040108b <+41>:    movzbl (%rbx,%rax,1),%ecx
0x000000000040108f <+45>:    mov    %cl,(%rsp)
0x0000000000401092 <+48>:    mov    (%rsp),%rdx
0x0000000000401096 <+52>:    and    $0xf,%edx 
0x0000000000401099 <+55>:    movzbl 0x4024b0(%rdx),%edx
0x00000000004010a0 <+62>:    mov    %dl,0x10(%rsp,%rax,1
0x00000000004010a4 <+66>:    add    $0x1,%rax
0x00000000004010a8 <+70>:    cmp    $0x6,%rax
0x00000000004010ac <+74>:    jne    0x40108b <phase_5+41>
```

拿上面取出的字符和"flyers"对比，相等则成功
```asm
0x00000000004010ae <+76>:    movb   $0x0,0x16(%rsp)
0x00000000004010b3 <+81>:    mov    $0x40245e,%esi
0x00000000004010b8 <+86>:    lea    0x10(%rsp),%rdi
0x00000000004010bd <+91>:    callq  0x401338 <strings_not_equal>
0x00000000004010c2 <+96>:    test   %eax,%eax
0x00000000004010c4 <+98>:    je     0x4010d9 <phase_5+119>
0x00000000004010c6 <+100>:   callq  0x40143a <explode_bomb>
```

# phase_6

```x86asm
0x00000000004010f4 <+0>:     push   %r14
0x00000000004010f6 <+2>:     push   %r13
0x00000000004010f8 <+4>:     push   %r12
0x00000000004010fa <+6>:     push   %rbp
0x00000000004010fb <+7>:     push   %rbx
0x00000000004010fc <+8>:     sub    $0x50,%rsp
0x0000000000401100 <+12>:    mov    %rsp,%r13
0x0000000000401103 <+15>:    mov    %rsp,%rsi
0x0000000000401106 <+18>:    callq  0x40145c <read_six_numbers>
```

读6个数字

```x86asm
0x000000000040110b <+23>:    mov    %rsp,%r14
0x000000000040110e <+26>:    mov    $0x0,%r12d
0x0000000000401114 <+32>:    mov    %r13,%rbp
0x0000000000401117 <+35>:    mov    0x0(%r13),%eax
0x000000000040111b <+39>:    sub    $0x1,%eax
0x000000000040111e <+42>:    cmp    $0x5,%eax
0x0000000000401121 <+45>:    jbe    0x401128 <phase_6+52>
0x0000000000401123 <+47>:    callq  0x40143a <explode_bomb>
```

$rsp=$rbp=$r13存放输入数字的数组首地址。
a[0]-1<=5

```x86asm
0x0000000000401128 <+52>:    add    $0x1,%r12d // %12d=1
0x000000000040112c <+56>:    cmp    $0x6,%r12d // =6
0x0000000000401130 <+60>:    je     0x401153 <phase_6+95>
0x0000000000401132 <+62>:    mov    %r12d,%ebx // 1
0x0000000000401135 <+65>:    movslq %ebx,%rax // 1  2
0x0000000000401138 <+68>:    mov    (%rsp,%rax,4),%eax // 拿到a[1] a[2]
0x000000000040113b <+71>:    cmp    %eax,0x0(%rbp) // a[1] != a[0], a[2] != a[0]
0x000000000040113e <+74>:    jne    0x401145 <phase_6+81>
0x0000000000401140 <+76>:    callq  0x40143a <explode_bomb>
0x0000000000401145 <+81>:    add    $0x1,%ebx // 1+1, 2+1
0x0000000000401148 <+84>:    cmp    $0x5,%ebx // <=5
0x000000000040114b <+87>:    jle    0x401135 <phase_6+65>
0x000000000040114d <+89>:    add    $0x4,%r13
0x0000000000401151 <+93>:    jmp    0x401114 <phase_6+32>
```

```C
for (int i = 0; i < 6; i++ ) {
   if (a[i] - 1 > 5) {
      bomb;
   }
   for (int j = i+1; j <= 5; j++) {
      if (a[i] == a[j]) {
         bomb;
      }
   }
}
```

六个数不相同，且需要<=6 

0x18=24d(输入的6个数，每个数4字节，总计24字节)

```x86asm
0x0000000000401153 <+95>:    lea    0x18(%rsp),%rsi // $rsi=0x7fffffffdf88, $rsp=0x7fffffffdf70
0x0000000000401158 <+100>:   mov    %r14,%rax // $r14=0x7fffffffdf70,$rsp=0x7fffffffdf70 
0x000000000040115b <+103>:   mov    $0x7,%ecx // $rcx=0x7
0x0000000000401160 <+108>:   mov    %ecx,%edx // %edx=0x7
0x0000000000401162 <+110>:   sub    (%rax),%edx // $edx= 7 - a[i]
0x0000000000401164 <+112>:   mov    %edx,(%rax) // a[i] = 7 - a[i]
0x0000000000401166 <+114>:   add    $0x4,%rax // a[i++];
0x000000000040116a <+118>:   cmp    %rsi,%rax //
0x000000000040116d <+121>:   jne    0x401160 <phase_6+108>
```
```C
for (int i = 0; i < 6; i++) {
   a[i] = 7 - a[i];
}
```

$rsp=6,5,4,3,2,1
$rsp=$r14=0x7fffffffdf70
$rsi=$rax=0x7fffffffdf88

```x86asm
0x000000000040116f <+123>:   mov    $0x0,%esi // $rsi=0
0x0000000000401174 <+128>:   jmp    0x401197 <phase_6+163>
0x0000000000401176 <+130>:   mov    0x8(%rdx),%rdx //  ($rdx+0x8=0x6032d0+0x8)=0x6032e0 (0x6032e0)=168

  (gdb) x 0x6032d0+0x8
  0x6032d8 <node1+8>:   0x006032e0
  (gdb) x 0x6032e0+0x8 
  0x6032e8 <node2+8>:   0x006032f0
  (gdb) x 0x6032f0+0x8
  0x6032f8 <node3+8>:   0x00603300
  (gdb) x 0x603300+0x8
  0x603308 <node4+8>:   0x00603310
  (gdb) x 0x603310+0x8
  0x603318 <node5+8>:   0x00603320

0x000000000040117a <+134>:   add    $0x1,%eax // 2
0x000000000040117d <+137>:   cmp    %ecx,%eax // 6
0x000000000040117f <+139>:   jne    0x401176 <phase_6+130>
0x0000000000401181 <+141>:   jmp    0x401188 <phase_6+148>
0x0000000000401183 <+143>:   mov    $0x6032d0,%edx
0x0000000000401188 <+148>:   mov    %rdx,0x20(%rsp,%rsi,2) // $rsp+0x20=0x00603320,$rsp+0x28=0x00603310,
0x000000000040118d <+153>:   add    $0x4,%rsi
0x0000000000401191 <+157>:   cmp    $0x18,%rsi
0x0000000000401195 <+161>:   je     0x4011ab <phase_6+183>
0x0000000000401197 <+163>:   mov    (%rsp,%rsi,1),%ecx // $rcx=6,5
0x000000000040119a <+166>:   cmp    $0x1,%ecx <= 1
0x000000000040119d <+169>:   jle    0x401183 <phase_6+143>
0x000000000040119f <+171>:   mov    $0x1,%eax // $rax=1
0x00000000004011a4 <+176>:   mov    $0x6032d0,%edx // ($rdx=0x6032d0)=332
0x00000000004011a9 <+181>:   jmp    0x401176 <phase_6+130>
```

```x86asm
$rsp+0x20 = 0x603320, (0x603320) = 443 => 7 - a[i] = 3 => a[0] = 4
$rsp+0x28 = 0x603310, (0x603310) = 447 => 7 - a[i] = 4 => a[1] = 3
$rsp+0x30 = 0x603300, (0x603300) = 691 => 7 - a[i] = 5 => a[2] = 2
$rsp+0x38 = 0x6032f0, (0x6032f0) = 924 => 7 - a[i] = 6 => a[3] = 1
$rsp+0x40 = 0x6032e0, (0x6032e0) = 168 => 7 - a[i] = 1 => a[4] = 6
$rsp+0x48 = 0x6032d0, (0x6032d0) = 332 => 7 - a[i] = 2 => a[5] = 5
```

关键是这个值要从小到大排列

```x86asm
0x00000000004011ab <+183>:   mov    0x20(%rsp),%rbx // %rbx = 0x603320
0x00000000004011b0 <+188>:   lea    0x28(%rsp),%rax // $rax = 0x28(%rsp) = 0x7fffffffdf98
0x00000000004011b5 <+193>:   lea    0x50(%rsp),%rsi // %rsi = 0x50(%rsp) = 0x7fffffffdfc0
0x00000000004011ba <+198>:   mov    %rbx,%rcx // $rcx = 0x603320
0x00000000004011bd <+201>:   mov    (%rax),%rdx // $rdx = 0x603310,  $rdx=0x603300
0x00000000004011c0 <+204>:   mov    %rdx,0x8(%rcx) // 
0x00000000004011c4 <+208>:   add    $0x8,%rax // $rax = 0x30(%rsp)
0x00000000004011c8 <+212>:   cmp    %rsi,%rax 
0x00000000004011cb <+215>:   je     0x4011d2 <phase_6+222>
0x00000000004011cd <+217>:   mov    %rdx,%rcx // %rcx=0x603310
0x00000000004011d0 <+220>:   jmp    0x4011bd <phase_6+201>
```

开辟一个新空间首地址0x20(%rsp),截止到0x50(%rsp),总共48个空间

一个链表结构，每个占8个空间
```C
node {
   int val;
   node* next;
}
```

```
0x603328=0x603310, 
0x603318=0x603300,
0x603308=0x6032f0,
0x6032f8=0x6032e0, 
0x6032e8=0x6032d0
```

```x86asm
0x00000000004011d2 <+222>:   movq   $0x0,0x8(%rdx) // 最后一个链表的next置为0
0x00000000004011da <+230>:   mov    $0x5,%ebp // $rbp=5
0x00000000004011df <+235>:   mov    0x8(%rbx),%rax // 0x603310
0x00000000004011e3 <+239>:   mov    (%rax),%eax // 477
0x00000000004011e5 <+241>:   cmp    %eax,(%rbx) // 477, 443  要求后一个值大于前一个值
0x00000000004011e7 <+243>:   jge    0x4011ee <phase_6+250>
0x00000000004011e9 <+245>:   callq  0x40143a <explode_bomb>
0x00000000004011ee <+250>:   mov    0x8(%rbx),%rbx
0x00000000004011f2 <+254>:   sub    $0x1,%ebp
0x00000000004011f5 <+257>:   jne    0x4011df <phase_6+235>
0x00000000004011f7 <+259>:   add    $0x50,%rsp
0x00000000004011fb <+263>:   pop    %rbx
0x00000000004011fc <+264>:   pop    %rbp
0x00000000004011fd <+265>:   pop    %r12
0x00000000004011ff <+267>:   pop    %r13
0x0000000000401201 <+269>:   pop    %r14
0x0000000000401203 <+271>:   retq 
```