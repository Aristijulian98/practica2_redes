################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../lwip/port/enet_ethernetif.c \
../lwip/port/enet_ethernetif_kinetis.c \
../lwip/port/sys_arch.c 

C_DEPS += \
./lwip/port/enet_ethernetif.d \
./lwip/port/enet_ethernetif_kinetis.d \
./lwip/port/sys_arch.d 

OBJS += \
./lwip/port/enet_ethernetif.o \
./lwip/port/enet_ethernetif_kinetis.o \
./lwip/port/sys_arch.o 


# Each subdirectory must supply rules for building sources it contributes
lwip/port/%.o: ../lwip/port/%.c lwip/port/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: MCU C Compiler'
	arm-none-eabi-gcc -std=gnu99 -D__REDLIB__ -DCPU_MK64FN1M0VLL12 -DCPU_MK64FN1M0VLL12_cm4 -D_POSIX_SOURCE -DUSE_RTOS=1 -DPRINTF_ADVANCED_ENABLE=1 -DFRDM_K64F -DFREEDOM -DLWIP_DISABLE_PBUF_POOL_SIZE_SANITY_CHECKS=1 -DSERIAL_PORT_TYPE_UART=1 -DSDK_OS_FREE_RTOS -DMCUXPRESSO_SDK -DSDK_DEBUGCONSOLE=1 -DCR_INTEGER_PRINTF -DPRINTF_FLOAT_ENABLE=0 -D__MCUXPRESSO -D__USE_CMSIS -DDEBUG -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\source" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\mdio" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\phy" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\lwip\src\include\lwip\apps" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\lwip\port" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\lwip\src" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\lwip\src\include" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\drivers" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\utilities" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\device" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\component\uart" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\component\serial_manager" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\component\lists" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\CMSIS" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\freertos\freertos_kernel\include" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\freertos\freertos_kernel\portable\GCC\ARM_CM4F" -I"C:\Users\frdmk64f_lwip_mqtt_freertos_practica_2\board" -O0 -fno-common -g3 -c -ffunction-sections -fdata-sections -ffreestanding -fno-builtin -fmerge-constants -fmacro-prefix-map="$(<D)/"= -mcpu=cortex-m4 -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -D__REDLIB__ -fstack-usage -specs=redlib.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.o)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-lwip-2f-port

clean-lwip-2f-port:
	-$(RM) ./lwip/port/enet_ethernetif.d ./lwip/port/enet_ethernetif.o ./lwip/port/enet_ethernetif_kinetis.d ./lwip/port/enet_ethernetif_kinetis.o ./lwip/port/sys_arch.d ./lwip/port/sys_arch.o

.PHONY: clean-lwip-2f-port

