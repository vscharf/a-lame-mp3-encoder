cmake_minimum_required(VERSION 2.8)
project (a-lame-mp3-encoder)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(LAME_INCDIR "Lame include directory" "")

# compiler options
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set (CMAKE_CXX_FLAGS "--std=c++11 ${CMAKE_CXX_FLAGS}")
endif ()

#message(SEND_ERROR ${LAME_INCDIR})
if(NOT ${LAME_INCDIR} STREQUAL "")
  include_directories(${LAME_INCDIR})
else()
  message(FATAL_ERROR "You need to define LAME_INCDIR")
endif()

#if(${LAME_LIBDIR})
#  link_directories(${LAME_LIBDIR})
#else()
#  message(FATAL_ERROR "You need to define LAME_LIBDIR")
#endif()

# static linking
find_library(LIBLAME libmp3lame.a)

# build binary
add_executable(a-lame-mp3-encoder
		 ${CMAKE_CURRENT_SOURCE_DIR}/src/wavdecoder.cpp
		 ${CMAKE_CURRENT_SOURCE_DIR}/src/directory.cpp
		 ${CMAKE_CURRENT_SOURCE_DIR}/src/mp3encoder.cpp
		 ${CMAKE_CURRENT_SOURCE_DIR}/src/batch-encoder.cpp)
target_link_libraries(a-lame-mp3-encoder ${LIBLAME} pthread)

# build tests
add_executable(dir_test ${CMAKE_CURRENT_SOURCE_DIR}/src/directory.cpp)
target_compile_definitions(dir_test PRIVATE TEST_DIR)
add_executable(wav_test ${CMAKE_CURRENT_SOURCE_DIR}/src/wavdecoder.cpp)
target_compile_definitions(wav_test PRIVATE TEST_WAV)
add_executable(enc_test ${CMAKE_CURRENT_SOURCE_DIR}/src/mp3encoder.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/wavdecoder.cpp)
target_compile_definitions(enc_test PRIVATE TEST_ENC)
target_link_libraries(enc_test ${LIBLAME})
