#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "bytecode.h"
#include "parallel.h"

using namespace std;

// **************************************************************
//    ADJACENCY ARRAY REPRESENTATION
// **************************************************************

struct symmetricVertex {
  intE* neighbors;
//  intT degree;
  void del() {free(neighbors); }
symmetricVertex(intE* n, intT d) : neighbors(n) {}
  intE* getInNeighbors() { return neighbors; }
  intE* getOutNeighbors() { return neighbors; }
  uintE getInNeighbor(intT j) { return neighbors[j]; } 
  uintE getOutNeighbor(intT j) { return neighbors[j]; } 
//  intT getInDegree() { return degree; }
  intT getInDegree() {
    return getCompressedDegree(neighbors);
  }
  intT getOutDegree() {
     return getCompressedDegree(neighbors);
   }
  void setInNeighbors(intE* _i) { neighbors = _i; }
  void setOutNeighbors(intE* _i) { neighbors = _i; }
  void setInDegree(intT _d) { 
//    degree = _d; 
  }
  void setOutDegree(intT _d) { 
//    degree = _d; 
  }
  void flipEdges() {}
};

struct asymmetricVertex {
  intE* inNeighbors;
  intE* outNeighbors;
//  intT outDegree;
//  intT inDegree;
  void del() {free(inNeighbors); free(outNeighbors);}
asymmetricVertex(intE* iN, intE* oN, intT id, intT od) : inNeighbors(iN), outNeighbors(oN) {}
  intE* getInNeighbors() { return inNeighbors; }
  intE* getOutNeighbors() { return outNeighbors; }
  uintE getInNeighbor(intT j) { return inNeighbors[j]; }
  uintE getOutNeighbor(intT j) { return outNeighbors[j]; }
//  intT getInDegree() { return inDegree; }
//  intT getOutDegree() { return outDegree; }
  intT getInDegree() {
    return getCompressedDegree(inNeighbors);
  }
  intT getOutDegree() {
     return getCompressedDegree(outNeighbors);
   }
  void setInNeighbors(intE* _i) { inNeighbors = _i; }
  void setOutNeighbors(intE* _i) { outNeighbors = _i; }
  void setInDegree(intT _d) { 
   // inDegree = _d; 
    }
  void setOutDegree(intT _d) { 
   // outDegree = _d; 
    }
  void flipEdges() { swap(inNeighbors,outNeighbors); 
//  swap(inDegree,outDegree); 
  }
};

template <class vertex>
struct graph {
  vertex *V;
  intT n;
  uintT m;
  intE* allocatedInplace;
  intE* inEdges;
  intT* flags;
  graph(vertex* VV, intT nn, uintT mm) 
  : V(VV), n(nn), m(mm), allocatedInplace(NULL), flags(NULL) {}
graph(vertex* VV, intT nn, uintT mm, intE* ai, intE* _inEdges = NULL) 
: V(VV), n(nn), m(mm), allocatedInplace(ai), inEdges(_inEdges), flags(NULL) {}
  void del() {
    if (flags != NULL) free(flags);
    if (allocatedInplace == NULL) 
      for (intT i=0; i < n; i++) V[i].del();
    else free(allocatedInplace);
    free(V);
    if(inEdges != NULL) free(inEdges);
  }
  void transpose() {
    if(sizeof(vertex) == sizeof(asymmetricVertex)) {
      parallel_for(intT i=0;i<n;i++) {
	V[i].flipEdges();
      }
    } 
  }
};

struct symmetricWghVertex {
  //weights are stored in the entry after the neighbor ID
  //so size of neighbor list is twice the degree
  intE* neighbors; 
  intT degree;
  void del() {free(neighbors);}
symmetricWghVertex(intE* n, intT d) : neighbors(n), degree(d) {}
  intE getInNeighbor(intT j) { return neighbors[2*j]; }
  intE getOutNeighbor(intT j) { return neighbors[2*j]; }
  intE getInWeight(intT j) { return neighbors[2*j+1]; }
  intE getOutWeight(intT j) { return neighbors[2*j+1]; }
  intT getInDegree() { return degree; }
  intT getOutDegree() { return degree; }
  void setInNeighbors(intE* _i) { neighbors = _i; }
  void setOutNeighbors(intE* _i) { neighbors = _i; }
  void setInDegree(intT _d) { degree = _d; }
  void setOutDegree(intT _d) { degree = _d; }
};

struct asymmetricWghVertex {
  //weights are stored in the entry after the neighbor ID
  //so size of neighbor list is twice the degree
  intE* inNeighbors; 
  intE* outNeighbors;
  intT inDegree;
  intT outDegree;
  void del() {free(inNeighbors); free(outNeighbors);}
asymmetricWghVertex(intE* iN, intE* oN, intT id, intT od) : inNeighbors(iN), outNeighbors(oN), inDegree(id), outDegree(od) {}
  intE getInNeighbor(intT j) { return inNeighbors[2*j]; }
  intE getOutNeighbor(intT j) { return outNeighbors[2*j]; }
  intE getInWeight(intT j) { return inNeighbors[2*j+1]; }
  intE getOutWeight(intT j) { return outNeighbors[2*j+1]; }
  intT getInDegree() { return inDegree; }
  intT getOutDegree() { return outDegree; }
  void setInNeighbors(intE* _i) { inNeighbors = _i; }
  void setOutNeighbors(intE* _i) { outNeighbors = _i; }
  void setInDegree(intT _d) { inDegree = _d; }
  void setOutDegree(intT _d) { outDegree = _d; }
};

template <class vertex>
struct wghGraph {
  vertex *V;
  intT n;
  uintT m;
  intE* allocatedInplace;
  intE* inEdges;
  intT* flags;
wghGraph(vertex* VV, intT nn, uintT mm) 
: V(VV), n(nn), m(mm), allocatedInplace(NULL), flags(NULL) {}
wghGraph(vertex* VV, intT nn, uintT mm, intE* ai, intE* _inEdges=NULL) 
: V(VV), n(nn), m(mm), allocatedInplace(ai), inEdges(_inEdges), flags(NULL) {}
  void del() {
    if(flags != NULL) free(flags);
    if (allocatedInplace == NULL) 
      for (intT i=0; i < n; i++) V[i].del();
    else { free(allocatedInplace); }
    free(V);
    if(inEdges != NULL) free(inEdges);
  }
};

#endif
