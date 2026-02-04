#!/usr/bin/bash

# what do i want?
# subcommand to build standalone binary from argument file path
# i want to pass to raec
#
# convert all source arguments to objects
# link all object arguments into a binary
#
# i want rcc to handle
# - running gpp, and knowing where the include directory is
# - linking with entry_point.c and the eventual std library?
# RCC_INCLUDE
# RCC_LIB

INCLUDE=$RCC_INCLUDE
if [ -z "$INCLUDE" ]; then
	echo "Set missing \$RCC_INCLUDE environment variable."
	exit 1
fi

LIB=$RCC_LIB
if [ -z "$LIB" ]; then
	echo "Set missing \$RCC_LIB environment variable."
	exit 1
fi
