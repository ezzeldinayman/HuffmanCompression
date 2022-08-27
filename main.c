#include <stdio.h>
#include "structures.h"
#include "huffman.h"
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <Windows.h>

static const char file[]=__FILE__;

int main(int argc, char **argv)
{

    ArrayHandle src, dst;
    unsigned long long t1, t2;

    if(argc!=4)
    {
        printf("Usage:\n program -c/d/m infile outfile\n");
        return 0;
    }


    src = load_bin(argv[2],0);

    if(!src)
    {
        printf("Failed to open %s.", argv[2]);
        return 1;
    }
    if(!strcmp(argv[1], "-c"))
    {
        dst = 0;
        t1 = __rdtsc();
        int success = huff_compress(src->data, src->count, &dst);
        t2 = __rdtsc();
        if(!success)
        {
            printf("Failure in Compressing %s :(", argv[2]);
            exit(1);
        }
        save_file_bin(argv[3], dst->data, dst->count);
        printf("Done Compressing %s :D\n", argv[2]);
        printf("Cycles Elapsed: %ld", t2-t1);
    }
    else if(!strcmp(argv[1], "-d"))
    {
        dst = 0;
        t1 = __rdtsc();
        int success = huff_decompress(src->data, src->count, &dst);
        t2 = __rdtsc();
        if(!success)
        {
            printf("Failure in decompressing %s :(", argv[2]);
            exit(1);
        }
        save_file_bin(argv[3], dst->data, dst->count);
        printf("Done Decompressing %s :D\n", argv[2]);
        printf("Cycles Elapsed: %ld", t2-t1);

    }
    else if(!strcmp(argv[1], "-m"))
    {
        t1 = __rdtsc();
        dst = load_bin(argv[3], 0);
        if(!dst)
        {
            return 1;
        }
        for(int k = 0; k<src->count&&k<dst->count;k++)
        {
            if(src->data[k]!=dst->data[k])
            {
                printf("Error at %d: %0.2X != %0.2X\n", k, src->data[k], dst->data[k]);
                return 1;
            }
        }
        t2 = __rdtsc();
        printf("Done! Both Files are similar %s :D");
        printf("Cycles Elapsed: %ld", t2-t1);

    }

    return 0;

}