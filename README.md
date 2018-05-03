Email: pslydhh@gmail.com

# PslyAlgorithm
old blog: http://blog.csdn.net/luoyuyou

cur blog: https://segmentfault.com/blog/psly

latest blog: https://zhuanlan.zhihu.com/c_145672944

Here are some of the concurrent code I wrote

    public final int getAndIncrement(int add) {
        for (;;) {
            int current = get();
            int next = current + add;
            if (compareAndSet(current, next))
                return current;
        }
    }
WaitFreeGetAndIncrement.java： wait-free atomic counter based on cas (Compared to the one above's lock-free Atomic)

WaitFreeQueueFastPath.java： wait-free mpmc Queue；（Based on the classic's MSQueue, and Make a modification on http://www.cs.technion.ac.il/~erez/Papers/wf-methodology-ppopp12.pdf ）

LockFreeLinkedListWithRefCount.java： An tool for statistical reference counters, version 1 (Java implementation)，with main function to test

LockFreeLinkedListWithRefCount2.java： An tool for statistical reference counters, version 2 (Java implementation)，with main function to test

LinkedTransferQueueFix.java： This file has made a modification on LinkedTransferQueue in JDK as my patch, modifying the problem of losing the signal because of the node's cancellation。

StampedLock.java： This file adds the preservation / recovery interrupt mechanism to StampedLock in JDK. It corrects the problem of Excessive occupancy of cpu. If you want to test, please note that the LockSupport.nextSecondarySeed used here is the package control domain.

TestForStampedLock.java： It is used to test StampedLock:. When the first read lock enters the waiting queue and timeout, then the read thread after it links will re join in the queue, so that the order of the lock is reversed. (the LockSupport.nextSecondarySeed in the source code should be annotated).

ConcurrentLinkedQueue.java： This take a little modification on ConcurrentLinkedQueue.java in JUC with poll method and offer method (retains the original code in the annotation) , uses the same idea as LinkedTransferQueue.

UnsafeSupport.java： Provide available Unsafe。

TestForMultiverse.java： A use case that runs Multiverse (software transaction memory)。

LockFreeLinkedList.java： The famous Harris's List, lock-free List. paper: https://www.microsoft.com/en-us/research/wp-content/uploads/2001/10/2001-disc.pdf

