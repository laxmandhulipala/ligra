// This code is part of the project "Ligra: A Lightweight Graph Processing
// Framework for Shared Memory", presented at Principles and Practice of 
// Parallel Programming, 2013.
// Copyright (c) 2013 Julian Shun and Guy Blelloch
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include "parallel.h"
#include "bytecode.h"
#include "quickSort.h"
#define graph sepGraph
#define __ii ___ii
#define __jj ___jj
#define wghGraph sepwghGraph
#define sequence aSequence
#define _seq __seq
#define words __words__
#define isSpace __isSpace
#define readStringFromFile rsfFile
#define readGraphFromFile rgfFile
#define stringToWords stWords
#include "../benchmarks/separator0/common/Separator.h"
#include "../benchmarks/separator0/parallelSeparator/Separator.C"
#include "../benchmarks/separator0/parallelSeparator/blockRadixSort.h"
#include "../benchmarks/common/graphUtils.h"
#include "../benchmarks/common/graphIO.h"
#undef stringToWords
#undef readStringFromFile
#undef readGraphFromFile
#undef isSpace
#undef words
#undef _seq
#undef sequence
#undef graph
#undef __ii
#undef __jj
#undef wghGraph
using namespace std;

typedef pair<uintE,uintE> intPair;

typedef pair<uintE, pair<uintE,intE> > intTriple;

struct edgeArrayT {
  edge<intT> *E;
  intT k;
edgeArrayT(edge<intT> *eP, intT kP) : E(eP), k(kP) {}
  
  bool srcTarg(intE src, intE target, intT edgeNumber) {
    E[k++] = edge<intT>(src, target);
    return true;
  }
};

struct printT {
  bool srcTarg(intE src, intE target, intT edgeNumber) {
    cout << "print : (src,targ) = (" << src << "," << target << ")" << endl;
    return true;
  }
};

template <class vertex>
void printGraph(graph<vertex> G) {
  vertex *V = G.V;
  for (intT i = 0; i < G.n; i++) {
    char *nghArr = (char *)(V[i].getOutNeighbors());
    intT d = V[i].getOutDegree();
    decode(printT(), nghArr, i, d);
  }
}

template <class intT>
sepGraph<intT> graphFromEdges(edgeArray<intT> EA) {
  edgeArray<intT> A;

  edge<intT> *E = newA(edge<intT>,EA.nonZeros);
  parallel_for (intT i=0; i < EA.nonZeros; i++) E[i] = EA.E[i];
  A = edgeArray<intT>(E,EA.numRows,EA.numCols,EA.nonZeros);

  intT m = A.nonZeros;
  intT n = max<intT>(A.numCols,A.numRows);
  intT* offsets = newA(intT,n*2);
  intSort::iSort(A.E,offsets,m,n,getuF<intT>());
  intT *X = newA(intT,m);
  vertex<intT> *v = newA(vertex<intT>,n);
  parallel_for (intT i=0; i < n; i++) {
    intT o = offsets[i];
    intT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    v[i].degree = l;
    v[i].Neighbors = X+o;
    for (intT j=0; j < l; j++) {
      v[i].Neighbors[j] = A.E[o+j].v;
    }
  }
  A.del();
  free(offsets);
  return sepGraph<intT>(v,n,m,X);
}

template <class intT, class vertex>
edgeArray<intT> toEdgeArray(graph<vertex> G) {
  intT numRows = G.n;
  intT nonZeros = G.m;
  vertex *V = G.V;
  edge<intT> *E = newA(edge<intT>, nonZeros);
  intT k = 0;
  edgeArrayT initialState = edgeArrayT(E, 0);
  for (intT j=0; j < numRows; j++) {
    char *nghArr = (char *)(V[j].getOutNeighbors());
    intT d = V[j].getOutDegree();
    decode(initialState, nghArr, j, d);
//     for (intT i = 0; i < V[j].getOutDegree(); i++) {
//       E[k++] = edge<intT>(j,V[j].Neighbors[i]);
//     }
  }
  edgeArray<intT> ea = edgeArray<intT>(E, numRows, numRows, nonZeros);
//  printEdgeArray(ea);
  return ea;
}

void printEdgeArray(edgeArray<intT> ea) {
  for (intT i = 0; i < ea.nonZeros; i++) {
    edge<intT> e = ea.E[i];
    intT src = e.u;
    intT targ = e.v;
    cout << "edge = (" << src << "," << targ << ")" << endl;
  }
}

template <class E>
struct pairFirstCmp {
  bool operator() (pair<intE,E> a, pair<intE,E> b) {
    return a.first < b.first;
  }
};

template <class IntType>
struct pairBothCmp {
  bool operator() (pair<intE,IntType> a, pair<intE,IntType> b) {
    if (a.first != b.first) return a.first < b.first;
    return a.second < b.second;
  }
};

template <class IntType>
struct singletonCmp {
  bool operator() (IntType a, IntType b) {
    return a < b;
  }
};

void compressEdges(intE *edges, intT *offsets, long n, long m) {
  sequentialCompressEdges(edges, offsets, n, m);
}

// A structure that keeps a sequence of strings all allocated from
// the same block of memory
struct words {
  long n; // total number of characters
  char* Chars;  // array storing all strings
  long m; // number of substrings
  char** Strings; // pointers to strings (all should be null terminated)
  words() {}
words(char* C, long nn, char** S, long mm)
: Chars(C), n(nn), Strings(S), m(mm) {}
  void del() {free(Chars); free(Strings);}
};
 
inline bool isSpace(char c) {
  switch (c)  {
  case '\r': 
  case '\t': 
  case '\n': 
  case 0:
  case ' ' : return true;
  default : return false;
  }
}

_seq<char> readStringFromFile(char *fileName) {
  ifstream file (fileName, ios::in | ios::binary | ios::ate);
  if (!file.is_open()) {
    std::cout << "Unable to open file: " << fileName << std::endl;
    abort();
  }
  long end = file.tellg();
  file.seekg (0, ios::beg);
  long n = end - file.tellg();
  char* bytes = newA(char,n+1);
  file.read (bytes,n);
  file.close();
  return _seq<char>(bytes,n);
}

// parallel code for converting a string to words
words stringToWords(char *Str, long n) {
  {parallel_for (long i=0; i < n; i++) 
      if (isSpace(Str[i])) Str[i] = 0; }

  // mark start of words
  bool *FL = newA(bool,n);
  FL[0] = Str[0];
  {parallel_for (long i=1; i < n; i++) FL[i] = Str[i] && !Str[i-1];}
    
  // offset for each start of word
  _seq<long> Off = sequence::packIndex<long>(FL, n);
  long m = Off.n;
  long *offsets = Off.A;

  // pointer to each start of word
  char **SA = newA(char*, m);
  {parallel_for (long j=0; j < m; j++) SA[j] = Str+offsets[j];}

  free(offsets); free(FL);
  return words(Str,n,SA,m);
}

struct logT {
  double* cost;
logT(double* costP) : cost(costP) {}
  bool srcTarg(intE src, intE target, intT edgeNumber) {
    double logcost = log(((double) abs(src - target)) + 1);
    *cost = (*cost) + logcost;
    return true;
  }
};


/*
  Generates the log-cost for a graph. This is an estimate of the 
  degree of locality utilized by this specific labeling of the graph.
*/
template <class vertex>
double logCost(graph<vertex> GA) {
  double logs = 0.0;
  vertex *G = GA.V;
  for (intT i = 0; i < GA.n; i++) {
    intT d = G[i].getInDegree();
    char *nghArr = (char *)(G[i].getInNeighbors());
    double costP = 0.0;
    decode(logT(&costP), nghArr, i, d);
    logs += costP;
  }
  cout << "logs == " << logs << endl;
  return logs/(GA.m*log(2.0));
}

/*
  Generates a new edge-list, frees the old edge list, and modifies offsets
  based on the vertex permutation, I. 
*/
intE *reorderEdges(intE *edges, intT *offsets, intT *I, intT n, intT m) {
  intE *nEdges = newA(intE, m);
  intT *nOffsets = newA(intT, n);
  {parallel_for(intE i=0; i<n; i++) {
    intT degree = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    nOffsets[I[i]] = degree; // new mapping for i now has i's degree. 
  }}
  intT total = sequence::plusScan(nOffsets, nOffsets, n);
  {parallel_for(intE i=0; i<n; i++) {
    intT degree = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    for (intT edg=0; edg<degree; edg++) {
      // Put the old i's edg# edge into the new i's edg# edge. 
      nEdges[nOffsets[I[i]] + edg] = edges[offsets[i] + edg];
    }
    // Done with offsets[i] - we can now move nOffsets[i] -> offsets[i]
  }}
  {parallel_for(intE i=0; i<n; i++) {
    offsets[i] = nOffsets[i];
  }}
  free(nOffsets);
  free(edges);
  return nEdges;
}

template <class vertex>
graph<vertex> readGraphFromFile(char* fname, bool isSymmetric) {
  _seq<char> S = readStringFromFile(fname);
  words W = stringToWords(S.A, S.n);
  if (W.Strings[0] != (string) "AdjacencyGraph") {
    cout << "Bad input file" << endl;
    abort();
  }

  long len = W.m -1;
  long n = atol(W.Strings[1]);
  long m = atol(W.Strings[2]);
  if (len != n + m + 2) {
    cout << "Bad input file" << endl;
    abort();
  }

  long* offsets = newA(long,n);
  intE* edges = newA(intE,m);

  {parallel_for(long i=0; i < n; i++) offsets[i] = atol(W.Strings[i + 3]);}
  {parallel_for(long i=0; i<m; i++) edges[i] = atol(W.Strings[i+n+3]);}

  //W.del(); // to deal with performance bug in malloc

  /* Steps : 
      1. Sort within each in-edge/out-edge segment 
      2. sequentially compress edges using difference coding  
  */

  vertex* vPreSep = newA(vertex,n);

  {parallel_for (uintT i=0; i < n; i++) {
    uintT o = offsets[i];
    uintT d = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    vPreSep[i].setOutDegree(d); 
    vPreSep[i].setOutNeighbors(edges+o);
    quickSort(edges+o, d, singletonCmp<intE>());
    }}

  if(!isSymmetric) {
    long* tOffsets = newA(long,n);
    {parallel_for(intT i=0;i<n;i++) tOffsets[i] = INT_T_MAX;}
    intE* inEdges = newA(intE,m);
    intPair* temp = newA(intPair,m);
    // Create m many new intPairs. 
    {parallel_for(intT i=0;i<n;i++){
      uintT o = offsets[i];
      for(intT j=0;j<vPreSep[i].getOutDegree();j++){
	      temp[o+j] = make_pair(vPreSep[i].getOutNeighbor(j),i);
      }
    }}

    // Compress the out-edges. 
    intE *nEdges = parallelCompressEdges(edges, offsets, n, m);
    {parallel_for(uintT i=0; i < n; i++) {
      vPreSep[i].setOutNeighbors((intE *)(((char *)nEdges) + offsets[i]));
    }}

    free(edges);
    free(offsets);
    
    edges = nEdges;

    /* From here on-out, we use temp, and tOffsets to guarantee 
       stability as after compression offsets is no longer a reliable source
       of degree information. */

    // Use pairBothCmp because when encoding we need monotonicity 
    quickSort(temp,m,pairBothCmp<intE>());
 
    tOffsets[0] = 0; inEdges[0] = temp[0].second;
    {parallel_for(intT i=1;i<m;i++) {
      inEdges[i] = temp[i].second;
      if(temp[i].first != temp[i-1].first) {
      	tOffsets[temp[i].first] = i;
      }
    }}
    free(temp);

    uintT currOffset = m;
    for(intT i=n-1;i>=0;i--) {
      if(tOffsets[i] == INT_T_MAX) tOffsets[i] = currOffset;
      else currOffset = tOffsets[i];
    }

    {parallel_for(uintT i=0;i<n;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == n-1) ? m : tOffsets[i+1])-tOffsets[i];
      vPreSep[i].setInDegree(l);
      vPreSep[i].setInNeighbors(inEdges+o);
      }}

    intE *ninEdges = parallelCompressEdges(inEdges, tOffsets, n, m);
    for (uintT i = 0; i < n; i++) {
      vPreSep[i].setInNeighbors((intE *)(((char *)ninEdges) + tOffsets[i]));
    }

    free(tOffsets);
    cout << "finished reading graph" << endl;

    graph<vertex> preSep = graph<vertex>(vPreSep,(intT)n,m,edges,inEdges);

    double costOurs = logCost<vertex>(preSep);
    cout << "logCostOurs = " << costOurs << endl;

    return preSep;
  }

  else {
    edges = parallelCompressEdges(edges, offsets, n, m);
    {parallel_for(uintT i=0; i < n; i++) {
      vPreSep[i].setOutNeighbors((intE *)(((char *)edges) + offsets[i]));
    }}
    free(offsets);
    cout << "finished reading graph" << endl;
    graph<vertex> preSep =  graph<vertex>(vPreSep,(intT)n,m,edges);
    return preSep;
  }
}

template <class vertex>
wghGraph<vertex> readWghGraphFromFile(char* fname, bool isSymmetric) {
  _seq<char> S = readStringFromFile(fname);
  words W = stringToWords(S.A, S.n);
  if (W.Strings[0] != (string) "WeightedAdjacencyGraph") {
    cout << "Bad input file" << endl;
    abort();
  }

  long len = W.m -1;
  long n = atol(W.Strings[1]);
  long m = atol(W.Strings[2]);
  if (len != n + 2*m + 2) {
    cout << "Bad input file" << endl;
    abort();
  }

  intT* offsets = newA(intT,n);
  intE* edgesAndWeights = newA(intE,2*m);

  {parallel_for(long i=0; i < n; i++) offsets[i] = atol(W.Strings[i + 3]);}
  {parallel_for(long i=0; i<m; i++) {
      edgesAndWeights[2*i] = atol(W.Strings[i+n+3]); 
      edgesAndWeights[2*i+1] = atol(W.Strings[i+n+m+3]);
    } }
  //W.del(); // to deal with performance bug in malloc

  vertex *v = newA(vertex,n);

  {parallel_for (uintT i=0; i < n; i++) {
    uintT o = offsets[i];
    uintT l = ((i == n-1) ? m : offsets[i+1])-offsets[i];
    v[i].setOutDegree(l);
    v[i].setOutNeighbors((intE*)(edgesAndWeights+2*o));
    }}
  
  if(!isSymmetric) {
    intT* tOffsets = newA(intT,n);
    {parallel_for(intT i=0;i<n;i++) tOffsets[i] = INT_T_MAX;}
    intE* inEdgesAndWghs = newA(intE,2*m);
    intTriple* temp = newA(intTriple,m);
    {parallel_for(intT i=0;i<n;i++){
      uintT o = offsets[i];
      for(intT j=0;j<v[i].getOutDegree();j++){	  
	temp[o+j] = make_pair(v[i].getOutNeighbor(j),make_pair(i,v[i].getOutWeight(j)));
      }
      }}
    free(offsets);

    quickSort(temp,m,pairFirstCmp<intPair>());

    tOffsets[0] = 0; 
    inEdgesAndWghs[0] = temp[0].second.first;
    inEdgesAndWghs[1] = temp[0].second.second;
    {parallel_for(intT i=1;i<m;i++) {
      inEdgesAndWghs[2*i] = temp[i].second.first; 
      inEdgesAndWghs[2*i+1] = temp[i].second.second;
      if(temp[i].first != temp[i-1].first) {
	      tOffsets[temp[i].first] = i;
      }
     }}

    free(temp);

    uintT currOffset = m;
    for(intT i=n-1;i>=0;i--) {
      if(tOffsets[i] == INT_T_MAX) tOffsets[i] = currOffset;
      else currOffset = tOffsets[i];
    }
    
    {parallel_for(uintT i=0;i<n;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == n-1) ? m : tOffsets[i+1])-tOffsets[i];
      v[i].setInDegree(l);
      v[i].setInNeighbors((intE*)(inEdgesAndWghs+2*o));
      }}

    free(tOffsets);
    return wghGraph<vertex>(v,(intT)n,m,edgesAndWeights, inEdgesAndWghs);
  }

  else {  
    free(offsets);
    return wghGraph<vertex>(v,(intT)n,m,edgesAndWeights); 
  }
}

template <class vertex>
graph<vertex> readGraphFromBinary(char* iFile, bool isSymmetric) {
  char* config = (char*) ".config";
  char* adj = (char*) ".adj";
  char* idx = (char*) ".idx";
  char configFile[strlen(iFile)+7];
  char adjFile[strlen(iFile)+4];
  char idxFile[strlen(iFile)+4];
  strcpy(configFile,iFile);
  strcpy(adjFile,iFile);
  strcpy(idxFile,iFile);
  strcat(configFile,config);
  strcat(adjFile,adj);
  strcat(idxFile,idx);

  ifstream in(configFile, ifstream::in);
  intT n;
  in >> n;
  in.close();

  ifstream in2(adjFile,ifstream::in | ios::binary); //stored as uints
  in2.seekg(0, ios::end);
  long size = in2.tellg();
  in2.seekg(0);
  uintT m = size/sizeof(uint);
  char* s = (char *) malloc(size);
  in2.read(s,size);
  in2.close();
  
  intE* edges = (intE*) s;
  ifstream in3(idxFile,ifstream::in | ios::binary); //stored as longs
  in3.seekg(0, ios::end);
  size = in3.tellg();
  in3.seekg(0);
  if(n != size/sizeof(long)) { cout << "File size wrong\n"; abort(); }

  char* t = (char *) malloc(size);
  in3.read(t,size);
  in3.close();
  intT* offsets = (intT*) t;

  vertex* v = newA(vertex,n);
  
  {parallel_for(long i=0;i<n;i++) {
    uintT o = offsets[i];
    uintT l = ((i==n-1) ? m : offsets[i+1])-offsets[i];
      v[i].setOutDegree(l); 
      v[i].setOutNeighbors((intE*)edges+o); }}

  cout << "n = "<<n<<" m = "<<m<<endl;

  if(!isSymmetric) {
    intT* tOffsets = newA(intT,n);
    {parallel_for(intT i=0;i<n;i++) tOffsets[i] = INT_T_MAX;}
    uintE* inEdges = newA(uintE,m);
    intPair* temp = newA(intPair,m);
    {parallel_for(intT i=0;i<n;i++){
      uintT o = offsets[i];
      for(intT j=0;j<v[i].getOutDegree();j++){
	temp[o+j].first = v[i].getOutNeighbor(j);
	temp[o+j].second = i;
      }
      }}

    quickSort(temp,m,pairFirstCmp<intE>());

    tOffsets[0] = 0; inEdges[0] = temp[0].second;
    {parallel_for(intT i=1;i<m;i++) {
      inEdges[i] = temp[i].second;
      if(temp[i].first != temp[i-1].first) {
	tOffsets[temp[i].first] = i;
      }
      }}
    free(temp);

    uintT currOffset = m;
    for(intT i=n-1;i>=0;i--) {
      if(tOffsets[i] == INT_T_MAX) tOffsets[i] = currOffset;
      else currOffset = tOffsets[i];
    }

    {parallel_for(uintT i=0;i<n;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == n-1) ? m : tOffsets[i+1])-tOffsets[i];
      v[i].setInDegree(l);
      v[i].setInNeighbors((intE*)inEdges+o);
      }}
    free(tOffsets);
    return graph<vertex>(v,(intT)n,m,(intE*)edges, (intE*)inEdges);
  }
  compressEdges(edges, offsets, n, m);
  for (uintT i = 0; i < n; i++) {
    v[i].setOutNeighbors((intE *)(((char *)edges) + offsets[i]));
  }
  free(offsets);
  return graph<vertex>(v,n,m,(intE*)edges);
}

template <class vertex>
wghGraph<vertex> readWghGraphFromBinary(char* iFile, bool isSymmetric) {
  char* config = (char*) ".config";
  char* adj = (char*) ".adj";
  char* idx = (char*) ".idx";
  char configFile[strlen(iFile)+7];
  char adjFile[strlen(iFile)+4];
  char idxFile[strlen(iFile)+4];
  strcpy(configFile,iFile);
  strcpy(adjFile,iFile);
  strcpy(idxFile,iFile);
  strcat(configFile,config);
  strcat(adjFile,adj);
  strcat(idxFile,idx);

  ifstream in(configFile, ifstream::in);
  intT n;
  in >> n;
  in.close();

  ifstream in2(adjFile,ifstream::in | ios::binary); //stored as uints
  in2.seekg(0, ios::end);
  long size = in2.tellg();
  in2.seekg(0);
  uintT m = size/sizeof(uint);

  char* s = (char *) malloc(size);
  in2.read(s,size);
  in2.close();

  intE* edges = (intE*) s;
  ifstream in3(idxFile,ifstream::in | ios::binary); //stored as longs
  in3.seekg(0, ios::end);
  size = in3.tellg();
  in3.seekg(0);
  if(n != size/sizeof(long)) { cout << "File size wrong\n"; abort(); }

  char* t = (char *) malloc(size);
  in3.read(t,size);
  in3.close();
  intT* offsets = (intT*) t;

  vertex *V = newA(vertex, n);
  intE* edgesAndWeights = newA(intE,2*m);
  {parallel_for(long i=0;i<m;i++) {
    edgesAndWeights[2*i] = edges[i];
    edgesAndWeights[2*i+1] = 1; //give them unit weight
    }}
  free(edges);

  {parallel_for(long i=0;i<n;i++) {
    uintT o = offsets[i];
    uintT l = ((i==n-1) ? m : offsets[i+1])-offsets[i];
    V[i].setOutDegree(l);
    V[i].setOutNeighbors(edgesAndWeights+2*o);
    }}
  cout << "n = "<<n<<" m = "<<m<<endl;

  if(!isSymmetric) {
    intT* tOffsets = newA(intT,n);
    {parallel_for(intT i=0;i<n;i++) tOffsets[i] = INT_T_MAX;}
    intE* inEdgesAndWghs = newA(intE,2*m);
    intPair* temp = newA(intPair,m);
    {parallel_for(intT i=0;i<n;i++){
      uintT o = offsets[i];
      for(intT j=0;j<V[i].getOutDegree();j++){
      	temp[o+j] = make_pair(V[i].getOutNeighbor(j),i);
      }
      }}

    quickSort(temp,m,pairFirstCmp<intE>());

    tOffsets[0] = 0; 
    inEdgesAndWghs[0] = temp[0].second;
    inEdgesAndWghs[1] = 1;
    {parallel_for(intT i=1;i<m;i++) {
      inEdgesAndWghs[2*i] = temp[i].second;
      inEdgesAndWghs[2*i+1] = 1;
      if(temp[i].first != temp[i-1].first) {
      	tOffsets[temp[i].first] = i;
      }
      }}
    free(temp);
    uintT currOffset = m;
    for(intT i=n-1;i>=0;i--) {
      if(tOffsets[i] == INT_T_MAX) tOffsets[i] = currOffset;
      else currOffset = tOffsets[i];
    }

    {parallel_for(uintT i=0;i<n;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == n-1) ? m : tOffsets[i+1])-tOffsets[i];
      V[i].setInDegree(l);
      V[i].setInNeighbors((intE*)(inEdgesAndWghs+2*o));
      }}
    free(tOffsets);
    return wghGraph<vertex>(V,(intT)n,m,edges,inEdgesAndWghs);
  }

  free(offsets);
  return wghGraph<vertex>(V,n,m,edgesAndWeights);
}

template <class vertex>
graph<vertex> readGraph(char* iFile, bool symmetric, bool binary) {
  if(binary) return readGraphFromBinary<vertex>(iFile,symmetric); 
  else return readGraphFromFile<vertex>(iFile,symmetric);
}

template <class vertex>
wghGraph<vertex> readWghGraph(char* iFile, bool symmetric, bool binary) {
  if(binary) return readWghGraphFromBinary<vertex>(iFile,symmetric); 
  else return readWghGraphFromFile<vertex>(iFile,symmetric);
}
