#include "mm.hpp"

namespace cmpe492 {

void
mm(int n1, int n2, int n3, float const* mat1, float const* mat2, float* res)
{
    float* mat2_trans = new float[n2 * n3];

    for (int i = 0; i < n2; i++) {
        for (int j = 0; j < n3; j++) {
            mat2_trans[j * n2 + i] = mat2[i * n3 + j];
        }
    }

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n3; j++) {
            float t = 0;

            for (int k = 0; k < n2; k++) {
                t += mat1[i * n2 + k] * mat2_trans[j * n2 + k];
            }

            res[i * n3 + j] = t;
        }
    }

    delete[] mat2_trans;
}

} // namespace cmpe492