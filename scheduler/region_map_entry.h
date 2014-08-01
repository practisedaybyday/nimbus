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
  * Each element in the region map which defines an unstructured region.
  *
  * Author: Omid Mashayekhi <omidm@stanford.edu>
  */

#ifndef NIMBUS_SCHEDULER_REGION_MAP_ENTRY_H_
#define NIMBUS_SCHEDULER_REGION_MAP_ENTRY_H_

#include <boost/unordered_map.hpp>
#include <set>
#include <vector>
#include <utility>
#include "shared/nimbus_types.h"
#include "shared/geometric_region.h"

namespace nimbus {

  class RegionMapEntry {
  public:
    typedef std::set<GeometricRegion> RegionSet;
    typedef RegionSet::iterator RegionSetIter;

    RegionMapEntry();

    RegionMapEntry(const RegionMapEntry& other);

    virtual ~RegionMapEntry();

    RegionMapEntry& operator= (const RegionMapEntry& right);

    void AddRegion(const GeometricRegion &region);

    void RemoveRegion(const GeometricRegion &region);

    double Intersect(const GeometricRegion &region);

  private:
    RegionSet region_set_;

    static void RemoveIntersect(const GeometricRegion *original,
                                const GeometricRegion *remove,
                                std::vector<GeometricRegion> *result,
                                bool append = false);
  };

}  // namespace nimbus

#endif  // NIMBUS_SCHEDULER_REGION_MAP_ENTRY_H_