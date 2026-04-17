# ptxCompiler

## Introduction
a compiler supporting ptx converting to riscv vector extension

## Install

### Prerequisites

-Ubuntu 22.04
-LLVM 17.0.0rc
-CUDA 12.2

### Installation

1. Clone from git
    clone project
    cd ptxCompiler

2. Build ptxCompiler
    mkdir build && cd build
    cmake ..
    make

### Test case running
1. vector multiply add ptx kernel test.....
    ./bin/ptx_compiler ../test/vector_mul_add.ptx vector_mul_add.ll

### TODO
1. Tensor-related PTX, from wmma and wgmma to RISCV tensor extensions.
2. RISCV SIMT extensions compatible.
