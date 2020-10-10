#include <chrono>
#include <cstdio>
#include <ostream>
#include <utility>

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
        
        auto old_prec = os_.precision(3);
        os_ << secs << " s\n";
        os_.precision(old_prec);
    }
};

} // namespace cmpe492