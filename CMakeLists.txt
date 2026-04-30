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

if(RESOLVED_MCU_TARGET STREQUAL "F103")
    set(ENROLL_MCU_TARGET_VALUE 0)
elseif(RESOLVED_MCU_TARGET STREQUAL "F407")
    set(ENROLL_MCU_TARGET_VALUE 1)
else()
    message(FATAL_ERROR "Unsupported resolved MCU target: ${RESOLVED_MCU_TARGET}")
endif()

# ============================== 维护说明 ==============================
#
# 1) 新增/删除 .c 文件：
#    - 按模块归类，修改 APP_LAYER_SOURCES / ENROLL_LAYER_SOURCES /
#      BSP_LAYER_SOURCES / MCU_SOURCES 中对应列表。
#    - 最终会汇总到 TARGET_SOURCES，因此只要列表里有就会参与编译。
#
# 2) 新增/删除头文件目录：
#    - 修改 MCU_INCLUDE_DIRS（不同 MCU 分支分别维护）。
#    - 仅新增 .h 文件（目录不变）通常不需要改 CMake。
#
# 3) F103/F407 差异文件：
#    - 放在 if(RESOLVED_MCU_TARGET STREQUAL "F103") / "F407" 各自分支。
#    - 避免把仅某一芯片可用的驱动误加到另一分支。
#
# 4) 改完后验证：
#    - 重新 Configure/Build；若报“undefined reference”通常是漏加 .c 文件。
#    - 若报“header not found”通常是漏加头文件目录到 MCU_INCLUDE_DIRS。
# ======================================================================

# 应用层（app）：业务入口与任务调度。
set(APP_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/app/main.c
    ${CMAKE_SOURCE_DIR}/app/Control/Control.c
    ${CMAKE_SOURCE_DIR}/app/Control_Task/Control_Task_Rtos.c
    ${CMAKE_SOURCE_DIR}/app/My_Usart/My_Usart.c
    ${CMAKE_SOURCE_DIR}/app/My_I2c/My_I2c.c
    ${CMAKE_SOURCE_DIR}/app/My_SPI/My_SPI.c
    ${CMAKE_SOURCE_DIR}/app/Control_Task/Control_Task.c
    ${CMAKE_SOURCE_DIR}/app/PID/PID.c
    ${CMAKE_SOURCE_DIR}/app/Filter/Filter.c
)

# 系统层（SYSTEM）：中断管理与板级系统封装。
set(SYS_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/SYSTEM/sys.c
)

# API 层：对上提供统一外设接口，对下分发到 MCU Core 实现。
set(API_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/API/src/adc.c
    ${CMAKE_SOURCE_DIR}/API/src/gpio.c
    ${CMAKE_SOURCE_DIR}/API/src/tim.c
    ${CMAKE_SOURCE_DIR}/API/src/usart.c
    ${CMAKE_SOURCE_DIR}/API/src/pwm.c
)

# 注册层（Enroll）：把 BSP 资源和具体 MCU 驱动绑定。
set(ENROLL_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/Enroll/Enroll.c
)

# 板级支持层（BSP）：对传感器/执行器做硬件抽象。
set(BSP_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/BSP/LED/LED.c
    ${CMAKE_SOURCE_DIR}/BSP/KEY/KEY.c
    ${CMAKE_SOURCE_DIR}/BSP/BMP280/BMP280.c
    ${CMAKE_SOURCE_DIR}/BSP/NRF24L01/NRF24L01.c
    ${CMAKE_SOURCE_DIR}/BSP/OLED/OLED.c
    ${CMAKE_SOURCE_DIR}/BSP/OLED/OLED_Data.c
    ${CMAKE_SOURCE_DIR}/BSP/QMC5883P/QMC5883P.c
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050/MPU6050.c
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050/MPU6050_Int.c
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050/eMPL/inv_mpu.c
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050/eMPL/inv_mpu_dmp_motion_driver.c
)

if(RESOLVED_MCU_TARGET STREQUAL "F103")
    set(TARGET_NAME OmniLayer_F103)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Core/STM32F103/cmake/STM32F103XX_FLASH.ld)
    set(OPENOCD_CFG ${CMAKE_SOURCE_DIR}/OpenOCD/F103_OpenOCD.cfg)
    set(OPENOCD_ERASE_CMD "stm32f1x mass_erase 0")
    set(MCU_CPU_FLAGS -mcpu=cortex-m3 -mthumb)
    set(MCU_FPU_FLAGS)
    set(MCU_DEFINITIONS
        STM32F10X_MD
    )
    set(MCU_INCLUDE_DIRS
        # 头文件搜索路径：新增目录在这里加，删除目录在这里移除。
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_SOURCE_DIR}/app/Control_Task
        ${CMAKE_SOURCE_DIR}/app/Control
        ${CMAKE_SOURCE_DIR}/app/My_Usart
        ${CMAKE_SOURCE_DIR}/app/My_I2c
        ${CMAKE_SOURCE_DIR}/app/My_SPI
        ${CMAKE_SOURCE_DIR}/app/PID
        ${CMAKE_SOURCE_DIR}/app/Filter
        ${CMAKE_SOURCE_DIR}/API
        ${CMAKE_SOURCE_DIR}/API/inc
        ${CMAKE_SOURCE_DIR}/Enroll
        ${CMAKE_SOURCE_DIR}/BSP/LED
        ${CMAKE_SOURCE_DIR}/BSP/KEY
        ${CMAKE_SOURCE_DIR}/BSP/BMP280
        ${CMAKE_SOURCE_DIR}/BSP/NRF24L01
        ${CMAKE_SOURCE_DIR}/BSP/OLED
        ${CMAKE_SOURCE_DIR}/BSP/QMC5883P
        ${CMAKE_SOURCE_DIR}/BSP/MPU6050
        ${CMAKE_SOURCE_DIR}/BSP/MPU6050/eMPL
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/inc
        ${CMAKE_SOURCE_DIR}/SYSTEM
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start
    )
    set(MCU_SOURCES
        # F103 专属源码：新增/删除 .c 文件在这里改。
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/f103_delay.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/f103_sys.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_adc.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_gpio.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_tim.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_usart.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_pwm.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start/system_stm32f10x.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start/startup_stm32f103c8tx_gcc.c
    )

    # FreeRTOS: F103 使用 Cortex-M3 移植层和独立配置。
    set(FREERTOS_PORT_DIR
        ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM3
    )
    set(FREERTOS_CONFIG_DIR
        ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-LTS/FreeRTOSConfig/F103
    )
elseif(RESOLVED_MCU_TARGET STREQUAL "F407")
    set(TARGET_NAME OmniLayer_F407)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Core/STM32F407/cmake/STM32F407VE_FLASH.ld)
    set(OPENOCD_CFG ${CMAKE_SOURCE_DIR}/OpenOCD/F407_OpenOCD.cfg)
    set(OPENOCD_ERASE_CMD "stm32f4x mass_erase 0")
    set(MCU_CPU_FLAGS -mcpu=cortex-m4 -mthumb)
    set(MCU_FPU_FLAGS -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
    set(MCU_DEFINITIONS
        STM32F40_41xxx
    )
    set(MCU_INCLUDE_DIRS
        # 头文件搜索路径：新增目录在这里加，删除目录在这里移除。
        ${CMAKE_SOURCE_DIR}/app
        ${CMAKE_SOURCE_DIR}/app/Control_Task
        ${CMAKE_SOURCE_DIR}/app/Control
        ${CMAKE_SOURCE_DIR}/app/My_Usart
        ${CMAKE_SOURCE_DIR}/app/My_I2c
        ${CMAKE_SOURCE_DIR}/app/My_SPI
        ${CMAKE_SOURCE_DIR}/app/PID
        ${CMAKE_SOURCE_DIR}/app/Filter
        ${CMAKE_SOURCE_DIR}/API
        ${CMAKE_SOURCE_DIR}/API/inc
        ${CMAKE_SOURCE_DIR}/Enroll
        ${CMAKE_SOURCE_DIR}/BSP/LED
        ${CMAKE_SOURCE_DIR}/BSP/KEY
        ${CMAKE_SOURCE_DIR}/BSP/BMP280
        ${CMAKE_SOURCE_DIR}/BSP/NRF24L01
        ${CMAKE_SOURCE_DIR}/BSP/OLED
        ${CMAKE_SOURCE_DIR}/BSP/QMC5883P
        ${CMAKE_SOURCE_DIR}/BSP/MPU6050
        ${CMAKE_SOURCE_DIR}/BSP/MPU6050/eMPL
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/inc
        ${CMAKE_SOURCE_DIR}/SYSTEM
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start
    )
    set(MCU_SOURCES
        # F407 专属源码：新增/删除 .c 文件在这里改。
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/f407_delay.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/f407_sys.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_adc.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_gpio.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_tim.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_usart.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_pwm.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start/system_stm32f4xx.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start/startup_stm32f407vetx_gcc.c
    )

    # FreeRTOS: F407 使用 Cortex-M4F(FPU) 移植层和独立配置。
    set(FREERTOS_PORT_DIR
        ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel/portable/GCC/ARM_CM4F
    )
    set(FREERTOS_CONFIG_DIR
        ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-LTS/FreeRTOSConfig/F407
    )
else()
    message(FATAL_ERROR "Unsupported MCU target: ${RESOLVED_MCU_TARGET}")
endif()

set(FREERTOS_KERNEL_DIR
    ${CMAKE_SOURCE_DIR}/Middlewares/FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel
)

set(FREERTOS_SOURCES
    ${FREERTOS_KERNEL_DIR}/list.c
    ${FREERTOS_KERNEL_DIR}/queue.c
    ${FREERTOS_KERNEL_DIR}/tasks.c
    ${FREERTOS_KERNEL_DIR}/timers.c
    ${FREERTOS_KERNEL_DIR}/event_groups.c
    ${FREERTOS_KERNEL_DIR}/stream_buffer.c
    ${FREERTOS_PORT_DIR}/port.c
    ${FREERTOS_KERNEL_DIR}/portable/MemMang/heap_4.c
)

set(TARGET_SOURCES
    ${APP_LAYER_SOURCES}
    ${SYS_LAYER_SOURCES}
    ${API_LAYER_SOURCES}
    ${ENROLL_LAYER_SOURCES}
    ${BSP_LAYER_SOURCES}
    ${MCU_SOURCES}
    ${FREERTOS_SOURCES}
)

add_executable(${TARGET_NAME} ${TARGET_SOURCES})

target_include_directories(${TARGET_NAME} PRIVATE
    ${MCU_INCLUDE_DIRS}
    ${FREERTOS_KERNEL_DIR}/include
    ${FREERTOS_PORT_DIR}
    ${FREERTOS_CONFIG_DIR}
)

target_compile_definitions(${TARGET_NAME} PRIVATE
    ENROLL_MCU_F103=0
    ENROLL_MCU_F407=1
    ENROLL_MCU_TARGET=${ENROLL_MCU_TARGET_VALUE}
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
    -u
    _printf_float
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
