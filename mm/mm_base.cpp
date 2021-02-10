#include "mm.hpp"

namespace cmpe492 {

void
mm(int n1, int n2, int n3, float const* mat1, float const* mat2, float* res)
{
    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n3; j++) {
            float t = 0;

            for (int k = 0; k < n2; k++) {
                t += mat1[i * n2 + k] * mat2[k * n3 + j];
            }

            res[i * n3 + j] = t;
        }
    }
}

} // namespace cmpe492