set(TARGET sandwich-tui)

add_executable(${TARGET}
    main.cpp
    strata_tui.cpp
)

target_include_directories(${TARGET} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/src/dispatch
    ${PROJECT_SOURCE_DIR}/tools/cli
    ${PROJECT_SOURCE_DIR}/tools/server
    ${PROJECT_SOURCE_DIR}/common
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/ggml/include
)

target_link_libraries(${TARGET} PRIVATE
    sandwich-dispatch
    llama-cli-impl
    llama-common
    llama
    ggml
    ${CMAKE_THREAD_LIBS_INIT}
)

if (GGML_USE_CUDA)
    target_include_directories(${TARGET} PRIVATE /opt/cuda/include /opt/cuda/targets/x86_64-linux/include)
    target_link_libraries(${TARGET} PRIVATE ggml-cuda)
endif()

target_compile_features(${TARGET} PRIVATE cxx_std_17)

if (LLAMA_TOOLS_INSTALL)
    install(TARGETS ${TARGET} RUNTIME)
endif()
