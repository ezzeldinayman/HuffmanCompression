#ifndef HUFFMAN_H
#define HUFFMAN_H

typedef struct HuffNodeStruct
{
    struct HuffNodeStruct *left, *right;
    size_t symbol, freq;
    BitStringHandle code;

} HuffNode, *HuffNodeHandle;

typedef struct HuffFileHeaderStruct
{
    int magic, zeros;
    size_t u_size, c_size;
    size_t hist[256];
    unsigned char data[];
} HuffFileHeader, *HuffFileHandle;

int huffnode_less(const void* left, const void* right);
void huffnode_print(const void* data);
void gen_histogram(const unsigned char *data, size_t nBytes, size_t *hist);
HuffNodeHandle gen_tree(size_t *hist);
void gen_alphabet(HuffNodeHandle root, BitStringHandle  *alphabet);
void print_histogram(size_t *hist);
void print_alphabet(BitStringHandle *alphabet, size_t *hist, size_t nsymbols);
void print_tree(HuffNodeHandle root, int depth);
void free_tree(HuffNodeHandle *root);
int huff_compress(const unsigned char *src, size_t srcSize, ArrayHandle *dst);
int huff_decompress(const unsigned char *src, size_t srcSize, ArrayHandle *dst);


#endif






