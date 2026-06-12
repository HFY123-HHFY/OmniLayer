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

set(MCU_TARGET "AUTO" CACHE STRING "Target MCU family: AUTO/F103/F407/G3507")
set_property(CACHE MCU_TARGET PROPERTY STRINGS AUTO F103 F407 G3507)

# AUTO 模式下，从 Enroll.h 的默认宏读取当前目标 MCU。
if(MCU_TARGET STREQUAL "AUTO")
    file(READ "${CMAKE_SOURCE_DIR}/Enroll/Enroll.h" ENROLL_HEADER_CONTENT)
    string(REGEX MATCH "(^|[\r\n])[ \t]*#define[ \t]+ENROLL_MCU_TARGET[ \t]+ENROLL_MCU_F407([ \t]|[\r\n]|$)" ENROLL_TARGET_IS_F407 "${ENROLL_HEADER_CONTENT}")
    string(REGEX MATCH "(^|[\r\n])[ \t]*#define[ \t]+ENROLL_MCU_TARGET[ \t]+ENROLL_MCU_F103([ \t]|[\r\n]|$)" ENROLL_TARGET_IS_F103 "${ENROLL_HEADER_CONTENT}")
    string(REGEX MATCH "(^|[\r\n])[ \t]*#define[ \t]+ENROLL_MCU_TARGET[ \t]+ENROLL_MCU_G3507([ \t]|[\r\n]|$)" ENROLL_TARGET_IS_G3507 "${ENROLL_HEADER_CONTENT}")

    if(ENROLL_TARGET_IS_F407)
        set(RESOLVED_MCU_TARGET "F407")
    elseif(ENROLL_TARGET_IS_F103)
        set(RESOLVED_MCU_TARGET "F103")
    elseif(ENROLL_TARGET_IS_G3507)
        set(RESOLVED_MCU_TARGET "G3507")
    else()
        message(FATAL_ERROR "Cannot detect ENROLL_MCU_TARGET from Enroll/Enroll.h.\n请检查 #define ENROLL_MCU_TARGET ... 是否为 ENROLL_MCU_F103/F407/G3507 之一。")
    endif()
else()
    set(RESOLVED_MCU_TARGET "${MCU_TARGET}")
endif()

message(STATUS "Resolved MCU target: ${RESOLVED_MCU_TARGET}")

###############################################################
# F103 平台专属配置与源文件
# - 仅在 STM32F103 构建时生效
# - 包含 F103 专属 BSP、头文件目录、MCU 源文件
###############################################################
if(RESOLVED_MCU_TARGET STREQUAL "F103")
    set(ENROLL_MCU_TARGET_VALUE 0)
###############################################################
# F407 平台专属配置与源文件
# - 仅在 STM32F407 构建时生效
# - 包含 F407 专属 BSP、头文件目录、MCU 源文件
###############################################################
elseif(RESOLVED_MCU_TARGET STREQUAL "F407")
    set(ENROLL_MCU_TARGET_VALUE 1)
###############################################################
# MSPM0G3507 平台专属配置与源文件
# - 仅在 MSPM0G3507 构建时生效
# - 包含 G3507 专属头文件目录、MCU 源文件
###############################################################
elseif(RESOLVED_MCU_TARGET STREQUAL "G3507")
    set(ENROLL_MCU_TARGET_VALUE 2)
else()
    message(FATAL_ERROR "Unsupported resolved MCU target: ${RESOLVED_MCU_TARGET}")
endif()

# ============================== 维护说明 ==============================
#
# 1) APP、API、SYS、ENROLL 层为三平台通用，只需维护一份源文件列表（见下方变量）。
#    - 新增/删除 .c 文件，直接在 APP_LAYER_SOURCES、API_LAYER_SOURCES、SYS_LAYER_SOURCES、ENROLL_LAYER_SOURCES 里增删。
#    - 这几层所有 MCU 都会自动引用，无需分平台重复写。
#
# 2) BSP 层（BSP_LAYER_SOURCES）按“公共 + 每个 MCU 独立追加”维护。
#    - 公共驱动放 BSP_LAYER_SOURCES。
#    - 各 MCU 专属驱动放各自分支中的 list(APPEND BSP_LAYER_SOURCES ...)。
#
# 3) 头文件目录按“通用 + 平台专属”维护：
#    - 通用目录放 COMMON_INCLUDE_DIRS / BSP_COMMON_INCLUDE_DIRS。
#    - 平台差异目录仅在各 MCU 分支追加。
#
# 4) MCU_SOURCES 只放各自平台专属代码。
#
# 5) 改完后验证：
#    - 重新 Configure/Build；若报“undefined reference”通常是漏加 .c 文件。
#    - 若报“header not found”通常是漏加头文件目录到 MCU_INCLUDE_DIRS。
# =====================================================================

# 应用层（app）：业务入口与任务调度。
set(APP_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/A_Entry/main.c
    ${CMAKE_SOURCE_DIR}/app/Control/Control.c
    ${CMAKE_SOURCE_DIR}/app/My_Usart/My_Usart.c
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
    ${CMAKE_SOURCE_DIR}/API/src/exti.c
    ${CMAKE_SOURCE_DIR}/API/src/gpio.c
    ${CMAKE_SOURCE_DIR}/API/src/tim.c
    ${CMAKE_SOURCE_DIR}/API/src/usart.c
    ${CMAKE_SOURCE_DIR}/API/src/pwm.c
    ${CMAKE_SOURCE_DIR}/API/src/Encoder.c
    ${CMAKE_SOURCE_DIR}/API/API_I2C/API_I2C.c
    ${CMAKE_SOURCE_DIR}/API/API_SPI/API_SPI.c
)

# 注册层（Enroll）：把 BSP 资源和具体 MCU 驱动绑定。
set(ENROLL_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/Enroll/Enroll.c
)

# 通用头文件目录：三平台共享。
set(COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/A_Entry
    ${CMAKE_SOURCE_DIR}/app
    ${CMAKE_SOURCE_DIR}/app/Control_Task
    ${CMAKE_SOURCE_DIR}/app/Control
    ${CMAKE_SOURCE_DIR}/app/My_Usart
    ${CMAKE_SOURCE_DIR}/app/PID
    ${CMAKE_SOURCE_DIR}/app/Filter
    ${CMAKE_SOURCE_DIR}/API
    ${CMAKE_SOURCE_DIR}/API/inc
    ${CMAKE_SOURCE_DIR}/API/API_I2C
    ${CMAKE_SOURCE_DIR}/API/API_SPI
    ${CMAKE_SOURCE_DIR}/Enroll
    ${CMAKE_SOURCE_DIR}/SYSTEM
)

# 通用 BSP 头文件目录：所有平台都会用到的公共外设。
set(BSP_COMMON_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/BSP/LED
    ${CMAKE_SOURCE_DIR}/BSP/KEY
    ${CMAKE_SOURCE_DIR}/BSP/OLED
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050/eMPL
    ${CMAKE_SOURCE_DIR}/BSP/TB6612
)

# 板级支持层（BSP）：对传感器/执行器做硬件抽象。
set(BSP_LAYER_SOURCES
    ${CMAKE_SOURCE_DIR}/BSP/LED/LED.c
    ${CMAKE_SOURCE_DIR}/BSP/KEY/KEY.c
    ${CMAKE_SOURCE_DIR}/BSP/OLED/OLED.c
    ${CMAKE_SOURCE_DIR}/BSP/OLED/OLED_Data.c
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050/MPU6050.c
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050/MPU6050_Int.c
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050/eMPL/inv_mpu.c
    ${CMAKE_SOURCE_DIR}/BSP/MPU6050/eMPL/inv_mpu_dmp_motion_driver.c
    ${CMAKE_SOURCE_DIR}/BSP/TB6612/TB6612.c
)
# 说明：可选传感器不在这里做合并条件，改为各 MCU 分支独立维护。


if(RESOLVED_MCU_TARGET STREQUAL "F103")
    set(TARGET_NAME OmniLayer_F103)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Core/STM32F103/cmake/STM32F103XX_FLASH.ld)
    set(OPENOCD_CFG ${CMAKE_SOURCE_DIR}/OpenOCD/F103_OpenOCD.cfg)
    set(OPENOCD_ERASE_CMD "stm32f1x mass_erase 0")
    set(MCU_CPU_FLAGS -mcpu=cortex-m3 -mthumb)
    set(MCU_FPU_FLAGS)
    set(MCU_DEFINITIONS STM32F10X_MD)
    # F103 专属 BSP（独立维护，不与其他 MCU 合并）
    list(APPEND BSP_LAYER_SOURCES
        ${CMAKE_SOURCE_DIR}/BSP/NRF24L01/NRF24L01.c
    )
    # F103 专属头文件目录和源文件
    set(MCU_INCLUDE_DIRS
        ${COMMON_INCLUDE_DIRS}
        ${BSP_COMMON_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/BSP/NRF24L01
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/inc
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/f103_soft_i2c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/f103_soft_spi
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start
    )
    # F103 专属 MCU 驱动实现
    set(MCU_SOURCES
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/f103_delay.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/f103_soft_i2c/f103_soft_i2c.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/f103_soft_spi/f103_soft_spi.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_exti.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_adc.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_gpio.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_tim.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_usart.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_pwm.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F103/src/f103_Encoder.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start/system_stm32f10x.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F1/Start/startup_stm32f103c8tx_gcc.c
    )
elseif(RESOLVED_MCU_TARGET STREQUAL "F407")
    set(TARGET_NAME OmniLayer_F407)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Core/STM32F407/cmake/STM32F407VE_FLASH.ld)
    set(OPENOCD_CFG ${CMAKE_SOURCE_DIR}/OpenOCD/F407_OpenOCD.cfg)
    set(OPENOCD_ERASE_CMD "stm32f4x mass_erase 0")
    set(MCU_CPU_FLAGS -mcpu=cortex-m4 -mthumb)
    set(MCU_FPU_FLAGS -mfpu=fpv4-sp-d16 -mfloat-abi=hard)
    set(MCU_DEFINITIONS STM32F40_41xxx)
    # F407 专属 BSP（独立维护，不与其他 MCU 合并）
    list(APPEND BSP_LAYER_SOURCES
        ${CMAKE_SOURCE_DIR}/BSP/BMP280/BMP280.c
        ${CMAKE_SOURCE_DIR}/BSP/NRF24L01/NRF24L01.c
        ${CMAKE_SOURCE_DIR}/BSP/QMC5883P/QMC5883P.c
    )
    # F407 专属头文件目录和源文件
    set(MCU_INCLUDE_DIRS
        ${COMMON_INCLUDE_DIRS}
        ${BSP_COMMON_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/BSP/BMP280
        ${CMAKE_SOURCE_DIR}/BSP/NRF24L01
        ${CMAKE_SOURCE_DIR}/BSP/QMC5883P
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/inc
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/f407_soft_i2c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/f407_soft_spi
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start
    )
    # F407 专属 MCU 驱动实现
    set(MCU_SOURCES
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/f407_delay.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/f407_soft_i2c/f407_soft_i2c.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/f407_soft_spi/f407_soft_spi.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_exti.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_adc.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_gpio.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_tim.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_usart.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_pwm.c
        ${CMAKE_SOURCE_DIR}/Core/STM32F407/src/f407_Encoder.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start/system_stm32f4xx.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_STM32F4/Start/startup_stm32f407vetx_gcc.c
    )
elseif(RESOLVED_MCU_TARGET STREQUAL "G3507")
    set(TARGET_NAME OmniLayer_MSPM0G3507)
    set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/cmake/mspm0g3507.lds)
    set(OPENOCD_CFG ${CMAKE_SOURCE_DIR}/OpenOCD/G3507_OpenOCD.cfg)
    set(OPENOCD_ERASE_CMD "mspm0 mass_erase 0")
    set(MCU_CPU_FLAGS -mcpu=cortex-m0plus -mthumb)
    set(MCU_FPU_FLAGS)
    set(MCU_DEFINITIONS __MSPM0G3507__)
    # G3507 专属 BSP（独立维护，不与其他 MCU 合并）
    list(APPEND BSP_LAYER_SOURCES
        # ${CMAKE_SOURCE_DIR}/BSP/TB6612/TB6612.c
    )
    # G3507 专属头文件目录和源文件
    set(MCU_INCLUDE_DIRS
        ${COMMON_INCLUDE_DIRS}
        ${BSP_COMMON_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/BSP/TB6612
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/inc
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/G3507_soft_i2c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/G3507_soft_spi
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_M0G3507/source
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_M0G3507/source/ti/devices/msp/m0p
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_M0G3507/source/third_party/CMSIS/Core/Include
    )
    # G3507 专属 MCU 驱动实现
    set(MCU_SOURCES
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/G3507_delay.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/G3507_sys.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/G3507_soft_i2c/G3507_soft_i2c.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/G3507_soft_spi/G3507_soft_spi.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/src/G3507_adc.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/src/G3507_exti.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/src/G3507_gpio.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/src/G3507_pwm.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/src/G3507_tim.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/src/G3507_usart.c
        ${CMAKE_SOURCE_DIR}/Core/MSPM0G3507/src/G3507_Encoder.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_M0G3507/source/ti/devices/msp/m0p/startup_system_files/gcc/startup_mspm0g350x_gcc.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_M0G3507/source/ti/driverlib/dl_adc12.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_M0G3507/source/ti/driverlib/m0p/sysctl/dl_sysctl_mspm0g1x0x_g3x0x.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_M0G3507/source/ti/driverlib/dl_timer.c
        ${CMAKE_SOURCE_DIR}/Drivers/Drivers_M0G3507/source/ti/driverlib/dl_uart.c
    )
else()
    message(FATAL_ERROR "Unsupported MCU target: ${RESOLVED_MCU_TARGET}")
endif()



###############################################################
# 汇总所有层的源文件，形成最终构建目标源文件列表
# - APP/API/SYS/ENROLL/BSP/MCU 层全部拼接
# - 便于统一管理和后续维护
###############################################################
set(TARGET_SOURCES
    ${APP_LAYER_SOURCES}
    ${SYS_LAYER_SOURCES}
    ${API_LAYER_SOURCES}
    ${ENROLL_LAYER_SOURCES}
    ${BSP_LAYER_SOURCES}
    ${MCU_SOURCES}
)


###############################################################
# 构建最终可执行文件（固件 ELF）
# - ${TARGET_NAME} 由平台分支决定
# - ${TARGET_SOURCES} 为所有参与编译的源文件
###############################################################
add_executable(${TARGET_NAME} ${TARGET_SOURCES})


###############################################################
# 添加头文件搜索路径
# - ${MCU_INCLUDE_DIRS} 已包含通用+平台专属目录
###############################################################
target_include_directories(${TARGET_NAME} PRIVATE
    ${MCU_INCLUDE_DIRS}
)


###############################################################
# 添加编译宏定义
# - ENROLL_MCU_TARGET 及各平台标志
# - ${MCU_DEFINITIONS} 为平台专属宏
###############################################################
target_compile_definitions(${TARGET_NAME} PRIVATE
    ENROLL_MCU_F103=0
    ENROLL_MCU_F407=1
    ENROLL_MCU_G3507=2
    ENROLL_MCU_TARGET=${ENROLL_MCU_TARGET_VALUE}
    ${MCU_DEFINITIONS}
)


###############################################################
# 添加编译器选项
# - CPU/FPU 架构、警告开关等
###############################################################
target_compile_options(${TARGET_NAME} PRIVATE
    ${MCU_CPU_FLAGS}
    ${MCU_FPU_FLAGS}
    -Wall
    -Wextra
)


###############################################################
# 添加链接器选项
# - 指定链接脚本、浮点格式、内存优化等
###############################################################
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


###############################################################
# 链接标准库
# - m: 数学库
# - c: 标准C库
###############################################################
target_link_libraries(${TARGET_NAME} PRIVATE
    m
    c
)


###############################################################
# 生成常用烧录文件格式（hex/bin）及打印固件大小
###############################################################
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${TARGET_NAME}> ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET_NAME}>
    COMMENT "Generate .hex/.bin and print size"
)


###############################################################
# OpenOCD 烧录/擦除目标
# - flash: 烧录固件，G3507 跳过 verify 加速
# - erase: 全片擦除
###############################################################
add_custom_target(flash
    COMMAND openocd -f ${OPENOCD_CFG} -c "program $<TARGET_FILE:${TARGET_NAME}> $<IF:$<STREQUAL:${RESOLVED_MCU_TARGET},G3507>,,verify> reset exit"
    DEPENDS ${TARGET_NAME}
    COMMENT "Flash firmware via OpenOCD"
)

add_custom_target(erase
    COMMAND openocd -f ${OPENOCD_CFG} -c "init; reset halt; ${OPENOCD_ERASE_CMD}; reset run; exit"
    COMMENT "Mass erase firmware via OpenOCD"
)
