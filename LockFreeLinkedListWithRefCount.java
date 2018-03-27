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
	static int 	REFCBIT_    =     ((1 << REFCB) -1);
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
	final static int[] flags = new int[1 << 22];
	
	static int psly_handle_records(RecordList list, long pointer, int flag, int listnum) {
		if(pointer == -1)
			System.out.println("DKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKKK\n");
		long key = (long) pointer;
		Record head = list.head;
		Record tail = list.tail;
		Record my;
		int removed = 0;
		Record prev;    
		boolean CasRemoveFlag = false;
		//
		Restart: 
		for(;;) { 
			prev = head;
			long prevNext = prev.nextRecord;
			Record curr = idx_Record(prevNext, tail, listnum, 1);
			Record found = null;
			long foundNext = 0;
			long New;
			// 由于JAVA缺少goto，用于从KeepPrev的外部跳回循环
			KeepPrevOuter:
			for(;;) {
				KeepPrev:
				for(;;) {
					int fl = UNSAFE.getIntVolatile(flags, byteOffset((int)(curr.self & ARRIDXBIT_)));
				    long currNext = curr.nextRecord;
				    if(tail != curr && (currNext == 39417 || currNext == 39400)) {
				    	System.out.println("\nNO self " + ((curr.self >> PSLY_Record_IDXNUM) & PSLY_Record_ARRBITR) + ":" + (curr.self & PSLY_Record_IDXBIT));
				    	System.out.println("prevNext:" + prevNext + " currNext:" + curr.nextRecord);
				    	System.out.println(curr == idx_Record(prevNext, tail, listnum, 2));
				    	System.out.println("Flag == " + (flag));
				    	System.out.println("changed? " + fl);
				    	System.out.println("nodeV: " + (currNext & NODEBITS) + " listnum: " + listnum);
				    	System.out.println();
				    }
				    long litc = curr.listCount;

				    long currPointer = curr.pointer;
				    New = prev.nextRecord;
				    if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
				    	continue Restart;
				    }
				    if((prevNext & NEXTTT) != (New & NEXTTT)) {
				    	prevNext = New;
				    	curr = idx_Record(prevNext, tail, listnum, 3);
				    	found = null;
				    	continue KeepPrev;
				    }
				    prevNext = New;
				    if(found != null){
				    	New = found.nextRecord;
				    	if((foundNext & NEXTTT) != (New & NEXTTT)) {
				    		foundNext = (foundNext & ~NEXTTT) | (New & NEXTTT);
				    		curr = idx_Record_(foundNext, tail, listnum);
				    		continue KeepPrev;
				    	}
				    }
				    if(litc != listnum){
				    	System.out.println("NO!!!!!!!!!!!!!!!!!!!!!!!!!!!");
				    }
/*				    if(tail != curr && currPointer == -1) {
				    	System.out.println("AfterOH myGOD!!!!!!!!!!! self " + ((curr.self >> PSLY_Record_IDXNUM) & PSLY_Record_ARRBITR) + ":" + (curr.self & PSLY_Record_IDXBIT));
				    	System.out.println("prevNext:" + prevNext + " New:" + New);
				    	System.out.println(curr == idx_Record(prevNext));
				    	System.out.println("Flag == " + (flag == REMOVE));
				    }*/
				    if(curr == tail || currPointer > key) {
				    	if(found != null) {
				            New = found.nextRecord;
				            for(;;) {
				                if((foundNext & NODEBITS) != (New & NODEBITS)) {
				                	// REMOVE用于处理删除之后的情况
				                	if(flag == REMOVE) {
				                		return 0;
				                	}
				                	New = prev.nextRecord;
				                	if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
				                		continue Restart;
				                	} else {
				                		//goto JUSTNEXTCHANGE;
					                	prevNext = New;
					                	curr = idx_Record(prevNext, tail, listnum, 5);
					                	found = null;
					                	continue KeepPrev;
				                	}
				                }
				                foundNext = New;
				                if((foundNext & REFCBITS) == DELETED) {
				                	curr = idx_Record(foundNext, tail, listnum, 6);
				                	CasRemoveFlag = true;
				                	break KeepPrev;
				                } else {
				                	if(flag == REMOVE) {
				                		long refNum_;
				                		if(((refNum_ = found.addAndGetNextRecord(-REFONE)) & REFCBITS) != DELETED) {
					    					int arr = (((int)refNum_) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
					                		int idx = ((int)refNum_) & PSLY_Record_IDXBIT;
					                		int tailArr = ((tail.self) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
					                		int tailIdx = 	(tail.self) & PSLY_Record_IDXBIT;	            	
//					                		System.out.println("self2 " + listNum + " num: " +(((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)+ " self " + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM) + ":" + (tail.self & PSLY_Record_IDXBIT));
//					                		System.out.println("is true? " + (listNum == (((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)));
					                	    if(idx < 64 && arr < 16 && ((idx != tailIdx) || (arr != tailArr))) {
					                	    	System.out.println("!!!!!!!!!self " + arr + ":" + idx);
					                	    	System.out.println("tailSelf " + tailArr + ":" + tailIdx);
					                	    	System.out.println(refNum_);
					                	    }
				                			return (int)((refNum_ >> RECBW) & REFCBIT_);
				                		}
				                		removed = 1;
				                		New = refNum_;
				                //		System.out.println(refNum_);
				                //		return 0;
				                	} else {
				                		long refNum_;   
				                		if((found.casNextRecord(foundNext, refNum_ = plusRecord(foundNext)))) 
				                			return (int)((refNum_ >> RECBW) & REFCBIT_);       
				                		New = found.nextRecord;
				                	}      
				                }
				            }
				    	}
				        if(flag == SEARCH)
				        	return 0;
				        if((prevNext & ARRIDXBIT_) != (curr.self & ARRIDXBIT_)) {
				        	CasRemoveFlag = true;
				        	break KeepPrev;   
				        } else { 
				        	if(removed != 0)
				        		return 0; 
				        	break KeepPrev; 
				        }
				    } else {
				    	New = curr.nextRecord;
				        if((currNext & NODEBITS) != (New & NODEBITS)) {
				        	New = prev.nextRecord;
				        	if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
				        		continue Restart;
				        	} else {
					        	//goto JUSTNEXTCHANGE;
					        	prevNext = New;
		  				      	curr = idx_Record(prevNext, tail, listnum, 7);
		  				      	found = null;
		  				      	continue KeepPrev;
				        	}
				        }
				        currNext = New;
				        if((currNext & REFCBITS) == DELETED) {
				        	curr = idx_Record(currNext, tail, listnum, 8);
				        	continue KeepPrev;
				        }
				        if(currPointer != key) {
				        	prev = curr;
				        	prevNext = currNext;
				        } else {
				        	if(removed != 0)
				        		return 0;
				        	found = curr;
				        	foundNext = currNext;
				            if(flag == SEARCH) {
				            	return (int)(((foundNext & REFCBITS) >> RECBW) & ((1 << REFCB) - 1)) ;
				            }
				        }
				        curr = idx_Record(currNext, tail, listnum, 9);
				        continue KeepPrev;
				    }
				}
				
				if(CasRemoveFlag) {
					CasRemoveFlag = false;
		        	CasRemove:
		        	for(;;){
		            	if(prev.casNextRecord(prevNext, New = nxtAddrV(prevNext, curr))) {
	    					int arr = (((int)New) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
	                		int idx = ((int)New) & PSLY_Record_IDXBIT;
	                		int tailArr = ((tail.self) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
	                		int tailIdx = 	(tail.self) & PSLY_Record_IDXBIT;	            	
//	                		System.out.println("self2 " + listNum + " num: " +(((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)+ " self " + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM) + ":" + (tail.self & PSLY_Record_IDXBIT));
//	                		System.out.println("is true? " + (listNum == (((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)));
	                	    if(idx < 64 && arr < 16 && ((idx != tailIdx) || (arr != tailArr))) {
	                	    	System.out.println("!!!!!!!!!self " + arr + ":" + idx);
	                	    	System.out.println("tailSelf " + tailArr + ":" + tailIdx);
	                	    	System.out.println(New);
	                	    }
		                    long local = prevNext;
		                    prevNext = New;
		                    int STEP = NONE;
		                    do {  
		                    	Record first = idx_Record(local, tail, listnum, 10);
		                    	local = first.nextRecord;
		                    	first.pointer = -1;
		       //             	System.out.println(local);
		                    	return_Record(first);
		                    	if(flag == RECORD) {
		                    		New = prev.nextRecord;
			                    	if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) 
			                    		STEP = (STEP < RESTART ? RESTART : STEP);
			                        if((prevNext & NEXTTT) != (New & NEXTTT)) 
			                        	STEP = (STEP < KEEPPREV ? KEEPPREV : STEP);
			                        prevNext = New;
		                    	} 
		                    } while((local & ARRIDXBIT_) != (curr.self & ARRIDXBIT_));
		                    if(flag == REMOVE)
		                    	return 0;
		                    if(STEP == RESTART)
		                    	continue Restart;
		                    if(STEP == KEEPPREV) {
		                    	//goto JUSTNEXTCHANGE; 
		  				      	prevNext = New;
		  				      	curr = idx_Record(prevNext, tail, listnum, 11);
		  				      	found = null;
		  				      	continue KeepPrevOuter;
		                    }
		                    break CasRemove;
		            	} else {
			            	New = prev.nextRecord;
			            	if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
			            		continue Restart;
			            	}
			            	if((prevNext & NEXTTT) != (New & NEXTTT)) {
			            		//goto JUSTNEXTCHANGE;
			            		prevNext = New;
			            		curr = idx_Record(prevNext, tail, listnum, 12);
			            		found = null;
			            		continue KeepPrevOuter;
			            	}
			            	prevNext = New;
			            	continue CasRemove;
		            	}
		        	}
				}
				
                CasAppend:
                for(;;) {
                    my = get_Record();
                    New = prev.nextRecord;
                    if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED) {
                    	return_Record(my); 
                    	continue Restart;
                    }
                    if((prevNext & NEXTTT) != (New & NEXTTT)) {
                    	return_Record(my);
                      	//goto JUSTNEXTCHANGE;
  				      	prevNext = New;
  				      	curr = idx_Record(prevNext, tail, listnum, 13);
  				      	found = null;
  				      	continue KeepPrevOuter;
                    }
                    
                    long localN = my.nextRecord;
        //            System.out.println(localN);
                    long localP = my.pointer;
                    int listC = my.listCount;
                    my.listCount = listnum;
                    my.pointer = pointer;
                    long L;
                    my.nextRecord = L = newNext(localN, curr);
					int arr = (((int)L) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
            		int idx = ((int)L) & PSLY_Record_IDXBIT;
            		int tailArr = ((tail.self) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
            		int tailIdx = 	(tail.self) & PSLY_Record_IDXBIT;	            	
//            		System.out.println("self2 " + listNum + " num: " +(((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)+ " self " + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM) + ":" + (tail.self & PSLY_Record_IDXBIT));
//            		System.out.println("is true? " + (listNum == (((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)));
            	    if(idx < 64 && arr < 16 && ((idx != tailIdx) || (arr != tailArr))) {
            	    	System.out.println("!!!!!!!!!self " + arr + ":" + idx);
            	    	System.out.println("tailSelf " + tailArr + ":" + tailIdx);
            	    	System.out.println(L);
            	    }
                    long kkk;
                    if(prev.casNextRecord(prevNext, kkk = nxtAddrV(prevNext, my))) { 
    					 arr = (((int)kkk) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
                		 idx = ((int)kkk) & PSLY_Record_IDXBIT;
                		 tailArr = ((tail.self) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
                		 tailIdx = 	(tail.self) & PSLY_Record_IDXBIT;	            	
//                		System.out.println("self2 " + listNum + " num: " +(((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)+ " self " + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM) + ":" + (tail.self & PSLY_Record_IDXBIT));
//                		System.out.println("is true? " + (listNum == (((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)));
                	    if(idx < 64 && arr < 16 && ((idx != tailIdx) || (arr != tailArr))) {
                	    	System.out.println("!!!!!!!!!self " + arr + ":" + idx);
                	    	System.out.println("tailSelf " + tailArr + ":" + tailIdx);
                	    	System.out.println(kkk);
                	    }
                    	UNSAFE.putIntVolatile(flags, byteOffset((int)(my.self & ARRIDXBIT_)), 1);

                    	return 1;
                    }
                    my.nextRecord = localN;
                    my.pointer = localP;
                    my.listCount = listC;
                    return_Record(my);
                    New = prev.nextRecord;
                    if((prevNext & NODEBITS) != (New & NODEBITS) || (New & REFCBITS) == DELETED)
                    	continue Restart;
                    if((prevNext & NEXTTT) != (New & NEXTTT)) {
                    	//goto JUSTNEXTCHANGE;
  				      	prevNext = New;
  				      	curr = idx_Record(prevNext, tail, listnum, 14);
  				      	found = null;
  				      	continue KeepPrevOuter;
                    }
                    prevNext = New;
                    continue CasAppend;  
                }
			}
		}
	}
	
	static int psly_record(long pointer) {
		int key = (int) pointer;
		RecordList list = map.lists[key & (LISTNUM-1)];
	//	System.out.println(key & (listNum - 1));
		return psly_handle_records(list, pointer, RECORD, key & (LISTNUM-1));  
	}

	static int psly_remove(long pointer) {
		int key = (int) pointer;
		RecordList list = map.lists[key & (LISTNUM-1)];
		return psly_handle_records(list, pointer, REMOVE, key & (LISTNUM-1));
	}

	static int psly_search(long pointer) {
		int key = (int) pointer;
		RecordList list = map.lists[key & (LISTNUM-1)];
		return psly_handle_records(list, pointer, SEARCH, key & (LISTNUM-1));  
	}
	
	
	static long nxtAddrV(long old, Record replace) {
		/*System.out.println(NEXTBITS >> 20);
		System.out.println(ARRIDXBIT_ );
		System.out.println(REFCBITS >>> 48);
		System.out.println(NODEBITS >> 34);*/
	  return (old & REFCBITS) | (old & NODEBITS) | ((old + NEXTONE) & NEXTBITS) | (replace.self & ARRIDXBIT_);
	}
	
	static long plusRecord(long old) {
	  return ((old + REFONE) & REFCBITS) | (old & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);
	}
	
	static long plusNodeV(long old) {
		return (old & REFCBITS) | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (old & ARRIDXBIT_);
	}
	
	static long newNext(long old, Record replace) {
	  long l = REFONE | ((old + NODEONE) & NODEBITS) | (old & NEXTBITS) | (replace.self & ARRIDXBIT_);
	  if(l == 39417 || l == 39400) {
		  System.out.println("DDDDDDDDDDDDDDDDVVVVVVVVVVVVVVVVVVVVVVVVVVVvv\n");
		  System.exit(1);
	  }
	  return l;
	}
		
	static class RecordList {
		volatile Record head ;
		volatile Record tail ;
	}
	
	static class RecordMap {
	    volatile RecordList[] lists;
	} 
	
	static volatile RecordMap map;
	
	static Record idx_Record(long index, Record tail, int listNum, int num) {   
		int arr = (((int)index) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
		int idx = ((int)index) & PSLY_Record_IDXBIT;
		int tailArr = ((tail.self) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM;
		int tailIdx = 	(tail.self) & PSLY_Record_IDXBIT;	            	
//		System.out.println("self2 " + listNum + " num: " +(((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)+ " self " + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM) + ":" + (tail.self & PSLY_Record_IDXBIT));
//		System.out.println("is true? " + (listNum == (((tail.self & PSLY_Record_IDXBIT) * 16 + ((tail.self & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM))/2)));
	    if(idx < 64 && arr < 16 && ((idx != tailIdx) || (arr != tailArr))) {
	    	System.out.println("~~~~~~~self " + arr + ":" + idx);
	    	System.out.println("tailSelf " + tailArr + ":" + tailIdx);
	    	System.out.println(index);
	    	System.out.println(num);
	    }
		return psly_Records[arr][idx];   
	} 
	
	static Record idx_Record_(long index, Record tail, int listNum) {   
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
	    public volatile int listCount = -1;

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
			if(val == 39417 || val == 39400) {
				System.out.println("DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDYYYYYYYYYYYYYYYYYYYYYYYY\n");
				System.exit(1);
			}
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
		if(argv.length != 2)
			return;
		nprocs = Integer.parseInt(argv[0]);
		recordremove =Integer.parseInt(argv[1]);
	 //   return psly_Records[(((int)index) & PSLY_Record_ARRBIT) >> PSLY_Record_IDXNUM][((int)index) & PSLY_Record_IDXBIT];   
	 /*   System.out.println(PSLY_Record_ARRBIT >> PSLY_Record_IDXNUM);
	    System.out.println(PSLY_Record_IDXNUM);
	    System.out.println(PSLY_Record_IDXBIT);*/
		psly_Records = new Record[1 << PSLY_Record_ARRNUM][1 << PSLY_Record_IDXNUM];
		psly_Record_queues = new RecordQueue[1 << PSLY_Record_ARRNUM];
		for(int i = 0; i < (1 << PSLY_Record_ARRNUM); ++i) {
			//初始化所有记录Records
			for(int j = 0; j < (1 << PSLY_Record_IDXNUM) - 1; ++j) {
				Record record = new Record((i << PSLY_Record_IDXNUM) | j, j + 1, (j % 2 == 1)?39417:39400, 0);
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
		    head.listCount = i;
		    Record tail = get_Record();
		    tail.listCount = i;
		    head.nextRecord = newNext(head.nextRecord, tail); 
//		    tail.nextRecord = 0;
		    list.head = head;
		    list.tail = tail;
	//	    System.out.println("list " + i + " self " + ((map.lists[i].head.self >> PSLY_Record_IDXNUM) & PSLY_Record_ARRBITR) + ":" + (map.lists[i].head.self & PSLY_Record_IDXBIT));
	//	    System.out.println("list " + i + " self " + ((map.lists[i].tail.self >> PSLY_Record_IDXNUM) & PSLY_Record_ARRBITR) + ":" + (map.lists[i].tail.self & PSLY_Record_IDXBIT));
		}
		
	//	testRecords();
		testRecordMaps();
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
	
	static int numOfItems = 1024;
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
    
    static void recordremoveRoutine() {
		ThreadLocal<Random> random = new ThreadLocal<Random>() {
			protected Random initialValue() {
				return new Random();
			}
		};
		Random r = random.get();
		int key = r.nextInt() & (numOfItems - 1);
		for(int j = 0; j < recordremove; ++j) {
			if(key == 0)
				key = 1;
			int refc = UNSAFE.getIntVolatile(refCount, byteOffset(key));
			for(;;) {
				if(refc <(( 1 << REFCB) -1)) {
					if(UNSAFE.compareAndSwapInt(refCount, byteOffset(key), refc, refc+1)) {
						psly_record(key * (listN));
						UNSAFE.getAndAddInt(L, totalOffset, 1);
						psly_remove(key * (listN));
						UNSAFE.getAndAddInt(refCount, byteOffset(key), -1);
						UNSAFE.getAndAddInt(L, totalOffset, 1);
						break;
					}
					refc = UNSAFE.getIntVolatile(refCount, byteOffset(key));
				} else break;
			}
			key = (key + 1) & (numOfItems - 1);
		}
    }
	static void testRecordMaps() {
		long start = System.currentTimeMillis();
		
		Thread[] tids = new Thread[nprocs];
		for(int i = 0; i < nprocs; ++i) {
			(tids[i] = new Thread() {
				public void run() {
					recordremoveRoutine();
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
		int errCount = 0;
		for(int i = 0; i < numOfItems; ++i) {
			int re;
			if(i == 0)
				continue;
			if(UNSAFE.getIntVolatile(refCount, byteOffset(i)) != (re = psly_search(i * listN))){
				++errCount;
			}
		}
	
		System.out.println("err numbers: " + errCount);
		System.out.println("total: " + total + ", adds " + add);
		System.out.println("actual op: " + totalOperation);
		System.out.println("total_use: " + ((System.currentTimeMillis() - start) / 1000.0) );
		int k = 0;
		for(int i = 0; i < LISTNUM; ++i) { 
		    RecordList list = map.lists[i];
		    Record head = list.head;
		    Record tail = list.tail;
		    while(head != tail) {
		    	++k;
		    	head = idx_Record(head.nextRecord, tail, 0, 15);
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
