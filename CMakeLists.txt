cmake_minimum_required(VERSION 3.20)
project(slipstream LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(slipstream_codec
        lib/libslipstream_codec.cpp
)

enable_testing()
find_package(GTest REQUIRED)

add_executable(codec_test
        tests/codec_test.cpp
        lib/libslipstream_codec.cpp
)

target_link_libraries(codec_test PRIVATE GTest::gtest_main)

foreach(target IN ITEMS slipstream_codec codec_test)
    target_compile_definitions(${target} PRIVATE
        SLIPSTREAM_CSV_PATH="${CMAKE_CURRENT_SOURCE_DIR}/data/QandT.csv"
    )
endforeach()

include(GoogleTest)
gtest_discover_tests(codec_test)
