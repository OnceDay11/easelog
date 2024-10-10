
# 添加源文件, 按照字母序排序
set(base_srcs
    log/easelog.cpp
    log/easelog_prefix.cpp
    log/easelog_llqueue.cpp
)

# 添加测试可执行文件, 按照字母序排序
set(test_srcs ${base_srcs}
    log/easelog_unittest.cpp
)

# 检查操作系统
if(WIN32)
    message(STATUS "Compiling on Windows")
elseif(UNIX AND NOT APPLE)
    message(STATUS "Compiling on a Unix-like system (Linux, BSD, etc.)")
elseif(APPLE)
    message(STATUS "Compiling on macOS")
else()
    message(STATUS "Unknown platform")
endif()

# 编译测试用例
add_executable(easelog-tests "${test_srcs}")

# 设置链接选项
# target_link_libraries(easelog-tests unwind unwind-generic)

# 添加头文件目录
target_include_directories(easelog-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# 导入gtest的cmake配置文件, 如果存在变量GTEST_DIR, 则使用该变量指定的路径
if(DEFINED GTEST_DIR)
  set(GTest_DIR ${GTEST_DIR})
endif()
find_package(GTest REQUIRED)
if(GTest_FOUND)
  # 链接 gtest 库
  target_link_libraries(easelog-tests GTest::gtest GTest::gtest_main)
  # 链接 gmock 库
  target_link_libraries(easelog-tests GTest::gmock)
else()
  message(FATAL_ERROR "GTest library not found")
endif()

# 导入gtest的cmake配置文件
include(GoogleTest)

# 自动发现测试用例
gtest_discover_tests(easelog-tests)

#################### 编译静态库 ####################

# 编译静态库
add_library(easelog-static STATIC ${base_srcs})

# 设置主要版本号和次要版本号
set_target_properties(easelog-static PROPERTIES
    OUTPUT_NAME "easelog")

# 编译为位置无关的代码
set_property(TARGET easelog-static PROPERTY POSITION_INDEPENDENT_CODE ON)

# 设置头文件导入路径
target_include_directories(easelog-static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

#################################################

# 添加子目录
# add_subdirectory(xxx)

# END
