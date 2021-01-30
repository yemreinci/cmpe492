/// used to calculate the FMA instruction's throughput

#include <iostream>

#include "timer.hpp"

int main()
{
    constexpr int n = 1'000'000'000;

    std::cout << n << " FMA ops takes " << std::flush;
    {
        cmpe492::timer _{ std::cout };

        for (int i = 0; i < n / 5; i++) {
            asm volatile ("vfmadd231ps %ymm0, %ymm1, %ymm2");
            asm volatile ("vfmadd231ps %ymm3, %ymm4, %ymm5");
            asm volatile ("vfmadd231ps %ymm6, %ymm7, %ymm8");
            asm volatile ("vfmadd231ps %ymm9, %ymm10, %ymm11");
            asm volatile ("vfmadd231ps %ymm12, %ymm13, %ymm14");
        }
    }

    return 0;
}