package org.psly.concurrent;

import java.lang.reflect.Field;
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;

import org.psly.concurrent.WaitFreeQueueFastPath.OpDescPoll.PNode;

import sun.misc.Unsafe;

//psly
public class WaitFreeQueueFastPath<E> {
	public WaitFreeQueueFastPath() {
		tail = new Node<E>(null);
		head = new NodeHead<E>(tail, null);

		@SuppressWarnings("unchecked")
		OpDesc<E>[] temState = (OpDesc<E>[]) new OpDesc<?>[Short.MAX_VALUE];
		state = temState;

		@SuppressWarnings("unchecked")
		HelpRecord[] temHelps = new WaitFreeQueueFastPath.HelpRecord[Short.MAX_VALUE];
		helpRecords = temHelps;
	}

	void helpIfNeeded() {
		int tid = tLocal.get();
		HelpRecord rec;
		if ((rec = helpRecords[tid]) == null) {
			helpRecords[tid] = rec = new HelpRecord();
		}
		if (rec.nextCheck-- == 0) {
			OpDesc<E> desc = getArrayAt(rec.curTid);
			if (desc instanceof OpDescAdd) {
				OpDescAdd<E> opDesc = (OpDescAdd<E>) desc;
				if (opDesc.pending == 1 && opDesc.phase == rec.willHelpPhase) {
//					helpEnqTimes.getAndIncrement();
					helpEnq(opDesc);
				}
			} else if (desc instanceof OpDescPoll) {
				OpDescPoll<E> opDesc = (OpDescPoll<E>) desc;
				if (opDesc.pNode.pending == 1 && opDesc.phase == rec.willHelpPhase) {
//					helpDeqTimes.getAndIncrement();
					helpDeq(opDesc);
				}
			}
			rec.reset();
		}
	}

	public boolean add(E item) {
		helpIfNeeded();
		Node<E> node = new Node<E>(item);
		int trials = 0;

		while (((trials++) & 0xF)< MAXFAILURES) {
			Node<E> last = tail;
			Node<E> next = last.next;
			if (last == tail) {
				if (next == null) {
					if (last.casNext(null, node)) {
//						enqValue.getAndIncrement();
//						enqFastValue.getAndIncrement();
						casTail(last, node);
						return true;
					}
				} else {
					fixTail(last, next);
				}
			}
		}
		return wfEnq(node);
	}

	void fixTail(Node<E> last, Node<E> next) {
		if (next.opDesc == null)
			casTail(last, next);
		else
			helpFinishEnq();
	}

	boolean wfEnq(Node<E> node) {
		int tid = tLocal.get();
		OpDesc<E> opDesc = getArrayAt(tid);
		long phase;
		if (opDesc == null)
			phase = -1;
		else
			phase = opDesc.phase;
		phase = ++phase > 0 ? phase : 0;
		setArrayAt(tid, opDesc = new OpDescAdd<E>((short) phase, 1, true, node));
		helpEnq((OpDescAdd<E>)opDesc);

		return true;
	}

	public void helpEnq(OpDescAdd<E> opDescAdd) {
		while (opDescAdd.pending == 1) {
			Node<E> last = tail;
			Node<E> next = last.next;
			if (last == tail) {
				if (next == null) {
					if (opDescAdd.pending == 1) {
						if (last.casNext(null, opDescAdd.node)) {
//							enqValue.getAndIncrement();
//							enqSlowValue.getAndIncrement();
							helpFinishEnq();
							return;
						}
					}
				} else {
					helpFinishEnq();
				}
			}

		}
	}

	private void helpFinishEnq() {
		Node<E> last = tail;
		Node<E> next = last.next;
		if (next != null) {
			if (next.opDesc instanceof OpDescAdd) {
				OpDescAdd<E> curDesc = (OpDescAdd<E>) next.opDesc;
				if (last == tail && curDesc.node == next) {
					curDesc.casPending(1, 0);
					casTail(last, next);
				}
			} else {
				casTail(last, next);
			}
		}
	}

	public E poll() {
		helpIfNeeded();
		int trials = 0;

		while (trials++ < MAXFAILURES) {
			NodeHead<E> localHead = head;
			Node<E> first = localHead.node;
			Node<E> last = tail;
			Node<E> next = first.next;
			if (localHead == head) {
				if (first == last) {
					if (next == null) {
//						deqNull.getAndIncrement();
//						deqFastNull.getAndIncrement();
						return null;
					}
					fixTail(first, next);
				} else if (localHead.opDesc == null) {
					E value = next.value;
					if (casHead(first, next, null, null)) {
//						deqValue.getAndIncrement();
//						deqFastValue.getAndIncrement();
//						headTransfer.getAndIncrement();
//						headTranDirect.getAndIncrement();
						return value;
					}
				} else {
					helpFinishDeq();
				}
			}
		}
		return wfDeq();
	}

	E wfDeq() {
		int tid = tLocal.get();
		OpDesc<E> opDesc = getArrayAt(tid);
		long phase;
		if (opDesc == null)
			phase = -1;
		else
			phase = opDesc.phase;
		phase = ++phase > 0 ? phase : 0;
		setArrayAt(tid, opDesc = new OpDescPoll<E>((short) phase, 1, false, null));
		helpDeq((OpDescPoll<E>)opDesc);

		Node<E> node = ((OpDescPoll<E>)opDesc).pNode.node;
		if (node == null)
			return null;
		return node.next.value;
	}

	public int size() {
		Node<E> head = this.head.node;
		Node<E> next;
		int count = 0;
		for (;;) {
			next = head.next;
			if (next == null)
				break;
			++count;
			head = next;
		}
		return count;
	}

	private void helpDeq(OpDescPoll<E> opDescPoll) {
		PNode<E> pNode;
		while ((pNode = opDescPoll.pNode).pending == 1) {
			NodeHead<E> localHead = head;
			Node<E> first = localHead.node;
			Node<E> last = tail;
			Node<E> next = first.next;
			if (localHead == head) {
				if (first == last) {
					if (next == null) {
						if (last == tail && pNode.pending == 1) {
							if (opDescPoll.casPNode(pNode, new PNode<E>(null, 0))) {
//								 deqNull.getAndIncrement();
//								 deqSlowNull.getAndIncrement();
								 return;
							}
						}
					} else {
						helpFinishEnq();
					}
				} else {
					if (pNode.pending == 0)
						return;
					if (localHead == head) {
						if (localHead.opDesc == null) {
							if (first != pNode.node) {
								opDescPoll.casPNode(pNode, new OpDescPoll.PNode<>(first, 1));
								first = (pNode = opDescPoll.pNode).node;
								if (pNode.pending == 0)
									return;
							}
							if (localHead == head) {
								if (casHead(first, first, null, opDescPoll)) {
//									 deqSlowValue.getAndIncrement();
//									 headTransformOne.getAndIncrement();
//									 headTransfer.getAndIncrement();
								}
							}
						}
						helpFinishDeq();
					}
				}
			}
		}
	}

	private void helpFinishDeq() {
		NodeHead<E> localHead = head;
		Node<E> first = localHead.node;
		Node<E> next = first.next;
		OpDescPoll<E> op = localHead.opDesc;
		if (op != null) {
			PNode<E> pNode = op.pNode;
			if (pNode.pending == 1){
				if (op.casPNode(pNode, new PNode<E>(pNode.node, 0))){
//					 deqValue.getAndIncrement();
				}
			}
			if (localHead == head && next != null) {
				if (casHead(first, next, op, null)) {
//					headTransformTwo.getAndIncrement();
				}
			}
		}
	}

	boolean casTail(Node<E> cmp, Node<E> val) {
		return UNSAFE.compareAndSwapObject(this, tailOffset, cmp, val);
	}

	boolean casHead(Node<E> cmp, Node<E> val, OpDescPoll<E> cmpOp, OpDescPoll<E> valOp) {
		NodeHead<E> localHead = head;
		return cmp == localHead.node && cmpOp == localHead.opDesc
				&& UNSAFE.compareAndSwapObject(this, headOffset, localHead, new NodeHead<E>(val, valOp));
	}

	final void setArrayAt(int i, OpDesc<E> v) {
		UNSAFE.putObjectVolatile(state, ((long) i << ASHIFT) + ABASE, v);
	}

	@SuppressWarnings("unchecked")
	final OpDesc<E> getArrayAt(int i) {
		return (OpDesc<E>) UNSAFE.getObjectVolatile(state, ((long) i << ASHIFT) + ABASE);
	}

	public volatile NodeHead<E> head;
	public volatile Node<E> tail;
	private static final int HELPINGDELAY = 8;
	private static final int MAXFAILURES = 8;

	final ThreadLocal<Integer> tLocal = new ThreadLocal<Integer>() {
		protected Integer initialValue() {
			return tidGenerator.getAndIncrement();
		}
	};
	final ThreadLocal<Integer> threadLocalRandom = new ThreadLocal<Integer>() {
		protected Integer initialValue() {
			return new Random().nextInt();
		}
	};

	private final AtomicInteger tidGenerator = new AtomicInteger();

/*	public AtomicInteger enqValue = new AtomicInteger();
	public AtomicInteger enqFastValue = new AtomicInteger();
	public AtomicInteger enqSlowValue = new AtomicInteger();

	public AtomicInteger deqValue = new AtomicInteger();
	public AtomicInteger deqNull = new AtomicInteger();

	public AtomicInteger deqFastValue = new AtomicInteger();
	public AtomicInteger deqSlowValue = new AtomicInteger();

	public AtomicInteger deqFastNull = new AtomicInteger();
	public AtomicInteger deqSlowNull = new AtomicInteger();

	public AtomicInteger headTransfer = new AtomicInteger();
	public AtomicInteger headTransformOne = new AtomicInteger();
	public AtomicInteger headTransformTwo = new AtomicInteger();
	public AtomicInteger headTranDirect = new AtomicInteger();

	public AtomicInteger helpEnqTimes = new AtomicInteger();
	public AtomicInteger helpDeqTimes = new AtomicInteger();*/

	private static class Node<E> {
		final E value;
		volatile Node<E> next;
		volatile OpDesc<E> opDesc;

		Node(E val) {
			value = val;
			next = null;
			opDesc = null;
		}

		boolean casNext(Node<E> cmp, Node<E> val) {
			return UNSAFE.compareAndSwapObject(this, nextOffset, cmp, val);
		}

		private static final sun.misc.Unsafe UNSAFE;
		private static final long nextOffset;
		static {
			try {
				UNSAFE = UtilUnsafe.getUnsafe();
				nextOffset = UNSAFE.objectFieldOffset(Node.class.getDeclaredField("next"));
			} catch (Exception e) {
				throw new Error(e);
			}
		}
	}

	private static class NodeHead<E> {
		final Node<E> node;
		final OpDescPoll<E> opDesc;

		NodeHead(Node<E> node, OpDescPoll<E> opDesc) {
			this.node = node;
			this.opDesc = opDesc;
		}

	}

	final OpDesc<E>[] state;

	static class OpDesc<E> {
		volatile short phase;
		final boolean enqueue;

		OpDesc(short ph, boolean enq) {
			phase = ph;
			enqueue = enq;
		}
	}

	static final class OpDescAdd<E> extends OpDesc<E> {
		final Node<E> node;
		volatile int pending;

		OpDescAdd(short ph, int pend, boolean enq, Node<E> n) {
			super(ph, enq);
			pending = pend;
			node = n;
			if (n != null)
				n.opDesc = this;
		}

		boolean casPending(int cmp, int val) {
			return UNSAFE.compareAndSwapInt(this, pendingOffsetAdd, cmp, val);
		}

		private static final sun.misc.Unsafe UNSAFE;
		private static final long pendingOffsetAdd;
		static {
			try {
				UNSAFE = UtilUnsafe.getUnsafe();
				pendingOffsetAdd = UNSAFE.objectFieldOffset(OpDescAdd.class.getDeclaredField("pending"));
			} catch (Exception e) {
				throw new Error(e);
			}
		}
	}

	static final class OpDescPoll<E> extends OpDesc<E> {
		volatile PNode<E> pNode;

		OpDescPoll(short ph, int pend, boolean enq, Node<E> n) {
			super(ph, enq);
			pNode = new PNode<E>(n, pend);
		}

		static final class PNode<E> {
			public PNode(Node<E> node, int pending) {
				this.node = node;
				this.pending = pending;
			}

			final Node<E> node;
			final int pending;
		}

		boolean casPNode(PNode<E> cmp, PNode<E> val) {
			return UNSAFE.compareAndSwapObject(this, pNodeOffset, cmp, val);
		}

		private static final sun.misc.Unsafe UNSAFE;
		private static final long pNodeOffset;
		static {
			try {
				UNSAFE = UtilUnsafe.getUnsafe();
				pNodeOffset = UNSAFE.objectFieldOffset(OpDescPoll.class.getDeclaredField("pNode"));
			} catch (Exception e) {
				throw new Error(e);
			}
		}
	}

	final HelpRecord[] helpRecords;

	final class HelpRecord {
		int curTid;
		long willHelpPhase;
		long nextCheck;

		HelpRecord() {
			curTid = -1;
			reset();
		}

		void reset() {
			curTid = (curTid + 1) % tidGenerator.get();
			OpDesc<E> op = WaitFreeQueueFastPath.this.getArrayAt(curTid);
			willHelpPhase = op != null ? op.phase : -1;
			nextCheck = HELPINGDELAY;

		}
	}

	private static class UtilUnsafe {
		private UtilUnsafe() {
		}

		public static Unsafe getUnsafe() {
			if (UtilUnsafe.class.getClassLoader() == null)
				return Unsafe.getUnsafe();
			try {
				final Field fld = Unsafe.class.getDeclaredField("theUnsafe");
				fld.setAccessible(true);
				return (Unsafe) fld.get(UtilUnsafe.class);
			} catch (Exception e) {
				throw new RuntimeException("Could not obtain access to sun.misc.Unsafe", e);
			}
		}
	}

	private static final sun.misc.Unsafe UNSAFE;
	private static final long headOffset;
	private static final long tailOffset;
	private static final int _Obase;
	private static final int _Oscale;

	private static final long ABASE;
	private static final int ASHIFT;

	static {
		try {
			UNSAFE = UtilUnsafe.getUnsafe();
			headOffset = UNSAFE.objectFieldOffset(WaitFreeQueueFastPath.class.getDeclaredField("head"));
			tailOffset = UNSAFE.objectFieldOffset(WaitFreeQueueFastPath.class.getDeclaredField("tail"));
			_Obase = UNSAFE.arrayBaseOffset(OpDesc[].class);
			_Oscale = UNSAFE.arrayIndexScale(OpDesc[].class);

			ABASE = _Obase;
			if ((_Oscale & (_Oscale - 1)) != 0)
				throw new Error("data type scale not a power of two");
			ASHIFT = 31 - Integer.numberOfLeadingZeros(_Oscale);
		} catch (Exception e) {
			throw new Error(e);
		}
	}
}
