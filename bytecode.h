#ifndef BYTECODE_H
#define BYTECODE_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include "parallel.h"

#define MAX_EDGE_SIZE_IN_BYTES 10
#define LAST_BIT_SET(b) (b & (0x80))
#define EDGE_SIZE_PER_BYTE 7

/* Reads the first edge of an out-edge list, which is the signed
   difference between the target and source. 
*/
intE eatFirstEdge(char *start, uintE *curOffset, uintE source) {
  intE edgeRead = 0;
  int bytesEaten = 0;
  int shiftAmount = 0;

  int sign = 1;

  char fb = start[*curOffset];
  if ((fb & 0x40) == 0x40) {
    sign = -1;
  }
  edgeRead += (fb & 0x3f);
  if (LAST_BIT_SET(fb)) {
    shiftAmount += 6;
  }
  bytesEaten++;
  (*curOffset)++;

  if (LAST_BIT_SET(fb)) {
    while (bytesEaten < MAX_EDGE_SIZE_IN_BYTES) {
      char b = start[*curOffset];
      edgeRead |= ((b & 0x7f) << shiftAmount);
      (*curOffset)++; 
      bytesEaten++;
      if (LAST_BIT_SET(b))
        shiftAmount += EDGE_SIZE_PER_BYTE;
      else 
        break;
    }
  }
  edgeRead *= sign;
  return (source + edgeRead);
}

/*
  Reads any edge of an out-edge list after the first edge. 
*/
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

/*
  A dummy test fn to debug/print things. 
*/
struct dummyT {
  bool srcTarg(intE src, intE target, intT edgeNumber) {
    return true;
  }
};


/*
  The main decoding work-horse. First eats the specially coded first 
  edge, and then eats the remaining |d-1| many edges that are normally
  coded. 
*/
template <class T>
void decode(T t, char* edgeStart, intE source, uintT degree) {
  int edgesRead = 0;
  uintE curOffset = 0;
  if (degree > 0) {
    // Eat first edge, which is compressed specially
    intE startEdge = eatFirstEdge(edgeStart, &curOffset, source);
    if (!t.srcTarg(source,startEdge,edgesRead)) {
      return;
    }
    intE prevEdge = startEdge;
    for (int edgesRead = 1; edgesRead < degree; edgesRead++) {
      // Eat the next 'edge', which is a difference, and reconstruct edge.
      intE edgeDiff = eatEdge(edgeStart, &curOffset);
      intE edge = prevEdge + edgeDiff;
      prevEdge = edge;
      if (!t.srcTarg(source, edge, edgesRead)) {
        break; 
      }
    }
  }
}

/*
  Compresses the first edge, writing target-source and a sign bit. 
*/
uintE compressFirstEdge(char *start, uintE offset, intE source, intE target) {
  char* saveStart = start;
  uintE saveOffset = offset;

  intE preCompress = target - source;
  int bytesUsed = 0;
  int shift = 0;
  char firstByte = 0;
  intE toCompress = abs(preCompress);
  firstByte = toCompress & 0x3f; // 0011|1111
  if (preCompress < 0) {
    firstByte |= 0x40;
  }
  shift += 6;
  if ((toCompress >> shift) > 0) {
    firstByte |= 0x80;
  }
  start[offset] = firstByte;
  offset++;

  char curByte = (toCompress >> shift) & 0x7f;
  while ((curByte > 0) || ((toCompress >> shift) > 0)) {
    bytesUsed++;
    char toWrite = curByte;
    shift += 7;
    // Check to see if there's any bits left to represent
    curByte = (toCompress >> shift) & 0x7f;
    if ((toCompress >> shift) > 0) {
      toWrite |= 0x80; 
    }
    start[offset] = toWrite;
    offset++;
  }

  intE ret = eatFirstEdge(saveStart, &saveOffset, source);
  return offset;
}

/*
  Should provide the difference between this edge and the previous edge
*/
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
    currentOffset = compressEdge((char *)edgeStart, currentOffset, 
                                 savedEdges[i]);
  }
  free(savedEdges);
}

/*
  Compresses the edge set in parallel. Does not modify offsets - empty space 
  can now be left between the end of adjacency lists. 
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
    uintT degree = ((i == n-1) ? m : oldOffsets[i+1])-oldOffsets[i];

    // The start of vertex i's edge list is from currentOffset
    offsets[i] = currentOffset;

    if (degree > 0) {
      // Compress the first edge whole, which is signed difference coded
      currentOffset = compressFirstEdge((char *)edges, currentOffset, 
                                        i, savedEdges[nWritten]);
      for (uintT edgeI=1; edgeI < degree; edgeI++) {
        // Store difference between cur and prev edge. 
        intE difference = savedEdges[nWritten + edgeI] - 
                          savedEdges[nWritten + edgeI - 1];
        uintE prevOffset = currentOffset;
        currentOffset = compressEdge((char *)edges, currentOffset, difference);
      }
      // Increment nWritten after all of vertex n's neighbors are written
      nWritten += degree;
    }

    // We've written - let's test this with the dummyT. 
    decode(dummyT(), ((char *)edges) + offsets[i], i, degree);
  }
  free(oldOffsets);
  free(savedEdges);
}

#endif
