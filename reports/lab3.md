Lab 3 Writeup
=============

My name: 李易

My Student number : 191830079

This lab took me about 10 hours to do. I did attend the lab session.

#### 1. Program Structure and Design:

1. Wrap和Unwrap函数实现

   wrap的实现非常简单，绝对序列号加上isn后static-cast转换一下即可。

   Unwrap的实现有点使用到ICS第二章里的数据转换原理。

   讲义里提示的就是我们实现的目标，即找到offset的求法，并使得unwrap前后两个数的offset保持不变。

   先看简单的情况。假如转换后的absolute_seqno只是32位数，那么不存在多个对应情况。只需要算出序列号与isn（WrappingInt32格式）的offset，再用isn（uint_64）加上offset即可。这样就保证了offset在变换前后的一致性。考虑到offset可能为负数，因此要检查结果是否为负，是的话应该加上2^32进行规则化。

   offset的规则是两者之间的距离。如果a-b中b通过减小可以更快与a相等，那么结果是负值。我们发现b要与a相等需要的增量和减量之和为2^32，那么为负值的条件应该是需要的增量大于2^31-1.这与int定义类似。因此我们只要把两个无符号数的差赋给int_32即可。然后因为有多个对应数，我们需要把isn换为checkpoint，保证序列号转换前后与checkpoint的offset不变。

   ![lab3](/home/lyee/Documents/lab3.png)

2. TCP Receiver实现

   这部分实现逻辑其实比较简单，总共只有三个需要考虑的状态。但主要涉及与之前的一些内容的整合，所以几乎一半的时间都用在了debug上面。。。

   主要要确定的参数（ack_no, windows）这些看PPT上的图就好，显示得非常清楚。接收端容量=窗口长度（基于reassembler）+字节流中字节数。

   ![lab3_2](/home/lyee/Documents/lab3_2.png)

#### 2. Implementation Challenges:

第一次跑的时候有很多trivial的细节没有注意，挂了好几个测试用例，不过一个一个调试还是能改好的，基本上一个测试用例对应一个bug。

1.考虑好syn和fin的处理逻辑顺序。因为有syn+data+fin这样特殊的测试用例出现。

2.注意好绝对序列号。接到syn后应该初始化为1，对应stream_index为0.不然的话初始化为0，对应传入的stream_index会变成2^32 - 1。

3.ackno考虑好fin和syn占的字节数。判断fin是否需要算入只需判断其是否已经写入字节流，即字节流是否已经结束输入。

#### 3. Remaining Bugs:

![2021-11-03 18-32-25 的屏幕截图](/home/lyee/Pictures/2021-11-03 18-32-25 的屏幕截图.png)
