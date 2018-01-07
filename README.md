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
WaitFreeGetAndIncrement：一个无等待的原子加一的实现（相对于以上这个无锁的实现）



