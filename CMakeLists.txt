project (minisat-js-ffi)
include_directories(${CMAKE_JS_INC})
file(GLOB SOURCE_FILES "src/*.cc")
add_subdirectory(minisat EXCLUDE_FROM_ALL)
include_directories(${CMAKE_SOURCE_DIR}/minisat)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${CMAKE_JS_SRC})
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_libraries(${PROJECT_NAME} ${CMAKE_JS_LIB} minisat-lib-static)

