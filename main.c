#include "undiet.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>


int main(int argc, char *argv[])
{
    int ret = EXIT_SUCCESS;

    struct stat st;
    uint8_t *out = NULL;
    uint8_t *in = NULL;
    ssize_t cnt = 0;
    int fd_out = -1;
    int fd_in = -1;


    if (argc != 3) {
        fprintf(stderr, "Usage: undiet <compressed file> <uncompressed file>\n");
        return EXIT_FAILURE;
    }

    fd_in = open(argv[1], O_RDONLY);
    if (fd_in == -1) {
        fprintf(stderr, "Error opening input file '%s': %s (errno: %d)\n", argv[1], strerror(errno), errno);
        ret = EXIT_FAILURE;
        goto exit;
    }

    if (fstat(fd_in, &st) == -1) {
        fprintf(stderr, "Failed to call stat for input file '%s': %s (errno: %d)\n", argv[1], strerror(errno), errno);
        ret = EXIT_FAILURE;
        goto exit;
    }

    if (st.st_size > UNDIET_MAX_PACKED_FILESIZE) {
        fprintf(stderr, "Input file too big: %ld bytes. Max allowed is %u bytes (1MB).\n", (long)st.st_size, UNDIET_MAX_PACKED_FILESIZE);
        ret = EXIT_FAILURE;
        goto exit;
    }

    in = malloc(st.st_size);
    if (in == NULL) {
        fprintf(stderr, "Memory allocation for input buffer failed (%ld bytes): %s\n", (long)st.st_size, strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    cnt = read(fd_in, in, st.st_size);
    if (cnt != st.st_size) {
        fprintf(stderr, "Error reading from input file '%s'. Expected %ld bytes, got %zd. Error: %s\n", argv[1], (long)st.st_size, cnt, strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    if (undiet_isvalid(in, st.st_size) == false) {
        fprintf(stderr, "Invalid input: File '%s' is not compressed with the DIET algorithm or header is invalid.\n", argv[1]);
        ret = EXIT_FAILURE;
        goto exit;
    }

    uint32_t in_size = undiet_get_compressed_size(in, st.st_size);
    if (in_size + UNDIET_HEADER_SIZE != st.st_size) {
        fprintf(stderr, "Format error: Internal compressed size (%u) + header size (%d) does not match file size (%ld).\n", 
                in_size, UNDIET_HEADER_SIZE, (long)st.st_size);
        ret = EXIT_FAILURE;
        goto exit;
    }

    uint16_t expected_crc = undiet_get_crc(in, in_size);
    uint16_t calculated_crc = undiet_calc_crc16(in, st.st_size);
    if (calculated_crc != expected_crc) {
        fprintf(stderr, "Integrity error: CRC mismatch in compressed data! (Expected: 0x%04X, Calculated: 0x%04X)\n", 
                expected_crc, calculated_crc);
        ret = EXIT_FAILURE;
        goto exit;
    }

    uint32_t out_size = undiet_get_uncompressed_size(in, st.st_size);
    if ((out_size == 0)||(out_size > UNDIET_MAX_UNPACKED_FILESIZE)) {
        fprintf(stderr, "Decompression error: Illegal output size specified in header: %u bytes (Limit: %u bytes).\n", 
                out_size, UNDIET_MAX_UNPACKED_FILESIZE);
        ret = EXIT_FAILURE;
        goto exit;
    }

    out = malloc(out_size);
    if (out == NULL) {
        fprintf(stderr, "Memory allocation for output buffer failed (%u bytes): %s\n", out_size, strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    memset(out, 0x00, out_size);
    int32_t decompressed_size = undiet_unpack(in, out);
    if (decompressed_size != (int32_t)out_size) {
        fprintf(stderr, "Decompression failure: Expected to unpack %u bytes, but got %d bytes.\n", out_size, decompressed_size);
        ret = EXIT_FAILURE;
        goto exit;
    }

    fd_out = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd_out == -1) {
        fprintf(stderr, "Error creating output file '%s': %s (errno: %d)\n", argv[2], strerror(errno), errno);
        ret = EXIT_FAILURE;
        goto exit;
    }

    cnt = write(fd_out, out, out_size);
    if (cnt != (ssize_t)out_size) {
        fprintf(stderr, "Error writing to output '%s'. Expected %u bytes, wrote %zd. Error: %s\n", argv[2], out_size, cnt, strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    printf("Successfully unpacked '%s' to '%s' (%u bytes).\n", argv[1], argv[2], out_size);

exit:
    if (fd_out != -1)
        close(fd_out);
    if (fd_in != -1)
        close(fd_in);

    if (out)
        free(out);
    if (in)
        free(in);

    return ret;
}
