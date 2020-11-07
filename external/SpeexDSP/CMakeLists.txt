cmake_minimum_required(VERSION 3.0.2)
project(speexdsp)

include(CheckTypeSize)
check_type_size("int16_t" SIZEOF_INT16)

set(SPEEXDSP_FOLDER ${CMAKE_CURRENT_SOURCE_DIR}/speexdsp-1.2.0)
set(SPEEXDSP_SOURCE ${SPEEXDSP_FOLDER}/libspeexdsp)

set(SIZE16 "int16_t")
set(USIZE16 "uint16_t")
set(SIZE32 "int32_t")
set(USIZE32 "uint32_t")
set(EXPORT "__attribute__((visibility(\"default\")))")
set(FLOATING_POINT 1)
set(USE_SMALLFT 1)

if(HAVE_SYS_TYPES_H)
    set(INCLUDE_STDINT "#include <sys/types.h>")
endif()
if(HAVE_STDINT_H)
    set(INCLUDE_STDINT "#include <stdint.h>")
endif()
if(HAVE_STDDEF_H)
    set(INCLUDE_STDINT "#include <inttypes.h>")
endif()

configure_file(
        ${SPEEXDSP_FOLDER}/include/speex/speexdsp_config_types.h.in
        ${CMAKE_CURRENT_BINARY_DIR}/speexdsp_config_types.h
)

configure_file(config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)

add_library(speexdsp STATIC
        ${SPEEXDSP_SOURCE}/resample.c
        ${SPEEXDSP_SOURCE}/buffer.c
        ${SPEEXDSP_SOURCE}/fftwrap.c
        ${SPEEXDSP_SOURCE}/filterbank.c
        ${SPEEXDSP_SOURCE}/jitter.c
        ${SPEEXDSP_SOURCE}/kiss_fft.c
        ${SPEEXDSP_SOURCE}/kiss_fftr.c
        ${SPEEXDSP_SOURCE}/mdf.c
        ${SPEEXDSP_SOURCE}/preprocess.c
        ${SPEEXDSP_SOURCE}/scal.c
        ${SPEEXDSP_SOURCE}/smallft.c
)
target_include_directories(speexdsp
    PUBLIC
        ${SPEEXDSP_FOLDER}/include
        ${CMAKE_CURRENT_BINARY_DIR}
    PRIVATE
        ${SPEEXDSP_FOLDER}
        ${SPEEXDSP_SOURCE}
        ${SPEEXDSP_FOLDER}/include
        ${SPEEXDSP_FOLDER}/include/speex
)
target_compile_options(speexdsp PRIVATE "-DHAVE_CONFIG_H")
