Lab 7 Writeup
=============

My name: 李易

My Student ID:  191830079

This lab took me about 6 hours to do. I did attend the lab session.

### Program Structure and Design of the Router:

这次最后的lab应该是相当友好了...思维量和代码量都不大。主要完成最长前缀匹配和一些简单的逻辑，模拟网络层的路由功能。

在我们的路由表结构中找到最长前缀匹配项，实现复杂度只要求O(N)，所以遍历就好。

找不到匹配项或者 ttl 减一后小于等于零就返回。否则的话，根据_next_hop变量是否为空来判断是直接将数据包发往数据报中的目的IP还是路径上的下一跳路由。

### Implementation Challenges:

唯一比较tricky的地方就只有设计前缀匹配用到的掩码。学过ICS的感觉这个应该还是不难的。

```Cpp
uint32_t mask = _routing_table[i]._prefix_length == 0 ? 0 : 0xffffffff << (32 - _routing_table[i]._prefix_length);
```



### Remaining Bugs:

![2021-12-31 23-39-40 的屏幕截图](/home/lyee/Pictures/2021-12-31 23-39-40 的屏幕截图.png)

![2021-12-31 23-52-01 的屏幕截图](/home/lyee/Pictures/2021-12-31 23-52-01 的屏幕截图.png)

做到这里lab终于完结撒花了......通过整个lab下来除了中间几个比较痛苦，其他的还是设计精良，对学生比较友好的。回过来看，把一个比较大的任务量做了很好的拆分。通过阅读Buffer和BufferList等数据结构的实现，也复习到了CPP一些遗漏的知识。

