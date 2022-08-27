#include <stdio.h>
#include "structures.h"
#include "huffman.h"
#include "stdlib.h"
#include "string.h"
#include "stdarg.h"

static const char file[]=__FILE__;
const int huffTag = 'H'|'U'<<8|'F'<<16|'F'<<24;



HuffFileHeader header = {0};
BitStringHandle alphabet[256] = {0};


int huffnode_less(const void* left, const void* right)
{
    HuffNode *L = *(HuffNode**)left, *R=*(HuffNode**)right;
    return L->freq>R->freq; //minheap required;
}

void huffnode_print(const void* data)
{
    HuffNodeHandle p = *(HuffNodeHandle*)data;

    if(p->symbol)
    {
        printf("%d:%d", p->freq, p->symbol);
    }
    else
        printf("%d", p->freq);
    if(p->code)
    {
        printf("code= ");
        bitstring_print(p->code);
    }
    printf("\n");
}

void gen_histogram(const unsigned char *data, size_t nBytes, size_t *hist)
{
    memset(hist,0,256*sizeof(size_t));
    for(int i=0; i<nBytes;i++)
        hist[data[i]]++;
}

HuffNodeHandle gen_tree(size_t *hist)
{
    PQueueHandle q;
    HuffNodeHandle node, L, R;
    PQUEUE_ALLOC(HuffNodeHandle, q, 0, huffnode_less, 0);

    for(int k=0;k<256;k++)
    {
        if(hist[k])
        {
            node = (HuffNodeHandle)malloc(sizeof(HuffNode));
            node->left=0;
            node->right=0;
            node->symbol = k;
            node->freq=hist[k];
            node->code=0;
            enqueue(&q,&node);
        }
    }

    for(int j=0; q->count>1; j++)
    {
        L=*(HuffNodeHandle*)q->data;
        dequeue(&q);
        R=*(HuffNodeHandle*)q->data;
        dequeue(&q);

        node = (HuffNodeHandle)malloc(sizeof(HuffNode));
        node->symbol=0;
        node->freq=L->freq + R->freq;
        node->left = L;
        node->right = R;
        node->code = 0;

        enqueue(&q, &node);
    }
    node = *(HuffNodeHandle*)q->data;
    pqueue_free(&q);
    return node;
}

void gen_alphabet(HuffNodeHandle root, BitStringHandle  *alphabet)
{
    SList s;
    HuffNodeHandle node;
    BitStringHandle str;
    char bit;

    slist_init(&s, sizeof(HuffNodeHandle), 0);
    STACK_PUSH(&s, &root);
    while(s.count)
    {
        node = *(HuffNodeHandle*)STACK_TOP(&s);
        STACK_POP(&s);

        if(!node->left && !node->right)
        {
            alphabet[node->symbol] = node->code;
            node->code = 0;
            continue;
        }
        if(node->left)
        {
            bit = 0;
            if(node->code)
                node->left->code = bitstring_construct(node->code->data, node->code->bitCount,0,1);
            else
                node->left->code = bitstring_construct(0,0,0,1);
            bitstring_append(&node->left->code, &bit, 1,0);
            STACK_PUSH(&s, &node->left);
        }
        if(node->right)
        {
            bit = 1;
            if(node->code)
                node->right->code = bitstring_construct(node->code->data, node->code->bitCount,0,1);
            else
                node->right->code = bitstring_construct(0,0,0,1);
            bitstring_append(&node->right->code, &bit, 1,0);
            STACK_PUSH(&s, &node->right);
        }
    }
    slist_clear(&s);
}

void print_histogram(size_t *hist)
{
    for(int i=0;i<256;i++)
    {
        if(hist[i])
            printf("0x%02X \'%c\': %lld\n", i,i, (long long)hist[i]);
    }
    printf("\n");
}

void print_alphabet(BitStringHandle *alphabet, size_t *hist, size_t nsymbols)
{
    for(size_t k=0; k<nsymbols;k++)
    {
        if(alphabet[k])
        {
            printf("\'%c\' %d ", (char)k, hist[k]);
            bitstring_print(alphabet[k]);
            printf("\n");
        }
    }
}

void print_tree(HuffNodeHandle root, int depth)
{
    if(root)
    {
        print_tree(root->left, depth+1);

        printf("%3d %*s",depth, depth, "");
        huffnode_print(&root);
        print_tree(root->right, depth+1);
    }
}

void free_tree(HuffNodeHandle *root)
{
    if(*root)
    {
        free_tree(&root[0]->left);
        free_tree(&root[0]->right);
        bitstring_free(&root[0]->code);
        free(*root);
        *root=0;
    }
}

int huff_compress(const unsigned char *src, size_t srcSize, ArrayHandle *dst)
{
    HuffNodeHandle root;
    BitStringHandle d2;
    int success;


    success = 1;
    gen_histogram(src, srcSize, header.hist);
    root = gen_tree(header.hist);
    gen_alphabet(root, alphabet);
    //print_alphabet(alphabet, header.hist, 256);
    free_tree(&root);

    d2 = bitstring_construct(0,0,0,srcSize);
    for(size_t i=0; i<srcSize;i++)
    {
        int sym =src[i];
        BitStringHandle code = alphabet[sym];
        bitstring_append(&d2, code->data, code->bitCount, 0);
        //bitstring_print(d2);
    }

    header.magic = huffTag;
    header.zeros = 0;
    header.u_size = srcSize;
    header.c_size = (d2->bitCount+7)/8;
    if(!(*dst))
        ARRAY_ALLOC(unsigned char, *dst, 0, sizeof(HuffFileHeader)+header.c_size, 0);
    if(dst[0]->esize!=1)
    {
        success = 0;
        goto finish;
    }
    ARRAY_APPEND(*dst, &header, sizeof(header), 1,0);
    ARRAY_APPEND(*dst, d2->data, header.c_size,1,0);
finish:
    for(int i=0;i<256;i++)
    {
        bitstring_free(alphabet+i);
    }
    bitstring_free(&d2);
    return success;
}

int huff_decompress(const unsigned char *src, size_t srcSize, ArrayHandle *dst)
{
    HuffNodeHandle root;
    size_t bitIdx, nSym;
    int success;

    success = 1;
    if(srcSize<sizeof(header)) //compressed file must contain the header with histogram
        return 0;

    memcpy(&header, src, sizeof(header));
    root = gen_tree(header.hist);


    if(!*dst)
    {
        ARRAY_ALLOC(unsigned char,*dst,0,header.u_size,0);
    }
    else
    {
        ARRAY_APPEND(*dst, 0, 0, 1,header.u_size);
        if(dst[0]->esize!=1)
        {
            free_tree(&root);
            return 0;
        }
    }

    src += sizeof(header);
    srcSize-=sizeof(header);
    for(bitIdx=0, nSym=0;bitIdx<(srcSize<<3) && nSym<header.u_size; ++nSym)
    {
        HuffNodeHandle node = root;
        while(node->left || node->right)
        {
            int bit = src[bitIdx>>3]>>(bitIdx&7)&1;

            if(bit)
                node=node->right;
            else
                node=node->left;
            ++bitIdx;
        }
        ARRAY_APPEND(*dst, &node->symbol,1,1,0);
    }
    free_tree(&root);
    return 1;
}


