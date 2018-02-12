################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../comp/src/compressor.cpp \
../comp/src/decompressor.cpp 

C_SRCS += \
../comp/src/list_csrc.c 

O_SRCS += \
../comp/src/compressor.o \
../comp/src/decompressor.o 

OBJS += \
./comp/src/compressor.o \
./comp/src/decompressor.o \
./comp/src/list_csrc.o 

CPP_DEPS += \
./comp/src/compressor.d \
./comp/src/decompressor.d 

C_DEPS += \
./comp/src/list_csrc.d 


# Each subdirectory must supply rules for building sources it contributes
comp/src/%.o: ../comp/src/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

comp/src/%.o: ../comp/src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


