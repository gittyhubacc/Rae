
# Rae
Rae is a general-purpose aspect-oriented programming language that facilitates the creation of deep embeddings for arbitrary Context-Free Languages via Attribute Grammar rules that synthesize Shallow Embeddings at compile time.

## `raec`
This repository has a compiler for Rae with various examples and documentation detailing how it can be used.
Rae's compiler, `raec`, is written in C and has a dependency on `LLVM`, which can be downloaded [here](https://llvm.org/).
It's designed to be used with a preprocessor like [gpp](https://github.com/logological/gpp/).
If you're interested in building `raec` you should read the [/makefile](/makefile).
It's not long and will make clear both how `raec` is built and used.

## Examples
The [/tst](/tst) and [/examples](/examples) directories contain small tests of functionality.
There's a small web application built with Rae in [/apps/web](/apps/web) 
and the beginnings of a small game built with Rae and [love2D](https://lovd2d.org/) in [/apps/love](/apps/love).
