cmake_minimum_required(VERSION 3.12)

# ---------- 交叉工具链路径 ----------
# 设置交叉编译器（完整路径）
set(CMAKE_C_COMPILER   "/home/wz/usr/local/arm/5.4.0/usr/bin/arm-linux-gcc")
set(CMAKE_CXX_COMPILER "/home/wz/usr/local/arm/5.4.0/usr/bin/arm-linux-g++")

set(CMAKE_C_COMPILER_WORKS 1)
set(CMAKE_CXX_COMPILER_WORKS 1)

project(lvgl_fb LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 可执行文件放到项目根目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

# ===================== 源文件 =====================
file(GLOB_RECURSE SOURCES
    "main.c"
    "cJSON.c"
    "my_font_*.c"
    "lvgl/src/**.c"
    "lv_drivers/**.c"
    "UIManager/**.c"
    "PatientManager/**.c"
)

# 排除build和tmp目录中的文件
list(FILTER SOURCES EXCLUDE REGEX ".*(build|tmp).*")

list(APPEND SOURCES my_font_cn_48.c my_font_cn.c mouse_cursor_icon.c)

add_executable(${PROJECT_NAME} ${SOURCES})

# ===================== 头文件路径 =====================
target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/lvgl
    ${PROJECT_SOURCE_DIR}/lv_drivers
    ${PROJECT_SOURCE_DIR}/UIManager
    ${PROJECT_SOURCE_DIR}/PatientManager
    ${PROJECT_SOURCE_DIR}/PatientManager/inc
    /home/wz/usr/local/openssl-arm/include
)

# ===================== 编译定义 =====================
target_compile_definitions(${PROJECT_NAME} PRIVATE
    LV_CONF_INCLUDE_SIMPLE
    LV_LVGL_H_INCLUDE_SIMPLE
)

# ===================== OpenSSL库路径 =====================
target_link_directories(${PROJECT_NAME} PRIVATE
    /home/wz/usr/local/openssl-arm/lib
)

target_link_libraries(${PROJECT_NAME} ssl crypto dl pthread rt)

