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

enqueue(Q: pointer to queue.l, value: data type)

 node= new _node()
 node->value = value
 node->next.ptr =NULL
 loop
	tail = Q->Tail
	next = tail.ptr->next
	if tail= Q->Tail
		if next.ptr ==NULL
			if CAS(&tail.ptr->next, next, <node, next.count+ 1>)
				break
			endif
		else
			CAS(&Q->Tail, tail, <next.ptr, tail.count+l>)
		endif
	end if
 endloop
 CAS(&Q->Tail, tail, <node, tail.count+l>)

WaitFreeQueueFastPath.java: 一个无等待的并发队列；（相对于以上这个经典的无锁MSQueue.）



