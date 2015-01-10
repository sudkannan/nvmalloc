include_directories(${CMAKE_SOURCE_DIR}/third_party/gtest-1.7.0/include)
include_directories(${CMAKE_SOURCE_DIR}/src/lib)
add_executable(test_interpose ${CMAKE_CURRENT_SOURCE_DIR}/test_interpose.cc)
target_link_libraries(test_interpose pthread gtest)

add_executable(test_dev ${CMAKE_CURRENT_SOURCE_DIR}/test_dev.cc)
target_link_libraries(test_dev pthread nvmemul)

add_test(NAME interpose COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_interpose)

set(ENV_COMMON "LD_PRELOAD=${CMAKE_BINARY_DIR}/src/emul/libnvmemul.so")

SET_PROPERTY(TEST interpose PROPERTY ENVIRONMENT ${ENV_COMMON} "ENUM_INI=emul.ini")
