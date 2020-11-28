#include <cassert>
#include <xmmintrin.h>

#include "mm.hpp"

namespace cmpe492 {

void
mm(int n1, int n2, int n3, float const* mat1, float const* mat2, float* res)
{
    constexpr int nt = 4;
    constexpr int nn = 4;

    const int na1 = (n1 + nn - 1) / nn * nn;
    const int na3 = (n3 + nn - 1) / nn * nn;
    const int na2 = (n2 + nt - 1) / nt * nt;

    float* const mat1_align = (float*)aligned_alloc(16, na1 * na2 * sizeof(float));
    float* const mat2_trans_align = (float*)aligned_alloc(16, na2 * na3 * sizeof(float));
    assert(mat1_align);
    assert(mat2_trans_align);

    for (int i = 0; i < na1; i++) {
        for (int j = 0; j < na2; j++) {
            if (i < n1 && j < n2) {
                mat1_align[i * na2 + j] = mat1[i * n2 + j];
            } else {
                mat1_align[i * na2 + j] = 0;
            }
        }
    }

    for (int i = 0; i < na2; i++) {
        for (int j = 0; j < na3; j++) {
            if (i < n2 && j < n3) {
                mat2_trans_align[j * na2 + i] = mat2[i * n3 + j];
            } else {
                mat2_trans_align[j * na2 + i] = 0;
            }
        }
    }

    for (int i = 0; i < na1 / nn; i++) {
        for (int j = 0; j < na3 / nn; j++) {
            __m128 t[nn][nn] = {};

            for (int k1 = 0; k1 < na2 / nt; k1++) {
                for (int ni = 0; ni < nn; ni++) {
                    for (int nj = 0; nj < nn; nj++) {
                        auto v1 = _mm_load_ps(mat1_align + (i * nn + ni) * na2 + k1 * nt);
                        auto v2 = _mm_load_ps(mat2_trans_align + (j * nn + nj) * na2 + k1 * nt);

                        t[ni][nj] = _mm_add_ps(_mm_mul_ps(v1, v2), t[ni][nj]);
                    }
                }
            }

            for (int ni = 0; ni < nn; ni++) {
                for (int nj = 0; nj < nn; nj++) {
                    if ((i * nn + ni) < n1 && (j * nn + nj) < n3)
                        res[(i * nn + ni) * n3 + (j * nn + nj)] =
                          t[ni][nj][0] + t[ni][nj][1] + t[ni][nj][2] + t[ni][nj][3];
                }
            }
        }
    }

    free(mat1_align);
    free(mat2_trans_align);
}

} // namespace cmpe492