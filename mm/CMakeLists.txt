file(GLOB files "mm*.cpp")

foreach(filename ${files})
    string(REGEX REPLACE "^.*mm([0-9]+).cpp$" "\\1" mmid ${filename})

    add_library("mm${mmid}" OBJECT ${filename})

    add_executable("test-mm${mmid}" test.cpp $<TARGET_OBJECTS:mm${mmid}>)

    add_executable("bench-mm${mmid}" bench.cpp $<TARGET_OBJECTS:mm${mmid}>)

    add_custom_target("mm${mmid}-asm"
                      ALL
                      ${CMAKE_OBJDUMP} -d $<TARGET_OBJECTS:mm${mmid}> > ${CMAKE_CURRENT_BINARY_DIR}/mm${mmid}.s)

    add_dependencies("mm${mmid}-assembly" "mm${mmid}")
endforeach()