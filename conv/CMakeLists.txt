file(GLOB files "conv*.cpp")

foreach(filename ${files})
    string(REGEX REPLACE "^.*conv([0-9]+).cpp$" "\\1" id ${filename})
    add_executable("test-conv${id}" test.cpp ${filename})
    add_executable("bench-conv${id}" bench.cpp ${filename})
endforeach()