cmake_minimum_required(VERSION 2.6)

project(sars C CXX)

set(CMAKE_CXX_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()


set(CXX_FLAGS
 -g
 # -DVALGRIND
 -DCHECK_PTHREAD_RETURN_VALUE
 -D_FILE_OFFSET_BITS=64
 -Wall
 -Wextra
#  -Werror
 -Wconversion
 -Wno-unused-parameter
#  -Wold-style-cast
 -Woverloaded-virtual
 -Wpointer-arith
 -Wshadow
 -Wwrite-strings
 -march=native
 # -MMD
 -std=c++11
 -rdynamic
#  -fPIC
 )
if(CMAKE_BUILD_BITS EQUAL 32)
  list(APPEND CXX_FLAGS "-m32")
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  list(APPEND CXX_FLAGS "-Wno-null-dereference")
  list(APPEND CXX_FLAGS "-Wno-sign-conversion")
  list(APPEND CXX_FLAGS "-Wno-unused-local-typedef")
  list(APPEND CXX_FLAGS "-Wthread-safety")
  list(REMOVE_ITEM CXX_FLAGS "-rdynamic")
endif()
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_FLAGS_DEBUG "-O0 -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O0 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)


string(TOUPPER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
message(STATUS "CXX_FLAGS = " ${CMAKE_CXX_FLAGS} " " ${CMAKE_CXX_FLAGS_${BUILD_TYPE}})


set(THIRDPARTY "${PROJECT_SOURCE_DIR}/thirdparty")




# webrtc-core lib
add_subdirectory(${THIRDPARTY}/webrtc-core)
include_directories(${THIRDPARTY}/webrtc-core/lib/openssl/include)
link_directories(${THIRDPARTY}/webrtc-core/lib/openssl/lib)
include_directories(${THIRDPARTY}/webrtc-core/lib/srtp/include)
link_directories(${THIRDPARTY}/webrtc-core/lib/srtp/lib)
include_directories(${THIRDPARTY}/webrtc-core/common)
link_directories(${THIRDPARTY}/webrtc-core/lib/usrsctp/lib)
include_directories(${THIRDPARTY}/webrtc-core/lib/usrsctp/include)


# coco lib
add_subdirectory(${THIRDPARTY}/coco)
include_directories(${THIRDPARTY}/coco/src)
include_directories(${THIRDPARTY}/coco/lib)
include_directories(${THIRDPARTY}/coco/lib/st/include)


# json
add_subdirectory(${THIRDPARTY}/json)

include_directories(${THIRDPARTY}/json/single_include)
include_directories(${THIRDPARTY}/webrtc-core)

# sars binary
# aux_source_directory(./src/  SRCS)
set(SRCS

  src/sars.cpp
  src/signalling.cpp
  src/rtc_server.cpp
  src/rtc_peer.cpp
  src/rtc_publisher.cpp
  src/rtc_player.cpp
  src/scache.cpp
  src/Stun.cpp
  src/Utils.cpp
)

include_directories(${PROJECT_SOURCE_DIR}/src)
add_executable(sars ${SRCS})
target_link_libraries(sars coco webrtc-core ssl crypto srtp2 usrsctp pthread dl)