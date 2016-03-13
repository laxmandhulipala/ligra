---
id: getting_started
sectionid: docs
layout: docs
title: Getting Started
next: tutorial_bfs.html
prev: introduction.html
redirect_from: "docs/index.html"
---

## Prerequisites 

Ligra can be compiled with multiple backend compilers. We currently support

* Intel `icpc` compiler
* `g++ >= 4.8.0` with support for `Cilk+`
* `OpenMP`

## Organization

[Download](https://github.com/jshun/ligra/archive/master.zip) the code if you 
have not already. The main directories are the `apps` directory, where we will
program and compile applications, and the `ligra` directory, where the framework 
code lives. 

``` bash
$ cd ligra
$ ls -1
apps        # code for Ligra applications
inputs      # example graph inputs
LICENSE
ligra       # code for the Ligra framework
README.md
utils       # utilities for parsing different graph formats
```

## Compilation

Compilation is done from within the `apps/` directory. Depending on the compiler
you wish to use, you will need to set a different environment variable. 

* `g++` using `Cilk`: define the environment variable `CILK`. 
* `icpc`: define the environment variable `MKLROOT` and make sure `CILK` is not defined. 
* `OpenMP`: define the environment variable `OPENMP` and make sure 
`CILK` and `MKLROOT` are not defined. 

Using `Cilk+` seems to give the best parallel performance in our experience. To compile 
with `g++` with no parallel support, make sure `CILK`, `MKLROOT` and `OPENMP` are not 
defined.

Note: `OpenMP` support in Ligra has not been thoroughly tested. If you experience 
any errors, please send an email to Julian Shun. A known issue is that `OpenMP`
will not work correctly when using the experimental version of `gcc 4.8.0`.

Example
--

``` bash
$ cd apps/
$ export CILK=1
$ make
ln -s ../ligra/encoder.C .
ln -s ../ligra/ligra.h .
...
g++ -fcilkplus -lcilkrts -O2 -DCILK   -o encoder encoder.C
g++ -fcilkplus -lcilkrts -O2 -DCILK   -o BFS BFS.C
...
```
