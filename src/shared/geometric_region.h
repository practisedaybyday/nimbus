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
  * A geometric region within a Nimbus simulation. Defined by integers in
  * x,y,z and dx, dy, dz.
  */

#ifndef NIMBUS_SRC_SHARED_GEOMETRIC_REGION_H_
#define NIMBUS_SRC_SHARED_GEOMETRIC_REGION_H_

#include <boost/tokenizer.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/unordered_map.hpp>
#include <stdio.h>
#include <stdlib.h>
#include <sstream> // NOLINT
#include <iostream> // NOLINT
#include <vector>
#include <string>
#include <algorithm>
#include "src/shared/nimbus_types.h"
#include "src/shared/protobuf_compiled/commands.pb.h"

namespace nimbus {

  struct Coord {
    int_dimension_t x;
    int_dimension_t y;
    int_dimension_t z;
    Coord();
    Coord(int_dimension_t xe, int_dimension_t ye, int_dimension_t ze);
    Coord& operator= (const Coord& right) {
      x = right.x;
      y = right.y;
      z = right.z;
      return *this;
    };
    inline bool operator== (const Coord& right) const {
      return (x == right.x &&
              y == right.y &&
              z == right.z);
    };
  };

  Coord ElementWiseMin(Coord a, Coord b);
  Coord ElementWiseMax(Coord a, Coord b);

  class GeometricRegion {
  public:
    GeometricRegion();
    GeometricRegion(int_dimension_t x,
                    int_dimension_t y,
                    int_dimension_t z,
                    int_dimension_t dx,
                    int_dimension_t dy,
                    int_dimension_t dz);

    GeometricRegion(const GeometricRegion& r);

    // Argument is a pointer to an array of 6 int_dimension_t
    explicit GeometricRegion(const int_dimension_t* values);
    // Transform a protobuf GeometricRegionPBuf into a GeometricRegion
    explicit GeometricRegion(const GeometricRegionPBuf* msg);
    // Read in as a serialized GeometricRegionPBuf from an istream
    explicit GeometricRegion(std::istream* is);
    // Read in as a serialized GeometricRegionPBuf from a string
    explicit GeometricRegion(const std::string& data);
    // Geometric region from coordinates (min corner and delta)
    explicit GeometricRegion(const Coord &min, const Coord &delta);
    // Geometric region from max and min coordinates
    static GeometricRegion GeometricRegionFromRange(const Coord &min,
                                                    const Coord &max);

    virtual ~GeometricRegion() {}

    void Rebuild(int_dimension_t x,
                 int_dimension_t y,
                 int_dimension_t z,
                 int_dimension_t dx,
                 int_dimension_t dy,
                 int_dimension_t dz);

    inline int_dimension_t x() const { return x_; }
    inline int_dimension_t y() const { return y_; }
    inline int_dimension_t z() const { return z_; }
    inline int_dimension_t dx() const { return dx_; }
    inline int_dimension_t dy() const { return dy_; }
    inline int_dimension_t dz() const { return dz_; }

    virtual void FillInValues(const int_dimension_t* values);
    virtual void FillInValues(const GeometricRegionPBuf* msg);
    virtual void FillInMessage(GeometricRegionPBuf* msg);

    Coord MinCorner() const;
    Coord MaxCorner() const;
    Coord Delta() const;

    bool NoneZeroArea() const;
    int_dimension_t GetSurfaceArea() const;

    void Union(const GeometricRegion& region);

    /* Increase the size of geometric region along each dimension and side. */
    void Enlarge(const int_dimension_t delta);
    void Enlarge(const Coord &delta);
    GeometricRegion NewEnlarged(const int_dimension_t delta) const;
    GeometricRegion NewEnlarged(const Coord &delta) const;

    /* Translate a geometric region in space. */
    void Translate(const int_dimension_t delta);
    void Translate(const Coord &delta);

    /* Covers returns whether this region covers (encompasses) the argument. */
    virtual bool Covers(const GeometricRegion* region) const;
    virtual bool Intersects(const GeometricRegion* region) const;
    virtual bool Adjacent(const GeometricRegion* region) const;
    virtual bool AdjacentOrIntersects(const GeometricRegion* region) const;
    virtual bool IsEqual(const GeometricRegion* region) const;

    /* Largest common rectangular region shared by 2 regions. */
    static GeometricRegion GetIntersection(const GeometricRegion &region1,
                                           const GeometricRegion &region2);

    static int_dimension_t GetDistance(const GeometricRegion *region1,
                                        const GeometricRegion *region2);

    /* Smallest rectangular region (bounding box) that contains the two
     * regions. */
    static GeometricRegion GetBoundingBox(const GeometricRegion &region1,
                                   const GeometricRegion &region2);


    virtual std::string ToNetworkData() const;

    virtual bool Parse(const std::string& input);

    GeometricRegion& operator= (const GeometricRegion& right);

    inline bool operator== (const GeometricRegion& right) const {
        return (x_  == right.x()  &&
                y_  == right.y()  &&
                z_  == right.z()  &&
                dx_ == right.dx() &&
                dy_ == right.dy() &&
                dz_ == right.dz());
    }

    inline bool operator!= (const GeometricRegion& right) const {
        return !(*this == right);
    }

    inline bool operator< (const GeometricRegion& right) const {
      if (this->x() != right.x())
          return this->x() < right.x();
      if (this->y() != right.y())
          return this->y() < right.y();
      if (this->z() != right.z())
          return this->z() < right.z();
      if (this->dx() != right.dx())
          return this->dx() < right.dx();
      if (this->dy() != right.dy())
          return this->dy() < right.dy();
      if (this->dz() != right.dz())
          return this->dz() < right.dz();
      return false;
    }


  private:
    int_dimension_t x_;
    int_dimension_t y_;
    int_dimension_t z_;
    int_dimension_t dx_;
    int_dimension_t dy_;
    int_dimension_t dz_;
  };

  typedef boost::unordered_map<partition_id_t, GeometricRegion> PartitionMap;

}  // namespace nimbus

#endif  // NIMBUS_SRC_SHARED_GEOMETRIC_REGION_H_
