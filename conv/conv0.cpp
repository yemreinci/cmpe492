#include <cassert>

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

            float t = 0;

            for (int k1 = 0; k1 < nw; k1++) {
                for (int k2 = 0; k2 < nw; k2++) {
                    t += padded_inp[(i + k1) * pd_n2 + j + k2] * win[k1 * nw + k2];
                }
            }

            res[i * n2 + j] = t;
        }
    }

    delete[] padded_inp;
}

} // namespace cmpe492