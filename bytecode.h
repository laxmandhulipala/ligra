#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "parallel.h"

// Endianness could be a problem here 
#define MAX_EDGE_LENGTH = 8
#define LAST_SET(b) = (b & (0x80))
#define EDGE_BYTES = 0x7f
#define EDGE_SIZE_PER_BYTE = 7


// A sequential difference code decoder
template <class T>
struct decoder {

  intE eatEdge(char* offset) {
    int bytesEaten = 0;
    intE edgeRead = 0;

    while (bytesEaten < MAX_EDGE_LENGTH) {
      offset++;
      char b = *offset; 
      if (LAST_SET(b)) {
        edgeRead += (b & EDGE_BYTES);
        edgeRead <<= EDGE_SIZE_PER_BYTE;
      } else {
        break;        
      }
    } 
    return edgeRead;
  }

  void decode(T t, char* edgeStart, intE source, intT degree) {
    int edgesRead = 0;
    char* cur = edgeStart;
    for (int edgesRead = 0; edgesRead < degree; edgesRead++) {
      intE edge = eatEdge(cur);
      t.func(source, edge);
    }
  }
};
