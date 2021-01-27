#include <cassert>
#include <iostream>

#include "conv.hpp"

namespace cmpe492 {

void
conv(int n1, int n2, int nw, float const* inp, float const* win, float* res)
{
    assert(nw % 2 == 1);

    int pd_n1 = n1 + nw - 1;
    int pd_n2 = n2 + nw - 1;
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
            padded_inp[(i + nw / 2) * pd_n2 + pd_n2 - 1 - j] = 0;
        }

        for (int j = 0; j < n2; j++) {
            padded_inp[(i + nw / 2) * pd_n2 + j + nw / 2] = inp[i * n2 + j];
        }
    }

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n2; j++) {

            constexpr int nnw = 8;
            float t[nnw] = {};

            for (int k1 = 0; k1 < nw; k1++) {
                for (int k2g = 0; k2g < nw / nnw; k2g++) {
                    for (int k2i = 0; k2i < nnw; k2i++) {
                        int k2 = k2g * nnw + k2i;
                        t[k2i] += padded_inp[(i + k1) * pd_n2 + j + k2] * win[k1 * nw + k2];
                    }
                }
                for (int k2 = (nw / nnw) * nnw; k2 < nw; k2++) {
                    t[0] += padded_inp[(i + k1) * pd_n2 + j + k2] * win[k1 * nw + k2];
                }
            }

            for (int k = 1; k < nnw; k++) {
                t[0] += t[k];
            }
            res[i * n2 + j] = t[0];
            
        }
    }

    delete[] padded_inp;
}

} // namespace cmpe492