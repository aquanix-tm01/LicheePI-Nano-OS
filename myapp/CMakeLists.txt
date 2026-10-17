cmake_minimum_required(VERSION 3.12)
project(myapp C)
set(CMAKE_C_STANDARD 11)

set(LV_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE STRING "" FORCE)
set(LV_CONF_BUILD_DISABLE_EXAMPLES ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_DEMOS ON CACHE BOOL "" FORCE)
set(LV_CONF_BUILD_DISABLE_THORVG_INTERNAL ON CACHE BOOL "" FORCE)
add_subdirectory(lvgl)

file(GLOB APP_SRC CONFIGURE_DEPENDS src/*.c)
add_executable(myapp ${APP_SRC})
target_link_libraries(myapp PRIVATE lvgl m)
install(TARGETS myapp RUNTIME DESTINATION bin)
