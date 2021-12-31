Lab 0 Writeup
=============

My name: 李易

My Student number : 1918300079

This lab took me about 8 hours to do. I did attend the lab session.

My secret code from section 2.1 was: ALOHA,NJUer!

Welcome to computer networking!

#### 1. Program Structure and Design:

##### (1)环境的搭建

相关虚拟机的安装，编译工具的安装等。

##### (2)获取网页内容

利用telnet程序与其他服务器建立字节流，从而获取网页的内容。

![屏幕截图(300)](/home/lyee/Pictures/屏幕截图(300).png)

##### (3)监听与连接

通过netcat建立一个简单的服务器，并在另一终端中利用telnet与之连接。现在两终端可以以一种“回声”的方式相互传递内容。

![屏幕截图(301)](/home/lyee/Pictures/屏幕截图(301).png)

##### (4)尝试一些简单的命令

Ping指令记录了信息在源和目的地之间的往返时间。而traceroute指令则更为详细一些：它能追踪记录连接过程中的每个跃点，显示出它们的时延。因此，Ping指令常用于测试连通性，而traceroute或tracert指令可用于检测路径中存在的问题，排查故障。（如多次出现同一地址，可能是由于出现了环路）

![屏幕截图(302)](/home/lyee/Pictures/屏幕截图(302).png)

##### （5）给自己发送邮件

利用telnet和SMTP（Simple Mail Transfer Protocol）给自己发送一封邮件。

![屏幕截图(303)](/home/lyee/Pictures/屏幕截图(303).png)

##### (6)完成Webget

利用提供的socket接口完成webget，实现get_URL函数。

```CPP
void get_URL(const string &host, const string &path) {
    TCPSocket soc;
    soc.connect(Address(host, "http"));
    soc.write("GET " + path + " HTTP/1.1\r\n" + "Host: " + host +"\r\n" + "Connection: close\r\n\r\n");
    while(!soc.eof())
    {
        cout << soc.read();
    }
    soc.shutdown(SHUT_WR);
}
```



#### 2. Implementation Challenges:

（1）代码部分实现的难点主要在于需要了解TCPSocket类中为我们提供的相关函数接口，这方面只需要阅读好html文件中对这些函数的介绍即可。我们使用eof函数来控制输出内容，shutdown函数需要我们传入参数，当我们传入SHUT_WR时，仅关闭写入流。比起close函数来说，shutdown函数的调控更细一些。

write函数中，我们需要构造出http的GET请求：

<!--For example, the browser translated the URL `http://www.nowhere123.com/doc/index.html` into the following request message:-->

```html
GET /docs/index.html HTTP/1.1
Host: www.nowhere123.com
Accept: image/gif, image/jpeg, */*
Accept-Language: en-us
Accept-Encoding: gzip, deflate
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.1)
(blank line)
```

另外，需要注意文档中提及的\r\n和Connection： close等细节。

#### 3. Remaining Bugs:

![2021-09-21 16-08-32 的屏幕截图](/home/lyee/Pictures/2021-09-21 16-08-32 的屏幕截图.png)



第一次作业代码量较小，以上为通过测试用例截图。

