#include "generator.hpp"

#include <iostream>

template<typename ValueT>
void
test(std::string_view type_name)
{
    {
        std::vector<ValueT> v(10);
        cmpe492::random_fill(v.begin(), v.end());

        std::cout << type_name << ":";
        for (int i = 0; i < 10; i++) {
            std::cout << " " << v[i];
        }
        std::cout << std::endl;
    }
}

int
main()
{
    test<int>("int");
    test<long long>("long long");
    test<float>("float");
    test<double>("double");
    // test<std::string>("string"); // compile error
    return 0;
}