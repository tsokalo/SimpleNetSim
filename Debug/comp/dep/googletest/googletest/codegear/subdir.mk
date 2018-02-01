################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../comp/dep/googletest/googletest/codegear/gtest_all.cc \
../comp/dep/googletest/googletest/codegear/gtest_link.cc 

CC_DEPS += \
./comp/dep/googletest/googletest/codegear/gtest_all.d \
./comp/dep/googletest/googletest/codegear/gtest_link.d 

OBJS += \
./comp/dep/googletest/googletest/codegear/gtest_all.o \
./comp/dep/googletest/googletest/codegear/gtest_link.o 


# Each subdirectory must supply rules for building sources it contributes
comp/dep/googletest/googletest/codegear/%.o: ../comp/dep/googletest/googletest/codegear/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


