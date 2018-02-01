################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../old-stuff/ExtNcPolicy.cpp \
../old-stuff/NcPolicy.cpp \
../old-stuff/SchedulingPolicy.cpp \
../old-stuff/Test.cpp 

OBJS += \
./old-stuff/ExtNcPolicy.o \
./old-stuff/NcPolicy.o \
./old-stuff/SchedulingPolicy.o \
./old-stuff/Test.o 

CPP_DEPS += \
./old-stuff/ExtNcPolicy.d \
./old-stuff/NcPolicy.d \
./old-stuff/SchedulingPolicy.d \
./old-stuff/Test.d 


# Each subdirectory must supply rules for building sources it contributes
old-stuff/%.o: ../old-stuff/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


