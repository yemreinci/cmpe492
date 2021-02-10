function(add_test_and_bench file)
    add_executable(test-${file} test.cpp ${file}.cpp)
    add_executable(bench-${file} bench.cpp ${file}.cpp)
endfunction()

add_test_and_bench("mm_base")
add_test_and_bench("mm_linit")
add_test_and_bench("mm_unroll")
add_test_and_bench("mm_simd1")
add_test_and_bench("mm_simd2")
add_test_and_bench("mm_simd2_mt")

# mm_simd2_mt.cpp compiled with -ffast-math is called mm_fma
add_executable(test-mm_fma test.cpp mm_simd2_mt.cpp)
add_executable(bench-mm_fma bench.cpp mm_simd2_mt.cpp)
set_target_properties(
    test-mm_fma bench-mm_fma
    PROPERTIES 
        COMPILE_OPTIONS -ffast-math        
)

find_package(CBLAS)
if(CBLAS_FOUND)
    add_executable(test-mm_blas test.cpp mm_blas.cpp)
    add_executable(bench-mm_blas bench.cpp mm_blas.cpp)
    target_include_directories(
        test-mm_blas PRIVATE ${CBLAS_INCLUDE_DIRS}
    )
    target_include_directories(
        bench-mm_blas PRIVATE ${CBLAS_INCLUDE_DIRS}
    )
    target_link_libraries(test-mm_blas PRIVATE ${CBLAS_LIBRARIES})
    target_link_libraries(bench-mm_blas PRIVATE ${CBLAS_LIBRARIES})
endif()