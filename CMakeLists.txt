cmake_minimum_required(VERSION 3.22)

project(OmniLayer C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# 方便 VS Code/clangd 索引。
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 统一输出目录，便于管理产物。
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/artifacts)

set(MCU_TARGET "AUTO" CACHE STRING "Target MCU family: AUTO/F103/F407")
set_property(CACHE MCU_TARGET PROPERTY STRINGS AUTO F103 F407)

# AUTO 模式下，从 Enroll.h 的默认宏读取当前目标 MCU。
if(MCU_TARGET STREQUAL "AUTO")
    file(READ "${CMAKE_SOURCE_DIR}/Enroll/Enroll.h" ENROLL_HEADER_CONTENT)
    string(REGEX MATCH "#define[ \t]+ENROLL_MCU_TARGET[ \t]+ENROLL_MCU_F407" ENROLL_TARGET_IS_F407 "${ENROLL_HEADER_CONTENT}")
    string(REGEX MATCH "#define[ \t]+ENROLL_MCU_TARGET[ \t]+ENROLL_MCU_F103" ENROLL_TARGET_IS_F103 "${ENROLL_HEADER_CONTENT}")

    if(ENROLL_TARGET_IS_F407)
        set(RESOLVED_MCU_TARGET "F407")
    elseif(ENROLL_TARGET_IS_F103)
        set(RESOLVED_MCU_TARGET "F103")
    else()
        message(FATAL_ERROR "Cannot detect ENROLL_MCU_TARGET from Enroll/Enroll.h")
    endif()
else()
    set(RESOLVED_MCU_TARGET "${MCU_TARGET}")
endif()

message(STATUS "Resolved MCU target: ${RESOLVED_MCU_TARGET}")

# 应用层（app）：业务入口与任务调度。
set(APP_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/app/main.c
)

# 注册层（Enroll）：把 BSP 资源和具体 MCU 驱动绑定。
set(ENROLL_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/Enroll/Enroll.c
)

# 板级支持层（BSP）：对传感器/执行器做硬件抽象。
set(BSP_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/BSP/LED/LED.c
)

if(RESOLVED_MCU_TARGET STREQUAL "F103")
    set(TARGET_NAME OmniLayer_F103)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Core/STM32F103/cmake/STM32F103XX_FLASH.ld)
    set(OPENOCD_CFG ${CMAKE_SOURCE_DIR}/oepnocd/f103_openocd.cfg)
    set(OPENOCD_ERASE_CMD "stm32f1x mass_erase 0")
    set(MCU_CPU_FLAGS -mcpu=cortex-m3 -mthumb)
    set(MCU_FPU_FLAGS)
    set(MCU_DEFINITIONS
        STM32F10X_MD
    )
    set(MCU_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_SOURCE_DIR}/Enroll
        ${CMAKE_SOURCE_DIR}/BSP/LED
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/inc
        ${CMAKE_SOURCE_DIR}/SYSTEM/F103_SYS
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/FWLIB/inc
    )
    set(MCU_SOURCES
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_gpio.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start/system_stm32f10x.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start/stm32f10x_it.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start/startup_stm32f103c8tx_gcc.c
    )
elseif(RESOLVED_MCU_TARGET STREQUAL "F407")
    set(TARGET_NAME OmniLayer_F407)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Core/STM32F407/cmake/STM32F407VE_FLASH.ld)
    set(OPENOCD_CFG ${CMAKE_SOURCE_DIR}/oepnocd/f407_oepnocd.cfg)
    set(OPENOCD_ERASE_CMD "stm32f4x mass_erase 0")
    set(MCU_CPU_FLAGS -mcpu=cortex-m4 -mthumb)
    set(MCU_FPU_FLAGS -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
    set(MCU_DEFINITIONS
        STM32F40_41xxx
    )
    set(MCU_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_SOURCE_DIR}/Enroll
        ${CMAKE_SOURCE_DIR}/BSP/LED
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/inc
        ${CMAKE_SOURCE_DIR}/SYSTEM/F407_SYS
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/FWLIB/inc
    )
    set(MCU_SOURCES
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_gpio.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start/system_stm32f4xx.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start/stm32f4xx_it.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start/startup_stm32f407vetx_gcc.c
    )
else()
    message(FATAL_ERROR "Unsupported MCU target: ${RESOLVED_MCU_TARGET}")
endif()

set(TARGET_SOURCES
    ${APP_LAYER_SOURCES}
    ${ENROLL_LAYER_SOURCES}
    ${BSP_LAYER_SOURCES}
    ${MCU_SOURCES}
)

add_executable(${TARGET_NAME} ${TARGET_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
    ${MCU_INCLUDE_DIRS}
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    ${MCU_DEFINITIONS}
)

target_compile_options(${TARGET_NAME} PRIVATE
    ${MCU_CPU_FLAGS}
    ${MCU_FPU_FLAGS}
    -Wall
    -Wextra
)

target_link_options(${TARGET_NAME} PRIVATE
    ${MCU_CPU_FLAGS}
    ${MCU_FPU_FLAGS}
    -T${LINKER_SCRIPT}
    --specs=nosys.specs
    --specs=nano.specs
    -Wl,--gc-sections
    -Wl,-Map=${TARGET_NAME}.map
    -Wl,--print-memory-usage
)

target_link_libraries(${TARGET_NAME} PRIVATE
    m
    c
)

# 生成常用烧录文件格式。
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMENT "Generate .hex/.bin and print size"
)

# OpenOCD 烧录：按 MCU 选择对应配置文件。
add_custom_target(flash
    COMMAND openocd -f ${OPENOCD_CFG} -c "program $<TARGET_FILE:${TARGET_NAME}> verify reset exit"
    DEPENDS ${TARGET_NAME}
    COMMENT "Flash firmware via OpenOCD"
)

add_custom_target(erase
    COMMAND openocd -f ${OPENOCD_CFG} -c "init; reset halt; ${OPENOCD_ERASE_CMD}; reset run; exit"
    COMMENT "Mass erase firmware via OpenOCD"
)
