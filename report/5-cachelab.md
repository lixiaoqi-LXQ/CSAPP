# cachelab报告

> 巨大的失误：将10、20等内存地址看作十进制。
> 
> ```shell
> $ ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace -v
> L 10,1 miss
> M 20,1 miss hit
> L 22,1 hit
> S 18,1 hit
> L 110,1 miss eviction
> L 210,1 miss eviction
> M 12,1 miss eviction hit
> hits:4 misses:5 evictions:3
> ```
> 
> 写之前总要搞清楚要求的缓存参数吧，然后我就去看示例输出，错把内存地址当作十进制(COD课上有过用十进制表示地址的作业题)，然后怎么看怎么不对，呆了半天……


