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

#define TEST_STDIO 1
#define MAX_RAM_SIZE_BYTES 464064

#define FILENAME "file.in"
#define FILENAME_OUT "file.out"

#define FILENAME_STDIO "file_stdio.in"
#define FILENAME_STDIO_OUT "file_stdio.out"

const unsigned buffer_start = 1024;
const unsigned size_mb = 1;
const unsigned buffer_end = 32 * 1024;

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

    memset(data, 20, buffer_size); // random data

    fp1 = xscope_open_file(FILENAME, "wb");
    xscope_fwrite(&fp1, data, sizeof(data) * sizeof(uint8_t));
    xscope_fclose(&fp1);

    fp2 = xscope_open_file(FILENAME_STDIO, "wb");
    xscope_fwrite(&fp2, data, sizeof(data) * sizeof(uint8_t));
    xscope_fclose(&fp2);

    free(data);
}

void create_file_system(const char* filename, size_t buffer_size) {
    char command[256];
    int r;

    // first delete the file
    sprintf(command, "del /S %s", filename);
    r = system(command);
    xassert(r == 0 && "Error deleting file");
    delay_milliseconds(100);

    // Create file
    sprintf(command, "fsutil file createnew %s %d", filename, buffer_size);
    r = system(command);
    xassert(r == 0 && "Error creating file");
    delay_milliseconds(100);
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

    // to avoid compiler optimization
    // file_size += rand() % 255; 
    //printf("File size: %d\n", file_size);

    // Seek back to the beginning
    xscope_fseek(&fp_read, 0, SEEK_SET);

    // Read file into buffer
    uint8_t buffer[1024];
    size_t num_bytes = 0;
    uint64_t t_read_total_time = 0;
    do {
        unsigned t0 = get_reference_time();
        num_bytes = xscope_fread(&fp_read, buffer, sizeof(buffer));
        unsigned t1 = get_reference_time();
        t_read_total_time += (t1 - t0);

        buffer[0] += rand() % 255; // to avoid compiler optimization 
    } while (num_bytes > 0);

    // Close file
    unsigned t_close = get_reference_time();
    xscope_fclose(&fp_read);
    unsigned t_close_end = get_reference_time();

    // Open file for writing
    fp_write = xscope_open_file(FILENAME_OUT, "wb");

    // Write buffer to another file
    size_t bytes_to_write = buffer_size;
    uint64_t t_write_total_time = 0;
    do {
        unsigned t2 = get_reference_time();
        xscope_fwrite(&fp_write, buffer, sizeof(buffer));
        unsigned t3 = get_reference_time();
        t_write_total_time += (t3 - t2);

        bytes_to_write -= sizeof(buffer);
    } while (bytes_to_write > 0);

    // Close file
    xscope_fclose(&fp_write);

    // Print results
    print_info(buffer_size, "Open", t_open_end - t_open);
    print_info(buffer_size, "Close", t_close_end - t_close);
    print_info(buffer_size, "Seek", t_seek_end - t_seek);
    print_info(buffer_size, "Tell", t_tell_end - t_tell);
    print_info(buffer_size, "Read", t_read_total_time);
    print_info(buffer_size, "Write", t_write_total_time);
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

    // to avoid compiler optimization
    // file_size += rand() % 255; 
    //printf("File size: %d\n", file_size);

    // Seek back to the beginning
    fseek(fp_read, 0, SEEK_SET);

    // Read file into buffer
    uint8_t buffer[1024];
    size_t num_bytes = 0;
    uint64_t t_read_total_time = 0;
    do {
        unsigned t0 = get_reference_time();
        num_bytes = fread(buffer, sizeof(uint8_t), sizeof(buffer), fp_read);
        unsigned t1 = get_reference_time();
        t_read_total_time += (t1 - t0);
        buffer[0] += rand() % 255; // to avoid compiler optimization 
    } while (num_bytes > 0);

    // Close file
    unsigned t_close = get_reference_time();
    fclose(fp_read);
    unsigned t_close_end = get_reference_time();

    // Open file for writing
    fp_write = fopen(FILENAME_STDIO_OUT, "wb");
    xassert(fp_write != NULL);

    // Write buffer to another file
    size_t bytes_to_write = buffer_size;
    uint64_t t_write_total_time = 0;
    do {
        unsigned t2 = get_reference_time();
        fwrite(buffer, sizeof(uint8_t), sizeof(buffer), fp_write);
        unsigned t3 = get_reference_time();
        t_write_total_time += (t3 - t2);

        bytes_to_write -= sizeof(buffer);
    } while (bytes_to_write > 0);

    // Close file
    fclose(fp_write);

    // Print results
    print_info(buffer_size, "Open", t_open_end - t_open);
    print_info(buffer_size, "Close", t_close_end - t_close);
    print_info(buffer_size, "Seek", t_seek_end - t_seek);
    print_info(buffer_size, "Tell", t_tell_end - t_tell);
    print_info(buffer_size, "Read", t_read_total_time);
    print_info(buffer_size, "Write", t_write_total_time);
}

void main_tile0(chanend_t xscope_chan) {
    xscope_io_init(xscope_chan);
    for (
        unsigned size_bytes = buffer_start;
        size_bytes <= buffer_end;
        size_bytes <<= 1
        ) {
#if TEST_STDIO
        create_file_system(FILENAME_STDIO, size_bytes);
        test_stdio(size_bytes);
#else
        create_file_system(FILENAME, size_bytes);
        test_xscope_fileio(size_bytes);
#endif
        delay_milliseconds(100);
    }
    xscope_close_all_files();
}
