# cmake file for D3D11MultiWindowExample
# Date		:  2019-11-14
# Author  	:  kniefliu (kniefliu@163.com)

file(GLOB SRC_Root "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
file(GLOB SRC_Resource "${CMAKE_CURRENT_SOURCE_DIR}/*.rc")
file(GLOB SRC_Resource_res "${CMAKE_CURRENT_SOURCE_DIR}/res/*.ico" "${CMAKE_CURRENT_SOURCE_DIR}/res/*.bmp")
source_group("res" FILES ${SRC_Resource_res})

add_definitions(-DD3D_RES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/res/")

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
add_executable(D3D11MultiWindowExample WIN32 ${SRC_Root} ${SRC_Resource} ${SRC_Resource_res})
target_link_libraries(D3D11MultiWindowExample d3d11 d3dcompiler dxguid winmm comctl32)

