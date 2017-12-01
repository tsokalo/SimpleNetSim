################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../comp/tst/comp.cpp \
../comp/tst/decomp.cpp \
../comp/tst/input.cpp \
../comp/tst/test.cpp 

OBJS += \
./comp/tst/comp.o \
./comp/tst/decomp.o \
./comp/tst/input.o \
./comp/tst/test.o 

CPP_DEPS += \
./comp/tst/comp.d \
./comp/tst/decomp.d \
./comp/tst/input.d \
./comp/tst/test.d 


# Each subdirectory must supply rules for building sources it contributes
comp/tst/%.o: ../comp/tst/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


