#include <cassert>
#include <cstdlib>
#include <thread>
#include <vector>

#include "mm.hpp"
#include "simd.hpp"

namespace cmpe492 {

void
mm(int n1, int n2, int n3, float const* mat1, float const* mat2, float* res)
{
    constexpr int nt = 4; // vector size
    constexpr int nn = 4; // unrolling contant

    const int na1 = (n1 + nn - 1) / nn * nn;
    const int na3 = (n3 + nn - 1) / nn * nn;
    const int na2 = (n2 + nt - 1) / nt;

    float4_t* const mat1_align =
      static_cast<float4_t*>(aligned_alloc(sizeof(float4_t), na1 * na2 * sizeof(float4_t)));
    float4_t* const mat2_trans_align =
      static_cast<float4_t*>(aligned_alloc(sizeof(float4_t), na2 * na3 * sizeof(float4_t)));

    assert(mat1_align);
    assert(mat2_trans_align);

    for (int i = 0; i < na1; i++) {
        for (int j1 = 0; j1 < na2; j1++) {
            for (int j2 = 0; j2 < nt; j2++) {
                int j = j1 * nt + j2;

                if (i < n1 && j < n2) {
                    mat1_align[i * na2 + j1][j2] = mat1[i * n2 + j];
                } else {
                    mat1_align[i * na2 + j1][j2] = 0;
                }
            }
        }
    }

    for (int i1 = 0; i1 < na2; i1++) {
        for (int j = 0; j < na3; j++) {
            for (int i2 = 0; i2 < nt; i2++) {
                int i = i1 * nt + i2;

                if (i < n2 && j < n3) {
                    mat2_trans_align[j * na2 + i1][i2] = mat2[i * n3 + j];
                } else {
                    mat2_trans_align[j * na2 + i1][i2] = 0;
                }
            }
        }
    }

    /// calculate the result matrix for a range of rows
    auto sub_mm = [=](int from_row, int to_row) {
        for (int i = from_row; i < to_row; i++) {
            for (int j = 0; j < na3 / nn; j++) {
                float4_t t[nn][nn] = {};

                for (int k1 = 0; k1 < na2; k1++) {
                    for (int ni = 0; ni < nn; ni++) {
                        for (int nj = 0; nj < nn; nj++) {
                            auto v1 = mat1_align[(i * nn + ni) * na2 + k1];
                            auto v2 = mat2_trans_align[(j * nn + nj) * na2 + k1];

                            t[ni][nj] += v1 * v2;
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
    };

    const int num_thr = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(num_thr);

    for (int i = 0; i < num_thr; i++) {
        int beg = i * ((na1 / nn + num_thr - 1) / num_thr);
        int end = (i + 1) * ((na1 / nn + num_thr - 1) / num_thr);
        end = std::min(end, na1 / nn);

        threads[i] = std::thread(sub_mm, beg, end);
    }

    for (int i = 0; i < num_thr; i++) {
        threads[i].join();
    }

    free(mat1_align);
    free(mat2_trans_align);
}

} // namespace cmpe492