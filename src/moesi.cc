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

#include "moesi_cc.h"
#include "moesi_dir.h"
#include "moesi_l1.h"
#include "moesi_l2.h"
#include "protocol.h"

namespace cc {

// Forwards:
namespace kernel {
class Kernel;
}

class MOESIProtocolBuilder : public ProtocolBuilder {
 public:
  // Create an instance of the L1 protocol
  L1CacheAgentProtocol* create_l1(kernel::Kernel* k) override {
    return moesi::build_l1_protocol(k);
  }

  // Create an instance of the L2 protocol
  L2CacheAgentProtocol* create_l2(kernel::Kernel* k) override {
    return moesi::build_l2_protocol(k);
  }

  // Create and instance of the Directory protocol
  DirProtocol* create_dir(kernel::Kernel* k) override {
    return moesi::build_dir_protocol(k);
  }

  CCProtocol* create_cc(kernel::Kernel* k) override {
    return moesi::build_cc_protocol(k);
  }
};

CC_DECLARE_PROTOCOL_BUILDER("moesi", MOESIProtocolBuilder);

}  // namespace cc
