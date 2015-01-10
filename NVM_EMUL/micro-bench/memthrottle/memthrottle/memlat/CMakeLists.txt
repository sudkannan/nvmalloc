find_package(Numa) # numa library
IF(NUMA_FOUND)
    add_executable(memlat8 memlat.c)
    set_target_properties (memlat8 PROPERTIES COMPILE_DEFINITIONS "ELEMSZ=8")
    target_link_libraries(memlat8 numa)

    add_executable(memlat64 memlat.c)
    set_target_properties (memlat64 PROPERTIES COMPILE_DEFINITIONS "ELEMSZ=64")
    target_link_libraries(memlat64 numa)

    add_executable(memlat8192 memlat.c)
    set_target_properties (memlat8192 PROPERTIES COMPILE_DEFINITIONS "ELEMSZ=8192")
    target_link_libraries(memlat8192 numa)
ENDIF()
