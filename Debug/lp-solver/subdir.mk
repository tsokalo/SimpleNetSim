################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../lp-solver/graph.cpp \
../lp-solver/lp-solver.cpp 

OBJS += \
./lp-solver/graph.o \
./lp-solver/lp-solver.o 

CPP_DEPS += \
./lp-solver/graph.d \
./lp-solver/lp-solver.d 


# Each subdirectory must supply rules for building sources it contributes
lp-solver/%.o: ../lp-solver/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


