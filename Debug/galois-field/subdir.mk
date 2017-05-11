################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../galois-field/galois-field-element.cpp \
../galois-field/galois-field-polynomial.cpp \
../galois-field/galois-field.cpp 

OBJS += \
./galois-field/galois-field-element.o \
./galois-field/galois-field-polynomial.o \
./galois-field/galois-field.o 

CPP_DEPS += \
./galois-field/galois-field-element.d \
./galois-field/galois-field-polynomial.d \
./galois-field/galois-field.d 


# Each subdirectory must supply rules for building sources it contributes
galois-field/%.o: ../galois-field/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -std=c++0x -O0 -g3 -Wall -c -fmessage-length=0 -std=c++11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


