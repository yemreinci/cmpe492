file(GLOB files "mm*.cpp")

foreach(filename ${files})
    string(REGEX REPLACE "^.*mm([0-9]+).cpp$" "\\1" mmid ${filename})

    add_executable("test-mm${mmid}" test.cpp ${filename})
    add_executable("bench-mm${mmid}" bench.cpp ${filename})
endforeach()