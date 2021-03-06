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
  * The graph vertex abstraction class, build as template class so that one can
  * choose the data structure to keep the data at each vertex dynamically.
  *
  * Author: Philip Levis <pal@cs.stanford.edu>
  * Author: Omid Mashayekhi <omidm@stanford.edu>
  */

#ifndef NIMBUS_SRC_SHARED_VERTEX_H_
#define NIMBUS_SRC_SHARED_VERTEX_H_

#include <boost/unordered_map.hpp>
#include <sstream> // NOLINT
#include <iostream> // NOLINT
#include <string>
#include <vector>
#include <map>
#include <set>
#include "src/shared/nimbus_types.h"
#include "src/shared/edge.h"



namespace nimbus {

template<typename T, typename key_t>
class Graph;

template<typename T, typename key_t>
class Edge;

template<typename T, typename key_t>
class Vertex {
  friend class Graph<T, key_t>;

  public:
    typedef typename boost::unordered_map<key_t, Vertex<T, key_t>*> Map;
    typedef typename boost::unordered_map<key_t, Vertex<T, key_t>*>::iterator Iter;
    typedef typename boost::unordered_map<key_t, Vertex<T, key_t>*>::const_iterator ConstIter;

    explicit Vertex(key_t id, T* entry);
    Vertex(const Vertex<T, key_t>& other);
    virtual ~Vertex();

    virtual typename Edge<T, key_t>::Map* outgoing_edges();
    virtual typename Edge<T, key_t>::Map* incoming_edges();
    virtual key_t id();
    virtual T* entry();

    Vertex<T, key_t>& operator=(const Vertex<T, key_t>& other);

  protected:
    virtual void AddOutgoingEdge(Edge<T, key_t>* e);

    virtual void AddIncomingEdge(Edge<T, key_t>* e);

    virtual bool HasOutgoingEdge(Edge<T, key_t>* e);

    virtual bool HasOutgoingEdge(Edge<T, key_t>* e,
                                 typename Edge<T, key_t>::Iter *iter);

    virtual bool HasIncomingEdge(Edge<T, key_t>* e);

    virtual bool HasIncomingEdge(Edge<T, key_t>* e,
                                 typename Edge<T, key_t>::Iter *iter);

    virtual bool HasOutgoingEdgeTo(Vertex<T, key_t>* end);

    virtual bool HasOutgoingEdgeTo(Vertex<T, key_t>* end,
                                   typename Edge<T, key_t>::Iter *iter);

    virtual bool HasIncomingEdgeFrom(Vertex<T, key_t>* start);

    virtual bool HasIncomingEdgeFrom(Vertex<T, key_t>* start,
                                     typename Edge<T, key_t>::Iter *iter);

    virtual void RemoveOutgoingEdge(Edge<T, key_t>* e);

    virtual void RemoveIncomingEdge(Edge<T, key_t>* e);

  private:
    key_t id_;
    T* entry_;
    typename Edge<T, key_t>::Map outgoing_edges_;
    typename Edge<T, key_t>::Map incoming_edges_;
};



template<typename T, typename key_t>
Vertex<T, key_t>::Vertex(key_t id, T* entry)
  : id_(id), entry_(entry) {
}

template<typename T, typename key_t>
Vertex<T, key_t>::Vertex(const Vertex<T, key_t>& other) {
  id_ = other.id_;
  entry_ = other.entry_;
  outgoing_edges_ = other.outgoing_edges_;
  incoming_edges_ = other.incoming_edges_;
}

template<typename T, typename key_t>
Vertex<T, key_t>::~Vertex() {
}


template<typename T, typename key_t>
typename Edge<T, key_t>::Map* Vertex<T, key_t>::outgoing_edges() {
  return &outgoing_edges_;
}

template<typename T, typename key_t>
typename Edge<T, key_t>::Map* Vertex<T, key_t>::incoming_edges() {
  return &incoming_edges_;
}

template<typename T, typename key_t>
key_t Vertex<T, key_t>::id() {
  return id_;
}

template<typename T, typename key_t>
T* Vertex<T, key_t>::entry() {
  return entry_;
}

template<typename T, typename key_t>
Vertex<T, key_t>& Vertex<T, key_t>::operator=(const Vertex<T, key_t>& other) {
  id_ = other.id_;
  entry_ = other.entry_;
  outgoing_edges_ = other.outgoing_edges_;
  incoming_edges_ = other.incoming_edges_;
  return *this;
}


template<typename T, typename key_t>
void Vertex<T, key_t>::AddOutgoingEdge(Edge<T, key_t>* e) {
  outgoing_edges_[e->end_vertex()->id()] = e;
}


template<typename T, typename key_t>
void Vertex<T, key_t>::AddIncomingEdge(Edge<T, key_t>* e) {
  incoming_edges_[e->start_vertex()->id()] = e;
}

template<typename T, typename key_t>
bool Vertex<T, key_t>::HasOutgoingEdge(Edge<T, key_t>* e) {
  return (outgoing_edges_.find(e->end_vertex()->id()) != outgoing_edges_.end());
}

template<typename T, typename key_t>
bool Vertex<T, key_t>::HasOutgoingEdge(Edge<T, key_t>* e,
                                       typename Edge<T, key_t>::Iter *iter) {
  *iter = outgoing_edges_.find(e->end_vertex()->id());
  return ((*iter) != outgoing_edges_.end());
}

template<typename T, typename key_t>
bool Vertex<T, key_t>::HasIncomingEdge(Edge<T, key_t>* e) {
  return (incoming_edges_.find(e->start_vertex()->id()) != incoming_edges_.end());
}

template<typename T, typename key_t>
bool Vertex<T, key_t>::HasIncomingEdge(Edge<T, key_t>* e,
                                       typename Edge<T, key_t>::Iter *iter) {
  *iter = incoming_edges_.find(e->start_vertex()->id());
  return ((*iter) != incoming_edges_.end());
}

template<typename T, typename key_t>
bool Vertex<T, key_t>::HasOutgoingEdgeTo(Vertex<T, key_t>* end) {
  return (outgoing_edges_.find(end->id()) != outgoing_edges_.end());
}

template<typename T, typename key_t>
bool Vertex<T, key_t>::HasOutgoingEdgeTo(Vertex<T, key_t>* end,
                                         typename Edge<T, key_t>::Iter *iter) {
  *iter = outgoing_edges_.find(end->id());
  return ((*iter) != outgoing_edges_.end());
}

template<typename T, typename key_t>
bool Vertex<T, key_t>::HasIncomingEdgeFrom(Vertex<T, key_t>* start) {
  return (incoming_edges_.find(start->id()) != incoming_edges_.end());
}

template<typename T, typename key_t>
bool Vertex<T, key_t>::HasIncomingEdgeFrom(Vertex<T, key_t>* start,
                                           typename Edge<T, key_t>::Iter *iter) {
  *iter = incoming_edges_.find(start->id());
  return ((*iter) != incoming_edges_.end());
}

template<typename T, typename key_t>
void Vertex<T, key_t>::RemoveOutgoingEdge(Edge<T, key_t>* e) {
  outgoing_edges_.erase(e->end_vertex()->id());
}

template<typename T, typename key_t>
void Vertex<T, key_t>::RemoveIncomingEdge(Edge<T, key_t>* e) {
  incoming_edges_.erase(e->start_vertex()->id());
}

}  // namespace nimbus

#endif  // NIMBUS_SRC_SHARED_VERTEX_H_
