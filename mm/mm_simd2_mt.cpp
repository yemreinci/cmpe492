#include <cassert>
#include <cstdlib>
#include <thread>
#include <vector>

#include "mm.hpp"
#include "simd.hpp"

namespace cmpe492 {

namespace {

constexpr int nv = 8; // vector size
constexpr int nu = 1; // unrolling constant

} // namespace

namespace {
void
mm_helper(const int fr_row,
          const int to_row,
          const int n1,
          const int n2,
          const int n3,
          const int n3r,
          float8_t const* const mat1_wrap,
          float8_t const* const mat2_t_wrap,
          float* res)
{
    for (int i = fr_row; i < to_row; i++) {
        for (int j = 0; j < n3r; j++) {
            float8_t t[nu][nu][nv] = {};

            for (int k = 0; k < n2; k++) {

                for (int q1 = 0; q1 < nu; q1++) {
                    for (int q2 = 0; q2 < nu; q2++) {
                        float8_t v0_000 = mat1_wrap[(i * n2 * nu) + (k * nu) + (q1)];
                        float8_t v1_000 = mat2_t_wrap[(j * n2 * nu) + (k * nu) + q2];

                        float8_t v1_001 =
                          __builtin_shufflevector(v1_000, v1_000, 1, 0, 3, 2, 5, 4, 7, 6);

                        float8_t v0_100 =
                          __builtin_shufflevector(v0_000, v0_000, 4, 5, 6, 7, 0, 1, 2, 3);
                        float8_t v0_010 =
                          __builtin_shufflevector(v0_000, v0_000, 2, 3, 0, 1, 6, 7, 4, 5);
                        float8_t v0_110 =
                          __builtin_shufflevector(v0_100, v0_100, 2, 3, 0, 1, 6, 7, 4, 5);

                        t[q1][q2][0] += v0_000 * v1_000;
                        t[q1][q2][1] += v0_000 * v1_001;
                        t[q1][q2][2] += v0_010 * v1_000;
                        t[q1][q2][3] += v0_010 * v1_001;
                        t[q1][q2][4] += v0_100 * v1_000;
                        t[q1][q2][5] += v0_100 * v1_001;
                        t[q1][q2][6] += v0_110 * v1_000;
                        t[q1][q2][7] += v0_110 * v1_001;
                    }
                }
            }

            for (int q1 = 0; q1 < nu; q1++) {
                for (int q2 = 0; q2 < nu; q2++) {

                    for (int w1 = 0; w1 < nv; w1++) {
                        for (int w2 = 0; w2 < nv; w2++) {
                            int ri = i * (nu * nv) + q1 * nv + w2 ^ (w1 & 6);
                            int rj = j * (nu * nv) + q2 * nv + w2 ^ (w1 & 1);

                            if (ri < n1 && rj < n3) {
                                res[ri * n3 + rj] = t[q1][q2][w1][w2];
                            }
                        }
                    }
                }
            }
        }
    }
}
} // namespace

void
mm(int n1, int n2, int n3, float const* mat1, float const* mat2, float* res)
{
    const int n1r = (n1 + nu * nv - 1) / (nu * nv);
    const int n3r = (n3 + nu * nv - 1) / (nu * nv);

    float8_t* const mat1_wrap =
      static_cast<float8_t*>(aligned_alloc(sizeof(float8_t), n1r * n2 * nu * sizeof(float8_t)));
    float8_t* const mat2_t_wrap =
      static_cast<float8_t*>(aligned_alloc(sizeof(float8_t), n3r * n2 * nu * sizeof(float8_t)));

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

    const int num_thr = std::thread::hardware_concurrency();
    std::vector<std::thread> threads(num_thr);

    for (int i = 0; i < num_thr; i++) {
        int beg = i * ((n1r + num_thr - 1) / num_thr);
        int end = (i + 1) * ((n1r + num_thr - 1) / num_thr);
        end = std::min(end, n1r);

        threads[i] = std::thread(mm_helper, beg, end, n1, n2, n3, n3r, mat1_wrap, mat2_t_wrap, res);
    }

    for (int i = 0; i < num_thr; i++) {
        threads[i].join();
    }

    free(mat1_wrap);
    free(mat2_t_wrap);
}

} // namespace cmpe492