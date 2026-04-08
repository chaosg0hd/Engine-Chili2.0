add_library(engine_core
    ../src/core/engine_core.cpp
    ../src/core/module_manager.cpp
    ../src/modules/diagnostics/diagnostics_module.cpp
    ../src/modules/file/file_module.cpp
    ../src/modules/gpu/gpu_module.cpp
    ../src/modules/gpu/gpu_compute_module.cpp
    ../src/modules/input/input_module.cpp
    ../src/modules/jobs/job_module.cpp
    ../src/modules/logger/logger_module.cpp
    ../src/modules/memory/memory_module.cpp
    ../src/modules/native_ui/native_button_host.cpp
    ../src/modules/native_ui/native_ui_module.cpp
    ../src/modules/platform/platform_module.cpp
    ../src/modules/platform/platform_window.cpp
    ../src/modules/render/render_module.cpp
    ../src/modules/resources/resource_module.cpp
    ../src/modules/render/render_backend_factory.cpp
    ../src/modules/render/backend/dx11_render_backend.cpp
    ../src/modules/render/backend/null_render_backend.cpp
    ../src/modules/timer/timer_module.cpp
    ../src/modules/webview/web_dialog_host.cpp
    ../src/modules/webview/webview_module.cpp
    ../src/core/engine_context.hpp
    ../src/core/engine_core.hpp
    ../src/core/module.hpp
    ../src/core/module_manager.hpp
    ../src/modules/diagnostics/diagnostics_module.hpp
    ../src/modules/file/file_module.hpp
    ../src/modules/gpu/gpu_module.hpp
    ../src/modules/gpu/gpu_compute_module.hpp
    ../src/modules/gpu/igpu_service.hpp
    ../src/modules/input/input_module.hpp
    ../src/modules/jobs/job_module.hpp
    ../src/modules/logger/logger_module.hpp
    ../src/modules/memory/memory_module.hpp
    ../src/modules/native_ui/native_button_host.hpp
    ../src/modules/native_ui/native_ui_module.hpp
    ../src/modules/platform/platform_module.hpp
    ../src/modules/platform/platform_window.hpp
    ../src/modules/render/render_module.hpp
    ../src/modules/resources/resource_module.hpp
    ../src/modules/render/render_types.hpp
    ../src/modules/render/render_frame_context.hpp
    ../src/modules/render/render_backend_factory.hpp
    ../src/modules/render/backend/dx11_render_backend.hpp
    ../src/modules/render/backend/irender_backend.hpp
    ../src/modules/render/backend/null_render_backend.hpp
    ../src/modules/render/scene/render_camera.hpp
    ../src/modules/render/scene/render_item.hpp
    ../src/modules/render/scene/render_scene.hpp
    ../src/modules/timer/timer_module.hpp
    ../src/modules/webview/web_dialog_host.hpp
    ../src/modules/webview/webview_module.hpp
)

target_include_directories(engine_core
    PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)

target_compile_features(engine_core PUBLIC cxx_std_17)
apply_project_warnings(engine_core)
enable_project_sanitizers(engine_core)

if(WIN32)
    target_link_libraries(engine_core
        PUBLIC
            ole32
            d3d11
            dxgi
    )
endif()

if(MINGW)
    target_link_libraries(engine_core
        PUBLIC
            gdi32
            winmm
    )
endif()

set_target_properties(engine_core PROPERTIES
    FOLDER "core"
)
