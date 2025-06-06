TARGET = target
DEBUG = 1
OPT = -Og
BUILD_DIR = build

# 源文件配置
C_SOURCES = \
../Public/CMSIS/Device/system_stm32f1xx.c \
../Public/SYSTEM/delay.c \
../Public/SYSTEM/sys.c \
../Public/SYSTEM/usart.c \
../Public/USMART/usmart.c \
../Public/USMART/usmart_str.c \
../Public/USMART/usmart_port.c \
../Public/USMART/usmart_config.c \
BSP/led.c \
BSP/lcd.c \
main.c

ASM_SOURCES = ../Public/startup_stm32f103xe.s

# 工具链配置
CC = arm-none-eabi-gcc
AS = arm-none-eabi-gcc -x assembler-with-cpp
SZ = arm-none-eabi-size
HEX = arm-none-eabi-objcopy -O ihex
BIN = arm-none-eabi-objcopy -O binary -S

# 编译选项
CPU = -mcpu=cortex-m3
MCU = $(CPU) -mthumb
C_DEFS = -DSTM32F103xE
C_INCLUDES = -I../Public/CMSIS/Device -I../Public/CMSIS/Include -IBSP -I../Public/SYSTEM  -I../Public/USMART

CFLAGS = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
ifeq ($(DEBUG),1)
CFLAGS += -g -gdwarf-2
endif

# 链接配置
LDSCRIPT = ../Public/STM32F103ZETx_FLASH.ld
# LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref,--gc-sections
LDFLAGS = $(MCU) -T$(LDSCRIPT) -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref,--gc-sections

# 构建目标
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o))) \
          $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin

$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@$(CC) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s | $(BUILD_DIR)
	@$(AS) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS)
	@$(CC) $^ $(LDFLAGS) -o $@
	@$(SZ) $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	@$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	@$(BIN) $< $@

$(BUILD_DIR):
	@mkdir -p $@

clean:
	@rm -rf $(BUILD_DIR)

-include $(wildcard $(BUILD_DIR)/*.d)