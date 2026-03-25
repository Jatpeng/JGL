add_executable(test_engine_scene
    test_engine_scene.cpp
)

target_include_directories(test_engine_scene
    PUBLIC
        ${JGL_PUBLIC_INCLUDE_DIRS}
)

if(JGL_PLATFORM_INCLUDE_DIRS)
    target_include_directories(test_engine_scene PUBLIC ${JGL_PLATFORM_INCLUDE_DIRS})
endif()

target_link_libraries(test_engine_scene
    PRIVATE
        jgl_core
        ${JGL_OPENGL_LIB}
        ${JGL_PLATFORM_LIBS}
)

if(WIN32)
    target_link_libraries(test_engine_scene PRIVATE comdlg32)
endif()

set_target_properties(test_engine_scene PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
