#include <iostream>
#include <tuple>
#include <vector>

#include "conv.hpp"
#include "generator.hpp"

bool
test_conv(int n1,
          int n2,
          int nw,
          std::vector<float> inp,
          std::vector<float> win,
          std::vector<float> expected_res)
{
    std::vector<float> res(n1 * n2);

    cmpe492::conv(n1, n2, nw, inp.data(), win.data(), res.data());

    for (int i = 0; i < n1 * n2; i++) {
        if (res[i] != res[i]) { // nan
            return false;
        }

        auto err = std::abs(res[i] - expected_res[i]);

        if (err > 1e-5) {
            return false;
        }
    }

    return true;
}

using test_case =
  std::tuple<int, int, int, std::vector<float>, std::vector<float>, std::vector<float>>;

std::vector<test_case>
hardcoded_test_cases()
{
    return { { 2, 3, 1, { 1, 2, 3, 4, 5, 6 }, { 0.5 }, { 0.5, 1.0, 1.5, 2.0, 2.5, 3.0 } },
             { 3,
               4,
               3,
               { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1, 1.2 },
               { 0.2, 0.1, 0.7, 0.5, 0.6, 0.4, 0.9, 0.8, 0.3 },
               { 0.72, 1.43, 1.78, 1.66, 1.71, 3.08, 3.53, 2.88, 1.41, 2.14, 2.39, 1.49 } } };
}

test_case generate_random_test_case(int n1, int n2, int nw) {
    std::vector<float> inp(n1*n2);
    std::vector<float> win(nw*nw);

    cmpe492::random_fill(inp.begin(), inp.end());
    cmpe492::random_fill(win.begin(), win.end());

    std::vector<float> expected(n1*n2);

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n2; j++) {
            for (int k1 = 0; k1 < nw; k1++) {
                for (int k2 = 0; k2 < nw; k2++) {
                    
                    int ii = i + k1 - nw/2;
                    int jj = j + k2 - nw/2;

                    if (ii >= 0 && ii < n1 && jj >= 0 && jj < n2) {
                        expected[i*n2 + j] += inp[ ii*n2 + jj ] * win[k1*nw + k2];
                    }

                }
            }
        }
    }

    return {n1, n2, nw, inp, win, expected};
}

auto random_test_sizes() {
    std::vector<std::tuple<int, int, int>> sizes;

    int nws[] = {1, 3, 5, 7, 11, 23};
    int nbases[] = {1, 50, 100};

    for (int nw: nws) {
        for (int nbase1: nbases) {
            for (int nbase2: nbases) {

                for (int i = 0; i < 4; i++) {
                    int n1 = nbase1 + i;
                    for (int j = 0; j < 4; j++) {
                        int n2 = nbase2 + j;

                        sizes.emplace_back(n1, n2, nw);
                    }
                }

            }
        }
    }

    return sizes;
}

int
main(int argc, char* argv[])
{
    if (argc == 4) {
        int n1 = std::atoi(argv[1]);
        int n2 = std::atoi(argv[2]);
        int nw = std::atoi(argv[3]);

        auto [_n1, _n2, _nw, inp, win, expected_res] = generate_random_test_case(n1, n2, nw);

        bool ok = test_conv(n1, n2, nw, inp, win, expected_res);

        std::cout << (ok ? "ok" : "failed") << std::endl;

        return 0;
    }
    
    std::vector<test_case> cases = hardcoded_test_cases();

    for (auto [n1, n2, nw, inp, win, expected_res] : cases) {
        std::cout << n1 << " " << n2 << " " << nw << " " << std::flush;

        bool ok = test_conv(n1, n2, nw, inp, win, expected_res);

        std::cout << (ok ? "ok" : "failed") << std::endl;
    }

    auto sizes = random_test_sizes();

    bool ever_failed = false;

    for (auto [n1, n2, nw]: sizes) {
        std::cout << n1 << " " << n2 << " " << nw << " " << std::flush;

        auto [_n1, _n2, _nw, inp, win, expected_res] = generate_random_test_case(n1, n2, nw);

        bool ok = test_conv(n1, n2, nw, inp, win, expected_res);

        std::cout << (ok ? "[ok]" : "[failed]") << std::endl;

        if (!ok) {
            ever_failed = true;
        }
    }

    if (ever_failed) {
        std::cout << "There were failing tests!" << std::endl;
    }

    return 0;
}