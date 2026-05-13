cmake_minimum_required(VERSION 3.22.1)

project("Yrener")
set(CMAKE_C_VISIBILITY_PRESET hidden) # Скрыть все по умолчанию
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(IMGUI_SOURCES
        wLibs/ImGui/imgui.cpp
        wLibs/ImGui/imgui_draw.cpp
        wLibs/ImGui/imgui_tables.cpp
        wLibs/ImGui/imgui_widgets.cpp
        wLibs/ImGui/backends/imgui_impl_opengl3.cpp
)



add_library(${CMAKE_PROJECT_NAME} SHARED
        main.cpp
        func/bools.cpp
        wLibs/iconvlite.cpp

        ${IMGUI_SOURCES}

)
find_library(EGL_LIBRARY EGL)
find_library(GLES3_LIBRARY GLESv3)

target_link_libraries(${CMAKE_PROJECT_NAME}
        ${EGL_LIBRARY}
        ${GLES3_LIBRARY}
        android
        log
        GLESv3
        dobby
        z
        curl
)


set(DOBBY_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/wLibs/libdobby.a")
if(EXISTS ${DOBBY_LIB_PATH})
    add_library(dobby STATIC IMPORTED)
    set_target_properties(dobby PROPERTIES
            IMPORTED_LOCATION "${DOBBY_LIB_PATH}"
    )
else()
    message(WARNING "Dobby library not found: ${DOBBY_LIB_PATH}")
endif()

# Импорт cURL
set(CURL_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/wLibs/curl/libcurl.a")
if(EXISTS ${CURL_LIB_PATH})
    add_library(curl STATIC IMPORTED)
    set_target_properties(curl PROPERTIES
            IMPORTED_LOCATION "${CURL_LIB_PATH}"
    )
else()
    message(WARNING "cURL library not found: ${CURL_LIB_PATH}")
endif()

# Включаем заголовки
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/wLibs/memory/Dobby"
        "${CMAKE_CURRENT_SOURCE_DIR}/wLibs/curl"
)
