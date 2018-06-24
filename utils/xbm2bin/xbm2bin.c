/************************************************
 xbm2bin.c
 ************************************************/

#if !defined XBM2BIN_INVERT
#error "XBM2BIN_INVERT must be defined (either as 1 or as 0)."
#endif
#if !defined XBM2BIN_U8 && !defined XBM2BIN_U32
#error "One of XBM2BIN_U8 or XBM2BIN_U32 must be defined."
#endif

#include <stdio.h>
#include <stdlib.h>

#define WIDTH 128
#define HEIGHT 32
#define SHORT_LENGTH 16
#define BYTE_LENGTH 8
#define EOL "\n"

// Name of defined image
#define NAME line_bits

// Filename to write to
#define FILENAME "line_bits.bin"

#include "line.inc"

int writeToFile(unsigned short * display, char * filename) {
    FILE * fp;
    fp = fopen(filename, "w");
    if (fp < 3)
        return -1;

    int i, j, k, bc = 0;
    unsigned char tmp;
    fprintf(fp, "{"EOL);
    for (i = 0; i < HEIGHT; i = i + BYTE_LENGTH) {
        for (j = (WIDTH - 1); j >= 0; j--) {
            tmp = 0;
            for (k = 0; k < BYTE_LENGTH; k++) {
                tmp = tmp + (display[(i + k) * WIDTH + j] << k);
            }
#if defined XBM2BIN_U8
            fprintf(fp, "0x%02x, ", tmp);
#elif defined XBM2BIN_U32
            if (bc == 0)
                fprintf(fp, "0x");
            fprintf(fp, "%02x", tmp);
            ++bc;
            if (bc == 4) {
                fprintf(fp, ", ");
                bc = 0;
            }
#else
            ;
#endif
        }
        fprintf(fp, EOL);
    }
    fprintf(fp, "},"EOL);

    fclose(fp);
    return 0;
}

unsigned short * expandBitmap(unsigned short * bits, unsigned short * display) {
    int i, j;
    for (i = 0; i < (WIDTH * HEIGHT / SHORT_LENGTH); i++) {
        for (j = 0; j < SHORT_LENGTH; j++) {
            display[i * SHORT_LENGTH + j] = (bits[i] & (1 << j)) >> j;
            if (XBM2BIN_INVERT == 1) {
                if (display[i * SHORT_LENGTH + j] == 0)
                    display[i * SHORT_LENGTH + j] = 1;
                else
                    display[i * SHORT_LENGTH + j] = 0;
            }
        }
    }
    return display;
}

int main() {
    unsigned short * display = malloc(2 * WIDTH * HEIGHT);
    display = expandBitmap(NAME, display);
    writeToFile(display, FILENAME);
    free(display);
    return 0;
}

