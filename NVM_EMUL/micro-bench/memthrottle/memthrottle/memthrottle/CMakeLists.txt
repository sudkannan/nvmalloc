add_executable(memthrottle main.cc memcpy.cc model.cc)
target_link_libraries(memthrottle pthread)
