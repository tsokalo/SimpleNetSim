################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../comp/dep/googletest/googletest/src/gtest-all.cc \
../comp/dep/googletest/googletest/src/gtest-death-test.cc \
../comp/dep/googletest/googletest/src/gtest-filepath.cc \
../comp/dep/googletest/googletest/src/gtest-port.cc \
../comp/dep/googletest/googletest/src/gtest-printers.cc \
../comp/dep/googletest/googletest/src/gtest-test-part.cc \
../comp/dep/googletest/googletest/src/gtest-typed-test.cc \
../comp/dep/googletest/googletest/src/gtest.cc \
../comp/dep/googletest/googletest/src/gtest_main.cc 

CC_DEPS += \
./comp/dep/googletest/googletest/src/gtest-all.d \
./comp/dep/googletest/googletest/src/gtest-death-test.d \
./comp/dep/googletest/googletest/src/gtest-filepath.d \
./comp/dep/googletest/googletest/src/gtest-port.d \
./comp/dep/googletest/googletest/src/gtest-printers.d \
./comp/dep/googletest/googletest/src/gtest-test-part.d \
./comp/dep/googletest/googletest/src/gtest-typed-test.d \
./comp/dep/googletest/googletest/src/gtest.d \
./comp/dep/googletest/googletest/src/gtest_main.d 

OBJS += \
./comp/dep/googletest/googletest/src/gtest-all.o \
./comp/dep/googletest/googletest/src/gtest-death-test.o \
./comp/dep/googletest/googletest/src/gtest-filepath.o \
./comp/dep/googletest/googletest/src/gtest-port.o \
./comp/dep/googletest/googletest/src/gtest-printers.o \
./comp/dep/googletest/googletest/src/gtest-test-part.o \
./comp/dep/googletest/googletest/src/gtest-typed-test.o \
./comp/dep/googletest/googletest/src/gtest.o \
./comp/dep/googletest/googletest/src/gtest_main.o 


# Each subdirectory must supply rules for building sources it contributes
comp/dep/googletest/googletest/src/%.o: ../comp/dep/googletest/googletest/src/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


