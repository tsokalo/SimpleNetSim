################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../comp/dep/googletest/googlemock/src/gmock-all.cc \
../comp/dep/googletest/googlemock/src/gmock-cardinalities.cc \
../comp/dep/googletest/googlemock/src/gmock-internal-utils.cc \
../comp/dep/googletest/googlemock/src/gmock-matchers.cc \
../comp/dep/googletest/googlemock/src/gmock-spec-builders.cc \
../comp/dep/googletest/googlemock/src/gmock.cc \
../comp/dep/googletest/googlemock/src/gmock_main.cc 

CC_DEPS += \
./comp/dep/googletest/googlemock/src/gmock-all.d \
./comp/dep/googletest/googlemock/src/gmock-cardinalities.d \
./comp/dep/googletest/googlemock/src/gmock-internal-utils.d \
./comp/dep/googletest/googlemock/src/gmock-matchers.d \
./comp/dep/googletest/googlemock/src/gmock-spec-builders.d \
./comp/dep/googletest/googlemock/src/gmock.d \
./comp/dep/googletest/googlemock/src/gmock_main.d 

OBJS += \
./comp/dep/googletest/googlemock/src/gmock-all.o \
./comp/dep/googletest/googlemock/src/gmock-cardinalities.o \
./comp/dep/googletest/googlemock/src/gmock-internal-utils.o \
./comp/dep/googletest/googlemock/src/gmock-matchers.o \
./comp/dep/googletest/googlemock/src/gmock-spec-builders.o \
./comp/dep/googletest/googlemock/src/gmock.o \
./comp/dep/googletest/googlemock/src/gmock_main.o 


# Each subdirectory must supply rules for building sources it contributes
comp/dep/googletest/googlemock/src/%.o: ../comp/dep/googletest/googlemock/src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


