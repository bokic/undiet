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


#define MAX_OUTPUT_FILESIZE 0x400000
#define MAX_INPUT_FILESIZE 0x40000

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
        fprintf(stderr, "Error opening input file %s. Error: %s\n", argv[1], strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    if (fstat(fd_in, &st) == -1) {
        fprintf(stderr, "Failed to call stat for input file %s. Error: %s\n", argv[1], strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    if (st.st_size > MAX_INPUT_FILESIZE) {
        fprintf(stderr, "Input file too bit. Should be less than 64kb\n");
        ret = EXIT_FAILURE;
        goto exit;
    }

    in = malloc(st.st_size);
    if (in == NULL) {
        fprintf(stderr, "Malloc for input buffer failed. Error: %s\n", strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    cnt = read(fd_in, in, st.st_size);
    if (cnt != st.st_size) {
        fprintf(stderr, "Error reading from input file %s. Error: %s\n", argv[1], strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    out = malloc(MAX_OUTPUT_FILESIZE);
    if (out == NULL) {
        fprintf(stderr, "Malloc for output buffer failed. Error: %s\n", strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    if (undiet_isvalid(in, st.st_size) == false) {
        fprintf(stderr, "Input file not compressed with diet.\n");
        ret = EXIT_FAILURE;
        goto exit;
    }

    memset(out, 0x00, MAX_OUTPUT_FILESIZE);
    int32_t out_size = undiet_unpack(in, out);
    if (out_size < 0) {
        fprintf(stderr, "Unpacking failed!\n");
        ret = EXIT_FAILURE;
        goto exit;
    }

    fd_out = open(argv[2], O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd_out == -1) {
        fprintf(stderr, "Error creating output file %s. Error: %s\n", argv[2], strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

    cnt = write(fd_out, out, out_size);
    if (cnt != out_size) {
        fprintf(stderr, "Error writing to output %s. Error: %s\n", argv[2], strerror(errno));
        ret = EXIT_FAILURE;
        goto exit;
    }

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
