#include <cassert>
#include <iostream>
#include <malloc/_malloc.h>

#include "conv.hpp"
#include "simd.hpp"

namespace cmpe492 {

void
conv(const int n1, const int n2, const int nw, float const* inp, float const* win, float* res)
{
    using vector_t = float8_t;
    using vector_unalgn_t = float8_unalgn_t;

    constexpr int vw = sizeof(vector_t) / sizeof(float); // vector width

    assert(nw % 2 == 1);

    const int wb = (nw + vw - 1) / vw; // number of vectors in a row of the window

    vector_t* algn_win =
      static_cast<vector_t*>(aligned_alloc(sizeof(vector_t), nw * wb * sizeof(vector_t)));

    for (int i = 0; i < nw; i++) {
        for (int j = 0; j < wb; j++) {
            for (int k = 0; k < vw; k++) {
                if (j * vw + k < nw) {
                    algn_win[i * wb + j][k] = win[i * nw + j * vw + k];
                } else {
                    algn_win[i * wb + j][k] = 0.0f;
                }
            }
        }
    }

    const int pd_n1 = n1 + nw - 1;
    const int pd_n2 = n2 + wb * vw - 1;
    float* padded_inp = new float[pd_n1 * pd_n2];

    for (int j = 0; j < nw / 2; j++) {
        for (int i = 0; i < pd_n2; i++) {
            padded_inp[j * pd_n2 + i] = 0;
            padded_inp[(pd_n1 - 1 - j) * pd_n2 + i] = 0;
        }
    }

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < nw / 2; j++) {
            padded_inp[(i + nw / 2) * pd_n2 + j] = 0;
        }
        for (int j = nw / 2 + n2; j < pd_n2; j++) {
            padded_inp[(i + nw / 2) * pd_n2 + j] = 0;
        }

        for (int j = 0; j < n2; j++) {
            padded_inp[(i + nw / 2) * pd_n2 + j + nw / 2] = inp[i * n2 + j];
        }
    }

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n2; j++) {
            constexpr int rll = 3; // unroll
            vector_t t[rll] = {};

            for (int k1g = 0; k1g < nw / rll; k1g++) {
                for (int k2 = 0; k2 < wb; k2++) {
                    for (int k1i = 0; k1i < rll; k1i++) {
                        const int k1 = k1g * rll + k1i;

                        vector_t inp_vec = *reinterpret_cast<vector_unalgn_t*>(
                          &padded_inp[(i + k1) * pd_n2 + j + k2 * vw]);

                        vector_t win_vec = algn_win[k1 * wb + k2];

                        t[k1i] += inp_vec * win_vec;
                    }
                }
            }

            for (int k1 = nw / rll * rll; k1 < nw; k1++) {
                for (int k2 = 0; k2 < wb; k2++) {
                    vector_t inp_vec = *reinterpret_cast<vector_unalgn_t*>(
                      &padded_inp[(i + k1) * pd_n2 + j + k2 * vw]);

                    vector_t win_vec = algn_win[k1 * wb + k2];

                    t[0] += inp_vec * win_vec;
                }
            }

            for (int k = 1; k < rll; k++) {
                t[0] += t[k];
            }
            for (int k = 1; k < vw; k++) {
                t[0][0] += t[0][k];
            }

            res[i * n2 + j] = t[0][0];
        }
    }

    delete[] padded_inp;
    free(algn_win);
}

} // namespace cmpe492