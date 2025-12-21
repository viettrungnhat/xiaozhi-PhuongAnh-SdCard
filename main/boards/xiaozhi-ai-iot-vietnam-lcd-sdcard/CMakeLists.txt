# Build audio assets binary trước khi build firmware

find_package(Python3 REQUIRED)

# Đường dẫn các file
set(AUDIO_OPUS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/audio_opus")
set(BUILD_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/build_audio_assets.py")
set(ASSETS_OUTPUT "${CMAKE_CURRENT_SOURCE_DIR}/scripts/assets/audio_assets.bin")

# Tạo target để build audio assets
add_custom_command(
    OUTPUT ${ASSETS_OUTPUT}
    COMMAND ${Python3_EXECUTABLE} ${BUILD_SCRIPT} ${AUDIO_OPUS_DIR} ${ASSETS_OUTPUT}
    DEPENDS ${BUILD_SCRIPT}
    COMMENT "Building audio assets binary..."
    VERBATIM
)

add_custom_target(audio_assets 
    DEPENDS ${ASSETS_OUTPUT}
    COMMENT "Audio assets target"
)

# Component dependency
idf_component_register(
    SRCS ""
    INCLUDE_DIRS "."
    REQUIRES "app_update" "esp_partition" "spi_flash"
)

# Thêm dependency
add_dependencies(${COMPONENT_LIB} audio_assets)

message(STATUS "Audio assets will be built before firmware")
message(STATUS "Flash command: python scripts/flash_audio_assets.py <port>")