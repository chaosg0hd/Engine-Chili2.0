function(enable_project_sanitizers target_name)
    option(ENABLE_SANITIZERS "Enable compiler sanitizers for supported toolchains." OFF)

    if(NOT ENABLE_SANITIZERS)
        return()
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /fsanitize=address /Zi)
        target_link_options(${target_name} PRIVATE /INCREMENTAL:NO /fsanitize=address)
        return()
    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
        return()
    endif()

    message(WARNING "ENABLE_SANITIZERS is ON, but the current compiler does not have a configured sanitizer setup.")
endfunction()
