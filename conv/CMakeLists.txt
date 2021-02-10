
function(add_test_and_bench file)
    add_executable(test-${file} test.cpp ${file}.cpp)
    add_executable(bench-${file} bench.cpp ${file}.cpp)
endfunction()

add_test_and_bench("conv_base")
add_test_and_bench("conv_unroll")
add_test_and_bench("conv_simd")
add_test_and_bench("conv_simd_mt")

add_executable(test-conv_fma test.cpp conv_simd_mt.cpp)
add_executable(bench-conv_fma bench.cpp conv_simd_mt.cpp)
set_target_properties(
    test-conv_fma bench-conv_fma
    PROPERTIES 
        COMPILE_OPTIONS -ffast-math        
)