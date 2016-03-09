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


