cmake_minimum_required(VERSION 3.15)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/arm-toolchain.cmake)

project(imt407_car_hud C ASM)

# ==================== 路径配置 ====================
set(HAL_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver)
set(DEV_DIR        ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx)
set(CORE_INC_DIR   ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc)

# CMSIS Core 与 FreeRTOS 来自 STM32Cube 的 pack, 安装路径随机器变化,
# 不要把某个用户的绝对路径写死在仓库里:
#   优先读环境变量 CMSIS_CORE_DIR / FREERTOS_PACK_DIR, 未设置时按 LOCALAPPDATA 兜底。
set(_PACK_ROOT "$ENV{LOCALAPPDATA}/stm32cube/packs/STMicroelectronics")

if(DEFINED ENV{CMSIS_CORE_DIR})
    set(CMSIS_CORE_DIR "$ENV{CMSIS_CORE_DIR}")
else()
    set(CMSIS_CORE_DIR "${_PACK_ROOT}/CMSIS/6.3.0/CMSIS/Core/Include")
endif()

if(DEFINED ENV{FREERTOS_PACK_DIR})
    set(FREERTOS_DIR "$ENV{FREERTOS_PACK_DIR}")
else()
    set(FREERTOS_DIR "${_PACK_ROOT}/freertos/2.1.0")
endif()

if(NOT EXISTS "${CMSIS_CORE_DIR}/core_cm4.h")
    message(FATAL_ERROR
        "找不到 CMSIS Core 头文件: ${CMSIS_CORE_DIR}\n"
        "请设置环境变量 CMSIS_CORE_DIR 指向 .../CMSIS/<version>/CMSIS/Core/Include")
endif()
if(NOT EXISTS "${FREERTOS_DIR}/tasks.c")
    message(FATAL_ERROR
        "找不到 FreeRTOS 源码: ${FREERTOS_DIR}\n"
        "请设置环境变量 FREERTOS_PACK_DIR 指向 FreeRTOS pack 根目录")
endif()

set(FATFS_DIR      ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/FatFs/source)

# ==================== 编译选项 ====================
set(MCU_FLAGS "-mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${MCU_FLAGS}")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSTM32F407xx -DUSE_HAL_DRIVER")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Os -ffunction-sections -fdata-sections -g")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-strict-aliasing -fno-builtin")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter -Wno-unused-function -Wno-unused-variable")

set(CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS} ${MCU_FLAGS} -x assembler-with-cpp")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${MCU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T${CMAKE_CURRENT_SOURCE_DIR}/Core/STM32F407ZGTx_FLASH.ld")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Wl,-Map=imt407-car-hud.map")
# newlib-nano 默认不链接浮点格式化: 不补 -u _printf_float 的话, httpd.c 里
# JSON 的 %.1f/%.2f 会退化成空字符串或 0.0, 而不会报任何错 —— 板子上表现为
# 电压/温度/能耗全部显示 0, 极难排查。这里强制把浮点 printf 拉进镜像。
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -specs=nano.specs -u _printf_float -lc -lm")

# ==================== 包含路径 ====================
# 注意: Core/Inc 必须在 FreeRTOS include 前面, 因为 FreeRTOSConfig.h 在那里
include_directories(
    ${CORE_INC_DIR}
    ${HAL_DIR}/Inc
    ${DEV_DIR}/Include
    ${CMSIS_CORE_DIR}
    ${FREERTOS_DIR}/include
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F
    ${FATFS_DIR}
)

# ==================== 源文件 ====================
# Core 源文件
file(GLOB CORE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/*.c")

# HAL 驱动源文件
set(HAL_SRC
    ${HAL_DIR}/Src/stm32f4xx_hal.c
    ${HAL_DIR}/Src/stm32f4xx_hal_gpio.c
    ${HAL_DIR}/Src/stm32f4xx_hal_rcc.c
    ${HAL_DIR}/Src/stm32f4xx_hal_rcc_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_dma.c
    ${HAL_DIR}/Src/stm32f4xx_hal_dma_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_cortex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_pwr.c
    ${HAL_DIR}/Src/stm32f4xx_hal_pwr_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_flash.c
    ${HAL_DIR}/Src/stm32f4xx_hal_flash_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_uart.c
    ${HAL_DIR}/Src/stm32f4xx_hal_i2c.c
    ${HAL_DIR}/Src/stm32f4xx_hal_i2c_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_sd.c
    ${HAL_DIR}/Src/stm32f4xx_ll_sdmmc.c
    ${HAL_DIR}/Src/stm32f4xx_hal_tim.c
    ${HAL_DIR}/Src/stm32f4xx_hal_tim_ex.c
    ${HAL_DIR}/Src/stm32f4xx_hal_exti.c
    ${HAL_DIR}/Src/stm32f4xx_hal_can.c
    ${HAL_DIR}/Src/stm32f4xx_hal_iwdg.c
)

# FreeRTOS 源文件
set(FREERTOS_SRC
    ${FREERTOS_DIR}/croutine.c
    ${FREERTOS_DIR}/event_groups.c
    ${FREERTOS_DIR}/list.c
    ${FREERTOS_DIR}/queue.c
    ${FREERTOS_DIR}/stream_buffer.c
    ${FREERTOS_DIR}/tasks.c
    ${FREERTOS_DIR}/timers.c
    ${FREERTOS_DIR}/portable/GCC/ARM_CM4F/port.c
    ${FREERTOS_DIR}/portable/MemMang/heap_4.c
)

# FatFs 源文件 (只编译核心文件，diskio.c 由我们自己实现，放在 Core/Src 下)
set(FATFS_SRC
    ${FATFS_DIR}/ff.c
    ${FATFS_DIR}/ffsystem.c
    ${FATFS_DIR}/ffunicode.c
)

# 启动文件和系统文件
set(STARTUP_SRC ${CMAKE_CURRENT_SOURCE_DIR}/Core/startup_stm32f407xx.s)
set(SYSTEM_SRC  ${DEV_DIR}/Source/Templates/system_stm32f4xx.c)

# ==================== 目标 ====================
message(STATUS "FATFS_DIR = ${FATFS_DIR}")
message(STATUS "FATFS_SRC = ${FATFS_SRC}")
add_executable(imt407-car-hud.elf
    ${CORE_SRC}
    ${HAL_SRC}
    ${FREERTOS_SRC}
    ${FATFS_SRC}
    ${SYSTEM_SRC}
    ${STARTUP_SRC}
)

# 链接库（必须放在目标文件之后）
target_link_libraries(imt407-car-hud.elf m c)

# ==================== 生成 bin/hex ====================
add_custom_command(TARGET imt407-car-hud.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:imt407-car-hud.elf> imt407-car-hud.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex   $<TARGET_FILE:imt407-car-hud.elf> imt407-car-hud.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:imt407-car-hud.elf>
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating bin/hex and printing size"
)
