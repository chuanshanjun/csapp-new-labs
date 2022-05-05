### cache 只做了part1,part2只粗略的写了个分块

<https://zhuanlan.zhihu.com/p/79058089>
<https://blog.csdn.net/zjwreal/article/details/80926046>
<https://yangtau.me/computer-system/csapp-cache.html>
<https://zhuanlan.zhihu.com/p/142942823>
<https://zero4drift.github.io/posts/csapp-cachelab-jie-ti-si-lu-ji-lu/>

### part2不理解部分
```C
for (i = 0; i < N; i+=8)
{
    for (j = 0; j < M; j+=8)
    {
        for (k = i; k < i+8; k++) 
        {
            for (l = j; l < j+8; l++)
            {
                B[l][k] = A[k][l];
            }
        }
    }
}
```
* 这种写法存在的问题：A矩阵对角线（从左上到右下）上的元素与B矩阵对角线上的元素映射到cache的同一行，这样在写B时会将cache中的A的一行刷掉，而读A下一个元素时需要再次将这一行读入cache中，这样就造成了conflict miss。



