#include <cstdlib>
#include <iostream>

#include "generator.hpp"
#include "mm.hpp"
#include "timer.hpp"

int
main(int argc, char* argv[])
{
    int n1, n2, n3;

    if (argc == 1) {
        n1 = 1500;
        n2 = 1500;
        n3 = 1500;
    } else if (argc == 4) {
        n1 = std::atoi(argv[1]);
        n2 = std::atoi(argv[2]);
        n3 = std::atoi(argv[3]);
    } else {
        std::cout << "usage: " << argv[0] << " [n1 n2 n3]" << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << n1 << " " << n2 << " " << n3 << std::endl;

    std::vector<float> mat1(n1 * n2);
    std::vector<float> mat2(n2 * n3);
    std::vector<float> res(n1 * n3);

    cmpe492::random_fill(mat1.begin(), mat1.end());
    cmpe492::random_fill(mat2.begin(), mat2.end());

    std::cout << "running time:\t" << std::flush;
    {
        cmpe492::timer t{ std::cout };
        cmpe492::mm(n1, n2, n3, mat1.data(), mat2.data(), res.data());
    }

    std::cout << "========" << std::endl;

    return 0;
}