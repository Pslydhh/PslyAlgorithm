# PslyAlgorithm
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

LockFreeLinkedListWithRefCount.java: 统计reference个数的在线工具，版本一（java实现）

LockFreeLinkedListWithRefCount2.java: 统计reference个数的在线工具，版本二（java实现）

