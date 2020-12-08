#include "cblas.h"

#include <iostream>
#include <vector>

int
main()
{
    std::vector<float> a = {
        1, 2,
        4, 5,
        1, 1,
    }, b{
        3,
        4,
    }, c(3*1);

    // 3x2 2x1

    cblas_sgemm(CblasRowMajor,
                CblasNoTrans,
                CblasNoTrans,
                3,
                1,
                2,
                1.0,
                a.data(),
                2,
                b.data(),
                1,
                1.0,
                c.data(),
                1);

    for (int i = 0; i < 3; i++) {
        std::cout << c[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}