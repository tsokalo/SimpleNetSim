################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CXX_SRCS += \
../comp/dep/googletest/build/CMakeFiles/feature_tests.cxx 

C_SRCS += \
../comp/dep/googletest/build/CMakeFiles/feature_tests.c 

CXX_DEPS += \
./comp/dep/googletest/build/CMakeFiles/feature_tests.d 

OBJS += \
./comp/dep/googletest/build/CMakeFiles/feature_tests.o 

C_DEPS += \
./comp/dep/googletest/build/CMakeFiles/feature_tests.d 


# Each subdirectory must supply rules for building sources it contributes
comp/dep/googletest/build/CMakeFiles/%.o: ../comp/dep/googletest/build/CMakeFiles/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

comp/dep/googletest/build/CMakeFiles/%.o: ../comp/dep/googletest/build/CMakeFiles/%.cxx
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


