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
the [previous tutorial](/ligra/docs/tutorial_bfs.html) or the API reference. 

In this tutorial, we will introduce and use other parts of the core API such
as `vertexFilter` in order to compute the *k-core* of a graph. Given 
$G = (V, E)$ the $k$-core of $G$ is the maximal set of vertices $S \subset V$
s.t. the degree for $v \in S$ in $G[S]$ (the induced subgraph of $G$ over $S$) 
is at least $k$. These objects have been useful in the study of *social networks*
and *bioinformatics*, and have natural applications in the visualization of 
complex networks. 

In the $k$-core problem, our goal will be to produce a map, $f : V \rightarrow \mathbb{N}$
s.t. for $v \in V$, $f(v)$ is the maximum core that $v$ is a part of. We refer to 
$f(v)$ as the core-number of $v$. A commonly used measure is the *degeneracy* of 
a graph, which is just the maximum non-empty core that $G$ supports. 

### A simple algorithm

Here's a simple algorithm for computing the $k$-cores of a graph. Our goal
is to produce an array `cores` that contains the core-number for each vertex. 
Initialize $k$ to be 1. Initially, all vertices are active. Check all active 
vertices, and remove any with induced degree less than $k$. Continue this process 
until no vertices are left. By definition, this is the $k$-core of the graph. 
For any vertices $v \in V$ removed during this step, set `cores[v] = k-1`. 
Decrement the induced degree of all neighbors of removed vertices. Increment 
$k$ and continue until there are no more active vertices. 

### Implementing the simple algorithm

Let's get started! Open up a new file, `Tutorial_KCore.C` in the `apps/` directory,
and type in the following function stub. 

``` cpp
#include "ligra.h"

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  const long n = GA.n; 
  bool* active = new bool[n];
  {parallel_for(long i=0;i<n;i++) active[i] = 1;}
  vertexSubset Frontier(n, n, active);
  long* coreNumbers = new long[n];
}
```

Last time we explicitly created a `vertexSubset`, we used a constructor that took a single
vertex, $v$ and produced the subset $\{s\}$. The default representation of such a small subset
is *sparse*. Here, we are creating a *dense* `vertexSubset`. The difference is that a `sparse`
subset will represent a subset of size $k$ using an array containing $k$ vertex-ids, but a 
`dense` representation will just use an array of $n$ bits. For large subsets that are roughly
$O(n)$, the dense implementation will be more space efficient, and will allow us to decide 
membership in a subset in $O(1)$. 

We will also need to keep around an array containing the induced degrees of vertices. 
Add the following snippet to the end of `Compute`

``` cpp
  long* Degrees = new long[n]; 
  parallel_for(long i=0;i<n;i++) {
    coreNumbers[i] = 0;
    Degrees[i] = GA.V[i].getOutDegree();
  }
  long largestCore = -1; 
```

### Filtering vertices

Great. We now need to implement the logic that separates a given `vertexSubset`
into two sets:  vertices with degree $< k$ and vertices with induced degree 
$\geq k$. In order to do this, we will use a new part of the Ligra API:
`vertexFilter`, which takes as input  a `vertexSubset` and a function 
that, given the id of a vertex, determines whether the vertex should be 
included in the returned vertexSubset or not. The function has the following 
signature: 

``` cpp
template <class F>
vertexSubset vertexFilter(vertexSubset V, F filter);
```

We need to supply it with a structure that is callable on values of the 
vertex-id type (in our case, `long`). We need to overload the `()` operator
for the structure for a `long` argument. The most basic structure that
can be passed to `vertexFilter` is:

```
struct Vertex_F {
  inline bool operator () (long i) {
    return true;
  }
};
```

Note that this leaves the input vertexSubset unchanged. We implement 
the structure to produce a `vertexSubset` containing vertices with degree 
$>= k$ as follows:

``` cpp
template<class vertex>
struct Deg_AtLeast_K {
  vertex* V;
  long *Degrees;
  long k;
  Deg_AtLeast_K(vertex* _V, long* _Degrees, long _k) : 
    V(_V), k(_k), Degrees(_Degrees) {}
  inline bool operator () (long i) {
    return Degrees[i] >= k;
  }
};
```

The following structure will filter all vertices with degree $< k$. As vertices
that satisfy this predicate are removed, we have to do some bookkeeping. In 
particular we 

* set their core-number to $k-1$
* set their induced degree to $0$. 

``` cpp
template<class vertex>
struct Deg_LessThan_K {
  vertex* V;
  long* coreNumbers;
  long* Degrees;
  long k;
  Deg_LessThan_K(vertex* _V, long* _Degrees, long* _coreNumbers, long _k) : 
    V(_V), k(_k), Degrees(_Degrees), coreNumbers(_coreNumbers) { }
  inline bool operator () (long i) {
    if(Degrees[i] < k) { coreNumbers[i] = k-1; Degrees[i] = 0; return true; }
    else return false;
  }
};
```

### Updating induced degrees

The last part of the implementaiton is to update the induced degrees of the
neighbors of removed vertices. We can do this by using `edgeMap`. We'll pass
the following struct to `edgeMap`. 

``` cpp
struct Update_Deg {
  intE* Degrees;
  Update_Deg(intE* _Degrees) : Degrees(_Degrees) {}
  inline bool update (uintE s, uintE d) {
    if(Degrees[d] > 0) Degrees[d]--;
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d) {
    if(Degrees[d] > 0) writeAdd(&Degrees[d],-1);
    return 1;
  }
  inline bool cond (uintE d) { return cond_true(d); }
};
```

### Putting it together

What should our loop do? Initially $k$ starts at one. We don't know what the
*degeneracy* (recall, the maximum non-empty $k$-core) of the graph is, but have
a simple upper bound of $n$. 

``` cpp
  for (long k = 1; k <= n; k++) {
    // TODO
  }
```

What should we do inside of the loop? Inductively assume that we've computed the
$k-1$ core, for some $1 < k < n$. In order to compute the $k$ core, we need to 
keep filtering vertices with induced degree $< k$ until either (1) there are
no more vertices remaining in the graph or (2) all vertices have induced degree
$>= k$. In code:

``` cpp
  for (long k = 1; k <= n; k++) {
    while (true) {
      vertexSubset toRemove =
        vertexFilter(Frontier,Deg_LessThan_K<vertex>(GA.V,Degrees,coreNumbers,k));
      vertexSubset remaining = 
        vertexFilter(Frontier,Deg_AtLeast_K<vertex>(GA.V,Degrees,k));
      Frontier.del();
      Frontier = remaining;
      if (0 == toRemove.numNonzeros()) { // fixed point. found k-core
      	toRemove.del();
        break;
      }
      else {
      	vertexSubset output = edgeMap(GA,toRemove,Update_Deg(Degrees));
      	toRemove.del(); output.del();
      }
    }
    if(Frontier.numNonzeros() == 0) { largestCore = k-1; break; }
  }
```

### The finished app

``` cpp
#include "ligra.h"

struct Update_Deg {
  intE* Degrees;
  Update_Deg(intE* _Degrees) : Degrees(_Degrees) {}
  inline bool update (uintE s, uintE d) { 
    if(Degrees[d] > 0) Degrees[d]--;
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d){
    if(Degrees[d] > 0) writeAdd(&Degrees[d],-1);
    return 1;
  }
  inline bool cond (uintE d) { return cond_true(d); }
};

template<class vertex>
struct Deg_LessThan_K {
  vertex* V;
  uintE* coreNumbers;
  intE* Degrees;
  uintE k;
  Deg_LessThan_K(vertex* _V, intE* _Degrees, uintE* _coreNumbers, uintE _k) : 
    V(_V), k(_k), Degrees(_Degrees), coreNumbers(_coreNumbers) {}
  inline bool operator () (uintE i) {
    if(Degrees[i] < k) { coreNumbers[i] = k-1; Degrees[i] = 0; return true; }
    else return false;
  }
};

template<class vertex>
struct Deg_AtLeast_K {
  vertex* V;
  intE *Degrees;
  uintE k;
  Deg_AtLeast_K(vertex* _V, intE* _Degrees, uintE _k) : 
    V(_V), k(_k), Degrees(_Degrees) {}
  inline bool operator () (uintE i) {
    return Degrees[i] >= k;
  }
};

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
  for (long k = 1; k <= n; k++) {
    while (true) {
      vertexSubset toRemove = 
        vertexFilter(Frontier,Deg_LessThan_K<vertex>(GA.V,Degrees,coreNumbers,k));
      vertexSubset remaining = 
        vertexFilter(Frontier,Deg_AtLeast_K<vertex>(GA.V,Degrees,k));
      Frontier.del();
      Frontier = remaining;
      if (0 == toRemove.numNonzeros()) { // fixed point. found k-core
        toRemove.del();
        break;
      }
      else {
        vertexSubset output = edgeMap(GA,toRemove,Update_Deg(Degrees));
        toRemove.del(); output.del();
      }
    }
    if(Frontier.numNonzeros() == 0) { largestCore = k-1; break; }
  }
  cout << "Degeneracy is " << largestCore << endl;
  Frontier.del(); free(coreNumbers); free(Degrees);
}
```

### Compilation

Compile your algorithm by adding it to the Ligra `Makefile`. Open up the
`Makefile` in the `apps/` directory in your editor, and change the definition of
`ALL` to:

``` bash
ALL= Tutorial_KCore encoder ... (other apps) ...
```

Now, compile the application by running `make`, which will produce a binary called 
`Tutorial_KCore`.

### Testing

Let's try computing the KCore of one of the input graphs. 

``` 
./KCore -s ../inputs/rMatGraph_J_5_100
largestCore was 4
Running time : 0.00594
largestCore was 4
Running time : 0.00559
largestCore was 4
Running time : 0.0057
```

Great! You can try running the KCore on more interesting examples, such 
as the twitter graph. 

