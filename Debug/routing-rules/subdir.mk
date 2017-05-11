################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../routing-rules/god-view-routing-rules.cpp \
../routing-rules/multicast-brr.cpp \
../routing-rules/nc-routing-rules.cpp 

OBJS += \
./routing-rules/god-view-routing-rules.o \
./routing-rules/multicast-brr.o \
./routing-rules/nc-routing-rules.o 

CPP_DEPS += \
./routing-rules/god-view-routing-rules.d \
./routing-rules/multicast-brr.d \
./routing-rules/nc-routing-rules.d 


# Each subdirectory must supply rules for building sources it contributes
routing-rules/%.o: ../routing-rules/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


