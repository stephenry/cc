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

#include "protocol.h"

#include "ccntrl.h"
#include "dir.h"
#include "l1cache.h"
#include "l2cache.h"
#include "noc.h"
#include "sim.h"
#include "utility.h"

namespace cc {

CohSrtMsg::CohSrtMsg() : Message(MessageClass::CohSrt) {}

std::string CohSrtMsg::to_string() const {
  KVListRenderer r;
  render_msg_fields(r);
  return r.to_string();
}

CohEndMsg::CohEndMsg() : Message(MessageClass::CohEnd) {}

std::string CohEndMsg::to_string() const {
  using cc::to_string;
  using std::to_string;
  KVListRenderer r;
  render_msg_fields(r);
  r.add_field("is", to_string(is()));
  r.add_field("pd", to_string(pd()));
  r.add_field("dt_n", to_string(dt_n()));
  return r.to_string();
}

CohCmdMsg::CohCmdMsg() : Message(MessageClass::CohCmd) {}

std::string CohCmdMsg::to_string() const {
  using cc::to_string;
  Hexer h;
  KVListRenderer r;
  render_msg_fields(r);
  r.add_field("opcode", to_string(opcode()));
  r.add_field("addr", h.to_hex(addr()));
  r.add_field("origin", origin()->path());
  return r.to_string();
}

CohCmdRspMsg::CohCmdRspMsg() : Message(MessageClass::CohCmdRsp) {}

std::string CohCmdRspMsg::to_string() const {
  KVListRenderer r;
  render_msg_fields(r);
  return r.to_string();
}

CohSnpMsg::CohSnpMsg() : Message(MessageClass::CohSnp) {}

std::string CohSnpMsg::to_string() const {
  KVListRenderer r;
  render_msg_fields(r);
  r.add_field("opcode", cc::to_string(opcode()));
  r.add_field("agent", agent() ? agent()->path() : "null");
  Hexer h;
  r.add_field("addr", h.to_hex(addr()));
  return r.to_string();
}

CohSnpRspMsg::CohSnpRspMsg() : Message(MessageClass::CohSnpRsp) {}

std::string CohSnpRspMsg::to_string() const {
  KVListRenderer r;
  render_msg_fields(r);
  r.add_field("dt", cc::to_string(dt()));
  r.add_field("pd", cc::to_string(pd()));
  r.add_field("is", cc::to_string(is()));
  r.add_field("wu", cc::to_string(wu()));
  return r.to_string();
}

using pbr = ProtocolBuilderRegistry;

// Coherence protocol registry
std::map<std::string, pbr::ProtocolBuilderFactory*> pbr::m_;

ProtocolBuilder* ProtocolBuilderRegistry::build(const std::string& name) {
  ProtocolBuilder* r = nullptr;
  auto it = m_.find(name);
  if (it != m_.end()) {
    ProtocolBuilderFactory* factory = it->second;
    r = factory->construct();
  }
  return r;
}

//
//
struct EmitMessageActionProxy : public CoherenceAction {
  EmitMessageActionProxy(MessageQueue* mq, const Message* msg)
      : mq_(mq), msg_(msg) {}
  std::string to_string() const override {
    KVListRenderer r;
    r.add_field("action", "emit message");
    r.add_field("mq", mq_->path());
    r.add_field("msg", msg_->to_string());
    return r.to_string();
  }
  bool execute() override { return mq_->issue(msg_); }

 private:
  MessageQueue* mq_ = nullptr;
  const Message* msg_ = nullptr;
};

//
//
struct EmitMessageActionProxyCC : public CCCoherenceAction {
  EmitMessageActionProxyCC(MessageQueue* mq, const Message* msg)
      : mq_(mq), msg_(msg) {}
  std::string to_string() const override {
    KVListRenderer r;
    r.add_field("action", "emit message");
    r.add_field("mq", mq_->path());
    r.add_field("msg", msg_->to_string());
    return r.to_string();
  }
  bool execute() override { return mq_->issue(msg_); }

 private:
  MessageQueue* mq_ = nullptr;
  const Message* msg_ = nullptr;
};

L1CacheAgentProtocol::L1CacheAgentProtocol(kernel::Kernel* k,
                                           const std::string& name)
    : Module(k, name) {}

L2CacheAgentProtocol::L2CacheAgentProtocol(kernel::Kernel* k,
                                           const std::string& name)
    : Module(k, name) {}

CCProtocol::CCProtocol(kernel::Kernel* k, const std::string& name)
    : Module(k, name) {}

DirProtocol::DirProtocol(kernel::Kernel* k, const std::string& name)
    : Module(k, name) {}

}  // namespace cc
