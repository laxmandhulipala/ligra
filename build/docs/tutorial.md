---
id: tutorial
sectionid: docs
layout: docs
title: Tutorial
next: tutorial.html
redirect_from: "docs/index.html"
---

## Introduction

Ligra is a lightweight graph processing framework for shared memory. It is particularly 
suited for implementing parallel graph traversal algorithms where only a subset of the 
vertices are processed in an iteration. 

The project was motivated by the fact that the 
largest publicly available real-world graphs all fit in shared memory. When graphs fit 
in shared-memory, processing them using Ligra can give performance improvements of up 
orders of magnitude compared to distributed-memory graph processing systems. 

## Preliminaries

After this tutorial, you will understand how to structure and implement a parallel 
breadth-first search algorithm in Ligra. 


## Datatypes 

There are two main abstractions in Ligra: the **graph** datatype and the **vertexSubset** datatype.
A graph can be parametrized by one of four vertex types:

* symmetricVertex
* asymmetricVertex
* compressedSymmetricVertex
* compressedAsymmetricVertex

The vertex type will depend on the type of graph the application is dealing with. When
reading a graph from file, this type will be automatically selected based on whether the
underlying graph is compressed or symmetric. 

A vertexSubset represents 


Ligra is designed to support algorithms that run in rounds by operating on subsets
of vertices. 



Ligra supports two data types, one representing a graph, and another for representing 
subsets of the vertices, which is referred to as vertexSubsets. Other than constructors 
and size queries, the interface supplies only two functions, one for mapping over 
vertices (vertexMap) and the other for mapping over edges (edgeMap). Since a vertexSubset
is a subset of vertices, the vertexMap can be used to map over any subset of the original 
vertices, and hence its utility in traversal algorithms---or more generally in any algorithm in which only (possibly small) subsets of the graph are processed on each round. The edgeMap also processes a subset of the edges, which is specified using a vertexSubset to indicate the valid sources, and a Boolean function to indicate the valid targets of each edge.





Open up `BFS.C` in your favorite text editor, and write the following stub for a
function called `Compute` which will serve as the entrypoint to our application.

```c
template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  ...
}
```

