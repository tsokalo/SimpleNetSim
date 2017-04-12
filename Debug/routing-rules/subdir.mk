################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../routing-rules/god-view-routing-rules.cpp \
../routing-rules/nc-routing-rules.cpp 

OBJS += \
./routing-rules/god-view-routing-rules.o \
./routing-rules/nc-routing-rules.o 

CPP_DEPS += \
./routing-rules/god-view-routing-rules.d \
./routing-rules/nc-routing-rules.d 


# Each subdirectory must supply rules for building sources it contributes
routing-rules/%.o: ../routing-rules/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


