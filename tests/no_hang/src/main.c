// Copyright 2023-2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// Test to see if it hangs when abused
#include <stdio.h>
#include <stdlib.h>
#include <xcore/assert.h>

#include "xscope_io_device.h"

void main_tile0(void) {
    printf("Commencer\n");

    // This should not cause xscopefileio to hang    
    xscope_file_t fpr = xscope_open_file("doesnt_exist", "rb" );
    uint8_t read_buff[256];
    xscope_fread(&fpr, read_buff, sizeof(read_buff));

    printf("Fin\n");
}
