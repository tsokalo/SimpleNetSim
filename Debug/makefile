################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include utils/subdir.mk
-include traffic/subdir.mk
-include test/subdir.mk
-include routing-rules/subdir.mk
-include old-stuff/subdir.mk
-include network/subdir.mk
-include lp-solver/subdir.mk
-include galois-field/subdir.mk
-include ccack/subdir.mk
-include build/utils/subdir.mk
-include build/traffic/subdir.mk
-include build/test/subdir.mk
-include build/routing-rules/subdir.mk
-include build/network/subdir.mk
-include build/lp-solver/subdir.mk
-include build/galois-field/subdir.mk
-include build/ccack/subdir.mk
-include build/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(CC_DEPS)),)
-include $(CC_DEPS)
endif
ifneq ($(strip $(C++_DEPS)),)
-include $(C++_DEPS)
endif
ifneq ($(strip $(C_UPPER_DEPS)),)
-include $(C_UPPER_DEPS)
endif
ifneq ($(strip $(CXX_DEPS)),)
-include $(CXX_DEPS)
endif
ifneq ($(strip $(CPP_DEPS)),)
-include $(CPP_DEPS)
endif
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: SnsMulticast

# Tool invocations
SnsMulticast: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: GCC C++ Linker'
	g++  -o "SnsMulticast" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '

# Other Targets
clean:
	-$(RM) $(CC_DEPS)$(C++_DEPS)$(EXECUTABLES)$(C_UPPER_DEPS)$(CXX_DEPS)$(OBJS)$(CPP_DEPS)$(C_DEPS) SnsMulticast
	-@echo ' '

.PHONY: all clean dependents
.SECONDARY:

-include ../makefile.targets
