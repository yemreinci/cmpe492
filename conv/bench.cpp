#include <iostream>
#include <vector>

#include "conv.hpp"
#include "generator.hpp"
#include "timer.hpp"

int
main(int argc, char* argv[])
{
    int n1, n2, nw;

    if (argc == 1) {
        n1 = n2 = 4000;
        nw = 13;
    } else if (argc == 4) {
        n1 = std::atoi(argv[1]);
        n2 = std::atoi(argv[2]);
        nw = std::atoi(argv[3]);
    } else {
        std::cout << "usage: " << argv[0] << " [n1 n2 nw]" << std::endl;
        return 1;
    }

    std::vector<float> inp(n1 * n2);
    std::vector<float> win(nw * nw);
    std::vector<float> res(n1 * n2);

    cmpe492::random_fill(inp.begin(), inp.end());
    cmpe492::random_fill(win.begin(), win.end());

    std::cout << "runtime:\t" << std::flush;
    {
        cmpe492::timer t{ std::cout };
        cmpe492::conv(n1, n2, nw, inp.data(), win.data(), res.data());
    }

    return 0;
}