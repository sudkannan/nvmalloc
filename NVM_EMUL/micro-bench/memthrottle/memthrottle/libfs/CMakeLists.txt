set(fs_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/fs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/model.cpp)

add_library(fs SHARED ${fs_SRCS})
target_link_libraries(fs dl)
