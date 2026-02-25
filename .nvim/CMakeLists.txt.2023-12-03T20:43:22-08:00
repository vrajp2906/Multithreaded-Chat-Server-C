cmake_minimum_required(VERSION 3.10)

project(a12)

set(CMAKE_C_STANDARD 99)

include_directories(include)

add_executable(server src/server.c)

add_executable(client src/client.c)

file(GLOB_RECURSE HEADER_FILES "include/*.h")

#target_source(server PRIVATE ${HEADER_FILES})
#target_source(client PRIVATE ${HEADER_FILES})

find_package(Threads REQUIRED)
if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(PUBLIC server "-pthread")
  target_compile_options(PUBLIC client "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INITT)
  target_link_libraries(server "${CMAKE_THREAD_LIBS_INIT}")
  target_link_libraries(client "${CMAKE_THREAD_LIBS_INIT}")
endif()
