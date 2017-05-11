################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../traffic/traffic-generator.cpp 

OBJS += \
./traffic/traffic-generator.o 

CPP_DEPS += \
./traffic/traffic-generator.d 


# Each subdirectory must supply rules for building sources it contributes
traffic/%.o: ../traffic/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


