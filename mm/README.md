# Dense Matrix Multiplication

This folder contains various dense matrix multiplication implementations and tools for evaluating them.

Here are the explanations for each matrix multiplication implementation, from `mm0` to `mm5`:

## Implementations

### mm0.cpp
This is the most naive and well-known matrix multiplication implementation. 
Just 3 for-loops and in the innermost loop, the statement:
```
res[i][j] += a[i][k] * b[k][j]
```

### mm1.cpp
This implementation is the same with `mm0` but it pre-computes the second matrix's transpose. This way, the second matrix is also traversed linearly, allowing for more cache reuse.  

### mm2.cpp
In addition to `mm1`, this one unrolls the innermost loop 4 steps to enable more instruction-level parallelism. 

### mm3.cpp
This implementation uses vector instructions and also computes the result matrix in 4x4 blocks in order to allow for more register and cache reuse.

### mm4.cpp
This one adds multi-threading on top of `mm3`.

### mm5.cpp
This implementation also uses multi-threading and vector instructions but unlike `mm3` and `mm4`, it holds the vectors vertically and uses vector shuffling to compute the result matrix in 8x8 blocks.
