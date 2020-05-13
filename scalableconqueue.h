#include <atomic>

using namespace std;

template<typename T>
class ScalableConQueue {
  const static long TOP = (-1);

  class EnqRequest {
    friend class ScalableConQueue;
    atomic<long> id;
    atomic<T*> val;
  };

  class DeqRequest {
    friend class ScalableConQueue;
    atomic<long> id;
    atomic<long> idx;
  };

  class Cell {
    friend class ScalableConQueue;
    atomic<T*> val;
    atomic<EnqRequest*> enq;
    atomic<long> help_enq_id;
    atomic<DeqRequest*> deq;
    void* pad[4];
  };

  // 1024 bytes;
  const static int NODE_SIZE = 64;

  class Node {
    friend class ScalableConQueue;
    atomic<Node*> next;
    atomic<long> id;
    Cell cells[NODE_SIZE];
  };

  class Handle {
    friend class ScalableConQueue;
    Handle(ScalableConQueue<T>* queue_outer) {
      this->next = nullptr;
      this->prev = nullptr;
      this->hzd_node_id = -1;
      this->Er.id = 0;
      this->Er.val = nullptr;
      this->Dr.id = 0;
      this->Dr.idx = -1;
      this->enq_helped = nullptr;
      this->deq_helped = nullptr;
      this->enq_help_id = 0;
      this->spare = new Node();
      this->delay = 0;
      this->spanning_cells = 0;
      this->cleanId = 0;
      {
        unique_lock<mutex> control(queue_outer->safe_for_reclaim);
        this->Ep.store(queue_outer->hp, memory_order_seq_cst);
        this->enq_node_id = this->Ep.load(memory_order_seq_cst)->id;
        this->Dp.store(queue_outer->hp, memory_order_seq_cst);
        this->deq_node_id = this->Dp.load(memory_order_seq_cst)->id;

        if (queue_outer->tail == nullptr) {
          this->next.store(this, memory_order_seq_cst);
          queue_outer->tail = this;

          this->prev = this;
        }
        else {
          Handle* next = queue_outer->tail->next.load(memory_order_seq_cst);
          this->next.store(next, memory_order_seq_cst);
          queue_outer->tail->next.store(this, memory_order_seq_cst);

          this->prev = queue_outer->tail;
          next->prev = this;
        }
        this->enq_helped = this->next.load(memory_order_seq_cst);
        this->deq_helped = this->next.load(memory_order_seq_cst);
        ++queue_outer->nprocess;
      }
    }

    atomic<Handle*> next;
    atomic<Handle*> prev;
    atomic<unsigned long> hzd_node_id;
    atomic<Node*> Ep;
    unsigned long enq_node_id;
    atomic<Node*> Dp;
    unsigned long deq_node_id;
    EnqRequest Er;
    DeqRequest Dr;
    Handle* enq_helped;
    Handle* deq_helped;
    long enq_help_id;
    Node* spare;
    long spanning_cells;
    int delay;
    unsigned long cleanId;
  };

  class HandleHolder {
    friend class ScalableConQueue;
    HandleHolder(ScalableConQueue<T>* queue_outer_) : queue_outer(queue_outer_), handle(new Handle(queue_outer_)) {
    }
    ~HandleHolder() {
      unique_lock<mutex> control(queue_outer->safe_for_reclaim);
      Handle* prev = handle->prev;
      Handle* next = handle->next.load(memory_order_seq_cst);
      prev->next.store(next, memory_order_seq_cst);
      next->prev = prev;

      if (handle == queue_outer->tail) {
        if (prev == handle)
          queue_outer->tail = nullptr;
        else
          queue_outer->tail = prev;
      }
      --queue_outer->nprocess;

      // handle->next.store(nullptr, memory_order_seq_cst);
      handle->prev = queue_outer->leave_nodes;
      queue_outer->leave_nodes = handle;
    }

    ScalableConQueue<T>* queue_outer;
    Handle* handle;
  };

  Handle* get_handle() {
    thread_local HandleHolder handle_for_thread(this);
    return handle_for_thread.handle;
  }

  Cell* find_cell(Node** ptr, long i, Handle* th) {
    Node* curr = *ptr;
    for (long j = curr->id; j < i / NODE_SIZE; ++j) {
      Node* next = curr->next.load(memory_order_seq_cst);

      if (next == nullptr) {
        Node* temp = th->spare;
        if (temp == nullptr) {
          temp = new Node();
          th->spare = temp;
        }

        temp->id = j + 1;
        if (curr->next.compare_exchange_strong(next, temp)) {
          next = temp;
          th->spare = nullptr;
        }
      }

      curr = next;
    }
    *ptr = curr;
    return &curr->cells[i & (NODE_SIZE - 1)];
  }

  int enq_fast(T* const val, Handle* th, long& id) {
    long i = ei.fetch_add(1, memory_order_seq_cst);

    Node* tmp = th->Ep.load(memory_order_seq_cst);
    Cell* cell = find_cell(&tmp, i, th);
    th->Ep.store(tmp, memory_order_seq_cst);
    T* old = nullptr;

    if (cell->val.compare_exchange_strong(old, val)) {
      //fast_push.fetch_add(1);
      return 1;
    }
    else {
      id = i;
      return 0;
    }
  }

  void enq_slow(T* val, Handle* th, long id) {
    EnqRequest* enq = &th->Er;
    enq->val.store(val, memory_order_seq_cst);
    enq->id.store(id, memory_order_seq_cst);

    Node* tail = th->Ep.load(memory_order_seq_cst);
    long i;
    Cell* c;
    do {
      i = ei.fetch_add(1, memory_order_seq_cst);
      c = find_cell(&tail, i, th);
      long chei = 0;

      if (c->help_enq_id.compare_exchange_strong(chei, id) || chei == id) {
        c->enq.store(enq, memory_order_seq_cst);
        if (c->val.load(memory_order_seq_cst) != static_cast<T*>((void*)-1)) {
          (enq->id.compare_exchange_strong(id, -i));
          //slow_push.fetch_add(1);
          break;
        }
      }
    } while (enq->id > 0);

    id = -enq->id;

    Node* tmp = th->Ep.load(memory_order_seq_cst);
    c = find_cell(&tmp, id, th);
    th->Ep.store(tmp, memory_order_seq_cst);

    if (id > i) {
      track_index(ei, id);
    }
    c->val.store(val, memory_order_seq_cst);
  }

  void move_next_help(Handle* th, Handle* ph) {
    th->enq_help_id = 0;
    th->enq_helped = ph->next;
  }

  void put_value_increase_ei(atomic_long& ei, long i, Cell* c, T* ev) {
    track_index(ei, i);
    c->val.store(ev, memory_order_seq_cst);
  }

  T* help_enq(Handle* th, Cell* c, long i) {
    T* v = spin(c->val);

    unsigned long prev_spanning_cells = th->spanning_cells;
    if (v != static_cast<T*>((void*)-1)) {
      if (v != nullptr)
        return v;
      if (c->val.compare_exchange_strong(v, static_cast<T*>((void*)-1)))
        ++th->spanning_cells;

      else if (v != static_cast<T*>((void*)-1))
        return v;
    }

    EnqRequest* e;
    long chei = c->help_enq_id.load(memory_order_seq_cst);
    if (chei == 0) {
      Handle* ph;
      EnqRequest* pe;
      long id;
      ph = th->enq_helped, pe = &ph->Er, id = pe->id.load(memory_order_seq_cst);

      if (th->enq_help_id != 0 && th->enq_help_id != id) {
        move_next_help(th, ph);
        ph = th->enq_helped, pe = &ph->Er, id = pe->id.load(memory_order_seq_cst);
      }

      if (id > 0 && id <= i) {
        if (c->help_enq_id.compare_exchange_strong(chei, id)) {
          c->enq.store(e = pe, memory_order_seq_cst);
          chei = id;
          move_next_help(th, ph);
        }
        else if (chei == id) {
          c->enq.store(e = pe, memory_order_seq_cst);
          move_next_help(th, ph);
        }
        else if (chei == TOP) {
          th->enq_help_id = id;
          return (ei.load(memory_order_seq_cst) <= i ? nullptr : static_cast<T*>((void*)-1));
        }
        else {
          th->enq_help_id = id;
          e = spin_util_val(c->enq);
        }
      }
      else {
        move_next_help(th, ph);
        if (c->help_enq_id.compare_exchange_strong(chei, TOP) || chei == TOP) {
          return (ei.load(memory_order_seq_cst) <= i ? nullptr : static_cast<T*>((void*)-1));
        }
        else {
          e = spin_util_val(c->enq);
        }
      }
    }
    else if (chei == TOP) {
      return (ei.load(memory_order_seq_cst) <= i ? nullptr : static_cast<T*>((void*)-1));
    }
    else {
      e = spin_util_val(c->enq);
    }

    long ei_ = e->id.load(memory_order_seq_cst);
    T* ev = e->val.load(memory_order_seq_cst);

    if (ei_ > 0) {
      if (chei == ei_) {
        if (e->id.compare_exchange_strong(ei_, -i)) {
          //slow_push.fetch_add(1);
          put_value_increase_ei(ei, i, c, ev);
        }

        if (ei_ == -i && c->val.load(memory_order_seq_cst) == static_cast<T*>((void*)-1)) {
          put_value_increase_ei(ei, i, c, ev);;
        }
      }
    }
    else if (ei_ == -i && c->val.load(memory_order_seq_cst) == static_cast<T*>((void*)-1)) {
      put_value_increase_ei(ei, i, c, ev);;
    }

    if (c->val.load(memory_order_seq_cst) != static_cast<T*>((void*)-1))
      th->spanning_cells = prev_spanning_cells;
    return c->val;
  }

  void track_index(atomic_long& alg, long i) {
    long ar = alg.load(memory_order_seq_cst);
    while (ar <= i && !alg.compare_exchange_weak(ar, i + 1))
      ;
  }

  void help_deq(Handle* th, Handle* ph) {
    DeqRequest* deq = &ph->Dr;
    long idx = deq->idx.load(memory_order_seq_cst);
    long id = deq->id.load(memory_order_seq_cst);

    if (idx < id) return;

    th->hzd_node_id.store(ph->hzd_node_id.load(memory_order_seq_cst), memory_order_seq_cst);
    Node* Dp = ph->Dp.load(memory_order_seq_cst);

    atomic_thread_fence(memory_order_seq_cst);

    idx = deq->idx.load(memory_order_seq_cst);
    long i = id + 1, old = id, newV = 0;
    Node* h;
    Cell* c;
    T* v = nullptr;

    if (idx != old)
      goto probe;

    while (1) {
      h = Dp;
      for (;;) {
        c = find_cell(&h, i, th);
        v = help_enq(th, c, i);

        if (v == nullptr) {
          track_index(di, i);
          newV = i++;
        real_null:
          if (deq->idx.compare_exchange_strong(idx, -newV))
            idx = -newV;
          goto probe;
        }

        if (v != static_cast<T*>((void*)-1) && c->deq.load(memory_order_seq_cst) == nullptr) {
          newV = i++;
        real_data:
          if (deq->idx.compare_exchange_strong(idx, newV))
            idx = newV;
          goto probe;
        }

        track_index(di, i++);
        idx = deq->idx.load(memory_order_seq_cst);
        if (idx != old)
          goto probe;
      }
    probe:
      if (idx < 0 || deq->id.load(memory_order_seq_cst) != id) return;

      track_index(di, idx);
      c = find_cell(&Dp, idx, th);
      DeqRequest* cd = nullptr;

      if (c->deq.compare_exchange_strong(cd, deq)) {
        deq->idx.compare_exchange_strong(idx, -idx);
        return;
      }
      if (cd == deq) {
        deq->idx.compare_exchange_strong(idx, -idx);
        return;
      }

      if (idx < newV) {
        if (v == nullptr)
          goto real_null;
        else
          goto real_data;
      }

      old = idx;
      if (idx >= i)
        i = idx + 1;
    }
  }

  T* deq_fast(Handle* th, long* id) {
    long i = di.fetch_add(1, memory_order_seq_cst);

    Node* tmp = th->Dp.load(memory_order_seq_cst);
    Cell* c = find_cell(&tmp, i, th);
    th->Dp.store(tmp, memory_order_seq_cst);

    T* v = help_enq(th, c, i);
    DeqRequest* old = nullptr;

    if (v == nullptr)
      return nullptr;
    if (v != static_cast<T*>((void*)-1) && c->deq.compare_exchange_strong(old, static_cast<DeqRequest*>((void*)-1))) {
      //deq_fast_num.fetch_add(1);
      return v;
    }

    *id = i;
    return static_cast<T*>((void*)-1);
  }

  T* deq_slow(Handle* th, long id) {
    DeqRequest* const deq = &th->Dr;
    deq->id.store(id, memory_order_seq_cst);
    deq->idx.store(id, memory_order_seq_cst);

    help_deq(th, th);
    long i = -deq->idx.load(memory_order_seq_cst);

    Node* tmp = th->Dp.load(memory_order_seq_cst);
    Cell* c = find_cell(&tmp, i, th);
    th->Dp.store(tmp, memory_order_seq_cst);

    T* val = c->val.load(memory_order_seq_cst);

    return val == static_cast<T*>((void*)-1) ? nullptr : val;
  }

  Node* check(atomic<unsigned long>& p_hzd_node_id, Node* cur, Node* old) {
    unsigned long hzd_node_id = p_hzd_node_id.load(memory_order_seq_cst);

    if (hzd_node_id < (unsigned long)cur->id) {
      Node* tmp = old;
      while ((unsigned long)tmp->id < hzd_node_id) {
        tmp = tmp->next;
      }
      cur = tmp;
    }

    return cur;
  }

  Node* update(atomic<Node*>& pPn, Node* cur, atomic<unsigned long>& p_hzd_node_id, Node* old) {
    Node* ptr = pPn.load(memory_order_seq_cst);

    if (ptr->id < cur->id) {
      if (!pPn.compare_exchange_strong(ptr, cur)) {
        if (ptr->id < cur->id) cur = ptr;
      }
      cur = check(p_hzd_node_id, cur, old);
    }

    return cur;
  }

  unsigned long cleanup(Handle* th, long id) {
    long newId = th->deq_node_id;
    long oid = hi.load(memory_order_seq_cst);
    if (newId - oid < MAX_SURROUND)
      return 0;

    long cleanId = clean_id.load(memory_order_seq_cst);
    if (cleanId > 0) {
      if (id != 0 && cleanId != id)
        return 0;
      return cleanId;
    }

    if ((id != 0 && cleanId != (-id)) || !clean_id.compare_exchange_strong(cleanId, 1 - cleanId))
      return 0;

    Node* newN = th->Dp;
    oid = hi.load(memory_order_seq_cst);
    track_index(ei, di.load(memory_order_seq_cst));

    Node* old;
    {
      unique_lock<mutex> control(safe_for_reclaim);
      old = this->hp;
      Handle* ph = th;
      if (nprocess > nhandle) {
        operator delete[](phs);
        // phs = static_cast<Handle**> (malloc(nprocess * sizeof(Handle*)));
        phs = static_cast<Handle**> (operator new[](nprocess * sizeof(Handle*)));
        nhandle = nprocess;
      }
      int i = 0;

      do {
        // cout << "theadId: " << this_thread::get_id() <<  " hzd_node_id: " << (long) ph->hzd_node_id.load(memory_order_seq_cst) << endl;
        newN = check(ph->hzd_node_id, newN, old);
        newN = update(ph->Ep, newN, ph->hzd_node_id, old);
        newN = update(ph->Dp, newN, ph->hzd_node_id, old);
        assert(i < nprocess);
        phs[i++] = ph;
        ph = ph->next;
      } while (newN->id > oid&& ph != th);

      while (newN->id > oid&& --i >= 0) {
        newN = check(phs[i]->hzd_node_id, newN, old);
      }

      long nid = newN->id;

      if (nid <= oid) {
        hi.store(oid, memory_order_seq_cst);
        clean_id.store(cleanId - 1, memory_order_seq_cst);
        return 0;
      }
      hp = newN;
      hi.store(nid, memory_order_seq_cst);
      clean_id.store(cleanId - 1, memory_order_seq_cst);
    }

    while (old != newN) {
      Node* tmp = old->next.load(memory_order_seq_cst);
      delete old;
      old = tmp;
    }
    return 0;
  }

  T* spin(atomic<T*>& val) {
    int p = SPIN_THREHOLD;

    T* value = val.load(memory_order_seq_cst);
    while (value == nullptr && p-- > 0) {
      this_thread::yield();
      value = val.load(memory_order_seq_cst);
    }
    return value;
  }

  EnqRequest* spin_util_val(atomic<EnqRequest*>& p) {
    EnqRequest* enq = p.load(memory_order_seq_cst);
    while (!enq) {
      this_thread::yield();
      enq = p.load(memory_order_seq_cst);
    }

    return enq;
  }

  mutex safe_for_reclaim;
  atomic_long ei;
  atomic_long di;
  atomic<unsigned long> hi;
  atomic<long> clean_id;
  Node* hp;
  atomic<int> nprocess;
  int nhandle;
  Handle** phs;
  Handle* tail;
  Handle* leave_nodes;
  const int SPIN_THREHOLD;
  const int ENQ_FAST_THREHOLD;
  const int DEQ_FAST_THREHOLD;
  const int MAX_SURROUND;
  const int MAX_CELLS;

public:
  ScalableConQueue<T>(int spin = 16, int push_fast = 32, int pop_fast = 16, int bytes = (1 << 15), int bytes_thread = (1 << 12)) : ei(1), di(1), hi(0), clean_id(0),
    hp(new Node()), nprocess(0), nhandle(0), phs(nullptr), tail(nullptr), leave_nodes(nullptr),
    SPIN_THREHOLD(spin), ENQ_FAST_THREHOLD(push_fast), DEQ_FAST_THREHOLD(pop_fast), MAX_SURROUND((bytes + sizeof(Node) - 1) / sizeof(Node))
    , MAX_CELLS((bytes_thread + sizeof(Cell) - 1) / sizeof(Cell)) {
    cout << MAX_SURROUND << endl;
  }
  ~ScalableConQueue<T>() {
    // reclaim nodes
    while (nullptr != hp) {
      Node* tmp = hp->next;
      delete hp;
      hp = tmp;
    }

    // no need for handle is allocates in get_handle by thread_local.
    // reclaim handles

    assert(tail == nullptr && nprocess == 0);
    while (leave_nodes != nullptr) {
      Handle* tmp = leave_nodes->prev;
      delete leave_nodes->spare;
      delete leave_nodes;
      leave_nodes = tmp;
    };

    // reclaim phs
    operator delete[](phs);
  }

  void push(const T& val) {
    Handle* th = get_handle();
    th->hzd_node_id.store(th->enq_node_id, memory_order_seq_cst);

    long id;
    int p = ENQ_FAST_THREHOLD;
    T* const newV = new T(val);
    while (!enq_fast(newV, th, id) && p-- > 0);
    if (p < 0)
      enq_slow(newV, th, id);

    th->enq_node_id = th->Ep.load(memory_order_seq_cst)->id;
    th->hzd_node_id.store(-1, memory_order_seq_cst);
  }

  unique_ptr<T> pop() {
    Handle* th = get_handle();
    th->hzd_node_id.store(th->deq_node_id, memory_order_seq_cst);

    T* v;
    long id = 0;
    int p = DEQ_FAST_THREHOLD;

    do {
      v = deq_fast(th, &id);
    } while (v == static_cast<T*>((void*)-1) && p-- > 0);
    if (v == static_cast<T*>((void*)-1)) {
      v = deq_slow(th, id);
    }

    if (v != nullptr) {
      ++th->spanning_cells;
      help_deq(th, th->deq_helped);
      th->deq_helped = th->deq_helped->next;
    }

    th->deq_node_id = th->Dp.load(memory_order_seq_cst)->id;
    th->hzd_node_id.store(-1, memory_order_seq_cst);

    if (th->cleanId > 0) {
      th->cleanId = cleanup(th, th->cleanId);
    } // cleanId == 0
    else {
      if (th->spare == nullptr) {
        th->cleanId = cleanup(th, 0);
        th->spare = new Node();
      }
      else if (th->spanning_cells >= MAX_CELLS) {
        th->spanning_cells = 0;
        th->cleanId = cleanup(th, 0);
      }
    }

    return unique_ptr<T>(v);
  }
};
