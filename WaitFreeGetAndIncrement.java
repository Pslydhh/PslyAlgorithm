package com.psly;

import java.lang.reflect.Field;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.atomic.AtomicInteger;

import sun.misc.Unsafe;

public class WaitFreeGetAndIncrement {
	public final static int N = 8;
	public final static int loops = 12500000;
	public static int MAX = N;
	public final static int STEPS = 0;
	public final static int bigYields = 32;
	public final static AtomicInteger inter= new AtomicInteger();
	public final static Map<Integer, Integer> mapMaxTims = new HashMap<Integer, Integer>();
	public final static int[] ints = new int[N * loops];
	public static void main(String[] args) throws InterruptedException {
		int errTimes = 0;
		long maxT = 0;
		for(int k = 0; k < 4096;) {
			atomicInteger.set(0);
			ato.set(0);
			valueObj = new ValueObj(0, null);
			for(int j = 0; j < N * loops; ++j) 
				ints[j] = 0;
			final CountDownLatch latch = new CountDownLatch(1);
			Thread[] threads = new Thread[N];
			long[] times = new long[N];
			for(int i = 0; i < N; ++i) {
	//			threadObjs[i] = new ThreadObj(null);
				states[i] = new StateObj(STEPS);
				int threadId = i;
				(threads[i] = new Thread(){
					public void run(){
						try {
							latch.await();
						} catch (InterruptedException e) {
							// TODO Auto-generated catch block
							e.printStackTrace();
						}
						
						for(int j = 0; j < loops; ++j) {
							long start = System.currentTimeMillis();
							ints[getAndIncrement(threadId, 1)] = 1;
							long costTime = System.currentTimeMillis() - start;
							if(costTime > times[threadId])
								times[threadId] = costTime;
						}
					}
				}).start();
			}
			long start =System.currentTimeMillis();
			latch.countDown();
			long costTimeMax = 0;
			for(Thread thread: threads)
				thread.join();
			for(int i = 0; i < N; ++i)
				if(times[i] > costTimeMax)
					costTimeMax = times[i];
			if(costTimeMax > maxT)
				maxT = costTimeMax;
			System.out.println("\n" + valueObj.value);
			for(int j = 0; j < N * loops; ++j) {
				if(ints[j] != 1) {
					System.out.println(j + " " + ints[j] + " wrong!");
					++errTimes;
				}
			}
			System.out.println("wrongTimes: " + errTimes + " MAX-COST-TIMES: " + (costTimeMax / 1000.0) + " maxInteger: " + maxInteger.get());
			System.out.println("times " + (++k) + " costTime: " + ((System.currentTimeMillis() - start) / 1000.0) + " seconds" + " maxT: " + maxT + " milliseconds");
			Thread.sleep(2000);
		}
	}
	final static boolean casValueObj(ValueObj cmp, ValueObj val) {
		return UNSAFE.compareAndSwapObject(valueBase, valueObjOffset, cmp, val);
	}
	
	static volatile ValueObj valueObj = new ValueObj(0, null);
	//value
	private static final Object valueBase;
	private static final long valueObjOffset;

	final static ThreadObj getThreadObj(long i) {
		return (ThreadObj) UNSAFE.getObjectVolatile(threadObjs, ((long) i << ASHIFT) + ABASE);
	}

	final static void setThreadObj(int i, ThreadObj v) {
		UNSAFE.putObjectVolatile(threadObjs, ((long) i << ASHIFT) + ABASE, v);
	}
	
	final static boolean casThreadObj(int i, ThreadObj cmp, ThreadObj finish) {
		return UNSAFE.compareAndSwapObject(threadObjs, ((long) i << ASHIFT) + ABASE, cmp, finish);
	}
	
	final static ThreadObj[]  threadObjs= new ThreadObj[N];
	final static StateObj[] states = new StateObj[N];
	private static final sun.misc.Unsafe UNSAFE;

	//thread array
	private static final int _Obase;
	private static final int _Oscale;
	private static final long ABASE;
	private static final int ASHIFT;
	
	static {
		try {
			UNSAFE = UtilUnsafe.getUnsafe();
			valueObjOffset = UNSAFE.staticFieldOffset(WaitFreeGetAndIncrement.class.getDeclaredField("valueObj"));
			valueBase = UNSAFE.staticFieldBase(WaitFreeGetAndIncrement.class.getDeclaredField("valueObj"));

			_Obase = UNSAFE.arrayBaseOffset(ThreadObj[].class);
			_Oscale = UNSAFE.arrayIndexScale(ThreadObj[].class);
			ABASE = _Obase;
			if ((_Oscale & (_Oscale - 1)) != 0)
				throw new Error("data type scale not a power of two");
			ASHIFT = 31 - Integer.numberOfLeadingZeros(_Oscale);

		} catch (Exception e) {
			throw new Error(e);
		}
	}
	
	static class ThreadObj {
		public ThreadObj(ValueObj valueObj, int add) {
			super();
			this.valueObj = valueObj;
			this.add = add;
		}
		
		ValueObj valueObj;
		long[] longs = new long[16];
		final int add;
		void casValueObj(ValueObj val) {
			UNSAFE.compareAndSwapObject(this, valueObjOffset, null, val);
		}
		
		private static final sun.misc.Unsafe UNSAFE;
		private static final long valueObjOffset;
		static {
			try {
				UNSAFE = UtilUnsafe.getUnsafe();
				valueObjOffset = UNSAFE.objectFieldOffset(ThreadObj.class.getDeclaredField("valueObj"));
			} catch (Exception e) {
				throw new Error(e);
			}
		}
	}
	
	private static class StateObj {
		public StateObj(int assistStep) {
			super();
			this.assistStep = assistStep;
			this.steps = 0;
			this.index = 0;
		}
		
		private final int assistStep;
		private int steps;
		private long index;
	}
	
	private static class ValueObj {
		private final int value;
		private final ThreadObj threadObj;
		public ValueObj(int value, ThreadObj threadObj) {
			super();
			this.value = value;
			this.threadObj = threadObj;
		}
		
	}
	
	public static AtomicInteger ato = new AtomicInteger();
	public static int getAndIncrementFast(int index) {
		//Thread.yield();
		return ato.getAndIncrement();
	}
	
	private static AtomicInteger atomicInteger = new AtomicInteger();
	private static AtomicInteger maxInteger = new AtomicInteger();
	public static int getAndIncrementFast2(int index) {
		int curr;
		for(;;) {
			curr = atomicInteger.get();
			if(atomicInteger.compareAndSet(curr, curr + 1))
				break;
			Thread.yield();
		} 

		return curr;
	}
	
    public static int getAndIncrement(int index, int add) {
        //fast-path， 最多MAX次。
        int count = MAX;
        for(;;) {
            ValueObj valueObj_ = valueObj;
            if(valueObj_.threadObj == null) {
                ValueObj valueObjNext = new ValueObj(valueObj_.value + add, null);
                if(casValueObj(valueObj_, valueObjNext)) {
                    StateObj myState = states[index];
                    //前进一步，每assistStep，尝试一个帮助。
                    if(((++myState.steps) & myState.assistStep) == 0){
                        long helpThread = myState.index;
                        help(helpThread);
                        //下一个协助的对象。
                        ++myState.index;
                    }
                    return valueObj_.value;
                }
                Thread.yield();Thread.yield();
            } else {
                helpTransfer(valueObj_);
            }
            
            if(--count == 0)
                break;
        }
//        System.out.println("here " + inter.incrementAndGet());
//        inter.incrementAndGet();
        for(int j = 0; j < bigYields; ++j)
            Thread.yield();
        
        //slow-path，将自己列为被帮助对象。
        ThreadObj myselfObj = new ThreadObj(null, add);
        setThreadObj(index, myselfObj);
        //开始帮助自己
        ValueObj result = help(index);
        setThreadObj(index, null);
        return result.value;
    }
    
    // valueObj->threadObj->wrapperObj->valueObj。
    // step 1-3，每一个步骤都不会阻塞其他步骤。
    // 严格遵守以下顺序: 
    // step 1: 通过将ValueObj指向ThreadObj:
    //         atomic: (value, null)->(value, ThreadObj)来锚定该值                      //确定该value归ThreadObj对应线程所有。
    // step 2: 通过将ThreadObj包裹的WrapperObj，
    //         atomic: 从(null, false)更新为(valueObj, true)来更新状态的同时传递value    //对应线程通过isFinish判定操作已完成。
    // step 3: 更新ValueObj，提升value，同时设置ThreadObj为null：
    //         atomic: (value, ThreadObj)->(value+1, null)完成收尾动作                 //此时value值回到了没有被线程锚定的状态，也可以看做step1之前的状态。
    private static ValueObj help(long helpIndex) {
        helpIndex = helpIndex % N;
        ThreadObj helpObj = getThreadObj(helpIndex);
        if(helpObj == null/* || helpObj.valueObj != null*/)
            return null;
        //判定句，是否该线程对应的操作未完成，(先取valueObj，再取isFinish，这很重要)。
        ValueObj valueObj_ = valueObj;
        while(helpObj.valueObj == null) {
            /*ValueObj valueObj_ = valueObj;*/
            if(valueObj_.threadObj == null) {
                ValueObj intermediateObj = new ValueObj(valueObj_.value, helpObj);
                //step1
                if(!casValueObj(valueObj_, intermediateObj)) {
                    valueObj_ = valueObj;
                    continue;
                }
                //step1: 锚定该ValueObj，接下来所有看到该valueObj的线程，都会一致地完成一系列操作.
                valueObj_ = intermediateObj;
            }
            //完成ValueObj、ThreadObj中的WrapperObj的状态迁移。
            helpTransfer(valueObj_);
            valueObj_ = valueObj;
        }
        valueObj_ = helpObj.valueObj;
        helpValueTransfer(valueObj_);
        //返回锚定的valueObj。
        return valueObj_;
    }
    
    private static void helpTransfer(ValueObj valueObj_) {
        ThreadObj threadObj = valueObj_.threadObj;
        //step2: 先完成ThreadObj的状态迁移，WrapperObj(valueObj，true)分别表示(值，完成)，原子地将这两个值喂给threadObj。
        if(threadObj.valueObj == null)
             threadObj.casValueObj(valueObj_);
        
        //step3: 最后完成ValueObj上的状态迁移
        helpValueTransfer(valueObj_);
    }
    
    private static ValueObj helpValueTransfer(ValueObj valueObj_) {
        if(valueObj_ == valueObj) {
            ValueObj valueObjNext = new ValueObj(valueObj_.value + valueObj_.threadObj.add, null);
            casValueObj(valueObj_, valueObjNext);
        }
        return valueObj_;
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
}
