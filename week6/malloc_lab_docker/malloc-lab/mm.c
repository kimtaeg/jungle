/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "ateam",
    /* First member's full name */
    "Harry Bovik",
    /* First member's email address */
    "bovik@cs.cmu.edu",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
#define WSIZE 4
#define DSIZE 8
// 1<<12 -> 비트 시프트 연산 
#define CHUNKSIZE (1<<12)

#define MAX(x, y) ((x) > (y)? (x) : (y))

#define PACK(size, alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int * )(p))
#define PUT(p, val) (*(unsigned int *)(p) = (val))

#define GET_SIZE(p) (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)

#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

#define NEXT_BLKP(bp) ((char * ) (bp) + GET_SIZE(((char * ) (bp) - WSIZE)))
#define PREV_BLKP(bp) ((char * ) (bp) - GET_SIZE(((char * ) (bp) - DSIZE)))
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)
static char *heap_listp = 0;

#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))

static void *extend_heap(size_t words);
static void *coalesce(void *bp);
static void *find_fit(size_t asize);
static void place(void *bp, size_t asize);

static void *find_fit(size_t asize){
    void *bp;
    for(bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)){
        if(!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))){
            return bp;
        }
    }
    return NULL;
}

static void place(void *bp, size_t asize){
    size_t csize = GET_SIZE(HDRP(bp));
    
    if((csize - asize) >= (2*DSIZE)){
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize-asize, 0));
        PUT(FTRP(bp), PACK(csize-asize, 0));
    }
    else{
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1){
        return -1;
    }
    PUT(heap_listp, 0);
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));
    heap_listp += (2*WSIZE);

    if(extend_heap(CHUNKSIZE/WSIZE) == NULL){
        return -1;
    }
    return 0;
}

static void *extend_heap(size_t words){
    char *bp;
    size_t size;

    size = (words%2)?(words+1)*WSIZE : words * WSIZE;
    if ((long)(bp = mem_sbrk(size)) == -1)
        return NULL;
    
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));

    return coalesce(bp);
}
/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    // int newsize = ALIGN(size + SIZE_T_SIZE);
    // void *p = mem_sbrk(newsize);
    // if (p == (void *)-1)
	// return NULL;
    // else {
    //     *(size_t *)p = size;
    //     return (void *)((char *)p + SIZE_T_SIZE);
    // }

    size_t asize;
    size_t extendsize;
    char *bp;

    if(size == 0){
        return NULL;
    }

    if(size <= DSIZE){
        asize = 2*DSIZE;
    } 
    else{
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }
    if((bp = find_fit(asize)) != NULL){
        place(bp, asize);
        return bp;
    }

    extendsize = MAX(asize, CHUNKSIZE);
    if((bp = extend_heap(extendsize/WSIZE)) == NULL){
        return NULL;
    }
    place(bp, asize);
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));

    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

static void *coalesce(void *bp){
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));

    // if (prev_alloc && next_alloc){
    //     // case 1
    //     return bp;
    // }

    // else if (prev_alloc && !next_alloc){
    //     // case 2
    //     size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
    // }

    // else if (!prev_alloc && next_alloc){
    //     // case 3
    //     size += GET_SIZE(HDRP(PREV_BLKP(bp)));
    //     PUT(FTRP(bp), PACK(size, 0));
    //     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    //     bp = PREV_BLKP(bp);
    // }

    // else{
    //     // case 4
    //     size += GET_SIZE(HDRP(PREV_BLKP(bp)))+GET_SIZE(FTRP(NEXT_BLKP(bp)));
    //     PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
    //     PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
    //     bp = PREV_BLKP(bp);
    // }

    if(!prev_alloc){
        // 이전 블록 합치기
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));

        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));

        bp = PREV_BLKP(bp);
    }

    if(!next_alloc){
        // 다음 블록 합치기
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));

        PUT(HDRP(bp), PACK(size, 0));          
        PUT(FTRP(bp), PACK(size, 0));
    }

    return bp;
}
/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    // 무조건 새로 realloc하지 않고 이미 바로 옆에 있는 빈 공간을 활용해서 제자리에서 확장
    void *oldptr = ptr;
    void *newptr;
    size_t copySize;
    
    // size -> asize
    size_t asize;

    if(size <= DSIZE){
        asize = 2 * DSIZE;
    }else{
        asize = DSIZE * ((size + (DSIZE - 1)) / DSIZE);
    }
    
    // 현재 블록 크기 -> cur
    size_t cur = GET_SIZE(HDRP(ptr));
    // 현재 블록으로 충분하면
    if(cur >= asize){
        return ptr;
    }

    // 다음 블록이 free이고 현재 + 다음 블록으로 충분하면 
    if(GET_ALLOC(FTRP(NEXT_BLKP(ptr))) == 0 && 
        cur + GET_SIZE(HDRP(NEXT_BLKP(ptr))) >= asize){
        // 현재 블록 + 다음블록 합치기
        cur += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        // 헤더 갱신
        PUT(HDRP(ptr), PACK(cur, 0));
        // 풋터 갱신
        PUT((char *)ptr + cur - DSIZE, PACK(cur, 0));

        return ptr;
    }

    newptr = mm_malloc(size);

    if(newptr == NULL){
        return NULL;
    }
    // 기존 payload 크기
    copySize = GET_SIZE(HDRP(oldptr)) - DSIZE;

    // 새로 요청한 크기보다 기존 payload가 크면 size만 복사
    if (size < copySize) {
        copySize = size;
    }

    // 기존 데이터 복사
    memcpy(newptr, oldptr, copySize);

    // 기존 블록 해제
    mm_free(oldptr);

    return newptr;
}