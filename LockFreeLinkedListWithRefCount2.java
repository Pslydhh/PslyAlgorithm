package org.concurrent;

import java.lang.reflect.Field; 
import java.util.Random;
import java.util.concurrent.atomic.AtomicInteger;
import sun.misc.Unsafe;

public class LockFreeLinkedListWithRefCount {
	static int idxNum = 16;
	static int PSLY_Record_IDXNUM = idxNum; 
	static int PSLY_Record_IDXBIT = ((1 << idxNum) - 1); 
	 
	static int arrNum = 4;
	static int PSLY_Record_ARRNUM = arrNum;             
	static int PSLY_Record_ARRAYNUM = (1 << arrNum);     
	static int PSLY_Record_ARRAYBITS = ((1 << arrNum) -1);  
	static int PSLY_Record_ARRBIT = (((1 << arrNum) - 1) << idxNum);  
	static int PSLY_Record_ARRBITR = ((1 << arrNum) - 1);  
	
	static int PSLY_Record_ARRIDXBIT = ((((1 << arrNum) - 1) << idxNum) | ((1 << idxNum) - 1)); 
	
	static int PSLY_Record_NEXTIDXNUM  = idxNum;  
	static int PSLY_Record_NEXTIDXBIT  = ((1 << idxNum) - 1);  
	             
	static int PSLY_Record_NEXTTAILNUM = 1;   
	static int PSLY_Record_NEXTTAILBIT = (((1 << 1) - 1) << idxNum);   
	   
	static int PSLY_Record_NEXTVERSIONNUM = (32 - 1 - idxNum);      
	static int PSLY_Record_NEXTVERSIONBIT = ((~0)^((((1 << 1) - 1) << idxNum) | ((1 << idxNum) - 1)));  
	static int PSLY_Record_NEXTVERSIONONE = (1 + ((((1 << 1) - 1) << idxNum) | ((1 << idxNum) - 1))); 
	  
	static int PSLY_Record_TAILIDXNUM = idxNum;  
	static int PSLY_Record_TAILIDXBIT = ((1 << idxNum) - 1);  
	  
	static int PSLY_Record_TAILVERSIONNUM = (32 - idxNum);  
	static int PSLY_Record_TAILVERSIONBIT = ((~0) ^ ((1 << idxNum) - 1));   
	static int PSLY_Record_TAILVERSIONONE = (1 + ((1 << idxNum) - 1));  
	   
	static int PSLY_Record_HEADIDXNUM = idxNum;  
	static int PSLY_Record_HEADIDXBIT = ((1 << idxNum) - 1);   
	   
	static int PSLY_Record_HEADVERSIONNUM = (32 - idxNum);   
	static int PSLY_Record_HEADVERSIONBIT = ((~0) ^ ((1 << idxNum) - 1));  
	static int PSLY_Record_HEADVERSIONONE = (1 + ((1 << idxNum) - 1));  
	
	static int  IDXNUM_ =    idxNum;                   
	static long  IDXBIT_=    ((1 << idxNum) - 1);  
	       
	static int  ARRNUM_  = arrNum;  
	static long  ARRBIT_ = (((1 << arrNum) - 1) << idxNum);  
	
	static long  ARRIDXBIT_= (((1 << idxNum) - 1) | (((1 << arrNum) - 1) << idxNum));  
	static int refNum = 16;
	static int  REFCB     =     refNum;
	static int 	REFCBITS_    =     ((1 << REFCB) -1);
	static long  REFCBITS  =     (((long)(-1)) << (64 - refNum));
	static long  REFONE    =     (((long)1) << (64 - refNum));
	static long  DELETED   =     0x0000000000000000;
	static int  RECBW     =     (64 - refNum);

	static int  NODEB     =     ((64 - refNum - arrNum - idxNum) >> 1);
	static long  NODEBITS  =     (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)));
	static long  NODEONE   =     ((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))));

	static int  NEXTB     =     ((64 - refNum - arrNum - idxNum) >> 1);
	static long  NEXTBITS  =     ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)));
	static long  NEXTONE   =     (((((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))));
	static long  NEXTTT    =   (((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) ^ ((((((((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) - 1) & (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) ^ (((((long)1) << (64 - refNum)) -1 ) ^ (((((long)1) << (64 - refNum)) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1)))) - 1) >> ((64 - refNum - arrNum - idxNum) >> 1))) | ((((1 << arrNum) - 1) << idxNum)|((1 << idxNum) - 1)));

	static int  RESTART  =    2;
	static int  KEEPPREV  =     1;
	static int  NONE   =   0;

	static int  RECORD =    0x00000004;
	static int  SEARCH    =     0x00000002;
	static int  REMOVE    =     0x00000001;
	
	final static int N = 124;
	final static int times = 800;
	final static int LISTNUM = 512;
	final static int listN = LISTNUM - 1;
	
	static int psly_handle_records(RecordList list, long pointer, int flag) {
		long key = (long) pointer;
		Record head = list.head;
		Record tail = list.tail;
		Record my = null;
		long localN = 0;
		int removed = 0;
		Record prev = head;    
		long prevNext = prev.nextRecord;
		Record curr = idx_Record(prevNext);
		KeepPrev:
		for(;;) { 
			long currNext = curr.nextRecord;
			long currPointer = curr.pointer;
			long New = prev.nextRecord;
		    if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
		    	prev = head;
		    	prevNext = prev.nextRecord;
		    	curr = idx_Record(prevNext);
		    	continue;
		    }
		    if((prevNext & NEXTTT) != (New & NEXTTT)) {
		    	prevNext = New;
		    	curr = idx_Record(prevNext);
		    	continue;
		    }
		    prevNext = New;
		    long currKey = (long) currPointer;
		    if(curr == tail || currKey > key) {
		    	if(flag == SEARCH)
		    		return 0;
		    	if(flag == REMOVE && (prevNext & ARRIDXBIT_) == (curr.self & ARRIDXBIT_))
		    		return 0;
		    	Record append;
		    	/*CasAppend:*/
		    	if(flag == RECORD) {
					if(my == null) {
						my = get_Record();
						localN = (my.nextRecord);
						my.pointer = pointer;
					}
		            my.nextRecord = newNext(localN, curr);
					append = my;
		    	} else {
		    		append = curr;
		    	}
			    for(;;){
			    	if(prev.casNextRecord(prevNext, New = nxtAddrV(prevNext, append))){
			    		long local = prevNext;
			    		while((local & ARRIDXBIT_) != (curr.self & ARRIDXBIT_)) {
			    			Record first = idx_Record(local);
			    			first.pointer = 0;
			    			local = first.nextRecord;
			    			return_Record(first);
			    		}
			    		if(flag == RECORD)
			    			return 1;
			    		else
			    			return 0;
			    	}
			    	New = prev.nextRecord;
					if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
						prev = head;
				        prevNext = (prev.nextRecord);
				        curr = idx_Record(prevNext);
				      	continue KeepPrev;
					}
					if((prevNext & NEXTTT) != (New & NEXTTT)) {
						prevNext = New;
						curr = idx_Record(prevNext);
						continue KeepPrev;
					}
					prevNext = New;
			    }
		    } else {
		  	  if(flag == SEARCH && currKey == key) 
		       	return (int)((currNext >> RECBW) & REFCBITS_);
		  	  New = curr.nextRecord;
		      if((currNext & NODEBITS) != (New & NODEBITS)) {
		    	  New = (prev.nextRecord);
		          if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
		        	  prev = head;
		        	  prevNext = (prev.nextRecord);
		        	  curr = idx_Record(prevNext);
		        	  continue;
		          }
		          prevNext = New;
		          curr = idx_Record(prevNext);
		          continue;
		      }
		      currNext = New;
		      if((currNext & REFCBITS) == DELETED) {
		        curr = idx_Record(currNext);
		        continue;
		      }
		      if(currKey != key) {
		        prev = curr;
		        prevNext = currNext;
		      } else {
		    	  if(removed == 1)
		    		  return 0;
		          Record found = curr;
		          long foundNext = currNext;
		          if(flag == REMOVE) {
		        	  long refNuM;
		        	  if(((refNuM = found.addAndGetNextRecord(-REFONE)) & REFCBITS) != DELETED) 
		        		  return (int) ((refNuM >> RECBW) & REFCBITS_);
		        	  removed = 1;
		        	  currNext = refNuM;
		          } else {
		  			for(;;) {
						long refNuM;   
						if((found.casNextRecord(foundNext, refNuM = plusRecord(foundNext)))) {
							if(my != null) {
								my.nextRecord = localN;
								return_Record(my);
							}
							return (int) ((refNuM >> RECBW) & REFCBITS_);      
						}
						New = found.nextRecord;
						if((foundNext & NODEBITS) != (New & NODEBITS)) {
							New = (prev.nextRecord);
							if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
								prev = head;
								prevNext = (prev.nextRecord);
								curr = idx_Record(prevNext);
								continue KeepPrev;	
							}
							prevNext = New;
							curr = idx_Record(prevNext);
							continue KeepPrev;
						}
						if((New & REFCBITS) == DELETED) {
							currNext = New;
							break;
						}
						foundNext = New;
					}
		          }
		      }
		      curr = idx_Record(currNext);
		    }
		}
	}
	
	static int psly_record(long pointer) {
		int key = (int) pointer;
		RecordList list = map.lists[key & (LISTNUM-1)];
		return psly_handle_records(list, pointer, RECORD);  
	}

	static int psly_remove(long pointer) {
		int key = (int) pointer;
		RecordList list = map.lists[key & (LISTNUM-1)];
		return psly_handle_records(list, pointer, REMOVE);
	}

	static int psly_search(long pointer) {
		int key = (int) pointer;
		RecordList list = map.lists[key & (LISTNUM-1)];
		return psly_handle_records(list, pointer, SEARCH);  
	}
	
	static long nxtAddrV(long old, Record replace) {
		return (old & REFCBITS) | (old & NODEBITS) | ((old + NEXTONE) & NEXTBITS) | (replace.self & ARRIDXBIT_);
	}
		
	static long plusRecord(long old) {
		return ((old + REFONE) & REFCBITS) | (old & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);
	}
		
	static long newNext(long old, Record replace) {
		return REFONE | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (replace.self & ARRIDXBIT_);
	}
		
	static class RecordList {
		volatile Record head ;
		volatile Record tail ;
	}
	
	static class RecordMap {
	    volatile RecordList[] lists;
	} 
	
	static volatile RecordMap map;
	
	static Record idx_Record(long index) {   
		int arr = (((int)index) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
		int idx = ((int)index) & PSLY_Record_IDXBIT;
		return psly_Records[arr][idx];   
	} 	
	
	static volatile AtomicInteger recordTake = new AtomicInteger();  
	static Record get_Record() {             
	    for(int i = 0; i < PSLY_Record_ARRAYNUM; ++i) {
	        int array = recordTake.getAndIncrement() & PSLY_Record_ARRAYBITS;
	        RecordQueue queue = psly_Record_queues[array];
	        Record[] arr = psly_Records[array];
	        for(;;){
	            int headIndex = queue.head;
	            int indexHead = headIndex & PSLY_Record_HEADIDXBIT;
	            Record head = arr[indexHead];
	            int tailIndex = queue.tail;
	            int indexTail = tailIndex & PSLY_Record_TAILIDXBIT;
	            int nextIndex = head.next;

	            if(headIndex == queue.head) {
	                if(indexHead == indexTail){
	                    if((nextIndex & PSLY_Record_NEXTTAILBIT) == PSLY_Record_NEXTTAILBIT)
	                        break;
	                    queue.casTail(tailIndex, (((tailIndex & PSLY_Record_TAILVERSIONBIT) + PSLY_Record_TAILVERSIONONE ) & PSLY_Record_TAILVERSIONBIT)|(nextIndex & PSLY_Record_TAILIDXBIT));    
	                } else {
	                    if(queue.casHead(headIndex, (((headIndex & PSLY_Record_HEADVERSIONBIT) + PSLY_Record_HEADVERSIONONE) & PSLY_Record_HEADVERSIONBIT)|(nextIndex & PSLY_Record_HEADIDXBIT))) {
	             //           head.nextRecord = plusNodeV(head.nextRecord);
	                    	return head;
	                    }
	                }
	            }
	        }
	    }
	  
	    for(int i = 0; i < PSLY_Record_ARRAYNUM; ++i) {
	        int array = i;
	        RecordQueue queue = psly_Record_queues[array];
	        Record[] arr = psly_Records[array];
	        for(;;){
	            int headIndex = queue.head;
	            int indexHead = headIndex & PSLY_Record_HEADIDXBIT;
	            Record head = arr[indexHead];
	            int tailIndex = queue.tail;
	            int indexTail = tailIndex & PSLY_Record_TAILIDXBIT;
	            int nextIndex = head.next;

	            if(headIndex == queue.head) {
	                if(indexHead == indexTail){
	                    if((nextIndex & PSLY_Record_NEXTTAILBIT) == PSLY_Record_NEXTTAILBIT)
	                        break;
	                    queue.casTail(tailIndex, (((tailIndex & PSLY_Record_TAILVERSIONBIT) + PSLY_Record_TAILVERSIONONE ) & PSLY_Record_TAILVERSIONBIT)|(nextIndex & PSLY_Record_TAILIDXBIT));    
	                } else {
	                    if(queue.casHead(headIndex, (((headIndex & PSLY_Record_HEADVERSIONBIT) + PSLY_Record_HEADVERSIONONE) & PSLY_Record_HEADVERSIONBIT)|(nextIndex & PSLY_Record_HEADIDXBIT))) {
	              //      	head.nextRecord = plusNodeV(head.nextRecord);
	                    	return head;
	                    }
	                }
	            }
	        }
	    }
	    return null;
	}
	
	static void return_Record(Record record) {
		record.next |= PSLY_Record_NEXTTAILBIT;
	    int self = record.self;
	    int array = (self >> PSLY_Record_IDXNUM) & PSLY_Record_ARRBITR;
	    Record[] arr = psly_Records[array];
	    RecordQueue queue = psly_Record_queues[array];
	    for(;;) {
	        int tailIndex = queue.tail;
	        int indexTail = tailIndex & PSLY_Record_TAILIDXBIT;
	        Record tail = arr[indexTail];
	        int nextIndex = tail.next;
	
	        if(tailIndex == queue.tail){
	            if((nextIndex & PSLY_Record_NEXTTAILBIT) == PSLY_Record_NEXTTAILBIT) {
	                if(tail.casNext(nextIndex, (((nextIndex & PSLY_Record_NEXTVERSIONBIT) + PSLY_Record_NEXTVERSIONONE) & PSLY_Record_NEXTVERSIONBIT)|(self & PSLY_Record_NEXTIDXBIT))){
	                    queue.casTail(tailIndex, (((tailIndex & PSLY_Record_TAILVERSIONBIT) + PSLY_Record_TAILVERSIONONE) & PSLY_Record_TAILVERSIONBIT)|(self & PSLY_Record_TAILIDXBIT));
	                    return;
	                }
	            } else {
	                queue.casTail(tailIndex, (((tailIndex & PSLY_Record_TAILVERSIONBIT) + PSLY_Record_TAILVERSIONONE) & PSLY_Record_TAILVERSIONBIT)|(nextIndex & PSLY_Record_TAILIDXBIT));
	            }
	        }
	    }
	}  
	
	static Record[][] psly_Records;
	static class Record {
		volatile int next; 
	    final int self; 

		public Record(int self, int next, long nextRecord, long pointer) {
			super();
			this.next = next;
			this.self = self;
			this.nextRecord = nextRecord;
			this.pointer = pointer;
		}
		public boolean casNext(int cmp, int val) {
			return UNSAFE.compareAndSwapInt(this, nextOffset, cmp, val);
		}
		public boolean casNextRecord(long cmp, long val) {
			return UNSAFE.compareAndSwapLong(this, nextRecordOffset, cmp, val);
		}
		public long addAndGetNextRecord(long delta) {
			return UNSAFE.getAndAddLong(this, nextRecordOffset, delta) + delta;
		}
		public void setPointer(long pointer) {
			this.pointer = pointer;
		}
		
		volatile long pointer;
		volatile long nextRecord; 
		private static final sun.misc.Unsafe UNSAFE;
		private static final long nextRecordOffset;
		private static final long nextOffset;
		static {
			try {
				UNSAFE = UtilUnsafe.getUnsafe();
				nextRecordOffset = UNSAFE.objectFieldOffset(Record.class.getDeclaredField("nextRecord"));
				nextOffset = UNSAFE.objectFieldOffset(Record.class.getDeclaredField("next"));
			} catch (Exception e) {
				throw new Error(e);
			}
		}
	}
	
	static RecordQueue[] psly_Record_queues;
	static class RecordQueue {
		volatile int head;
		volatile int tail;
		
		public boolean casHead(int cmp, int val) {
			return UNSAFE.compareAndSwapInt(this, headOffset, cmp, val);
		}
		
		public boolean casTail(int cmp, int val) {
			return UNSAFE.compareAndSwapInt(this, tailOffset, cmp, val);
		}
		
		private static final sun.misc.Unsafe UNSAFE;
		private static final long headOffset;
		private static final long tailOffset;
		static {
			try {
				UNSAFE = UtilUnsafe.getUnsafe();
				headOffset = UNSAFE.objectFieldOffset(RecordQueue.class.getDeclaredField("head"));
				tailOffset = UNSAFE.objectFieldOffset(RecordQueue.class.getDeclaredField("tail"));
			} catch (Exception e) {
				throw new Error(e);
			}
		}
	}
	
	public static void main(String[] argv) {
		if(argv.length != 3)
			return;
		nprocs = Integer.parseInt(argv[0]);
		recordremove =Integer.parseInt(argv[1]);
		int times_ = Integer.parseInt(argv[2]);
		psly_Records = new Record[1 << PSLY_Record_ARRNUM][1 << PSLY_Record_IDXNUM];
		psly_Record_queues = new RecordQueue[1 << PSLY_Record_ARRNUM];
		for(int i = 0; i < (1 << PSLY_Record_ARRNUM); ++i) {
			//初始化所有记录Records
			for(int j = 0; j < (1 << PSLY_Record_IDXNUM) - 1; ++j) {
				Record record = new Record((i << PSLY_Record_IDXNUM) | j, j + 1, 0, 0);
				psly_Records[i][j] = record;
			}
			Record record = new Record((i << PSLY_Record_IDXNUM) | ((1 << PSLY_Record_IDXNUM) - 1), PSLY_Record_NEXTTAILBIT, 39417, 0);
			psly_Records[i][((1 << PSLY_Record_IDXNUM) - 1)] = record;
			//初始化队列
			psly_Record_queues[i] = new RecordQueue();
			psly_Record_queues[i].head = 0;
			psly_Record_queues[i].tail = (1 << PSLY_Record_IDXNUM) - 1;
		}
		
		map = new RecordMap();
		map.lists = new RecordList[LISTNUM];
		for(int i = 0; i < LISTNUM; ++i) { 
		    map.lists[i] = new RecordList();
		    RecordList list = map.lists[i];
		    Record head = get_Record();
		    Record tail = get_Record();
		    head.nextRecord = newNext(head.nextRecord, tail); 
		    list.head = head;
		    list.tail = tail;
		}
		
	//	testRecords();
		int[] errNum = new int[2];
		errNum[0] = 0;
		errNum[1] = 0;
		for(int i = 0; i < times_; ++i)
			testRecordMaps(errNum);
	}
	
	private static class UtilUnsafe {
		private UtilUnsafe() {
		}

		/** Fetch the Unsafe. Use With Caution. */
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
	
	static int numOfItems = 65536;
	static int nprocs = 10000;
	static int recordremove = 1000;
	static int refCount[] = new int[numOfItems];
	static volatile int totalOperation;
	private static Object L;
	private static final long totalOffset;
    private static final int base;
    private static final int shift;
	private static final sun.misc.Unsafe UNSAFE;
	static {
		try {
			UNSAFE = UtilUnsafe.getUnsafe();
			L = UNSAFE.staticFieldBase(LockFreeLinkedListWithRefCount.class.getDeclaredField("totalOperation"));;
			totalOffset = UNSAFE.staticFieldOffset(LockFreeLinkedListWithRefCount.class.getDeclaredField("totalOperation"));
			base = UNSAFE.arrayBaseOffset(int[].class);
	        int scale = UNSAFE.arrayIndexScale(int[].class);
	        if ((scale & (scale - 1)) != 0)
	            throw new Error("data type scale not a power of two");
	        shift = 31 - Integer.numberOfLeadingZeros(scale);
		} catch (Exception e) {
			throw new Error(e);
		}
	}
    private static long byteOffset(int i) {
        return ((long) i << shift) + base;
    }
    
    static void recordremoveRoutine(int k) {
		ThreadLocal<Random> random = new ThreadLocal<Random>() {
			protected Random initialValue() {
				return new Random();
			}
		};
		Random r = random.get();
		if((k & 1) == 1){
			for(int j = 0; j < recordremove; ++j) {
				int key = r.nextInt() & (numOfItems - 1);
				int refc = UNSAFE.getIntVolatile(refCount, byteOffset(key));
				for(;;) {
					if(refc <(( 1 << REFCB) -1)) {
						if(UNSAFE.compareAndSwapInt(refCount, byteOffset(key), refc, refc+1)) {
							psly_record(key * (listN));
							UNSAFE.getAndAddInt(L, totalOffset, 1);
							break;
						}
						refc = UNSAFE.getIntVolatile(refCount, byteOffset(key));
					} else break;
				}
			}
		} else {
			int key = r.nextInt() & (numOfItems - 1);
			int f = key & 1;
			for(int j = 0; j < recordremove; ++j) {
				int refc = UNSAFE.getIntVolatile(refCount, byteOffset(key));
				for(;;) {
					if(refc > 0) {
						if(UNSAFE.compareAndSwapInt(refCount, byteOffset(key), refc, refc-1)) {
							psly_remove(key * (listN));
							UNSAFE.getAndAddInt(L, totalOffset, 1);
				//			break;
						}
						refc = UNSAFE.getIntVolatile(refCount, byteOffset(key));
					} else break;
				}
				if( f == 1)
					key = (key + 1) & (numOfItems - 1);
				else 
					key = (key - 1) & (numOfItems - 1);
			}
		}
    }
	static void testRecordMaps(int[] errNum) {
		long start = System.currentTimeMillis();
		System.out.println("\n");
		System.out.println(++errNum[1] + " times: ");
		Thread[] tids = new Thread[nprocs];
		for(int i = 0; i < nprocs; ++i) {
			(tids[i] = new Thread() {
				public void run() {
					recordremoveRoutine(errNum[1]);
				}
			}).start();
		}
		for(int i = 0; i < nprocs; ++i) {
			try {
				tids[i].join();
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
		int total = nprocs * recordremove;
		int add = 0;
		for(int i = 0; i < numOfItems; ++i) {
			int lo = psly_search(i * listN);
			int lo2 = UNSAFE.getIntVolatile(refCount, byteOffset(i));
			if(i < 8)	System.out.println(i + ": " + lo2 + "==" + lo);
			if(lo != lo2){
				++errNum[0];
				System.out.println(i + ": " + lo2 + "==" + lo + " WRONG!!");
			}
		}
	
		System.out.println("err numbers: " + errNum[0]);
		System.out.println("total: " + total + ", adds " + add);
		System.out.println("actual op: " + totalOperation);
		System.out.println("total_use: " + ((System.currentTimeMillis() - start) / 1000.0) );
		int k = 0;
		for(int i = 0; i < LISTNUM; ++i) { 
		    RecordList list = map.lists[i];
		    Record head = idx_Record(list.head.nextRecord);
		    Record tail = list.tail;
		    while(head != tail) {
		    	++k;
		    	head = idx_Record(head.nextRecord);
		    }
		}
		System.out.println(Integer.toString(LISTNUM) + ", " + k + ", " + ((LISTNUM ==k)? 1:0));
	}
	
	static void testRecords() {
		int total = 0;
		for(int i = 0; i < PSLY_Record_ARRAYNUM; ++i) {
			RecordQueue q = psly_Record_queues[i];
			Record[] currs =  psly_Records[i];
			Record curr = currs[q.head & PSLY_Record_IDXBIT];
			++total;
			while((curr) != currs[q.tail & PSLY_Record_IDXBIT]){
				++total;
				curr = currs[curr.next & PSLY_Record_IDXBIT];
			}
		}
		System.out.println("total: " + total);
		Thread[] tids = new Thread[N];
		for(int i = 0; i < N; ++i) {
			tids[i] = new Thread() {
				public void run() {
					Record[] records = new Record[times];
					for(int j = 0; j < times; ++j) {
						Record record = get_Record();
						records[j] = record;
					}
					for(int j = times-1; j >= 0; --j) 
						return_Record(records[j]);
				}
			};
			tids[i].start();
		}
		
		for(int i = 0; i < N; ++i) {
			try {
				tids[i].join();
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		total = 0;
		for(int i = 0; i < PSLY_Record_ARRAYNUM; ++i) {
			RecordQueue q = psly_Record_queues[i];
			Record[] currs =  psly_Records[i];
			Record curr = currs[q.head & PSLY_Record_IDXBIT];
			++total;
			while((curr) != currs[q.tail & PSLY_Record_IDXBIT]){
				++total;
				curr = currs[curr.next & PSLY_Record_IDXBIT];
			}
		}
		
		System.out.println("total: " + (total /*+ N * (times-1)*/));
	}
}
