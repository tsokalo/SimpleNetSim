################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../ccack/ccack.cpp \
../ccack/matrix-echelon.cpp \
../ccack/matrix-operations.cpp 

OBJS += \
./ccack/ccack.o \
./ccack/matrix-echelon.o \
./ccack/matrix-operations.o 

CPP_DEPS += \
./ccack/ccack.d \
./ccack/matrix-echelon.d \
./ccack/matrix-operations.d 


# Each subdirectory must supply rules for building sources it contributes
ccack/%.o: ../ccack/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


