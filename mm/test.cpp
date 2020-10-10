#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <tuple>
#include <vector>

#include "generator.hpp"
#include "mm.hpp"

constexpr std::string_view ok = "[\033[32;1m  OK  \033[0m]";
constexpr std::string_view fail = "[\033[31;1m FAIL \033[0m]";

bool
check(int n1, int n2, int n3, float* mat1, float* mat2, float* res)
{
    constexpr long double tolerance = 1e-6;

    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n3; j++) {
            float r = res[i * n3 + j];

            if (r != r) { // nan
                return false;
            }

            long double t = 0;

            for (int k = 0; k < n2; k++) {
                t += (long double)mat1[i * n2 + k] * mat2[k * n3 + j];
            }

            long double err = t - r;
            err = err * err;

            if (err > tolerance) {
                return false;
            }
        }
    }

    return true;
}

auto
get_cases()
{
    using tup3 = std::tuple<int, int, int>;
    std::vector<tup3> cases;

    for (int i = 1; i <= 8; i++)
        for (int j = 1; j <= 8; j++)
            for (int k = 1; k <= 8; k++) {
                cases.emplace_back(i, j, k);

                cases.emplace_back(i + 50, j, k);
                cases.emplace_back(i, j + 50, k);
                cases.emplace_back(i, j + 50, k + 50);

                cases.emplace_back(i + 50, j + 50, k);
                cases.emplace_back(i, j + 50, k + 50);
                cases.emplace_back(i + 50, j, k + 50);

                cases.emplace_back(i + 50, j + 50, k + 50);
            }

    // sort by complexity
    sort(cases.begin(), cases.end(), [](tup3 x, tup3 y) {
        return std::get<0>(x) * std::get<1>(x) * std::get<2>(x) <
               std::get<0>(y) * std::get<1>(y) * std::get<2>(y);
    });

    return cases;
}

int
main(int argc, char* argv[])
{
    std::vector<std::tuple<int, int, int>> cases;

    if (argc == 1) {
        cases = get_cases();
    } else if (argc == 4) {
        cases.emplace_back(
          std::atoi(argv[1]), std::atoi(argv[2]), std::atoi(argv[3]));
    } else {
        std::cout << "usage: " << argv[0] << " [n1 n2 n3]" << std::endl;
        return EXIT_FAILURE;
    }

    bool ever_failed = false;

    for (auto [n1, n2, n3] : cases) {
        std::cout << std::setw(4) << n1 << " " << std::setw(4) << n2 << " "
                  << std::setw(4) << n3 << "\t\t" << std::flush;

        std::vector<float> mat1(n1 * n2);
        std::vector<float> mat2(n2 * n3);
        std::vector<float> res(n1 * n3);

        cmpe492::random_fill(mat1.begin(), mat1.end());
        cmpe492::random_fill(mat2.begin(), mat2.end());

        cmpe492::mm(n1, n2, n3, mat1.data(), mat2.data(), res.data());

        bool check_res =
          check(n1, n2, n3, mat1.data(), mat2.data(), res.data());

        if (!check_res)
            ever_failed = true;

        std::cout << (check_res ? ok : fail) << std::endl;
    }

    if (ever_failed) {
        std::cerr << "\033[31;1mSome tests has failed!\033[0m" << std::endl;
        return EXIT_FAILURE;
    }

    return 0;
}