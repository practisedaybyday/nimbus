/*
 * Copyright 2013 Stanford University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 *
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 *
 * - Neither the name of the copyright holders nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

 /*
  * Author: Hang Qu <quhang@stanford.edu>
  */

#ifndef NIMBUS_SRC_WORKER_LDO_INDEX_CACHE_H_
#define NIMBUS_SRC_WORKER_LDO_INDEX_CACHE_H_

#include <map>
#include <string>
#include "src/shared/geometric_region.h"
#include "src/shared/logical_data_object.h"

namespace nimbus {

class Application;

class LdoIndexCache {
 public:
  LdoIndexCache() {
    application_ = NULL;
  }
  void Initialize(Application* application, const std::string& variable) {
    application_ = application;
    variable_ = variable;
  }
  ~LdoIndexCache() {}
  void GetResult(const GeometricRegion& region,
                 nimbus::IDSet<logical_data_id_t>* result);
 private:
  typedef std::map<GeometricRegion, nimbus::IDSet<logical_data_id_t> > MapType;
  MapType content_;
  Application* application_;
  std::string variable_;
};

}  // namespace nimbus

#endif  // NIMBUS_SRC_WORKER_LDO_INDEX_CACHE_H_
