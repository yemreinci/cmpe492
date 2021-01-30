#include "generator.hpp"

#include <iostream>

template<typename ValueT>
void
test_random_fill(std::string_view type_name)
{
    {
        std::vector<ValueT> v(10);
        cmpe492::random_fill(v.begin(), v.end());

        std::cout << type_name << ":";
        for (int i = 0; i < 10; i++) {
            std::cout << " " << v[i];
        }
        std::cout << '\n';
    }
}

int
main()
{
    test_random_fill<int>("int");
    test_random_fill<long long>("long long");
    test_random_fill<float>("float");
    test_random_fill<double>("double");
    // test<std::string>("string"); // compile error
    return 0;
}