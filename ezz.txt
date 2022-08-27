#include <stdio.h>
#include "structures.h"
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

static const char file[]=__FILE__;

int	nErrors=0;

int	log_error(const char *file, int line, const char *format, ...)
{
	va_list args;

	printf("[%d] %s(%d)", nErrors, file, line);
	if(format)
	{
		printf(" ");
		va_start(args, format);
		vprintf(format, args);
		va_end(args);
		printf("\n");
	}
	else
		printf("Unknown error\n");
	++nErrors;
	return 0;
}

void pause()
{
	int k;

	printf("Enter 0 to continue: ");
	scanf("%d", &k);
}

char g_buf[G_BUF_SIZE]={0};

int					valid(const void *p)
{
	size_t val=(size_t)p;

	if(sizeof(size_t)==4)
	{
		switch(val)
		{
		case 0:
		case 0xCCCCCCCC:
		case 0xFEEEFEEE:
		case 0xEEFEEEFE:
		case 0xCDCDCDCD:
		case 0xFDFDFDFD:
		case 0xBAAD0000:
			return 0;
		}
	}
	else
	{
		if(val==0xCCCCCCCCCCCCCCCC)
			return 0;
		if(val==0xFEEEFEEEFEEEFEEE)
			return 0;
		if(val==0xEEFEEEFEEEFEEEFE)
			return 0;
		if(val==0xCDCDCDCDCDCDCDCD)
			return 0;
		if(val==0xBAADF00DBAADF00D)
			return 0;
		if(val==0xADF00DBAADF00DBA)
			return 0;
	}
	return 1;
}

void memfill(void *dst, const void *src, size_t dstbytes, size_t srcbytes)
{
	unsigned copied;
	char *d=(char*)dst;
	const char *s=(const char*)src;
	if(dstbytes<srcbytes)
	{
		memcpy(dst, src, dstbytes);
		return;
	}
	copied=srcbytes;
	memcpy(d, s, copied);
	while(copied<<1<=dstbytes)
	{
		memcpy(d+copied, d, copied);
		copied<<=1;
	}
	if(copied<dstbytes)
		memcpy(d+copied, d, dstbytes-copied);
}
void memswap_slow(void *p1, void *p2, size_t size)
{
	unsigned char *s1=(unsigned char*)p1, *s2=(unsigned char*)p2, *end=s1+size;
	for(;s1<end;++s1, ++s2)
	{
		const unsigned char t=*s1;
		*s1=*s2;
		*s2=t;
	}
}
void memswap(void *p1, void *p2, size_t size, void *temp)
{
	memcpy(temp, p1, size);
	memcpy(p1, p2, size);
	memcpy(p2, temp, size);
}
void memreverse(void *p, size_t count, size_t esize)
{
	size_t totalsize=count*esize;
	unsigned char *s1=(unsigned char*)p, *s2=s1+totalsize-esize;
	void *temp=malloc(esize);
	while(s1<s2)
	{
		memswap(s1, s2, esize, temp);
		s1+=esize, s2-=esize;
	}
	free(temp);
}
void memrotate(void *p, size_t byteoffset, size_t bytesize, void *temp)
{
	unsigned char *buf=(unsigned char*)p;
	
	if(byteoffset<bytesize-byteoffset)
	{
		memcpy(temp, buf, byteoffset);
		memmove(buf, buf+byteoffset, bytesize-byteoffset);
		memcpy(buf+bytesize-byteoffset, temp, byteoffset);
	}
	else
	{
		memcpy(temp, buf+byteoffset, bytesize-byteoffset);
		memmove(buf+bytesize-byteoffset, buf, byteoffset);
		memcpy(buf, temp, bytesize-byteoffset);
	}
}
void memshuffle(void *base, ptrdiff_t count, size_t esize, int (*rand_fn)(void))
{
	ptrdiff_t k, k2;
	void *temp;

	temp=malloc(esize);
	for(k=0;k<count-1;++k)
	{
		k2=rand_fn()%count;
		memswap(base+k*esize, base+k2*esize, esize, temp);
	}
	free(temp);
}
int binary_search(void *base, size_t count, size_t esize, int (*threeway)(const void*, const void*), const void *val, size_t *idx)
{
	unsigned char *buf=(unsigned char*)base;
	ptrdiff_t L=0, R=(ptrdiff_t)count-1, mid;
	int ret;

	while(L<=R)
	{
		mid=(L+R)>>1;
		ret=threeway(buf+mid*esize, val);
		if(ret<0)
			L=mid+1;
		else if(ret>0)
			R=mid-1;
		else
		{
			if(idx)
				*idx=mid;
			return 1;
		}
	}
	if(idx)
		*idx=L+(L<(ptrdiff_t)count&&threeway(buf+L*esize, val)<0);
	return 0;
}
void isort(void *base, size_t count, size_t esize, int (*threeway)(const void*, const void*))
{
	unsigned char *buf=(unsigned char*)base;
	size_t k;
	void *temp;

	if(count<2)
		return;

	temp=malloc((count>>1)*esize);
	for(k=1;k<count;++k)
	{
		size_t idx=0;
		binary_search(buf, k, esize, threeway, buf+k*esize, &idx);
		if(idx<k)
			memrotate(buf+idx*esize, (k-idx)*esize, (k+1-idx)*esize, temp);
	}
	free(temp);
}

//C BitString

BitStringHandle bitstring_construct(const void* src, size_t bitCount, size_t bitOffset, size_t bytePad)
{
	//bytePad: 3shan a7gez el array mara wahda w abaa azawed feha;
	BitStringHandle str;
	size_t srcBytes, cap = 1; //bytes
	unsigned char *srcbytes = (unsigned char*)src; //3malt cast 3shan amshy 3al src byte by byte

	srcBytes = (bitCount+7)>>3;
	cap = srcBytes + bytePad;  //3adad el bits + el pad w round up
	str = (BitStringHandle)malloc(sizeof(BitStringHeader)+cap);
	str->bitCount = bitCount;
	str->byteCap = cap;

	memset(str->data, 0, cap);
	if(src)
	{
		for(size_t b=0;b<bitCount;b++)
		{
			int bit = srcbytes[(bitOffset+b)>>3]>>((bitOffset+b)&7)&1; //keda a5adna bit mo3yna men el byte
			str->data[b>>3] |= bit<<(b&7);
		}
	}
	return str;
}

void bitstring_free(BitStringHandle *str)
{
	free(*str);
	*str=0;
}

void bitstring_append(BitStringHandle *str, const void *src, size_t bitCount, size_t bitOffset)
{
	size_t reqcap,newcap;
	void *p;
	unsigned char *srcbytes = (unsigned char*)src;

	newcap = str[0]->byteCap; //3ashan ashof elbytecap eli 3andi hykaffy wla laa;
	newcap +=! newcap; //guard against 0;
	reqcap = (str[0]->bitCount + bitCount+7)/8; //3adad el bytes el gdeed eli ana mehtago


	for(;newcap<reqcap;newcap<<=1); //keda msh ha7tag a3ml resize 8er ba3d kteer awe;

	if(str[0]->byteCap < newcap)
	{
		p = (BitStringHandle)realloc(*str, sizeof(BitStringHeader)+newcap);
		if(!p)
			LOG_ERROR("Realloc Returned Null Pointer");
		*str = p;
		str[0]->byteCap = newcap;
	}

	size_t byteIdx = (str[0]->bitCount+7)/8;
	memset(str[0]->data + byteIdx, 0, newcap-byteIdx); //elbits el gdeda ba5leha b 0

	if(src)
	{
		for(size_t b=0;b<bitCount;b++)
		{
			int bit = srcbytes[(bitOffset+b)>>3]>>((bitOffset+b)&7)&1;
			str[0]->data[(b+str[0]->bitCount)>>3] |= bit<<((b+str[0]->bitCount)&7);
		}
		str[0]->bitCount += bitCount;
	}

}

int bitstring_get(BitStringHandle *str, size_t bitIdx)
{
	if(!*str)
	{
		LOG_ERROR("bitstring_get OOB: bitIdx=%d", bitIdx);
		return 0;
	}
	if(bitIdx>=str[0]->bitCount)
	{
		LOG_ERROR("bitstring_get OOB: bitCount=%d, bitIdx=%d",(long long)str[0]->bitCount, (long long)bitIdx);
		return 0;
	}
	return str[0]->data[bitIdx>>3]>>(bitIdx&7)&1; //gebt el byte w 3mltlha right shift 3shan el atl3 el index bzbt

}

void bitstring_set(BitStringHandle *str, size_t bitIdx, int bit)
{
	if(!*str)
	{
		LOG_ERROR("bitstring_get OOB: bitIdx=%d", bitIdx);
	}


	if(bitIdx>=str[0]->bitCount)
	{
		LOG_ERROR("bitstring_get OOB: bitCount=%d, bitIdx=%d",(long long)str[0]->bitCount, (long long)bitIdx);
	}


	if(bit)
		str[0]->data[bitIdx>>3]|= 1<<(bitIdx&7);
	else
		str[0]->data[bitIdx>>3]&= 0<<(bitIdx&7);

}

void bitstring_print(BitStringHandle str)
{
	for(int i = 0; i<str->bitCount;i++)
	{
		printf("%d", bitstring_get(&str,i));
	}
	printf("\n");
}
//END_OF_BITSTRING

void slist_init(SListHandle list, size_t esize, void (*destructor)(void*))
{
	list->count=0;
	list->destructor=destructor;
	list->back = 0;
	list->front = 0;
	list->esize = esize;
}

void slist_clear(SListHandle list)
{
	while(list->front)
	{	
		slist_pop_front(list);
	}
	list = 0;
}

void* slist_push_front(SListHandle list, const void *data)
{
	SNodeHandle node = (SNodeHandle)malloc(sizeof(SNode) + list->esize);
	memcpy(node->data, data, list->esize);
	node->prev = list->front;
	if(!list->count)
	{
		list->front = list->back = node;
	}
	else
	{
		node->prev = list->front;
		list->front = node;
	}
	list->count++;
	return node->data;
}

void* slist_push_back(SListHandle list, const void *data)
{
	SNodeHandle node = (SNodeHandle)malloc(sizeof(SNode)+ list->esize);
	memcpy(node->data, data, list->esize);
	node->prev = 0;
	if(!list->count)
	{
		list->front = list->back = node;
	}
	else
	{
		list->back->prev = node;
		list->back = node;
	}
	list->count++;
	return node->data;
}

void* slist_front(SListHandle list)
{
	if(!list->count)
	{
		LOG_ERROR("Popping from empty list");
		return 0;
	}
	return list->front->data;
}

void* slist_back(SListHandle list)
{
	if(!list->count)
	{
		LOG_ERROR("Popping from empty list");
		return 0;
	}
	return list->back->data;
}

void slist_pop_front(SListHandle list)
{
	if(!list->count)
	{
		LOG_ERROR("Popping from empty list.");
	}
	else
	{
		SNodeHandle node = list->front;
		list->front = node->prev;
		if(list->destructor)
			list->destructor(node->data);
		free(node);
		list->count--;
	}
}

//PQUEUE


static void	pqueue_realloc(PQueueHandle *pq, size_t count, size_t pad)//CANNOT be nullptr, array must be initialized with array_alloc()
{
	void* p2;
	size_t reqSize, newcap;

	ASSERT_P(*pq);
	reqSize=(count+pad)*pq[0]->esize, newcap=pq[0]->esize;
	for(;newcap<reqSize;newcap<<=1);
	if(newcap>pq[0]->byteCap)
	{
		p2=(PQueueHandle)realloc(*pq, sizeof(PQueueHeader)+newcap);
		ASSERT_P(p2);
		*pq=(PQueueHandle)p2;
		if(pq[0]->byteCap<newcap)
			memset(pq[0]->data+pq[0]->byteCap, 0, newcap-pq[0]->byteCap);
		pq[0]->byteCap=newcap;
	}
	pq[0]->count=count;
}

void pqueue_heapify_down(PQueueHandle *pq, size_t idx, void* temp)
{
    size_t L, R, largest;
    temp = (unsigned char*)malloc(pq[0]->esize);
    
    for(;idx<pq[0]->count;)
    {
        L = idx*2 + 1;
        R = idx*2 + 2;

        if(L<pq[0]->count && pq[0]->less(pq[0]->data+idx*pq[0]->esize, pq[0]->data+L*pq[0]->esize))
        {
            largest = L;
        }
        else
            largest = idx;

        if(R<pq[0]->count && pq[0]->less(pq[0]->data+largest*pq[0]->esize, pq[0]->data+R*pq[0]->esize))
            largest = R;

        if(idx == largest)
            break;

        memswap(pq[0]->data + largest*pq[0]->esize, pq[0]->data + idx*pq[0]->esize,pq[0]->esize, temp);
        idx = largest;
    }
    free(temp);
}

void pqueue_build_heap(PQueueHandle *pq)
{
    void* temp;
    for(size_t i = pq[0]->count / 2; i>=0; i--)
    {
        pqueue_heapify_down(pq, i, temp);
    }
}


PQueueHandle pqueue_construct( 
                              size_t esize,
                              size_t pad,
                              void (*destructor)(void*),
                              int (*less)(const void*, const void*))
{
	PQueueHandle pq;
	size_t srcsize, dstsize, cap;
    cap = esize+pad*esize;
	pq=(PQueueHandle)malloc(sizeof(PQueueHeader)+cap);
	ASSERT_P(pq);
	pq->count=0;
	pq->esize=esize;
	pq->byteCap=cap;
	pq->destructor=destructor;
    pq->less = less;
    /*
	if(src)
	{
		ASSERT_P(src);
        memcpy(pq->data, src, srcsize);
        memset(pq->data + srcsize, 0, cap-srcsize);
        pqueue_buildheap(&pq);
		//memfill(pq->data, src, dstsize, srcsize);
	}
    */
	memset(pq->data, 0, cap);
	return pq;
}


void pqueue_free(PQueueHandle *pq)
{
    if(*pq && pq[0]->destructor)
    {
        for(size_t k=0; k<pq[0]->count; k++)
        {
            pq[0]->destructor(pq[0]->data+k*pq[0]->count);
        }

        free(*pq);
        *pq=0;
    }
}

void pqueue_heapify_upwards(PQueueHandle *pq, size_t idx, void* temp)
{
    size_t parent;
    temp = (unsigned char*)malloc(pq[0]->esize);
    while(idx>0)
    {
        parent = (idx-1)>>1;
        if(pq[0]->less(pq[0]->data + parent*pq[0]->esize, pq[0]->data + idx*pq[0]->esize))
            memswap(pq[0]->data + parent*pq[0]->esize, pq[0]->data + idx*pq[0]->esize,pq[0]->esize, temp);
        else
            break;
        idx = parent;
    }
}

void enqueue(PQueueHandle *pq, const void *src) //src cant be null ptr //return void pointer to be casted at runtime to a datatype.
{
    size_t start;
    void *temp;

    ASSERT_P(*pq);
    start = pq[0]->count*pq[0]->esize;
    pqueue_realloc(pq, pq[0]->count+1, 0);

    memcpy(pq[0]->data + start, src, pq[0]->esize);

    temp = malloc(pq[0]->esize);
    pqueue_heapify_upwards(pq,pq[0]->count-1,temp); //3shan realloc 7atet el count el gded
    free(temp);
}

void* pqueue_front(PQueueHandle *pq)
{
    return pq[0]->data;
}

void dequeue(PQueueHandle *pq)
{
    size_t k;
    void* temp = malloc(pq[0]->esize);

    ASSERT_P(*pq);    
    if(!pq[0]->count)
    {
		printf("no elements to dequeue");
    }
    if(pq[0]->destructor)
        pq[0]->destructor(pq[0]->data);

    memswap(pq[0]->data, pq[0]->data+(pq[0]->count - 1)*pq[0]->esize, pq[0]->esize, temp);
    pq[0]->count--;
    pqueue_heapify_down(pq,0,temp);
    free(temp);
}

//C array
#if 1
static void array_realloc(ArrayHandle *arr, size_t count, size_t pad)//CANNOT be nullptr, array must be initialized with array_alloc()
{
	ArrayHandle p2;
	size_t size, newcap;

	ASSERT_P(*arr);
	size=(count+pad)*arr[0]->esize, newcap=arr[0]->esize;
	for(;newcap<size;newcap<<=1);
	if(newcap>arr[0]->cap)
	{
		p2=(ArrayHandle)realloc(*arr, sizeof(ArrayHeader)+newcap);
		ASSERT_P(p2);
		*arr=p2;
		if(arr[0]->cap<newcap)
			memset(arr[0]->data+arr[0]->cap, 0, newcap-arr[0]->cap);
		arr[0]->cap=newcap;
	}
	arr[0]->count=count;
}

//Array API
ArrayHandle	array_construct(const void *src, size_t esize, size_t count, size_t rep, size_t pad, void (*destructor)(void*))
{
	ArrayHandle arr;
	size_t srcsize, dstsize, cap;
	
	srcsize=count*esize;
	dstsize=rep*srcsize;
	for(cap=esize+pad*esize;cap<dstsize;cap<<=1);
	arr=(ArrayHandle)malloc(sizeof(ArrayHeader)+cap);
	ASSERT_P(arr);
	arr->count=count;
	arr->esize=esize;
	arr->cap=cap;
	arr->destructor=destructor;
	if(src)
	{
		ASSERT_P(src);
		memfill(arr->data, src, dstsize, srcsize);
	}
	else
		memset(arr->data, 0, dstsize);
		
	if(cap-dstsize>0)//zero pad
		memset(arr->data+dstsize, 0, cap-dstsize);
	return arr;
}
ArrayHandle	array_copy(ArrayHandle *arr)
{
	ArrayHandle a2;
	size_t bytesize;

	if(!*arr)
		return 0;
	bytesize=sizeof(ArrayHeader)+arr[0]->cap;
	a2=(ArrayHandle)malloc(bytesize);
	ASSERT_P(a2);
	memcpy(a2, *arr, bytesize);
	return a2;
}
void array_clear(ArrayHandle *arr)//can be nullptr
{
	if(*arr)
	{
		if(arr[0]->destructor)
		{
			for(size_t k=0;k<arr[0]->count;++k)
				arr[0]->destructor(array_at(arr, k));
		}
		arr[0]->count=0;
	}
}
void array_free(ArrayHandle *arr)//can be nullptr
{
	if(*arr&&arr[0]->destructor)
	{
		for(size_t k=0;k<arr[0]->count;++k)
			arr[0]->destructor(array_at(arr, k));
	}
	free(*arr);
	*arr=0;
}
void array_fit(ArrayHandle *arr, size_t pad)//can be nullptr
{
	ArrayHandle p2;
	if(!*arr)
		return;
	arr[0]->cap=(arr[0]->count+pad)*arr[0]->esize;
	p2=(ArrayHandle)realloc(*arr, sizeof(ArrayHeader)+arr[0]->cap);
	ASSERT_P(p2);
	*arr=p2;
}

void* array_insert(ArrayHandle *arr, size_t idx, const void *data, size_t count, size_t rep, size_t pad)//cannot be nullptr
{
	size_t start, srcsize, dstsize, movesize;
	
	ASSERT_P(*arr);
	start=idx*arr[0]->esize;
	srcsize=count*arr[0]->esize;
	dstsize=rep*srcsize;
	movesize=arr[0]->count*arr[0]->esize-start;
	array_realloc(arr, arr[0]->count+rep*count, pad);
	memmove(arr[0]->data+start+dstsize, arr[0]->data+start, movesize);
	if(data)
		memfill(arr[0]->data+start, data, dstsize, srcsize);
	else
		memset(arr[0]->data+start, 0, dstsize);
	return arr[0]->data+start;
}
void* array_erase(ArrayHandle *arr, size_t idx, size_t count)
{
	size_t k;

	ASSERT_P(*arr);
	if(arr[0]->count<idx+count)
	{
		LOG_ERROR("array_erase() out of bounds: idx=%lld count=%lld size=%lld", (long long)idx, (long long)count, (long long)arr[0]->count);
		if(arr[0]->count<idx)
			return 0;
		count=arr[0]->count-idx;//erase till end of array if OOB
	}
	if(arr[0]->destructor)
	{
		for(k=0;k<count;++k)
			arr[0]->destructor(array_at(arr, idx+k));
	}
	memmove(arr[0]->data+idx*arr[0]->esize, arr[0]->data+(idx+count)*arr[0]->esize, (arr[0]->count-(idx+count))*arr[0]->esize);
	arr[0]->count-=count;
	return arr[0]->data+idx*arr[0]->esize;
}

size_t array_size(ArrayHandle const *arr)//can be nullptr
{
	if(!arr[0])
		return 0;
	return arr[0]->count;
}
void* array_at(ArrayHandle *arr, size_t idx)
{
	if(!arr[0])
		return 0;
	if(idx>=arr[0]->count)
		return 0;
	return arr[0]->data+idx*arr[0]->esize;
}
const void* array_at_const(ArrayConstHandle *arr, int idx)
{
	if(!arr[0])
		return 0;
	return arr[0]->data+idx*arr[0]->esize;
}
void* array_back(ArrayHandle *arr)
{
	if(!*arr||!arr[0]->count)
		return 0;
	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
}
const void*	array_back_const(ArrayConstHandle *arr)
{
	if(!*arr||!arr[0]->count)
		return 0;
	return arr[0]->data+(arr[0]->count-1)*arr[0]->esize;
}
#endif


ArrayHandle	load_bin(const char *filename, int pad)
{
	struct stat info={0};
	FILE *f;
	ArrayHandle str;

	int error=stat(filename, &info);
	if(error)
	{
		strerror_s(g_buf, G_BUF_SIZE, errno);
		LOG_ERROR("Cannot open %s\n%s", filename, g_buf);
		return 0;
	}
	fopen_s(&f, filename, "rb");
	//f=fopen(filename, "r");
	//f=fopen(filename, "rb");
	//f=fopen(filename, "r, ccs=UTF-8");//gets converted to UTF-16 on Windows

	str=array_construct(0, 1, info.st_size, 1, pad+1, 0);
	str->count=fread(str->data, 1, info.st_size, f);
	fclose(f);
	memset(str->data+str->count, 0, str->cap-str->count);
	return str;
}

int save_file_bin(const char *filename, const unsigned char *src, size_t srcSize)
{
	FILE *f;
	size_t bytesRead;

	fopen_s(&f, filename, "wb");
	if(!f)
	{
		printf("Failed to save %s\n", filename);
		return 0;
	}
	bytesRead = fwrite(src,1, srcSize, f);
	fclose(f);
	if(bytesRead!=srcSize)
	{
		printf("Failed to save %s\n", filename);
		return 0;
	}
	return 1;

}

