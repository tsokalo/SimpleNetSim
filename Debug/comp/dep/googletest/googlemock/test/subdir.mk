################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CC_SRCS += \
../comp/dep/googletest/googlemock/test/gmock-actions_test.cc \
../comp/dep/googletest/googlemock/test/gmock-cardinalities_test.cc \
../comp/dep/googletest/googlemock/test/gmock-generated-actions_test.cc \
../comp/dep/googletest/googlemock/test/gmock-generated-function-mockers_test.cc \
../comp/dep/googletest/googlemock/test/gmock-generated-internal-utils_test.cc \
../comp/dep/googletest/googlemock/test/gmock-generated-matchers_test.cc \
../comp/dep/googletest/googlemock/test/gmock-internal-utils_test.cc \
../comp/dep/googletest/googlemock/test/gmock-matchers_test.cc \
../comp/dep/googletest/googlemock/test/gmock-more-actions_test.cc \
../comp/dep/googletest/googlemock/test/gmock-nice-strict_test.cc \
../comp/dep/googletest/googlemock/test/gmock-port_test.cc \
../comp/dep/googletest/googlemock/test/gmock-spec-builders_test.cc \
../comp/dep/googletest/googlemock/test/gmock_all_test.cc \
../comp/dep/googletest/googlemock/test/gmock_ex_test.cc \
../comp/dep/googletest/googlemock/test/gmock_leak_test_.cc \
../comp/dep/googletest/googlemock/test/gmock_link2_test.cc \
../comp/dep/googletest/googlemock/test/gmock_link_test.cc \
../comp/dep/googletest/googlemock/test/gmock_output_test_.cc \
../comp/dep/googletest/googlemock/test/gmock_stress_test.cc \
../comp/dep/googletest/googlemock/test/gmock_test.cc 

CC_DEPS += \
./comp/dep/googletest/googlemock/test/gmock-actions_test.d \
./comp/dep/googletest/googlemock/test/gmock-cardinalities_test.d \
./comp/dep/googletest/googlemock/test/gmock-generated-actions_test.d \
./comp/dep/googletest/googlemock/test/gmock-generated-function-mockers_test.d \
./comp/dep/googletest/googlemock/test/gmock-generated-internal-utils_test.d \
./comp/dep/googletest/googlemock/test/gmock-generated-matchers_test.d \
./comp/dep/googletest/googlemock/test/gmock-internal-utils_test.d \
./comp/dep/googletest/googlemock/test/gmock-matchers_test.d \
./comp/dep/googletest/googlemock/test/gmock-more-actions_test.d \
./comp/dep/googletest/googlemock/test/gmock-nice-strict_test.d \
./comp/dep/googletest/googlemock/test/gmock-port_test.d \
./comp/dep/googletest/googlemock/test/gmock-spec-builders_test.d \
./comp/dep/googletest/googlemock/test/gmock_all_test.d \
./comp/dep/googletest/googlemock/test/gmock_ex_test.d \
./comp/dep/googletest/googlemock/test/gmock_leak_test_.d \
./comp/dep/googletest/googlemock/test/gmock_link2_test.d \
./comp/dep/googletest/googlemock/test/gmock_link_test.d \
./comp/dep/googletest/googlemock/test/gmock_output_test_.d \
./comp/dep/googletest/googlemock/test/gmock_stress_test.d \
./comp/dep/googletest/googlemock/test/gmock_test.d 

OBJS += \
./comp/dep/googletest/googlemock/test/gmock-actions_test.o \
./comp/dep/googletest/googlemock/test/gmock-cardinalities_test.o \
./comp/dep/googletest/googlemock/test/gmock-generated-actions_test.o \
./comp/dep/googletest/googlemock/test/gmock-generated-function-mockers_test.o \
./comp/dep/googletest/googlemock/test/gmock-generated-internal-utils_test.o \
./comp/dep/googletest/googlemock/test/gmock-generated-matchers_test.o \
./comp/dep/googletest/googlemock/test/gmock-internal-utils_test.o \
./comp/dep/googletest/googlemock/test/gmock-matchers_test.o \
./comp/dep/googletest/googlemock/test/gmock-more-actions_test.o \
./comp/dep/googletest/googlemock/test/gmock-nice-strict_test.o \
./comp/dep/googletest/googlemock/test/gmock-port_test.o \
./comp/dep/googletest/googlemock/test/gmock-spec-builders_test.o \
./comp/dep/googletest/googlemock/test/gmock_all_test.o \
./comp/dep/googletest/googlemock/test/gmock_ex_test.o \
./comp/dep/googletest/googlemock/test/gmock_leak_test_.o \
./comp/dep/googletest/googlemock/test/gmock_link2_test.o \
./comp/dep/googletest/googlemock/test/gmock_link_test.o \
./comp/dep/googletest/googlemock/test/gmock_output_test_.o \
./comp/dep/googletest/googlemock/test/gmock_stress_test.o \
./comp/dep/googletest/googlemock/test/gmock_test.o 


# Each subdirectory must supply rules for building sources it contributes
comp/dep/googletest/googlemock/test/%.o: ../comp/dep/googletest/googlemock/test/%.cc
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/usr/include/ -I/usr/local/include -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


