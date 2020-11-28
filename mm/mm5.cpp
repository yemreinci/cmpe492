#include <cassert>
#include <thread>
#include <vector>
#include <cstdlib>

#include "mm.hpp"
#include "simd.hpp"

namespace cmpe492 {

void
mm(int n1, int n2, int n3, float const* mat1, float const* mat2, float* res)
{
    constexpr int nv = 4; // vector size
    constexpr int nu = 2; // unrolling constant

    const int n1r = (n1 + nu * nv - 1) / (nu * nv);
    const int n3r = (n3 + nu * nv - 1) / (nu * nv);

    float4_t* const mat1_wrap =
      static_cast<float4_t*>(aligned_alloc(sizeof(float4_t), n1r * n2 * nu * sizeof(float4_t)));
    float4_t* const mat2_t_wrap =
      static_cast<float4_t*>(aligned_alloc(sizeof(float4_t), n3r * n2 * nu * sizeof(float4_t)));

    assert(mat1_wrap);
    assert(mat2_t_wrap);

    for (int i = 0; i < n1r; i++) {
        for (int j = 0; j < n2; j++) {
            for (int k1 = 0; k1 < nu; k1++) {
                for (int k2 = 0; k2 < nv; k2++) {
                    int row = (i * nu * nv + k1 * nv + k2);

                    mat1_wrap[(i * n2 * nu) + (j * nu) + (k1)][k2] =
                      (row < n1) ? mat1[row * n2 + j] : 0.0f;
                }
            }
        }
    }

    for (int i = 0; i < n3r; i++) {
        for (int j = 0; j < n2; j++) {
            for (int k1 = 0; k1 < nu; k1++) {
                for (int k2 = 0; k2 < nv; k2++) {
                    int col = (i * nu * nv + k1 * nv + k2);

                    mat2_t_wrap[(i * n2 * nu) + (j * nu) + (k1)][k2] =
                      (col < n3) ? mat2[j * n3 + col] : 0.0f;
                }
            }
        }
    }

    auto sub_mm = [=](int from_row, int to_row) {
        for (int i = from_row; i < to_row; i++) {
            for (int j = 0; j < n3r; j++) {
                float4_t t[nu][nu][nv] = {};

                for (int k = 0; k < n2; k++) {
                    for (int q1 = 0; q1 < nu; q1++) {
                        float4_t v0_00 = mat1_wrap[(i * n2 * nu) + (k * nu) + (q1)];

                        float4_t v0_01 = __builtin_shufflevector(v0_00, v0_00, 1, 0, 3, 2);
                        float4_t v0_10 = __builtin_shufflevector(v0_00, v0_00, 2, 3, 0, 1);
                        float4_t v0_11 = __builtin_shufflevector(v0_00, v0_00, 3, 2, 1, 0);

                        for (int q2 = 0; q2 < nu; q2++) {
                            float4_t v1_00 = mat2_t_wrap[(j * n2 * nu) + (k * nu) + q2];

                            t[q1][q2][0] += v0_00 * v1_00;
                            t[q1][q2][1] += v0_01 * v1_00;
                            t[q1][q2][2] += v0_10 * v1_00;
                            t[q1][q2][3] += v0_11 * v1_00;
                        }
                    }
                }

                for (int q1 = 0; q1 < nu; q1++) {
                    for (int q2 = 0; q2 < nu; q2++) {

                        for (int w1 = 0; w1 < nv; w1++) {
                            for (int w2 = 0; w2 < nv; w2++) {
                                int ri = i * (nu * nv) + q1 * nv + w1;
                                int rj = j * (nu * nv) + q2 * nv + w2;

                                if (ri < n1 && rj < n3) {
                                    res[ri * n3 + rj] = t[q1][q2][w1^w2][w2];
                                }
                            }
                        }
                    }
                }
            }
        }
    };

    const int num_thr = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(num_thr);

    for (int i = 0; i < num_thr; i++) {
        int beg = i * ((n1r + num_thr - 1) / num_thr);
        int end = (i + 1) * ((n1r + num_thr - 1) / num_thr);
        end = std::min(end, n1r);

        threads[i] = std::thread(sub_mm, beg, end);
    }

    for (int i = 0; i < num_thr; i++) {
        threads[i].join();
    }

    free(mat1_wrap);
    free(mat2_t_wrap);
}

} // namespace cmpe492