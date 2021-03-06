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

#include "driver.h"
#include "builder.h"
#include "cc/cfgs.h"
#include "cc/stimulus.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;
#include <fstream>

namespace cc {

class SocConfigBuilderJson {
#define CHECK(__name)                                                   \
  MACRO_BEGIN                                                           \
  if (!j.contains(#__name)) {                                           \
    BuilderException ex("Required argument not found: " #__name);       \
    ex.set_line(__LINE__);                                              \
    ex.set_file(__FILE__);                                              \
    throw ex;                                                           \
  }                                                                     \
  MACRO_END
  
  
#define CHECK_AND_SET(__name)                                           \
  MACRO_BEGIN                                                           \
  if (j.contains(#__name)) {                                            \
    c.__name = j[#__name];                                              \
  } else  {                                                             \
    BuilderException ex("Required argument not found: " #__name);       \
    ex.set_line(__LINE__);                                              \
    ex.set_file(__FILE__);                                              \
    throw ex;                                                           \
  }                                                                     \
  MACRO_END

#define THROW_EX(__desc)                        \
  MACRO_BEGIN                                   \
  BuilderException ex(__desc);                  \
  ex.set_line(__LINE__);                        \
  ex.set_file(__FILE__);                        \
  throw ex;                                     \
  MACRO_END
  

#define CHECK_AND_SET_OPTIONAL(__name)                  \
  if (j.contains(#__name)) c.__name = j[#__name]

 public:
  SocConfigBuilderJson(std::istream& is) : is_(is) {
    is >> jtop_;
  }

  void build(SocConfig& soc) {
    build(soc, jtop_);
    post(soc);
  }

 private:
  void build(CacheModelConfig& c, json j) {
    // Set .sets_n
    CHECK_AND_SET_OPTIONAL(sets_n);
    // Set .ways_n
    CHECK_AND_SET_OPTIONAL(ways_n);
    // Set .line_bytes_n
    CHECK_AND_SET_OPTIONAL(line_bytes_n);
  }

  void build(CpuConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
  }

  void build(L1CacheAgentConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
    // Set .epoch
    CHECK_AND_SET_OPTIONAL(epoch);
    // Set .cpu_l1__cmd_n
    CHECK_AND_SET_OPTIONAL(cpu_l1__cmd_n);
    // Set .l2_l1__rsp_n
    CHECK_AND_SET_OPTIONAL(l2_l1__rsp_n);
    // Set .tt_entries_n
    CHECK_AND_SET_OPTIONAL(tt_entries_n);
    // Set .cconfig
    CHECK(cconfig);
    build(c.cconfig, j["cconfig"]);
    
  }

  void build(L2CacheAgentConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
    // Set .epoch
    CHECK_AND_SET_OPTIONAL(epoch);
    // Set .cconfig
    CHECK(cconfig);
    build(c.cconfig, j["cconfig"]);
  }

  void build(NocModelConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
    // Set .ingress_q_n
    CHECK_AND_SET_OPTIONAL(ingress_q_n);
    // TODO: edges
  }

  void build(LLCAgentConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
    // Set .epoch
    CHECK_AND_SET_OPTIONAL(epoch);
    // Set .cmd_queue_n
    CHECK_AND_SET_OPTIONAL(cmd_queue_n);
    // Set .rsp_queue_n
    CHECK_AND_SET_OPTIONAL(rsp_queue_n);
  }

  void build(MemModelConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
    // Set .epoch
    CHECK_AND_SET_OPTIONAL(epoch);
  }

  void build(DirAgentConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
    // Set .epoch
    CHECK_AND_SET_OPTIONAL(epoch);
    // Set .cmd_queue_n
    CHECK_AND_SET_OPTIONAL(cmd_queue_n);
    // Set .rsp_queue_n
    CHECK_AND_SET_OPTIONAL(rsp_queue_n);
    // Set .is_null_filter
    CHECK_AND_SET_OPTIONAL(is_null_filter);
    // Set .cconfig
    CHECK(cconfig);
    build(c.cconfig, j["cconfig"]);
    // Set .llcconfig
    CHECK(llcconfig);
    build(c.llcconfig, j["llcconfig"]);
  }

  void build(CCAgentConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
  }

  void build(CpuClusterConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
    // Set .cc_config
    CHECK(cc_config);
    build(c.cc_config, j["cc_config"]);
    // Set .l2c_config
    CHECK(l2c_config);
    build(c.l2c_config, j["l2c_config"]);
    // Set .l1c_config
    CHECK(l1c_config);
    for (const auto& item : j["l1c_config"]) {
      L1CacheAgentConfig cmc;
      build(cmc, item);
      c.l1c_configs.push_back(cmc);
    }
    if (c.l1c_configs.empty()) {
      THROW_EX("No L1 are defined");
    }
    // Set .cpu_configs
    CHECK(cpu_configs);
    for (const auto& item : j["cpu_configs"]) {
      CpuConfig cc;
      build(cc, item);
      c.cpu_configs.push_back(cc);
    }
    if (c.cpu_configs.empty()) {
      THROW_EX("No CPU are defined");
    }
    if (c.cpu_configs.size() != c.l1c_configs.size()) {
      THROW_EX("CPU count does not equal L1 count.");
    }
  }

  void build(StimulusConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
    // Set .type
    // Type is presently an enum; convert from string.
    //CHECK_AND_SET(type);
    // Parse type related options
    if (c.type == StimulusType::Trace) {
      // Set .filename
      CHECK(filename);
      const std::string filename = j["filename"];
      c.is = new std::ifstream(filename);
    } else {
      std::string reason = "Unknown/Invalid stimulus type: ";
      reason += to_string(c.type);
      throw BuilderException(reason);
    }
  }
  
  void build(SocConfig& c, json j) {
    // Set .name
    CHECK_AND_SET(name);
    // Set .protocol
    CHECK(protocol);
    // Set .enable_verif
    CHECK_AND_SET(enable_verif);
    // Set .enable_stats
    CHECK_AND_SET(enable_stats);
    // Construct protocol definition.
    const std::string protocol = jtop_["protocol"];
    pb_ = construct_protocol_builder(protocol);
    if (pb_ == nullptr) {
      const std::string msg = "Invalid protocol: " + protocol;
      throw BuilderException(msg);
    }
    // Set .ccls (CpuClusterConfig)
    CHECK(ccls);
    for (const auto& item : j["ccls"]) {
      CpuClusterConfig ccc;
      build(ccc, item);
      c.ccls.push_back(ccc);
    }
    if (c.ccls.empty()) {
      throw BuilderException("No CPU clusters configured.");
    }
    // Set .dcfgs (DirAgentConfig)
    CHECK(dcfgs);
    for (const auto& item : j["dcfgs"]) {
      DirAgentConfig dmc;
      build(dmc, item);
      c.dcfgs.push_back(dmc);
    }
    if (c.dcfgs.empty()) {
      throw BuilderException("No directories configured.");
    }
    // Set .mcfgs (MemModelConfig)
    CHECK(mcfgs);
    for (const auto& item : j["mcfgs"]) {
      MemModelConfig mmc;
      build(mmc, item);
      c.mcfgs.push_back(mmc);
    }
    if (c.mcfgs.empty()) {
      throw BuilderException("No Memories configured.");
    }
    // Set .scfg (StimulusConfig)
    CHECK(scfg);
    build(c.scfg, j["scfg"]);
    // Set .noccfg (NocModelConfig)
    CHECK(noccfg);
    build(c.noccfg, j["noccfg"]);
  }

  void post(SocConfig& cfg) {
    for (CpuClusterConfig& c : cfg.ccls) {
      post(c);
    }
    for (DirAgentConfig& c : cfg.dcfgs) {
      post(c);
    }
    cfg.pbuilder = pb_;
  }

  void post(CpuClusterConfig& cfg) {
    post(cfg.cc_config);
    post(cfg.l2c_config);
    for (L1CacheAgentConfig& l1c : cfg.l1c_configs) {
      post(l1c);
    }
  }

  void post(CCAgentConfig& cfg) {
    cfg.pbuilder = pb_;
  }

  void post(L1CacheAgentConfig& cfg) {
    cfg.pbuilder = pb_;
  }

  void post(L2CacheAgentConfig& cfg) {
    cfg.pbuilder = pb_;
  }

  void post(DirAgentConfig& c) {
    c.pbuilder = pb_;
  }
  //
  ProtocolBuilder* pb_ = nullptr;
  // Input configuration stream.
  std::istream& is_;
  // Top json object.
  json jtop_;

#undef CHECK_AND_SET_OPTIONAL
#undef CHECK_AND_SET
#undef CHECK  
};

void build_soc_config(std::istream& is, SocConfig& cfg) {
  SocConfigBuilderJson builder(is);
  builder.build(cfg);
}

} // namespace cc
