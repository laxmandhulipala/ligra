#ifndef BYTECODE_H
#define BYTECODE_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "parallel.h"

#define MAX_EDGE_SIZE_IN_BYTES 10
#define LAST_BIT_SET(b) (b & (0x80))
#define EDGE_SIZE_PER_BYTE 7

intE eatEdge(char* start, uintE *curOffset) {
  int bytesEaten = 0;
  intE edgeRead = 0;
  int shiftAmount = 0;

  while (bytesEaten < MAX_EDGE_SIZE_IN_BYTES) {
    char b = start[*curOffset];
    edgeRead += ((b & 0x7f) << shiftAmount);
    (*curOffset)++; 
    bytesEaten++;
    if (LAST_BIT_SET(b))
      shiftAmount += EDGE_SIZE_PER_BYTE;
    else 
      break;
  } 
  return edgeRead;
}

struct dummyT {
  bool srcTarg(intE src, intE target, intT edgeNumber) {
    return true;
  }
};

template <class T>
void decode(T t, char* edgeStart, intE source, uintT degree) {
  int edgesRead = 0;
  uintE curOffset = 0;
  if (degree > 0) {
    // Eat first edge, which is uncompressed. 
    intE startEdge = eatEdge(edgeStart, &curOffset);
    if (!t.srcTarg(source,startEdge,edgesRead)) {
      return;
    }
    intE prevEdge = startEdge;
    for (int edgesRead = 0; edgesRead < degree; edgesRead++) {
      // Eat the next 'edge', which is a difference, and reconstruct edge.
      intE edgeDiff = eatEdge(edgeStart, &curOffset);
      intE edge = prevEdge + edgeDiff;
      if (!t.srcTarg(source, edge, edgesRead)) {
        break; 
      }
    }
  }
}


// Should provide the difference between this edge and the previous edge
uintE compressEdge(char *start, uintE curOffset, intE e) {
  int shift = 0;
  char curByte = e & 0x7f;
  int bytesUsed = 0;
  while ((curByte > 0) || ((e >> shift) > 0)) {
    bytesUsed++;
    char toWrite = curByte;
    shift += 7;
    // Check to see if there's any bits left to represent
    curByte = (e >> shift) & 0x7f;
    if ((e >> shift) > 0) {
      toWrite |= 0x80; 
    }
    start[curOffset] = toWrite;
    curOffset++;
  }
  return curOffset;
}

/*
   Sequentially compress the edge list. Edges should 
   already be sorted 
*/
void compressEdgeSet(intE source, intE *edgeStart, uintT degree) {
  intE t = 0;
  if (degree <= 0) {
    return;
  }
  intE *savedEdges = newA(intE, degree);
  {parallel_for(long i=0; i < degree; i++) savedEdges[i] = edgeStart[i];}

  uintE currentOffset = 0;
  for (uintT i=0; i < degree; i++) {
    currentOffset = compressEdge((char *)edgeStart, currentOffset, savedEdges[i]);
  }
  free(savedEdges);
}

/*
  Compresses the edge set in parallel. Does not modify offsets - empty space can now
  be left between the end of adjacency lists. 
*/
void parallelCompressEdges(intE *edges, intT *offsets, long n, long m) {
  {parallel_for(intE i=0; i<n; i++) {
    intE *edgesStart = edges + offsets[i];
    uintT degree = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    compressEdgeSet(i, edgesStart, degree);
  }}
}


/*
  Performs a sequential compression. This guarantees the adjacencies for vertex 
  k and k+1 are actually contiguous in memory even after compression. 
*/
void sequentialCompressEdges(intE *edges, intT *offsets, long n, long m) {
  // Create copies of edges and offsets
  intT *oldOffsets = newA(intT, n);
  intE *savedEdges = newA(intE, m);
  {parallel_for(long i=0; i < m; i++) savedEdges[i] = edges[i];}
  {parallel_for(intT i=0; i < n; i++) oldOffsets[i] = offsets[i];}

  // Offset into edges where the next compressed edge should be written
  uintE currentOffset = 0; 
  uintE nWritten = 0;
  offsets[0] = 0; 
  for (intE i=0; i<n; i++) {
    intE *edgesStart = edges + oldOffsets[i];
    uintT degree = ((i == n-1) ? m : oldOffsets[i+1])-oldOffsets[i];

    // The start of vertex i's edge list is from currentOffset
    offsets[i] = currentOffset;
    // Compress each edge sequentially
    if (degree > 0) {
      // Compress the first edge whole (no difference coding on first edge yet)
      currentOffset = compressEdge((char *)edges, currentOffset, savedEdges[nWritten]);
      for (uintT edgeI=1; edgeI < degree; edgeI++) {
        // Store difference between cur and prev edge. 
        intE difference = savedEdges[nWritten + edgeI] - savedEdges[nWritten + edgeI - 1];
        currentOffset = compressEdge((char *)edges, currentOffset, difference);
      }
      // Increment nWritten after all of vertex n's neighbors are written
      nWritten += degree;
    }
  }
}

#endif
