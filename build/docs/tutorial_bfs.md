---
id: tutorial_bfs
sectionid: docs
layout: docs
title: "Tutorial: Breadth-First Search"
next: tutorial_kcore.html
prev: getting_started.html
redirect_from: "docs/index.html"
---

In this project, we'll implement a parallel version of Breadth-First Search in 
Ligra. Given a graph, $G = (V,E)$, and a source node $s$, a Breadth-First Search 
processes the graph level by level, starting from $s$. The output of the 
algorithm is an array $A$ s.t. $\forall v \in V, A[v]$ holds the parent of $v$ in 
the *breadth-first tree* rooted at $s$. 

### Writing generic graph algorithms

One of the main goals of Ligra is to abstract away implementation specific details
about how the graph is represented (Is the graph compressed/uncompressed? Is it
directed/undirected?). The standard way of implementing an algorithm in Ligra is 
to implement a method, `Compute`, which has the following signature: 

``` cpp
template <class vertex>
void Compute(graph<vertex>& G, commandLine P);
```

To get started, open up a new file in the `apps/` directory, `Tutorial_BFS.C`, and 
enter in the following stub. The only include we will require is the `ligra.h` header
file.

``` cpp
#inclde "ligra.h"

template <class vertex>
void Compute(graph<vertex>& G, commandLine P) {
  long start = P.getOptionLongValue("-start",0);
  long n = GA.n;
  //creates Parents array, initialized to all -1, except for start
  uintE* Parents = newA(uintE,n);
  parallel_for(long i=0;i<n;i++) Parents[i] = UINT_E_MAX;
  Parents[start] = start;
}
```

So far, we have set up the variables and data necessary for the algorithm. 
Next, we will discuss how to represent frontiers in the BFS and process them. 

### Representing a frontier

We can cleanly express the BFS by using the **`vertexSubset`** datatype in 
Ligra, which allows us to represent a subset of the vertices. Each level of the 
traversal will be represented by a `vertexSubset`. We can construct the initial
`vertexSubset` using the following constructor

``` cpp
vertexSubset Frontier(n, source); // creates initial frontier
```
add this line to the `Compute` function. 

### Traversing a frontier

We now need to describe the logic to produce the next level given the current level.
Enter **`edgeMap`**. `edgeMap` allows us to process the out edges of a `vertexSubset`,
 and produce a new `vertexSubset`. The function has the following signature:

``` cpp
template <class vertex, class F>
inline vertexSubset edgeMap(graph<vertex> GA, vertexSubset &V, F f);
```

`edgeMap` expects a parameter of some template class `F` that has the following fields: 

``` cpp
struct F {
  F(...) { ... }
  inline bool update (uintE s, uintE d) {
    // logic for how to process the edge (s,d)
    // return 1 if d should be included in the returned vertexSubset
    // return 0 otherwise
  }

  inline bool cond (uintE d) {
    // return 1 if update should be applied on and edge (s,d) 
    // return 0 otherwise
  }
};
```

Note that the `update` logic must be atomic. If a vertex has more than one
in-edge from the current frontier, then multiple calls to `update` can 
happen in parallel which may result in an incorrect result.

For BFS, we will implement a struct, `BFS_F` that 

- `update`: atomically update `Parents` array. We can implement this using a
  compare and swap operation. A simple, implementation independent version
  is provided by the framework as `CAS` in `ligra/utils.h`.  
- `cond`: avoid revisiting previously visited vertices.

``` cpp
struct BFS_F {
  uintE* Parents;
  BFS_F(uintE* _Parents) : Parents(_Parents) {}
  inline bool update (uintE s, uintE d) { // atomically update
    return (CAS(&Parents[d],UINT_E_MAX,s));
  }
  //cond function checks if vertex has been visited yet
  inline bool cond (uintE d) { return (Parents[d] == UINT_E_MAX); }
};
```

Add this code before the definition of `Compute`. 

Notice that while 
`BFS_F` will correctly produce the next frontier, the tree computed 
by the BFS is still non-deterministic! We will discuss how to fix this 
in a later section.

### Implementing traversal logic

All we need to do to finish up the BFS is to actually call `edgeMap`, and
add a termination condition that signifies when the traversal is done. Given
a current frontier, our condition should just apply the edgeMap while the current
frontier is non-empty. In code:

``` cpp
while(!Frontier.isEmpty()){ //loop until frontier is empty
  vertexSubset output = edgeMap(GA, Frontier, BFS_F(Parents));
  Frontier.del();
  Frontier = output; //set new frontier
}
```

### The final algorithm

The last step is to just free any memory allocated in the body of `Compute`. Our
finished BFS algorithm should look as follows:

``` cpp
#include "ligra.h"

struct BFS_F {
  uintE* Parents;
  BFS_F(uintE* _Parents) : Parents(_Parents) {}
  inline bool update (uintE s, uintE d) { //Update
    if(Parents[d] == UINT_E_MAX) { Parents[d] = s; return 1; }
    else return 0;
  }
  inline bool updateAtomic (uintE s, uintE d){ //atomic version of Update
    return (CAS(&Parents[d],UINT_E_MAX,s));
  }
  //cond function checks if vertex has been visited yet
  inline bool cond (uintE d) { return (Parents[d] == UINT_E_MAX); }
};

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  long start = P.getOptionLongValue("-r",0);
  long n = GA.n;
  //creates Parents array, initialized to all -1, except for start
  uintE* Parents = newA(uintE,n);
  parallel_for(long i=0;i<n;i++) Parents[i] = UINT_E_MAX;
  Parents[start] = start;
  vertexSubset Frontier(n,start); //creates initial frontier
  while(!Frontier.isEmpty()){ //loop until frontier is empty
    vertexSubset output = edgeMap(GA, Frontier, BFS_F(Parents));
    Frontier.del();
    Frontier = output; //set new frontier
  }
  Frontier.del();
  free(Parents);
}
```

### Compilation

You can compile your algorithm by adding it to the Ligra `Makefile`. Open up the
`Makefile` in the `apps/` directory in your editor, and change the definition of
`ALL` to:

``` bash
ALL= Tutorial_BFS encoder ... (other apps) ...
```

Now, compile the application by running `make`, which will produce a binary called 
`Tutorial_BFS`.

### Testing

Let's try running our program on one of the test-inputs provided by ligra in the `inputs/`
directory. 

``` 
./Tutorial_BFS -s -start 1 ../inputs/rMatGraph_J_5_100
Running time : 0.000234
Running time : 0.000359
Running time : 0.000243
```

Great! Note that the `-s` flag tells Ligra that the graph is symmetric (undirected). 
