#linux 下 mutex测试
##测试环境及文件说明
**mytest_mutex.c** --- 使用mutex模拟读写者多线程访问临界区数据
编译: gcc -o mytest_mutex -pthread mytest_mutex.c 
用法: /.mytest_mutex 10 2 10 [-v] 参数依次为: 读线程数目 写线程数目 主进程等待时间(duration) -v可选参数显示详细信息

**mytest_urcu.c** --- 使用urcu模拟读写者多线程访问临界区数据(需安装[urcu库](http://lttng.org/download)，安装步骤参见下载文件夹中的README文件)
编译: gcc -o mytest_urcu.c -pthread -lurcu mytest_urcu.c 
用法: /.mytest_urcu 10 2 10 [-v] 参数意义同上


点击查看[测试结果](http://1drv.ms/1gONjCE)  
最右边的折线图，空心圆代表mutex的五次测试数据，不同颜色代表不同的测试，空心三角为urcu测试。左边为每一次的测试说明以及CPU绑定图示  

##发现的问题
1. 暂时只考虑mutex的测试数据，考虑test2 以及 test4，test2是将虚拟机的两个vcpu(虚拟CPU)分别绑定在两个pcpu(物理CPU)上，而test4是将虚拟机的两个vcpu同时绑定在一个pcpu上，但是test4的性能反而要比test2好很多，这是很反常的一个现象
2. 对于test1来说，是虚拟环境下的一个典型表现，但是mutex在此情况下性能特别不好，可能需要优化。
3. 尽管urcu生来是以读为主的，但是它的写性能尤其不肯定，波动幅度特别的大。


