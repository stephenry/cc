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

#ifndef CC_INCLUDE_CC_SIM_H
#define CC_INCLUDE_CC_SIM_H

#include "kernel.h"
#include "primitives.h"
#include <vector>
#include <deque>
#include <set>
#include <string>

namespace cc {

//
//
class Transaction {
 public:
  Transaction() {}

  std::string to_string() const {
    return "Some transaction.";
  };
};


#define MESSAGE_CLASSES(__func)                 \
  __func(Invalid)                               \
  __func(Cpu)

//
//
class Message {
 public:
#define __declare_enum(__t) __t,
  enum Cls {
    MESSAGE_CLASSES(__declare_enum)
  };
#undef __declare_enum
  
  Message(Transaction* t, Cls cls) : t_(t), cls_(cls) {}
  virtual ~Message() = default;

  Transaction* t() const { return t_; }
  Cls cls() const { return cls_; }
  std::string to_string() const {
    return "Some message";
  }
  
 private:
  // Parent transaction;
  Transaction* t_;
  // Message type
  Cls cls_;
};


//
//
class CpuMessage : public Message {
 public:
  enum Opcode { Load, Store };
  
  CpuMessage(Transaction* t) : Message(t, Cpu) {}

  Opcode opcode() const { return opcode_; }
  addr_t addr() const { return addr_; }

  void set_addr(addr_t addr) { addr_ = addr; }
  void set_opcode(Opcode opcode) { opcode_ = opcode; }
 private:
  addr_t addr_;
  Opcode opcode_;
};


//
//
class MessageQueue : public kernel::Module, public RequesterIntf<const Message*> {
 public:
  MessageQueue(kernel::Kernel* k, const std::string& name, std::size_t n);

  // Queue depth.
  std::size_t n() const { return q_->n(); }
  
  // Requester Interface:
  bool has_req() const override;
  const Message* peek() const override;
  const Message* dequeue() override;
  kernel::Event& request_arrival_event() override;
 private:
  // Construct module
  void build(std::size_t n);
  // Queue primitive.
  Queue<const Message*>* q_ = nullptr;
};


//
//
class ProcessorModel : public kernel::Module, public RequesterIntf<const Message*> {
  class MainProcess;
 public:
  ProcessorModel(kernel::Kernel* k, const std::string& name);

  // Set stimulus object for processor.
  void set_stimulus(Stimulus* stim) { stim_ = stim; }
  
  // Requester Interface:
  bool has_req() const override;
  const Message* peek() const override;
  const Message* dequeue() override;
  kernel::Event& request_arrival_event() override;
 private:
  void build();
  void elab() override;
  void drc() override;
  void construct_new_message() const;
  //
  kernel::Event mature_event_;
  //
  Stimulus* stim_ = nullptr;
  //
  MainProcess* main_ = nullptr;
  // Set of outstanding transactions.
  std::set<Transaction*> ts_;
  // Current head-of-queue message to be emitted.
  mutable const Message* msg_ = nullptr;
};


// Elementary realization of a transaction source. Transactions are
// programmatically constructed and issued to the source before the
// start of the simulation. Upon exhaustion of the transactions
// the source remained exhausted for the duration of the simulation.
//
class ProgrammaticStimulus : public Stimulus {
 public:
  ProgrammaticStimulus() {}

  bool done() const override { return true; }
  Frontier& front() override { return cs_.front(); }
  const Frontier& front() const override { return cs_.front(); }

  void consume() override { cs_.pop_front(); }
  void push_back(kernel::Time t, const Command& c) { cs_.push_back({t, c}); }
 private:
  std::deque<Frontier> cs_;
};

} // namespace cc

#endif
