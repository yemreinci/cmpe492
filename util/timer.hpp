#include <chrono>
#include <cstdio>
#include <ostream>
#include <utility>
#include <iomanip>

namespace cmpe492 {

class timer
{
    using clock = std::chrono::high_resolution_clock;
    using tp = decltype(clock::now());

    std::ostream& os_;
    tp start_;

public:
    timer(std::ostream& os)
      : os_(os)
    {
        start_ = clock::now();
    }

    ~timer()
    {
        tp::duration dur = clock::now() - start_;
        double secs = std::chrono::duration<double>(dur).count();

        os_ << std::setprecision(3) << std::fixed << secs << " s\n";
    }
};

} // namespace cmpe492