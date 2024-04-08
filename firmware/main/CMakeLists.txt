file(
    GLOB_RECURSE files
        *.h
        *.cpp
        *.c
)

set(dependencies
        freertos
        esp_system
        arduino-esp32
        fmt
        ArduinoJson
        espchrono
        cpputils
        espcpputils
        FastLED-idf
        espconfiglib
        espwifistack
        sunset-idf
        esphttpdutils
        esp-idf-bmx280
        mdns
        mqtt
        esp_app_format
        espasyncota
        app_update
)

idf_component_register(
    SRCS
        ${files}
    INCLUDE_DIRS
        .
    REQUIRES
        ${dependencies}
)

file(
    GLOB_RECURSE webapp_files
        ${COMPONENT_DIR}/../webapp/src/*
)

add_custom_command(OUTPUT communication/webserver_files.h
        COMMAND ${COMPONENT_DIR}/../webapp/tools/web_codegen.js
        WORKING_DIRECTORY ${COMPONENT_DIR}/../
        DEPENDS ${COMPONENT_DIR}/../webapp/tools/web_codegen.js ${webapp_files}
        VERBATIM)

add_custom_target(webserver_files DEPENDS communication/webserver_files.h)
add_dependencies(${COMPONENT_LIB} webserver_files)

set_property(DIRECTORY "${COMPONENT_DIR}" APPEND PROPERTY
        ADDITIONAL_CLEAN_FILES "communication/webserver_files.h")

execute_process(
    COMMAND git describe --tags --always --dirty --long
    WORKING_DIRECTORY ${COMPONENT_DIR}
    OUTPUT_VARIABLE GIT_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

message(STATUS "Git version: ${GIT_VERSION}")

target_compile_options(${COMPONENT_TARGET}
    PRIVATE
        -fstack-reuse=all
        -fstack-protector-all
        -fdiagnostics-color=always
        -Wno-unused-function
        -Wno-deprecated-declarations
        -Wno-missing-field-initializers
        -Wno-parentheses
        -DVERSION=\"${GIT_VERSION}\"
)
