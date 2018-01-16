# PslyAlgorithm
old blog: http://blog.csdn.net/luoyuyou
cur blog: https://segmentfault.com/blog/psly
Here are some of the concurrent code I wrote

    public final int getAndIncrement() {
        for (;;) {
            int current = get();
            int next = current + 1;
            if (compareAndSet(current, next))
                return current;
        }
    }
WaitFreeGetAndIncrement.java: 一个无等待的原子加一的实现（相对于以上这个无锁的实现）；

WaitFreeQueueFastPath.java: 一个无等待的并发队列；（相对于经典的无锁MSQueue.）

LockFreeLinkedListWithRefCount.java: 统计reference个数的在线工具，版本一（java实现），附带main函数用于测试

LockFreeLinkedListWithRefCount2.java: 统计reference个数的在线工具，版本二（java实现），附带main函数用于测试

LinkedTransferQueueFix.java: 这是为JDK中的LinkedTransferQueue增加了补丁之后的文件，修正了因节点的取消而丧失信号的问题。

StampedLock.java：这是为JDK中的StampedLock增加了保存/恢复中断机制之后的文件，修正了原来的CPU占有问题，如果要测试，请注意其中使用的LockSupport.nextSecondarySeed等方法是包控制域。

TestForStampedLock.java：用于测试StampedLock:当第一个读锁进入等待队列并且超时，那么链接它后面的读线程将会重新在队列中拼接，从而取得锁的顺序反转了（将源码中LockSupport.nextSecondarySeed注释掉）。

ConcurrentLinkedQueue.java：对JUC中的这个类在poll和offer上进行了一点修改（注释中保留原代码），采用了和LinkedTransferQueue一样的思想。

UnsafeSupport.java：提供可用的Unsafe。

TestForMultiverse.java：一个运行Multiverse(软件事务内存)的用例。

