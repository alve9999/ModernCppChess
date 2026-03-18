cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_COMPILER "clang++")
project(chess2000)
set(CMAKE_CXX_STANDARD 23)

include(CheckCXXCompilerFlag)

if(NOT DEFINED SIMD_LEVEL)
    file(WRITE ${CMAKE_BINARY_DIR}/check_avx512.c
    "#include <immintrin.h>\n"
    "int main(void) {\n"
    "    __m512i a = _mm512_set1_epi32(1);\n"
    "    __m512i b = _mm512_add_epi32(a, a);\n"
    "    return _mm512_reduce_add_epi32(b);\n"
    "}\n"
    )
    try_run(AVX512_RUN AVX512_COMPILE ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/check_avx512.c
        COMPILE_DEFINITIONS "-mavx512f -mavx512bw"
    )

    file(WRITE ${CMAKE_BINARY_DIR}/check_avx2.c
    "#include <immintrin.h>\n"
    "int main(void) {\n"
    "    __m256i a = _mm256_set1_epi32(1);\n"
    "    __m256i b = _mm256_mullo_epi32(a, a);\n"
    "    return _mm256_extract_epi32(b, 0);\n"
    "}\n"
    )
    try_run(AVX2_RUN AVX2_COMPILE ${CMAKE_BINARY_DIR}
        ${CMAKE_BINARY_DIR}/check_avx2.c
        COMPILE_DEFINITIONS "-mavx2"
    )

    if(AVX512_COMPILE AND AVX512_RUN EQUAL 0)
        set(SIMD_LEVEL "AVX512" CACHE STRING "Detected SIMD level" FORCE)
    elseif(AVX2_COMPILE AND AVX2_RUN EQUAL 0)
        set(SIMD_LEVEL "AVX2" CACHE STRING "Detected SIMD level" FORCE)
    else()
        set(SIMD_LEVEL "SCALAR" CACHE STRING "Detected SIMD level" FORCE)
    endif()
endif()

add_executable(chess2000
    main.cpp
    sliding.cpp
    uci.cpp
    hash.cpp
    SEE.cpp
    parameter.cpp
    nnue.cpp
    board.hpp
    check.hpp
    pawns.hpp
    constants.hpp
    movegen.hpp
    move.hpp
    eval.hpp
    ai.hpp
    minimax_info.hpp
)

target_precompile_headers(chess2000 PRIVATE pch.h)

target_compile_options(chess2000 PRIVATE
    -Wno-narrowing
    -O3
)

if(SIMD_LEVEL STREQUAL "AVX512")
    target_compile_options(chess2000 PRIVATE -mavx512f -mavx512bw)
    message(STATUS "SIMD: AVX-512 (F+BW) enabled")
elseif(SIMD_LEVEL STREQUAL "AVX2")
    target_compile_options(chess2000 PRIVATE -mavx2)
    message(STATUS "SIMD: AVX2 enabled")
else()
    message(STATUS "SIMD: scalar fallback")
endif()
