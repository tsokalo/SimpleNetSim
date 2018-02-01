################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../comp/dep/googletest/googletest/xcode/Samples/FrameworkSample/widget.cc \
../comp/dep/googletest/googletest/xcode/Samples/FrameworkSample/widget_test.cc 

CC_DEPS += \
./comp/dep/googletest/googletest/xcode/Samples/FrameworkSample/widget.d \
./comp/dep/googletest/googletest/xcode/Samples/FrameworkSample/widget_test.d 

OBJS += \
./comp/dep/googletest/googletest/xcode/Samples/FrameworkSample/widget.o \
./comp/dep/googletest/googletest/xcode/Samples/FrameworkSample/widget_test.o 


# Each subdirectory must supply rules for building sources it contributes
comp/dep/googletest/googletest/xcode/Samples/FrameworkSample/%.o: ../comp/dep/googletest/googletest/xcode/Samples/FrameworkSample/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


