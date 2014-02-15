// Copyright 2009, Joe Tsai. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE.md file.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>


/* Helper macros */
#define FUNC_RETURN(fn, rc) { fn(); return rc; }
#define FUNC_PRINT_RETURN(fn, st, rc) { fn(); printf(st); return rc; }
#define IS_POWER_2(d) (((d) & ((d)-1)) == 0)


/* Struct definitions */
typedef struct __attribute__((packed)) {
    char     chunk_id[4];
    uint32_t chunk_size;
    char     format[4];
} WaveHeader;

typedef struct __attribute__((packed)) {
    char     chunk_id[4];
    uint32_t chunk_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
} FormatChunk;

typedef struct __attribute__((packed)) {
    char     chunk_id[4];
    uint32_t chunk_size;
    uint8_t* data;
} DataChunk;

typedef struct {
    FILE*   out;
    size_t  out_cnt;
    uint8_t buf[16];
    size_t  buf_cnt;
} HexFile;


/* Global constants */
const char help_msg[] = (
    "This program will generate an Intel Hex file containing the sound data\n"
    "from a series of wave files. Only monophonic sounds at rates of 8000,\n"
    "11025, or 22050 samples per second are supported.\n\n"
);


int get_input(char*** _wav_files, int* _num_files);
int process_wavfile(HexFile* hf, const char* wav_file, int wav_idx);
HexFile* hexfile_open(const char* filename);
size_t hexfile_tell(HexFile* hf);
int hexfile_write(HexFile* hf, void* buf, size_t size);
int hexfile_close(HexFile* hf);


int main(int argc, char* argv[]) {
    int scan;
    int num_files = 0;
    char** wav_files = NULL;
    HexFile* hex_file = NULL;
    void ret_func() {
        hexfile_close(hex_file);
        if (argc <= 1) {
            for (scan = 0; scan < num_files; scan++)
                free(wav_files[scan]);
            free(wav_files);
        }
    }

    // Get list of wave files to read
    if (argc > 1) {
        wav_files = &argv[1];
        num_files = argc-1;
    } else {
        if (get_input(&wav_files, &num_files))
            FUNC_RETURN(ret_func, -1);
    }

    // Open the Intel HEX file
    hex_file = hexfile_open("eeprom.hex");
    if (hex_file == NULL)
        FUNC_PRINT_RETURN(ret_func, "Could not open hex file\n", -1);

    // Process each wave file
    printf("Begin processing...\n\n");
    for (scan = 0; scan < num_files; scan++)
        if (process_wavfile(hex_file, wav_files[scan], scan))
            FUNC_RETURN(ret_func, -1);
    printf("Finish processing...\n");

    // Close the Intel HEX file
    bool fail = hexfile_close(hex_file);
    hex_file = NULL;
    if (fail)
        FUNC_PRINT_RETURN(ret_func, "Could not close hex file\n", -1);

    FUNC_RETURN(ret_func, 0);
}


// Read a list wave files to process from stdin. This function
// will allocate an array for wav_files and report the number
// of filenames present in the array. It is the callee's
// responsibility to free the array.
int get_input(char*** _wav_files, int* _num_files) {
    int num_files = 0;
    char** wav_files = NULL;
    void ret_func() {
        (*_wav_files) = wav_files;
        (*_num_files) = num_files;
    }

    num_files = 0;
    wav_files =  malloc(1*sizeof(uintptr_t));
    if (wav_files == NULL)
        FUNC_PRINT_RETURN(ret_func, "Memory error\n", -1);

    printf(help_msg);
    while (true) {
        printf("Enter wave file to convert (empty line to stop): ");

        // Read line from stdin
        char* line = NULL;
        size_t len = 0;
        if (getline(&line, &len, stdin) == -1)
            FUNC_PRINT_RETURN(ret_func, "Memory error\n", -1);
        strtok(line, "\r\n");

        // Store the file if not a blank line
        if (!iscntrl(line[0])) {
            wav_files[num_files] = line;
            num_files++;

            // If num_files is power of two, then expand array
            if (IS_POWER_2(num_files)) {
                void* ptr = wav_files;
                wav_files = realloc(ptr, 2*num_files*sizeof(uintptr_t));
                if (wav_files == NULL) {
                    wav_files = ptr;
                    FUNC_PRINT_RETURN(ret_func, "Memory error\n", -1);
                }
            }
        } else {
            free(line);
            break;
        }
    }
    printf("\n");

    FUNC_RETURN(ret_func, 0);
}


// Opens wav_file, parses it as a WAVE file, and then dumps the
// sound samples into hex_file.
int process_wavfile(HexFile* hex_file, const char* wav_file, int wav_idx) {
    WaveHeader wav_hdr;
    FormatChunk fmt_chk;
    DataChunk data_chk;

    int fsize = 0;
    FILE* fwave = NULL;
    void* buf = NULL;
    void ret_func() {
        if (fwave != NULL)
            fclose(fwave);
        free(buf);
    }

    printf("Wave %d: %s\n    ", wav_idx, wav_file);

    // Open wave file
    fwave = fopen(wav_file, "rb");
    if (fwave == NULL)
        FUNC_PRINT_RETURN(ret_func, "Could not open file\n", -1);

    // Get the file size
    if (fseek(fwave, 0, SEEK_END) == -1)
        FUNC_PRINT_RETURN(ret_func, "Could not fseek in file\n", -1);
    fsize = ftell(fwave);
    if (fsize == -1)
        FUNC_PRINT_RETURN(ret_func, "Could not ftell in file\n", -1);
    if (fseek(fwave, 0, SEEK_SET) == -1)
        FUNC_PRINT_RETURN(ret_func, "Could not fseek in file\n", -1);

    buf = malloc(fsize);
    if (buf == NULL)
        FUNC_PRINT_RETURN(ret_func, "Memory error\n", -1);
    if (fread(buf, fsize, 1, fwave) != 1)
        FUNC_PRINT_RETURN(ret_func, "Read error\n", -1);

    int hdr_size = sizeof(wav_hdr) + sizeof(fmt_chk) + sizeof(data_chk);
    hdr_size -= sizeof(data_chk.data); // Data pointer doesn't count
    if (fsize < hdr_size)
        FUNC_PRINT_RETURN(ret_func, "Filesize too small for headers\n", -1);

    void* _buf = buf;

    // Process the wave header
    memcpy(&wav_hdr, _buf, sizeof(wav_hdr));
    _buf += sizeof(wav_hdr);
    if (memcmp(wav_hdr.chunk_id, "RIFF", sizeof(wav_hdr.chunk_id)))
        FUNC_PRINT_RETURN(ret_func, "Wave header chunk ID mismatch\n", -1);
    if (wav_hdr.chunk_size != (fsize-8))
        FUNC_PRINT_RETURN(ret_func, "Wave filesize mismatch\n", -1);
    if (memcmp(wav_hdr.format, "WAVE", sizeof(wav_hdr.format)))
        FUNC_PRINT_RETURN(ret_func, "Wave format mismatch\n", -1);

    // Process the format chunk
    memcpy(&fmt_chk, _buf, sizeof(fmt_chk));
    _buf += sizeof(fmt_chk);
    if (memcmp(fmt_chk.chunk_id, "fmt ", sizeof(fmt_chk.chunk_id)))
        FUNC_PRINT_RETURN(ret_func, "Format chunk ID mismatch\n", -1);
    if (fmt_chk.chunk_size != sizeof(fmt_chk) - 8)
        FUNC_PRINT_RETURN(ret_func, "Format chunk size mismatch\n", -1);
    if (fmt_chk.audio_format != 1)
        FUNC_PRINT_RETURN(ret_func, "Audio format isn't linear encoding\n", -1);
    if (fmt_chk.num_channels != 1)
        FUNC_PRINT_RETURN(ret_func, "Only one channel supported\n", -1);
    if (fmt_chk.sample_rate != 8000 &&
        fmt_chk.sample_rate != 11025 &&
        fmt_chk.sample_rate != 22050)
        FUNC_PRINT_RETURN(ret_func, "Invalid sampling rate\n", -1);
    if (fmt_chk.sample_rate != fmt_chk.byte_rate)
        FUNC_PRINT_RETURN(ret_func, "Sample rate must equal byte rate\n", -1);
    if (fmt_chk.block_align != 1)
        FUNC_PRINT_RETURN(ret_func, "Block alignment must be 1 byte\n", -1);
    if (fmt_chk.bits_per_sample != 8)
        FUNC_PRINT_RETURN(ret_func, "Sample resolution must be 8-bits\n", -1);

    // Process the data chunk
    memcpy(&data_chk, _buf, sizeof(data_chk) - sizeof(data_chk.data));
    _buf += sizeof(data_chk) - sizeof(data_chk.data);
    data_chk.data = _buf;
    if (memcmp(data_chk.chunk_id, "data", sizeof(data_chk.chunk_id)))
        FUNC_PRINT_RETURN(ret_func, "Data chunk ID mismatch\n", -1);
    if (data_chk.chunk_size != fsize - (_buf - buf))
        FUNC_PRINT_RETURN(ret_func, "Data chunk size mismath\n", -1);

    // Write data to the hex file
    size_t offset = hexfile_tell(hex_file);
    if (hexfile_write(hex_file, data_chk.data, data_chk.chunk_size))
        FUNC_PRINT_RETURN(ret_func, "Failure to write to hex file\n", -1);

    printf("Data offset: 0x%08X\n    ", (int)offset);
    printf("Data length: 0x%08X\n    ", data_chk.chunk_size);
    printf("Sample rate: %d\n    ", fmt_chk.sample_rate);
    printf("\n");

    FUNC_RETURN(ret_func, 0);
}


// Allocate a struct to manage writing the hex file.
HexFile* hexfile_open(const char* filename) {
    HexFile* hf = malloc(sizeof(HexFile));
    if (hf == NULL)
        return NULL;

    FILE* out = fopen(filename, "w");
    if (out == NULL) {
        free(hf);
        return NULL;
    }

    hf->out = out;
    hf->out_cnt = 0;
    hf->buf_cnt = 0;
    return hf;
}


// Tell the number of bytes written in the hex file.
size_t hexfile_tell(HexFile* hf) {
    return hf->out_cnt + hf->buf_cnt;
}


// Write a buffer of length size into the hex file.
int hexfile_write(HexFile* hf, void* buf, size_t size) {
    int scan;
    bool flush = false;
    char cbuf[sizeof(hf->buf)*2+1];
    memset(cbuf, 0, sizeof(cbuf));

    // If the internal buffer is used, then flush
    if (buf == hf->buf) {
        flush = true;
        hf->buf_cnt = 0;
    }

    // For each byte in the buffer
    while (size > 0) {
        hf->buf[hf->buf_cnt] = *((uint8_t*)buf);
        hf->buf_cnt++;
        buf++;
        size--;

        // Only write out to the hex-file if we have a full line or flushing
        if ((hf->buf_cnt == sizeof(hf->buf)) || (flush && size == 0)) {
            uint8_t rec_type = 0x00;
            uint16_t offset = (hf->out_cnt >> 4) << 4;
            uint8_t checksum = hf->buf_cnt + rec_type +
                ((offset >> 0) & 0xFF) + ((offset >> 8) & 0xFF);
            for (scan = 0; scan < hf->buf_cnt; scan++)
                checksum += hf->buf[scan];
            checksum = ~checksum + 1;

            for (scan = 0; scan < hf->buf_cnt; scan++)
                sprintf(&cbuf[scan*2],"%02X", hf->buf[scan]);

            if (fprintf(hf->out, ":%02X%04X%02X%s%02X\n",
                (int)hf->buf_cnt, offset, rec_type, cbuf, checksum) < 0) {
                return -1;
            }

            // Update output counter, reset buffer counter
            hf->out_cnt += hf->buf_cnt;
            hf->buf_cnt = 0;

            // Switch banks
            if (hf->out_cnt % 0x00010000 == 0) {
                uint8_t rec_type = 0x04;
                uint16_t offset = (hf->out_cnt >> 8*sizeof(uint16_t));
                uint8_t checksum = sizeof(offset) + rec_type +
                    ((offset >> 0) & 0xFF) + ((offset >> 8) & 0xFF);
                checksum = ~checksum + 1;

                if (fprintf(hf->out, ":%02X%04X%02X%04X%02X\n",
                    (int)sizeof(offset), 0, rec_type, offset, checksum) < 0) {
                    return -1;
                }
            }
        }
    }

    // This is the end-of-file, so flush the buffer
    if (flush) {
        const char* eof = ":00000001FF\n";
        if (fwrite(eof, strlen(eof), 1, hf->out) != 1) {
            return -1;
        }
    }
    return 0;
}


// Close the hex file and free resources.
int hexfile_close(HexFile* hf) {
    int error = 0;
    if (hf != NULL && hf->out != NULL) {
        error = hexfile_write(hf, hf->buf, hf->buf_cnt);
        error |= fclose(hf->out);
        hf->out = NULL;
    }
    free(hf);
    return error;
}
