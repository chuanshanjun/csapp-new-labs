```C
/* 
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
unsigned floatScale2(unsigned uf) {
  // 1 get exponent, sign, frac
  int sign = (uf>>31)&(0x1);
  int exponent = (uf>>23)&(0xFF);
  int frac = (uf)&(0x7FFFFF);

  // uf = 0
  if (exponent == 0 & frac == 0) {
    return uf;
  }

  // infinity or not a number
  if (exponent == 0xFF) {
    return uf;
  }

  // denormalize
  if (exponent == 0) {
    frac = frac<<1;
  }

  if (exponent != 0) {
    exponent++;
  }

  return (sign<<31) | (exponent<<23) | (frac);
}
```

观察浮点数的表现形式:  
* 非规格化表示: 小数部分\*2, 即可整个浮点数\*2
* 规格化表示: 指数部分*2即可, 小数部分不动

```C
/* 
 * floatFloat2Int - Return bit-level equivalent of expression (int) f
 *   for floating point argument f.
 *   Argument is passed as unsigned int, but
 *   it is to be interpreted as the bit-level representation of a
 *   single-precision floating point value.
 *   Anything out of range (including NaN and infinity) should return
 *   0x80000000u.
 *   Legal ops: Any integer/unsigned operations incl. ||, &&. also if, while
 *   Max ops: 30
 *   Rating: 4
 */
int floatFloat2Int(unsigned uf) {
  // 1 get sign exponent frac
  int sign = (uf>>31)&(0x1);
  int exponent = (uf>>23)&(0xFF);
  int frac = uf&(0x7FFFFF);
  
  // uf = 0 
  if (exponent == 0 & frac == 0) {
    return 0;
  }

  // infinity NaN
  if (exponent == 0xFF) {
    return 1<<31;
  }

  // denormalize
  if (exponent == 0) {
    // 0.111111(binary) < 1
    return 0;
  }

  // normalize
  int M = frac | (0x1<<23);
  int E = exponent - 127;
  if (E > 31) {
    return 1<<31;
  } else if (E > 23) {
    M = M<<E;
  } else if (E >= 0) {
    M >>= (23 - E);
  } else {
    return 0;
  }

  if (sign) {
    return ~M+1;
  } else {
    return M;
  }
}
```

题目要求传进来的浮点数(用unsigned int)能用int表示  
* 如果为0则返回0
* 无穷大和NaN返回0x1<<31
* 非规格化:因此模式下只存在小数部分,例: 0.111101(二进制表示), 肯定是小于1的, 所以直接返回0;
* 规格化:关键是M*2E
    * 构建E = exponent - 127;
    * 构建M(关键), 因为M有一个隐藏的1, M = frace | 0x1<<23;
    * E < 0,则小数部分*E只会更小,返回0
    * E >= 0 && E <= 23,此时需要将E右移动(23-E),因为整个小数部分为23位,如果指数不够大的话，末尾则需要舍去
    * E > 23 && E <= 31,因为int只有32位,所以E最多只能移动E-23位
    * E > 31 则溢出 