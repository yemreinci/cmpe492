# Dense Matrix Multiplication

This folder contains dense matrix multiplication implementations and tools for evaluating them.

Here are the explanations for each implementation:

## Implementations

### mm_base.cpp
This is the most naive and well-known matrix multiplication implementation. 
Just 3 for-loops and in the innermost loop, the statement:
```
res[i][j] += a[i][k] * b[k][j]
```

### mm_linit.cpp
This implementation is the same with `mm_base` but it pre-computes the second matrix's transpose. This way, the second matrix is also traversed linearly, allowing for more cache reuse.  

### mm_unroll.cpp
In addition to `mm_linit`, this one unwraps the innermost loop 4 steps to enable more instruction-level parallelism. 

### mm_simd1.cpp
This implementation uses vector instructions and also computes the result matrix in 4x4 blocks in order to allow for more register and cache reuse.

### mm_simd2.cpp
This implementation also uses vector instructions but unlike `mm_simd`, it holds the vectors vertically and uses vector shuffling to compute the result matrix in 8x8 blocks.

### mm_simd2_mt.cpp
This one adds multi-threading on top of `mm_simd2`.

