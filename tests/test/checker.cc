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

#include "test/checker.h"
#include "src/l1cache.h"
#include "src/cache.h"
#include "src/protocol.h"

namespace test {

L1Checker::L1Checker(const cc::L1CacheAgent* agent) : agent_(agent) {}

bool L1Checker::is_hit(cc::addr_t addr) const {
  const cc::CacheModel<cc::L1LineState*>* cache = agent_->cache();
  return cache->hit(addr);
}

bool L1Checker::is_readable(cc::addr_t addr) const {
  const cc::CacheModel<cc::L1LineState*>* cache = agent_->cache();
  const cc::CacheAddressHelper& ah = cache->ah();
  const auto set = cache->set(ah.set(addr));

  bool ret = false;
  if (auto it = set.find(ah.tag(addr)); it != set.end()) {
    const cc::L1LineState* line = it->t();
    ret = line->is_readable();
  }
  return ret;
}

bool L1Checker::is_writeable(cc::addr_t addr) const {
  const cc::CacheModel<cc::L1LineState*>* cache = agent_->cache();
  const cc::CacheAddressHelper& ah = cache->ah();
  const auto set = cache->set(ah.set(addr));

  bool ret = false;
  if (auto it = set.find(ah.tag(addr)); it != set.end()) {
    const cc::L1LineState* line = it->t();
    ret = line->is_writeable();
  }
  return ret;
}

DirChecker::DirChecker(const cc::DirAgent* agent) {
}

bool DirChecker::is_sharer(const cc::Agent* agent) const {
  return true;
}

bool DirChecker::is_owner(const cc::Agent* agent) const {
  return true;
}

} // namespace test
