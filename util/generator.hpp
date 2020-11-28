#include <random>
#include <type_traits>

namespace cmpe492 {

template<typename ForwardIt>
void
random_fill(ForwardIt first, ForwardIt last)
{
    using value_t = std::remove_cv_t<std::remove_reference_t<decltype(*first)>>;
    static_assert(std::is_arithmetic_v<value_t>, "value type must be an arithmetic type");

    using distribution = std::conditional_t<std::is_integral_v<value_t>,
                                            std::uniform_int_distribution<value_t>,
                                            std::uniform_real_distribution<value_t>>;

    distribution dist;
    static std::mt19937 rng;

    for (; first != last; ++first) {
        *first = dist(rng);
    }
}

} // namespace cmpe492