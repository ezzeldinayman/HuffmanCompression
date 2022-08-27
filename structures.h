#ifndef DS_H
#define DS_H
#if defined _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif
#include<stddef.h>//for size_t

#define		COUNTOF(ARR)		(sizeof(ARR)/sizeof(*(ARR)))
#define		G_BUF_SIZE	2048
extern char g_buf[G_BUF_SIZE];

int			log_error(const char *file, int line, const char *format, ...);
int			valid(const void *p);
#define		LOG_ERROR(format, ...)	log_error(file, __LINE__, format, ##__VA_ARGS__)
#define		ASSERT(SUCCESS)			((SUCCESS)!=0||log_error(file, __LINE__, #SUCCESS))
#define		ASSERT_P(POINTER)		(valid(POINTER)||log_error(file, __LINE__, #POINTER " == 0"))

void		memfill(void *dst, const void *src, size_t dstbytes, size_t srcbytes);
void		memswap_slow(void *p1, void *p2, size_t size);//TODO improve the simple byte loop
void 		memswap(void *p1, void *p2, size_t size, void *temp);
void		memreverse(void *p, size_t count, size_t size);//calls memswap
void 		memrotate(void *p, size_t byteoffset, size_t bytesize, void *temp);//temp buffer is min(byteoffset, bytesize-byteoffset)
void		memshuffle(void *base, ptrdiff_t count, size_t esize, int (*rand_fn)(void));
int 		binary_search(void *base, size_t count, size_t esize, int (*threeway)(const void*, const void*), const void *val, size_t *idx);//returns true if found, otherwise the idx is where val should be inserted
void 		isort(void *base, size_t count, size_t esize, int (*threeway)(const void*, const void*));//binary insertion sort



//pqueue
typedef struct PQueueStruct
{
    size_t count, esize, byteCap;
    int (*less)(const void*, const void*);
    void (*destructor)(void*);
    unsigned char data[];
} PQueueHeader, *PQueueHandle;


static void	pqueue_realloc(PQueueHandle *pq, size_t count, size_t pad);
void pqueue_heapify_down(PQueueHandle *pq, size_t idx, void* temp);
void pqueue_build_heap(PQueueHandle *pq);
PQueueHandle pqueue_construct(size_t esize, size_t pad, void (*destructor)(void*), int (*less)(const void*, const void*));
void pqueue_free(PQueueHandle *pq);
void pqueue_heapify_upwards(PQueueHandle *pq, size_t idx, void* temp);
void enqueue(PQueueHandle *pq, const void *src);
void* pqueue_front(PQueueHandle *pq);
void dequeue(PQueueHandle *pq);

#define PQUEUE_ALLOC(TYPE, Q, PAD, LESS, DESTRUCTOR)    Q=pqueue_construct(sizeof(TYPE), PAD, DESTRUCTOR, LESS)

//bitString
typedef struct BitStringStruct
{
	size_t bitCount, byteCap;
	unsigned char data[]; //8bits element
} BitStringHeader, *BitStringHandle;


BitStringHandle bitstring_construct(const void* src, size_t bitCount, size_t bitOffset, size_t bytePad);
void bitstring_free(BitStringHandle *str);
void bitstring_append(BitStringHandle *str, const void *src, size_t bitCount, size_t bitOffset);
int bitstring_get(BitStringHandle *str, size_t bitIdx);
void bitstring_set(BitStringHandle *str, size_t bitIdx, int bit);
void bitstring_print(BitStringHandle str);

//SList

//SList
typedef struct SNodeStruct
{
	struct SNodeStruct *prev;
	unsigned char data[];

} SNode, *SNodeHandle;

typedef struct SListStruct
{
	size_t esize, count;
	void (*destructor)(void*);
	SNodeHandle back, front;
} SList, *SListHandle;

void slist_init(SListHandle list, size_t esize, void (*destructor)(void*));
void slist_clear(SListHandle list);
void* slist_push_front(SListHandle list, const void *data);
void* slist_push_back(SListHandle list, const void *data);
void* slist_front(SListHandle list);
void* slist_back(SListHandle list);
void slist_pop_front(SListHandle list);

//STACK
#define STACK_PUSH(LIST, DATA)	slist_push_front(LIST, DATA)
#define STACK_TOP(LIST)			slist_front(LIST)
#define STACK_POP(LIST)			slist_pop_front(LIST)



//array
#if 1
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4200)//no default-constructor for struct with zero-length array
#endif

typedef struct ArrayHeaderStruct
{
	size_t count, esize, cap;//cap is in bytes
	void (*destructor)(void*);
	unsigned char data[];
} ArrayHeader, *ArrayHandle;



typedef const ArrayHeader *ArrayConstHandle;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
ArrayHandle		array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, void (*destructor)(void*));
ArrayHandle		array_copy(ArrayHandle *arr);//shallow
void			array_clear(ArrayHandle *arr);//keeps allocation
void			array_free(ArrayHandle *arr);
void			array_fit(ArrayHandle *arr, size_t pad);

void*			array_insert(ArrayHandle *arr, size_t idx, const void *data, size_t count, size_t rep, size_t pad);//cannot be nullptr
void*			array_erase(ArrayHandle *arr, size_t idx, size_t count);

size_t			array_size(ArrayHandle const *arr);
void*			array_at(ArrayHandle *arr, size_t idx);
const void*		array_at_const(ArrayConstHandle *arr, int idx);
void*			array_back(ArrayHandle *arr);
const void*		array_back_const(ArrayConstHandle *arr);



#define			ARRAY_ALLOC(ELEM_TYPE, ARR, COUNT, PAD, DESTRUCTOR)	ARR=array_construct(0, sizeof(ELEM_TYPE), COUNT, 1, PAD, DESTRUCTOR)
#define			ARRAY_APPEND(ARR, DATA, COUNT, REP, PAD)			array_insert(&(ARR), (ARR)->count, DATA, COUNT, REP, PAD)
#define			ARRAY_DATA(ARR)			(ARR)->data
#define			ARRAY_I(ARR, IDX)		*(int*)array_at(&ARR, IDX)
#define			ARRAY_U(ARR, IDX)		*(unsigned*)array_at(&ARR, IDX)
#define			ARRAY_F(ARR, IDX)		*(double*)array_at(&ARR, IDX)


//null terminated array
#define			ESTR_ALLOC(TYPE, STR, DATA, LEN)	STR=array_construct(DATA, sizeof(TYPE), LEN, 1, 1, 0)
#define			STR_APPEND(STR, SRC, LEN, REP)		array_insert(&(STR), (STR)->count, SRC, LEN, REP, 1)
#define			STR_FIT(STR)						array_fit(&STR, 1)
#define			ESTR_AT(TYPE, STR, IDX)				*(TYPE*)array_at(&(STR), IDX)

#define			STR_ALLOC(STR, LEN)				ESTR_ALLOC(char, STR, 0, LEN)
#define			STR_COPY(STR, DATA, LEN)		ESTR_ALLOC(char, STR, DATA, LEN)
#define			STR_AT(STR, IDX)				ESTR_AT(char, STR, IDX)

#define			WSTR_ALLOC(STR, LEN)			ESTR_ALLOC(wchar_t, STR, 0, LEN)
#define			WSTR_COPY(STR, DATA, LEN)		ESTR_ALLOC(wchar_t, STR, DATA, LEN)
#define			WSTR_AT(STR, IDX)				ESTR_AT(wchar_t, STR, IDX)
#endif

ArrayHandle		load_bin(const char *filename, int pad);
int save_file_bin(const char *filename, const unsigned char *src, size_t srcSize);

#endif
