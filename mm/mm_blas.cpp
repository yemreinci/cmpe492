#include "mm.hpp"

#include <cblas.h>

namespace cmpe492 {

void
mm(int n1, int n2, int n3, float const* mat1, float const* mat2, float* res)
{
    cblas_sgemm(
      CblasRowMajor, CblasNoTrans, CblasNoTrans, n1, n3, n2, 1.0, mat1, n2, mat2, n3, 1.0, res, n3);
}

} // namespace cmpe492