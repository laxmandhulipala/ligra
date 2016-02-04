---
id: tutorial
sectionid: docs
layout: docs
title: Tutorial
next: tutorial.html
redirect_from: "docs/index.html"
---

In this tutorial we develop a parallel breadth-first search algorithm in Ligra. 
If you don't already have Ligra setup, please first follow the [installation instructions](/react/docs/installation.html).

## Preliminaries

Open up `BFS.C` in your favorite text editor, and write the following stub for a
function called `Compute` which will serve as the entrypoint to our application.

```c
template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  ...
}
```

