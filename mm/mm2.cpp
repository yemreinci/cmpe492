#include "mm.hpp"

namespace cmpe492 {

void mm(int n1, int n2, int n3, float const* mat1, float const* mat2, float* res) {
    float *mat2_trans = new float[n2*n3];

    for (int i = 0; i < n2; i++) {
        for (int j = 0; j < n3; j++) {
            mat2_trans[j*n2 + i] = mat2[i*n3 + j];
        }
    }

    constexpr int nt = 4;

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n3; j++) {
            float t[nt] = {};

            for (int k1 = 0; k1 < n2 / nt; k1++) {
                t[0] += mat1[i*n2 + k1*nt + 0] * mat2_trans[j*n2 + k1*nt + 0];
                t[1] += mat1[i*n2 + k1*nt + 1] * mat2_trans[j*n2 + k1*nt + 1];
                t[2] += mat1[i*n2 + k1*nt + 2] * mat2_trans[j*n2 + k1*nt + 2];
                t[3] += mat1[i*n2 + k1*nt + 3] * mat2_trans[j*n2 + k1*nt + 3];
            }

            for (int k = n2 / nt * nt; k < n2; k++) {
                t[0] += mat1[i*n2 + k] * mat2_trans[j*n2 + k];
            }

            for (int k = 1; k < nt; k++) {
                t[0] += t[k];
            }
            res[i*n3 + j] = t[0];
        }
    }

    delete[] mat2_trans;
}

}