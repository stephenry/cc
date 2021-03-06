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

#ifndef CC_SRC_PROTOCOL_H
#define CC_SRC_PROTOCOL_H

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "amba.h"
#include "cc/types.h"
#include "msg.h"

namespace cc {

// Message Forwards:
class L1CommandList;
class L1CacheContext;

class L2CommandList;
class L2CacheContext;

class CCCommandList;
class CCContext;

class CCSnpCommandList;
class CCSnpContext;

class DirCommandList;
class DirContext;

class MessageQueue;
class Agent;

//
//
class CohSrtMsg : public Message {
  template <typename>
  friend class PooledItem;

  CohSrtMsg();
 public:

  //
  std::string to_string() const override;

  //
  addr_t addr() const { return addr_; }

  //
  void set_addr(addr_t addr) { addr_ = addr; }

 private:
  addr_t addr_;
};

//
//
class CohEndMsg : public Message {
  template <typename>
  friend class PooledItem;

  CohEndMsg();
 public:
  //
  std::string to_string() const override;

  bool is() const { return is_; }
  bool pd() const { return pd_; }
  std::size_t dt_n() const { return dt_n_; }

  void set_is(bool is) { is_ = is; }
  void set_pd(bool pd) { pd_ = pd; }
  void set_dt_n(std::size_t dt_n) { dt_n_ = dt_n; }

 private:
  // Is Shared
  bool is_ = false;
  // Pass Dirty
  bool pd_ = false;
  // Data Transfer count (where applicable).
  std::size_t dt_n_ = 0;
};

//
//
class CohCmdMsg : public Message {
  template <typename>
  friend class PooledItem;

  CohCmdMsg();
 public:

  //
  std::string to_string() const override;

  //
  AceCmdOpcode opcode() const { return opcode_; }
  Agent* agent() const { return agent_; }
  addr_t addr() const { return addr_; }

  //
  void set_opcode(AceCmdOpcode opcode) { opcode_ = opcode; }
  void set_agent(Agent* agent) { agent_ = agent; }
  void set_addr(addr_t addr) { addr_ = addr; }

 private:
  AceCmdOpcode opcode_;
  Agent* agent_ = nullptr;
  addr_t addr_;
};

//
//
class CohCmdRspMsg : public Message {
  template <typename>
  friend class PooledItem;

  CohCmdRspMsg();
 public:

  //
  std::string to_string() const override;
};

//
//
class CohSnpMsg : public Message {
  template <typename>
  friend class PooledItem;

  CohSnpMsg();
 public:
  // Human readable version of message.
  std::string to_string() const override;

  // Current snoop opcode
  AceSnpOpcode opcode() const { return opcode_; }

  // Current destination agent
  Agent* agent() const { return agent_; }

  // Current line address
  addr_t addr() const { return addr_; }


  // Set snoop opcode
  void set_opcode(AceSnpOpcode opcode) { opcode_ = opcode; }

  // Set agent to which intervention should be passed. Ifs set no
  // nullptr (no agent defined), message denotes that line should be
  // written back to LLC if presently dirty.
  void set_agent(Agent* agent) { agent_ = agent; }

  // Cache line address
  void set_addr(addr_t addr) { addr_ = addr; }

 private:
  // Command opcode.
  AceSnpOpcode opcode_ = AceSnpOpcode::Invalid;

  // Agent to which intervention occurs
  Agent* agent_ = nullptr;

  // Line address
  addr_t addr_ = 0;
};

//
//
class CohSnpRspMsg : public Message {
  template <typename>
  friend class PooledItem;

  CohSnpRspMsg();
 public:
  //
  std::string to_string() const override;

  //
  bool dt() const { return dt_; }
  bool pd() const { return pd_; }
  bool is() const { return is_; }
  bool wu() const { return wu_; }

  //
  void set_dt(bool dt) { dt_ = dt; }
  void set_pd(bool pd) { pd_ = pd; }
  void set_is(bool is) { is_ = is; }
  void set_wu(bool wu) { wu_ = wu; }

 private:
  // Data Transfer
  bool dt_ = false;
  // Pass Dirty
  bool pd_ = false;
  // Is Shared
  bool is_ = false;
  // Was Unique
  bool wu_ = false;
};

//
//
class L1LineState {
 public:
  L1LineState() {}
  virtual ~L1LineState() = default;

  // Release line back to pool, or destruct.
  virtual void release() { delete this; }

  // Flag indiciating if the line is currently residing in a stable
  // state.
  virtual bool is_stable() const = 0;

  // Flag indicating that the line is currently residing in a readable
  // state.
  //
  virtual bool is_readable() const = 0;

  // Flag indicating that the line is currently residing in a writeable
  // state.
  //
  virtual bool is_writeable() const = 0;

  // Flag indiciating if the line is currently evictable (not in a
  // transient state).
  virtual bool is_evictable() const { return is_stable(); }
};

//
enum class L1UpdateStatus { CanCommit, IsBlocked };

//
//
class L1CacheAgentProtocol : public kernel::Module {
 public:
  L1CacheAgentProtocol(kernel::Kernel* k, const std::string& name);
  virtual ~L1CacheAgentProtocol() = default;

  //
  //
  virtual L1LineState* construct_line() const = 0;

  //
  //
  virtual void apply(L1CacheContext& c, L1CommandList& cl) const = 0;

  //
  //
  virtual void evict(L1CacheContext& c, L1CommandList& cl) const = 0;

  //
  //
  virtual void set_line_shared_or_invalid(L1CacheContext& c, L1CommandList& cl,
                                          bool shared) const = 0;
};

//
//
class CoherenceAction {
 public:
  virtual std::string to_string() const = 0;

  // Invoke/Execute coherence action
  virtual bool execute() = 0;

  virtual void release() { delete this; }

 protected:
  virtual ~CoherenceAction() = default;
};

//
//
class L2LineState {
 public:
  L2LineState() {}
  virtual ~L2LineState() = default;

  // Flag indiciating if the line is currently residing in a stable
  // state.
  virtual bool is_stable() const = 0;

  // Flag indiciating if the line is currently evictable (not in a
  // transient state).
  virtual bool is_evictable() const { return is_stable(); }
};

//
//
class L2CacheAgentProtocol : public kernel::Module {
 public:
  L2CacheAgentProtocol(kernel::Kernel* k, const std::string& name);
  virtual ~L2CacheAgentProtocol() = default;

  //
  //
  virtual L2LineState* construct_line() const = 0;

  //
  //
  virtual void apply(L2CacheContext& ctxt, L2CommandList& cl) const = 0;

  //
  //
  virtual void evict(L2CacheContext& ctxt, L2CommandList& cl) const = 0;

  //
  //
  virtual void set_modified_status(L2CacheContext& ctxt,
                                   L2CommandList& cl) const = 0;
};

//
//
class DirLineState {
 public:
  DirLineState() {}
  virtual void release() { delete this; }

  // Flag indiciating if the line is currently residing in a stable
  // state.
  virtual bool is_stable() const = 0;

  // Flag indiciating if the line is currently evictable (not in a
  // transient state).
  virtual bool is_evictable() const { return is_stable(); }

 protected:
  virtual ~DirLineState() = default;
};

using DirActionList = std::vector<CoherenceAction*>;

//
//
class DirCoherenceContext {
 public:
  DirCoherenceContext() = default;

  DirLineState* line() const { return line_; }
  const Message* msg() const { return msg_; }

  void set_line(DirLineState* line) { line_ = line; }
  void set_msg(const Message* msg) { msg_ = msg; }

 private:
  DirLineState* line_ = nullptr;
  const Message* msg_ = nullptr;
};

//
//
class DirProtocol : public kernel::Module {
 public:
  DirProtocol(kernel::Kernel* k, const std::string& name);
  virtual ~DirProtocol() = default;

  //
  //
  virtual DirLineState* construct_line() const = 0;

  //
  //
  virtual void apply(DirContext& ctxt, DirCommandList& cl) const = 0;

  //
  //
  virtual void recall(DirContext& ctxt, DirCommandList& cl) const = 0;
};

//
//
class CCLineState {
 public:
  CCLineState() = default;
  virtual void release() const { delete this; }

 protected:
  virtual ~CCLineState() = default;
};

//
//
class CCSnpLineState {
 public:
  CCSnpLineState() = default;
  virtual void release() const { delete this; }

 protected:
  virtual ~CCSnpLineState() = default;
};

enum class CCMessageID {};

using CCMessageIDList = std::vector<CCMessageID>;

using CCActionList = std::vector<CoherenceAction*>;

//
//
class CCProtocol : public kernel::Module {
 public:
  CCProtocol(kernel::Kernel* k, const std::string& name);
  virtual ~CCProtocol() = default;

  //
  //
  virtual CCLineState* construct_line() const = 0;

  //
  //
  virtual CCSnpLineState* construct_snp_line() const = 0;

  //
  //
  virtual void apply(CCContext& ctxt, CCCommandList& cl) const = 0;

  //
  //
  virtual bool is_complete(CCContext& ctxt, CCCommandList& cl) const = 0;

  //
  //
  virtual void apply(CCSnpContext& ctxt, CCSnpCommandList& cl) const = 0;
};

//
//
class ProtocolBuilder {
 public:
  virtual ~ProtocolBuilder() = default;

  // Create an instance of the L1 protocol
  virtual L1CacheAgentProtocol* create_l1(kernel::Kernel*) = 0;

  // Create an instance of the L2 protocol
  virtual L2CacheAgentProtocol* create_l2(kernel::Kernel*) = 0;

  // Create an instance of the Dir protocol
  virtual DirProtocol* create_dir(kernel::Kernel*) = 0;

  // Create an instance of a Cache Controller protocol.
  virtual CCProtocol* create_cc(kernel::Kernel*) = 0;
};

//
//
class ProtocolBuilderRegistry {
 public:
  static ProtocolBuilder* build(const std::string& name);

 protected:
  struct ProtocolBuilderFactory {
    virtual ~ProtocolBuilderFactory() = default;
    virtual ProtocolBuilder* construct() = 0;
  };
  void register_protocol(const std::string& name, ProtocolBuilderFactory* f) {
    m_[name] = f;
  }

 private:
  static std::map<std::string, ProtocolBuilderFactory*> m_;
};

//
//
#define CC_DECLARE_PROTOCOL_BUILDER(__name, __builder)          \
  static struct Register##__builder : ProtocolBuilderRegistry { \
    Register##__builder() {                                     \
      factory_ = new __Builder##Factory{};                      \
      register_protocol(__name, factory_);                      \
    }                                                           \
    ~Register##__builder() { delete factory_; }                 \
                                                                \
   private:                                                     \
    struct __Builder##Factory : ProtocolBuilderFactory {        \
      ProtocolBuilder* construct() { return new __builder{}; }  \
    };                                                          \
    ProtocolBuilderFactory* factory_;                           \
  } __register

}  // namespace cc

#endif
