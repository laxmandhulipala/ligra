---
id: introduction
sectionid: docs
layout: docs
title: The Ligra Graph Processing Framework
next: getting_started.html
redirect_from: "docs/index.html"
---

## Introduction

Welcome! These documents will teach you about the Ligra Graph Processing Framework. Ligra 
is a lightweight framework for processing graphs in shared memory. It is particularly 
suited for implementing parallel graph traversal algorithms where only a subset of the 
vertices are processed in an iteration. The project was motivated by the fact that the 
largest publicly available real-world graphs all fit in shared memory. When graphs fit 
in shared-memory, processing them using Ligra can give performance improvements of up 
orders of magnitude compared to distributed-memory graph processing systems. 

This document is split up into a number of sections.  

* Getting Started - Set up your machine to use Ligra
* Tutorial: BFS - Develop a simple breadth-first search in Ligra
* Tutorial: KCore - 
