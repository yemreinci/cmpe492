#pragma once

namespace cmpe492 {

/// multiply matrices mat1 and mat2 and put the result in res
/// mat1 is n1 by n2, mat2 is n2 by n3 and res is n1 by n3.
void
mm(int n1, int n2, int n3, float const* mat1, float const* mat2, float* res);

} // namespace cmpe492