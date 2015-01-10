project(nvmemul)

set(nvmemul_src
    config.c
    debug.c
    dev.c
    init.c
    interpose.c
    measure_bw.c
    monotonic_timer.c
    model_bw.c
    model_lat.c
    pmalloc.c
    stat.c
    thread.c
    topology.c
)

include_directories(${CMAKE_SOURCE_DIR}/third_party)
include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${CMAKE_SOURCE_DIR}/src/lib)
add_definitions(-fPIC)
add_definitions(-march=native)
add_definitions(-fopenmp)
#add_definitions(-std=c99)
add_subdirectory(cpu)
add_library(nvmemul SHARED ${nvmemul_src} $<TARGET_OBJECTS:cpu>)
target_link_libraries(nvmemul dl)
target_link_libraries(nvmemul config)
target_link_libraries(nvmemul numa)
target_link_libraries(nvmemul rt)
target_link_libraries(nvmemul gomp)
