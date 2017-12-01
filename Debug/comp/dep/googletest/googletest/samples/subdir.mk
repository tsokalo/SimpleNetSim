################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../comp/dep/googletest/googletest/samples/sample1.cc \
../comp/dep/googletest/googletest/samples/sample10_unittest.cc \
../comp/dep/googletest/googletest/samples/sample1_unittest.cc \
../comp/dep/googletest/googletest/samples/sample2.cc \
../comp/dep/googletest/googletest/samples/sample2_unittest.cc \
../comp/dep/googletest/googletest/samples/sample3_unittest.cc \
../comp/dep/googletest/googletest/samples/sample4.cc \
../comp/dep/googletest/googletest/samples/sample4_unittest.cc \
../comp/dep/googletest/googletest/samples/sample5_unittest.cc \
../comp/dep/googletest/googletest/samples/sample6_unittest.cc \
../comp/dep/googletest/googletest/samples/sample7_unittest.cc \
../comp/dep/googletest/googletest/samples/sample8_unittest.cc \
../comp/dep/googletest/googletest/samples/sample9_unittest.cc 

CC_DEPS += \
./comp/dep/googletest/googletest/samples/sample1.d \
./comp/dep/googletest/googletest/samples/sample10_unittest.d \
./comp/dep/googletest/googletest/samples/sample1_unittest.d \
./comp/dep/googletest/googletest/samples/sample2.d \
./comp/dep/googletest/googletest/samples/sample2_unittest.d \
./comp/dep/googletest/googletest/samples/sample3_unittest.d \
./comp/dep/googletest/googletest/samples/sample4.d \
./comp/dep/googletest/googletest/samples/sample4_unittest.d \
./comp/dep/googletest/googletest/samples/sample5_unittest.d \
./comp/dep/googletest/googletest/samples/sample6_unittest.d \
./comp/dep/googletest/googletest/samples/sample7_unittest.d \
./comp/dep/googletest/googletest/samples/sample8_unittest.d \
./comp/dep/googletest/googletest/samples/sample9_unittest.d 

OBJS += \
./comp/dep/googletest/googletest/samples/sample1.o \
./comp/dep/googletest/googletest/samples/sample10_unittest.o \
./comp/dep/googletest/googletest/samples/sample1_unittest.o \
./comp/dep/googletest/googletest/samples/sample2.o \
./comp/dep/googletest/googletest/samples/sample2_unittest.o \
./comp/dep/googletest/googletest/samples/sample3_unittest.o \
./comp/dep/googletest/googletest/samples/sample4.o \
./comp/dep/googletest/googletest/samples/sample4_unittest.o \
./comp/dep/googletest/googletest/samples/sample5_unittest.o \
./comp/dep/googletest/googletest/samples/sample6_unittest.o \
./comp/dep/googletest/googletest/samples/sample7_unittest.o \
./comp/dep/googletest/googletest/samples/sample8_unittest.o \
./comp/dep/googletest/googletest/samples/sample9_unittest.o 


# Each subdirectory must supply rules for building sources it contributes
comp/dep/googletest/googletest/samples/%.o: ../comp/dep/googletest/googletest/samples/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


