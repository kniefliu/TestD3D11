# CMake root file for TestD3D11 related apps
# Date		:  2019-11-14
# Author  	:  kniefliu (kniefliu@163.com)
#
# 	CMake files for Win32 API Example
# 	file for each app is under the related sub-dir
#

cmake_minimum_required(VERSION 2.8)

project(TestD3D11)

message(STATUS, "CMake project files for TestD3D11")

# _MBCS or (UNICODE _UNICODE)
add_definitions(-D_UNICODE -DUNICODE)

# add each CMake file
add_subdirectory(D3D11MultiWindow)
