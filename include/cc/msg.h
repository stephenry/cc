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

#ifndef CC_INCLUDE_CC_MSG_H
#define CC_INCLUDE_CC_MSG_H

#include "cc/types.h"
#include <string>

namespace cc {

// Forwards:
namespace kernel {

template<typename> class Agent;
template<typename> class RequesterIntf;
template<typename> class EndPointIntf;
}
  
//
//
class Transaction {
 public:
  Transaction() {}

  std::string to_string_short() const { return "Some transaction"; }
  std::string to_string() const { return "Some transaction."; };
};

// clang-format off
#define MESSAGE_CLASSES(__func)			\
  __func(Invalid)				\
  __func(CpuL1__CmdMsg)                         \
  __func(L1Cpu__RspMsg)                         \
  __func(L1L2__CmdMsg)                          \
  __func(L2CC__AceCmd)                          \
  __func(CCL2__AceSnoop)                        \
  __func(Noc)
// clang-format on

enum class MessageClass {
#define __declare_enum(__name) __name,
  MESSAGE_CLASSES(__declare_enum)
#undef __declare_enum
};

//
//
class Message {
 public:

  Message(Transaction* t, MessageClass cls) : t_(t), cls_(cls) {}
  virtual ~Message() = default;

  Transaction* t() const { return t_; }
  MessageClass cls() const { return cls_; }
  kernel::Agent<const Message*>* agent() const { return origin_; }

  std::string to_string_short() const { return "Some message"; }
  std::string to_string() const { return "Some message"; }

  void set_origin(kernel::Agent<const Message*>* origin) { origin_ = origin; }
  void set_t(Transaction* t) { t_ = t; }
  void set_cls(MessageClass cls) { cls_ = cls; }

  // Release message; return to owning message pool or destruct where
  // applicable.
  virtual void release() const { delete this; }

 private:
  // Parent transaction;
  Transaction* t_;
  // Message type
  MessageClass cls_;
  // Originating agent.
  kernel::Agent<const Message*>* origin_;
};


using MsgRequesterIntf = kernel::RequesterIntf<const Message*>;

using MsgEpIntf = kernel::EndPointIntf<const Message*>;



} // namespace cc

#endif