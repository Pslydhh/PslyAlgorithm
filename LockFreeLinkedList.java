package org.psly.concurrent;

import java.lang.reflect.Field;

import sun.misc.Unsafe;

public class LockFreeLinkedList<E extends Comparable<E>> { 
	
	public LockFreeLinkedList() {
		super();
		this.tail = new Node<E>(null, null);
		this.head = new Node<E>(null, new NodePacket<E>(this.tail, false));
	}
		
	public boolean insert(E e){
		Node<E> node = new Node<E>(e, null);
		Object[] nodes;
		//查找该元素
		for(;;){
			nodes = search(e);
			@SuppressWarnings("unchecked")
			Node<E> left = (Node<E>) nodes[0];
			@SuppressWarnings("unchecked")
			NodePacket<E> leftNext = (NodePacket<E>) nodes[1];
			if(leftNext.node != tail && (leftNext.node.value.compareTo(e) == 0))
				return false;
			node.next = new NodePacket<E>(leftNext.node, false);
			if(left.casNext(leftNext, node, leftNext.deleting))
				return true;
		}
	}
	
	@SuppressWarnings("unchecked")
	public boolean delete(E e){
		Object[] nodes;
		Node<E> left;
		NodePacket<E> leftNext;
		Node<E> right;
		for(;;) {
			nodes = search(e);
			left = (Node<E>) nodes[0];
			leftNext = (NodePacket<E>) nodes[1];
			NodePacket<E> rightNext;
			if((right = leftNext.node) == tail || (right.value.compareTo(e) != 0))
				return false;
			if((!(rightNext = right.next).deleting)
				&& right.casNext(rightNext, rightNext.node, true))
					break;
		}
		if(!left.casNext(leftNext, right.next.node, leftNext.deleting))
			search(e);
		return true;
	}
	
	private Object[] search(E key){
		
		searchAgain: 
		for(;;){
			Node<E> prev = head;
			NodePacket<E> prevNext = head.next;
			Node<E> curNode = prevNext.node;
			/* 1: Find left_node and right_node */
			for(;;){
				if(curNode == tail)
					break;
				if(curNode.next.deleting){
					curNode = curNode.next.node;
					continue;
				}
				if(key.compareTo(curNode.value) <= 0){
					break; 
				} else {
					prev = curNode;
					prevNext = curNode.next;
					curNode = prevNext.node;
				}
			}
			Node<E> rightNode = curNode;
			
			//相邻的两个
			if(prevNext.node == rightNode){
				if((rightNode != tail) && rightNode.next.deleting)
					continue searchAgain;
				else
					return new Object[]{prev, prevNext};
			}
			
			
			//不相邻，将中间的这些通通删除
			if(prev.casNext(prevNext, prevNext = new NodePacket<E>(rightNode, prevNext.deleting))){
				if((rightNode != tail) && rightNode.next.deleting)
					continue searchAgain;
				else
					return new Object[]{prev, prevNext};
			}
		}
		
	}

	public String toString(){
		NodePacket<E> nNext = head.next;
		StringBuilder strBuild = new StringBuilder();
		for(;;){
			if(nNext.node == tail)
				break;
			//获取节点
			Node<E> node = nNext.node;
			strBuild.append(node.value).append("\n");
			nNext = node.next;
		}
		return strBuild.toString();
	}
	
	public int size(){
		NodePacket<E> nNext = head.next;
		int num = 0;
		for(;;){
			if(nNext.node == tail)
				break;
			//获取节点
			Node<E> node = nNext.node;
			++num;
			nNext = node.next;
		}
		return num;
	}
	
	public final Node<E> head;
	public final Node<E> tail;
	
	public static class Node<E>{
		final E value;
		volatile NodePacket<E> next;
		
		private static final sun.misc.Unsafe UNSAFE;
		public Node(E value, NodePacket<E> next) {
			super();
			this.value = value;
			this.next = next;
		}

		private static final long nextOffset;
		
		boolean casNext(NodePacket<E> cmp, Node<E> node, boolean deleting){
			return casNext(cmp, new NodePacket<E>(node, deleting));
		}
		
		boolean casNext(NodePacket<E> cmp, NodePacket<E> val){
			return UNSAFE.compareAndSwapObject(this, nextOffset, cmp, val);
		}
		
		static{
			try{
				UNSAFE = UtilUnsafe.getUnsafe();
				nextOffset = UNSAFE.objectFieldOffset(Node.class.getDeclaredField("next"));
			} catch(Exception e){
				throw new Error(e);
			}
		}
	}
	
	public static class NodePacket<E>{
		final Node<E> node;
		final boolean deleting;
		
		public NodePacket(Node<E> node, boolean deleting) {
			super();
			this.node = node;
			this.deleting = deleting;
		}
		
		public Node<E> getNode() {
			return node;
		}
		
		public boolean isDeleting() {
			return deleting;
		}
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
}
