---
id: tutorial_kcore
sectionid: docs
layout: docs
title: "Tutorial: KCore"
next: getting_started.html
prev: tutorial_bfs.html
redirect_from: "docs/index.html"
---

At this point, you should be familiar with some of the basic features 
of Ligra such as the `vertexSubset` datatype and core parts of the API
such as `edgeMap`. For examples of how to use this, please refer to either
the [previous tutorial](/ligra/tutorial_bfs) or the API reference. 

In this tutorial, we will introduce and use other parts of the core API such
as *`vertexFilter`* in order to compute the *k-core* of a graph. Given 
$G = (V, E)$ the $k$-core of $G$ is the maximal set of vertices $S \subset V$
s.t. the degree for $v \in S$ in $G[S]$ (the induced subgraph of $G$ over $S$) 
is at least $k$. These objects have been useful in the study of *social networks*
and *bioinformatics*, and have natural applications in the visualization of 
complex networks. 

In the $k$-core problem, our goal will be to produce a map, $f : V \rightarrow \mathbb{N}$
s.t. for $v \in V$, $f(v)$ is the maximum core that $v$ is a part of. We refer to 
$f(v)$ as the core-number of $v$. A commonly used definition is the *degeneracy* of 
a graph, which is just the maximum non-empty core that $G$ supports. 

### A simple algorithm

Here's a simple algorithm for computing the $k$-cores of a graph. Our goal
is to produce an array `cores` that contains the core-number for each vertex. 
Initialize $k$ to be 1. Initially, all vertices are active. Check all active 
vertices, and remove any with induced degree less than $k$. Continue this process 
until no vertices are left. By definition, this is the $k$-core of the graph. 
For any vertices $v \in V$ removed during this step, set `cores[v] = k-1`. 
Increment $k$ and continue until there are no more active vertices. 

### Implementing the simple algorithm

Let's get started! Open up a new file, `Tutorial_KCore.C` in the `apps/` directory,
and type in the following function stub. 

``` cpp
#include "ligra.h"

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  const long n = GA.n; 
  bool* active = newA(bool,n);
  {parallel_for(long i=0;i<n;i++) active[i] = 1;}
  vertexSubset Frontier(n, n, active);
  uintE* coreNumbers = newA(uintE,n);
  intE* Degrees = newA(intE,n);
  {parallel_for(long i=0;i<n;i++) {
      coreNumbers[i] = 0;
      Degrees[i] = GA.V[i].getOutDegree();
    }}
  long largestCore = -1;
}
```

All we have done so rar

