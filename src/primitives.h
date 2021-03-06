//========================================================================== //
// Copyright (c) 2020, Stephen Henry
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//========================================================================== //

#ifndef CC_SRC_PRIMITIVES_H
#define CC_SRC_PRIMITIVES_H

#include <vector>

#include "cc/kernel.h"
#include "cc/types.h"

namespace cc {

class Message;

// Class type used to model a generic clock source; a periodic and
// deterministic tick from which to initiate other actions.
//
class Clock : public kernel::Module {
 public:
  Clock(kernel::Kernel* k, const std::string& name, int ticks, int period = 10);
  ~Clock();

  int ticks() const { return ticks_; }
  int period() const { return period_; }

  kernel::Event* rising_edge_event() { return rising_edge_event_; }
  const kernel::Event* rising_edge_event() const { return rising_edge_event_; }

 private:
  kernel::Event* rising_edge_event_;
  kernel::Process* p_;
  int ticks_, period_;
};

// Class type used to model the behavior of a queue type data
// structure with events corresponding to enqueue and dequeue events
// where necessary.
//
template <typename T>
class Queue : public kernel::Module {
 public:
  Queue(kernel::Kernel* k, const std::string& name, std::size_t n)
      : kernel::Module(k, name) {
    ts_.resize(n);
    reset_state();

    enqueue_event_ = new kernel::Event(k, "enqueue_event");
    dequeue_event_ = new kernel::Event(k, "dequeue_event");
    non_empty_event_ = new kernel::Event(k, "non_empty_event");
    non_full_event_ = new kernel::Event(k, "non_full_event");
  }
  ~Queue() {
    delete enqueue_event_;
    delete dequeue_event_;
    delete non_empty_event_;
    delete non_full_event_;
  }

  // The capacity of the queue.
  std::size_t n() const { return ts_.size(); }
  // The number of free entries in the queue.
  std::size_t free() const { return n() - size(); }
  // The occupancy of the queue.
  std::size_t size() const { return size_; }
  // Flag denoting full status of the queue.
  bool full() const { return full_; }
  // Flag denoting empty status of the queue.
  bool empty() const { return empty_; }
  // kernel::Event notified on the enqueue of an entry into the queue.
  kernel::Event* enqueue_event() { return enqueue_event_; }
  // kernel::Event notified on the dequeue of an entry into the queue.
  kernel::Event* dequeue_event() { return dequeue_event_; }
  // kernel::Event notified on the transition to non-empty state.
  kernel::Event* non_empty_event() { return non_empty_event_; }
  // kernel::Event notified on the transition out of the full state.
  kernel::Event* non_full_event() { return non_full_event_; }

  // Enqueue entry into queue; returns true on success.
  bool enqueue(const T& t) {
    if (full()) return false;

    ts_[wr_ptr_] = t;
    if (++wr_ptr_ == n()) wr_ptr_ = 0;

    // If was empty, not empty after an enqueue therefore notify,
    // awaitees waiting for the queue become non-empty.
    if (empty()) non_empty_event_->notify();

    empty_ = false;
    full_ = (++size_ == n());

    enqueue_event_->notify();
    return true;
  }

  bool peek(T& t) const {
    if (empty()) return false;

    t = ts_[rd_ptr_];
    return true;
  }

  // Dequeue entry from queue; returns false on success.
  bool dequeue(T& t) {
    if (empty()) return false;

    t = ts_[rd_ptr_];

    // If was full, not full after dequeue therefore notify non-full
    // event to indicate transition away from full state.
    if (full()) non_full_event_->notify();

    if (++rd_ptr_ == n()) rd_ptr_ = 0;
    empty_ = (--size_ == 0);
    full_ = false;

    dequeue_event_->notify();
    return true;
  }

  // Resize queue to 'n'.
  void resize(std::size_t n) {
    ts_.resize(n);
    reset_state();
  }

 private:
  void reset_state() {
    empty_ = true;
    full_ = false;
    wr_ptr_ = 0;
    rd_ptr_ = 0;
    size_ = 0;
  }

  // Occupancy flags.
  bool full_ = false, empty_ = true;
  // Write and read pointer
  std::size_t wr_ptr_ = 0, rd_ptr_ = 0;
  // Total number of entries in the queue; some integer smaller than
  // or equal to the total capacity of the queue.
  std::size_t size_;
  // Underlying queue state.
  std::vector<T> ts_;

  kernel::Event* enqueue_event_ = nullptr;
  kernel::Event* dequeue_event_ = nullptr;
  kernel::Event* non_empty_event_ = nullptr;
  kernel::Event* non_full_event_ = nullptr;
};

//
//
template <typename T>
class Arbiter : public kernel::Module {
 public:
  // Helper class which encapsulates the concept of a single
  // arbitration round.
  class Tournament {
    friend class Arbiter;
    Tournament(Arbiter* parent) : parent_(parent) { execute(); }

   public:
    Tournament() : parent_(nullptr) {}

    // Return the winning requester interface.
    T* winner() const { return winner_; }
    bool has_requester() const { return winner_ != nullptr; }
    bool deadlock() const { return deadlock_; }

    // Advance arbitration state to the next index if prior
    // arbitration has succeeded.
    void advance() const {
      if (winner_ != nullptr) {
        parent_->idx_ = (idx_ + 1) % parent_->n();
      }
    }

   private:
    void execute() {
      std::size_t requesters = 0;
      winner_ = nullptr;
      for (std::size_t i = 0; i < parent_->n(); i++) {
        // Compute index of next requester interface in roundrobin order.
        idx_ = (parent_->idx_ + i) % parent_->n();
        T* cur = parent_->ts_[idx_];

        if (!cur->has_req()) continue;
        // Current agent is requesting, proceed.

        requesters++;
        if (!cur->blocked()) {
          // Current agent is requesting and is not blocked by some
          // protocol condition.
          winner_ = cur;
          return;
        }
      }
      // A deadlock has occurred iff there are pending work items in the
      // child queues, but all of these queues are currently blocked
      // awaiting the completion of some other action.
      deadlock_ = (requesters == parent_->n());
    }
    bool deadlock_ = false;
    T* winner_ = nullptr;
    Arbiter* parent_ = nullptr;
    std::size_t idx_ = 0;
  };

  Arbiter(kernel::Kernel* k, const std::string& name)
      : kernel::Module(k, name) {
    build();
  }
  virtual ~Arbiter() { delete request_arrival_event_; }

  // The number of requesting agents.
  std::size_t n() const { return ts_.size(); }
  // Event denoting rising edge to the ready to grant state.
  kernel::Event* request_arrival_event() { return request_arrival_event_; }
  // Initiate an arbitration tournament.
  Tournament tournament() {
    const Tournament t = Tournament(this);
    if (t.deadlock()) {
      const LogMessage msg("A protocol deadlock has been detected.",
                           Level::Fatal);
      log(msg);
    }
    return t;
  }
  // Add a requester to the current arbiter (Build-/Elaboration-Phases only).
  void add_requester(T* t) { ts_.push_back(t); }

 private:
  void build() {
    request_arrival_event_ = new kernel::EventOr(k(), "request_arrival_event");
    add_child(request_arrival_event_);
  }

  bool elab() override {
    if (!ts_.empty()) {
      // Construct EventOr denoting the event which is notified when the
      // arbiter goes from having no requestors to having non-zero
      // requestors.
      for (T* t : ts_) {
        request_arrival_event_->add_child_event(t->non_empty_event());
      }
      request_arrival_event_->finalize();
    } else {
      const LogMessage msg{"Arbiter has no associated requestors.",
                           Level::Error};
      log(msg);
    }
    return false;
  }

  void drc() override {}

  //
  kernel::EventOr* request_arrival_event_ = nullptr;
  // Current arbitration index.
  std::size_t idx_ = 0;
  //
  std::vector<T*> ts_;
};

//
//
template <typename K, typename V>
class Table : public kernel::Module {
  using table_type = std::map<K, V>;

 public:
  using iterator = typename table_type::iterator;
  using const_iterator = typename table_type::const_iterator;

  Table(kernel::Kernel* k, const std::string& name, std::size_t n)
      : Module(k, name), n_(n) {
    non_full_event_ = new kernel::Event(k, "non_full_event");
  }

  virtual ~Table() { delete non_full_event_; }

  kernel::Event* non_full_event() const { return non_full_event_; }

  // Accessors:
  std::size_t n() const { return n_; }
  std::size_t size() const { return m_.size(); }

  bool full() const { return size() == n(); }

  // Flag denoting whether current table instance has at least 'i'
  // free entries.
  bool has_at_least(std::size_t i) { return n() - size() >= i; }

  iterator begin() { return m_.begin(); }
  const_iterator begin() const { return m_.begin(); }

  iterator end() { return m_.end(); }
  const_iterator end() const { return m_.end(); }

  iterator find(K k) { return m_.find(k); }
  const iterator find(K k) const { return m_.find(k); }

  //
  virtual void install(K k, V v) { m_.insert_or_assign(k, v); }

  //
  virtual void remove(K k) {
    const bool was_full = full();
    if (auto it = m_.find(k); it != m_.end()) {
      m_.erase(it);
      if (was_full) {
        non_full_event_->notify();
      }
    }
  }

 private:
  // Table size.
  std::size_t n_;
  // Table state.
  table_type m_;
  //
  kernel::Event* non_full_event_;
};

}  // namespace cc

#endif
