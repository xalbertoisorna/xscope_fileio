// Copyright 2024 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
#include <platform.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include <xscope.h>
#include <xcore/assert.h>
#include <xcore/hwtimer.h>

#include "xscope_io_device.h"

/*
This test compares the performance of the standard C stdio
file I/O functions with the xscope_fileio functions.
It generates a table with the results of the tests.
*/

#define TEST_STDIO 0

#define FILENAME "file.in"
#define FILENAME_OUT "file.out"

#define FILENAME_STDIO "file_stdio.in"
#define FILENAME_STDIO_OUT "file_stdio.out"

const unsigned start = 1024; // buffer start size
const unsigned end = 464064; // tile limitation

float ticks_to_KBPS(unsigned ticks, unsigned num_bytes) {
    const float ticks_per_second = 100000000;
    float kb = (float)num_bytes / 1024;
    float time_s = (float)ticks / ticks_per_second;
    return kb / time_s;
}

void create_file(size_t buffer_size) {
    xscope_file_t fp1, fp2;
    uint8_t* data = (uint8_t*)malloc(buffer_size);
    xassert(data != NULL && "Memory allocation failed");
    
    memset(data, 20, buffer_size);
    memset(data+32, 10, buffer_size-32);

    fp1 = xscope_open_file(FILENAME, "wb");
    xscope_fwrite(&fp1, data, sizeof(data) * sizeof(uint8_t));
    xscope_fclose(&fp1);

    fp2 = xscope_open_file(FILENAME_STDIO, "wb");
    xscope_fwrite(&fp2, data, sizeof(data) * sizeof(uint8_t));
    xscope_fclose(&fp2);
    
    free(data);
}

void print_info(size_t buff_size, const char* name, unsigned elapsed) {
    float throughput = ticks_to_KBPS(elapsed, buff_size);
    printf("%d,%s,%d,%.2f\n", buff_size, name, elapsed, throughput);
}

void test_xscope_fileio(size_t buffer_size) {
    xscope_file_t fp_read;
    xscope_file_t fp_write;

    // Open file for reading
    unsigned t_open = get_reference_time();
    fp_read = xscope_open_file(FILENAME, "rb");
    unsigned t_open_end = get_reference_time();

    // Seek
    unsigned t_seek = get_reference_time();
    xscope_fseek(&fp_read, 0, SEEK_END);
    unsigned t_seek_end = get_reference_time();
    // Tell
    unsigned t_tell = get_reference_time();
    unsigned file_size = xscope_ftell(&fp_read);
    unsigned t_tell_end = get_reference_time();

    // Seek back to the beginning
    xscope_fseek(&fp_read, 0, SEEK_SET);

    // Allocate buffer for reading
    uint8_t* data = (uint8_t*)malloc(buffer_size);
    xassert(data != NULL && "Memory allocation failed");

    // Read file into buffer
    unsigned t_read = get_reference_time();
    xscope_fread(&fp_read, data, buffer_size);
    unsigned t_read_end = get_reference_time();

    // Close file
    unsigned t_close = get_reference_time();
    xscope_fclose(&fp_read);
    unsigned t_close_end = get_reference_time();

    // Open file for writing
    fp_write = xscope_open_file(FILENAME_OUT, "wb");
    
    // Write buffer to another file
    unsigned t_write = get_reference_time();
    xscope_fwrite(&fp_write, data, buffer_size);
    unsigned t_write_end = get_reference_time();

    // Close file
    xscope_fclose(&fp_write);
    free(data);

    // Print results
    print_info(buffer_size, "Open", t_open_end - t_open);
    print_info(buffer_size, "Close", t_close_end - t_close);
    print_info(buffer_size, "Seek", t_seek_end - t_seek);
    print_info(buffer_size, "Tell", t_tell_end - t_tell);
    print_info(buffer_size, "Read", t_read_end - t_read);
    print_info(buffer_size, "Write", t_write_end - t_write);
}

void test_stdio(size_t buffer_size) {
    FILE* fp_read;
    FILE* fp_write;

    // Open file for reading
    unsigned t_open = get_reference_time();
    fp_read = fopen(FILENAME_STDIO, "rb");
    unsigned t_open_end = get_reference_time();

    // Seek
    unsigned t_seek = get_reference_time();
    fseek(fp_read, 0, SEEK_END);
    unsigned t_seek_end = get_reference_time();
    // Tell
    unsigned t_tell = get_reference_time();
    unsigned file_size = ftell(fp_read);
    unsigned t_tell_end = get_reference_time();

    // Seek back to the beginning
    fseek(fp_read, 0, SEEK_SET);

    // Allocate buffer for reading
    uint8_t* data = (uint8_t*)malloc(buffer_size);
    xassert(data != NULL && "Memory allocation failed");

    // Read file into buffer
    unsigned t_read = get_reference_time();
    fread(data, sizeof(uint8_t), buffer_size, fp_read);
    unsigned t_read_end = get_reference_time();

    // Close file
    unsigned t_close = get_reference_time();
    fclose(fp_read);
    unsigned t_close_end = get_reference_time();

    // Open file for writing
    fp_write = fopen(FILENAME_STDIO_OUT, "wb");
    xassert(fp_write != NULL);

    // Write buffer to another file
    unsigned t_write = get_reference_time();
    fwrite(data, sizeof(uint8_t), buffer_size, fp_write);
    unsigned t_write_end = get_reference_time();

    // Close file
    fclose(fp_write);
    free(data);

    // Print results
    print_info(buffer_size, "Open", t_open_end - t_open);
    print_info(buffer_size, "Close", t_close_end - t_close);
    print_info(buffer_size, "Seek", t_seek_end - t_seek);
    print_info(buffer_size, "Tell", t_tell_end - t_tell);
    print_info(buffer_size, "Read", t_read_end - t_read);
    print_info(buffer_size, "Write", t_write_end - t_write);
}

void main_tile0(chanend_t xscope_chan) {
    xscope_io_init(xscope_chan);
    for (unsigned size_bytes = start; size_bytes < end; size_bytes *= 2) {
        create_file(size_bytes);
        #if TEST_STDIO
            test_stdio(size_bytes);
        #else
            test_xscope_fileio(size_bytes);
        #endif
        delay_milliseconds(100);
    }
    xscope_close_all_files();
}
