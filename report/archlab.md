# archlab报告

> 誓死和这个实验斗争到底。

## 前言：何出此言？

为什么说斗争到底这句话呢？因为一看到这个实验的文档和框架，就大致觉得这不是个简单的实验，涉及到晦涩的汇编代码、编译相关的文法检查、cpu模拟器等等，想全部搞懂要花费不少精力。

要命的是，我拿到手的handout还有bug。比如刚拿到手编译Y86-64 tools，不成功，原因并不是网上说的缺少tk库等，我碰到的问题主要是源码中缺少的`extern`标志。经过几番查询，发现解决[方案](https://stackoverflow.com/questions/63152352/fail-to-compile-the-y86-simulatur-csapp)。

在需要的gcc的子文件夹，在makefile的flags中加入`-fcommon`。

此后回到sim文件夹，make成功。


