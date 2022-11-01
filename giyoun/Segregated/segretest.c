// Tony Kim - 98점

/*
 * mm.c - malloc using segregated list
 * In this approach, 
 * Every block has a header and a footer 
 * in which header contains reallocation information, size, and allocation info
 * and footer contains size and allocation info.
 * Free list are tagged to the segregated list.
 * Therefore all free block contains pointer to the predecessor and successor.
 * The segregated list headers are organized by 2^k size.
 * 
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
    "3조",
    /* First member's full name */
    "Hongwook Kim",
    /* First member's email address */
    "woogisky@naver.com",
    /* Second member's full name (leave blank if none) */
    "",
    /* Second member's email address (leave blank if none) */
    ""
};

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8
/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


/*
 * Constants and macros
 */
#define WSIZE 4                     // word size in bytes
#define DSIZE 8                     // double word size in bytes
#define INITCHUNKSIZE (1<<6)        // 
#define CHUNKSIZE (1<<12)           // page size in bytes

#define LISTLIMIT 20                // number of segregated lists
#define REALLOC_BUFFER (1<<7)       // reallocation buffer

#define MAX(x, y) ((x) > (y) ? (x) : (y)) 
#define MIN(x, y) ((x) < (y) ? (x) : (y)) 

/* Pack size and allocation bit into a word */
#define PACK(size, alloc) ((size) | (alloc))

/* Read and write a word at address p */
#define GET(p)            (*(unsigned int *)(p))
#define PUT(p, val)       (*(unsigned int *)(p) = (val) | GET_TAG(p))
#define PUT_NOTAG(p, val) (*(unsigned int *)(p) = (val))

/* Store predecessor or successor pointer for free blocks */
#define SET_PTR(p, bp) (*(unsigned int *)(p) = (unsigned int)(bp))

/* Adjust the reallocation tag */
#define SET_RATAG(p)   (GET(p) |= 0x2)
#define REMOVE_RATAG(p) (GET(p) &= ~0x2)

/* Read the size and allocation bit from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)
#define GET_TAG(p)   (GET(p) & 0x2)


/* Address of block's header and footer */
#define HDRP(bp) ((char *)(bp) - WSIZE)
#define FTRP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

/* Address of next and previous blocks */
#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE((char *)(bp) - WSIZE))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp) - DSIZE))

/* Address of free block's predecessor and successor entries */
#define SUCC_PTR(bp) ((char *)(bp))
#define PRED_PTR(bp) ((char *)(bp) + WSIZE)

/* Address of free block's predecessor and successor on the segregated list */
#define SUCC(bp) (*(char **)(bp))
#define PRED(bp) (*(char **)(PRED_PTR(bp)))


/*
 * Global variables
 */
void *segregated_free_lists[LISTLIMIT]; 


/*
 * Function prototypes
 */
static void *extend_heap(size_t size);
static void *coalesce(void *bp);
static void *place(void *bp, size_t asize);
static void put_free(void *bp, size_t size);
static void remove_free(void *bp);

//static void checkheap(int verbose);


///////////////////////////////// Block information /////////////////////////////////////////////////////////
/*
 
A   : Allocated? (1: true, 0:false)
RA  : Reallocation tag (1: true, 0:false)
 
 < Allocated Block >
 
 
             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |  | A|
    bp ---> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                                                                                               |
            |                                                                                               |
            .                              Payload and padding                                              .
            .                                                                                               .
            .                                                                                               .
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       |     | A|
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 
 
 < Free block >
 
             31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Header :   |                              size of the block                                       |  |RA| A|
    bp ---> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to its predecessor in Segregated list                          |
bp+WSIZE--> +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            |                        pointer to its successor in Segregated list                            |
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
            .                                                                                               .
            .                                                                                               .
            .                                                                                               .
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 Footer :   |                              size of the block                                       |     | A|
            +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 
 
*/
///////////////////////////////// End of Block information /////////////////////////////////////////////////////////

//////////////////////////////////////// Helper functions //////////////////////////////////////////////////////////
static void *extend_heap(size_t size)
{
    void *bp;                   
    size_t asize;                // Adjusted size 
    
    asize = ALIGN(size);
    
    if ((bp = mem_sbrk(asize)) == (void *)-1)
        return NULL;
    
    // Set headers and footer 
    PUT_NOTAG(HDRP(bp), PACK(asize, 0));  
    PUT_NOTAG(FTRP(bp), PACK(asize, 0));   
    PUT_NOTAG(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); 
    put_free
(bp, asize);

    return coalesce(bp);
}

static void put_free(void *bp, size_t size) {
    int list = 0;
    void *search_ptr = bp;
    void *insert_ptr = NULL;
    
    // Select segregated list 
    while ((list < LISTLIMIT - 1) && (size > 1)) {
        size >>= 1;
        list++;
    }
    
    // Keep size ascending order and search
    search_ptr = segregated_free_lists[list];
    while ((search_ptr != NULL) && (size > GET_SIZE(HDRP(search_ptr)))) {
        insert_ptr = search_ptr;
        search_ptr = SUCC(search_ptr);
    }
    
    // Set predecessor and successor 
    if (search_ptr != NULL) {
        if (insert_ptr != NULL) {
            SET_PTR(SUCC_PTR(bp), search_ptr);
            SET_PTR(PRED_PTR(search_ptr), bp);
            SET_PTR(PRED_PTR(bp), insert_ptr);
            SET_PTR(SUCC_PTR(insert_ptr), bp);
        } else {
            SET_PTR(SUCC_PTR(bp), search_ptr);
            SET_PTR(PRED_PTR(search_ptr), bp);
            SET_PTR(PRED_PTR(bp), NULL);
            segregated_free_lists[list] = bp;
        }
    } else {
        if (insert_ptr != NULL) {
            SET_PTR(SUCC_PTR(bp), NULL);
            SET_PTR(PRED_PTR(bp), insert_ptr);
            SET_PTR(SUCC_PTR(insert_ptr), bp);
        } else {
            SET_PTR(SUCC_PTR(bp), NULL);
            SET_PTR(PRED_PTR(bp), NULL);
            segregated_free_lists[list] = bp;
        }
    }
    
    return;
}


static void remove_free(void *bp) {
    int list = 0;
    size_t size = GET_SIZE(HDRP(bp));
    
    // Select segregated list 
    while ((list < LISTLIMIT - 1) && (size > 1)) {
        size >>= 1;
        list++;
    }
    
    if (SUCC(bp) != NULL) {
        if (PRED(bp) != NULL) {
            SET_PTR(PRED_PTR(SUCC(bp)), PRED(bp));
            SET_PTR(SUCC_PTR(PRED(bp)), SUCC(bp));
        } else {
            SET_PTR(PRED_PTR(SUCC(bp)), NULL);
            segregated_free_lists[list] = SUCC(bp);
        }
    } else {
        if (PRED(bp) != NULL) {
            SET_PTR(SUCC_PTR(PRED(bp)), NULL);
        } else {
            segregated_free_lists[list] = NULL;
        }
    }
    
    return;
}


static void *coalesce(void *bp)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    

    // Do not coalesce with previous block if the previous block is tagged with Reallocation tag
    if (GET_TAG(HDRP(PREV_BLKP(bp))))
        prev_alloc = 1;

    if (prev_alloc && next_alloc) {                         // Case 1
        return bp;
    }
    else if (prev_alloc && !next_alloc) {                   // Case 2
        remove_free(bp);
        remove_free(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {                 // Case 3 
        remove_free(bp);
        remove_free(PREV_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    } else {                                                // Case 4
        remove_free(bp);
        remove_free(PREV_BLKP(bp));
        remove_free(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    
    put_free
(bp, size);
    
    return bp;
}

static void *place(void *bp, size_t asize)
{
    size_t ptr_size = GET_SIZE(HDRP(bp));
    size_t remainder = ptr_size - asize;
    
    remove_free(bp);
    
    
    if (remainder <= DSIZE * 2) { // 남는 블록이 8이하이면 분리할 필요가 없다.
        // Do not split block 
        PUT(HDRP(bp), PACK(ptr_size, 1)); 
        PUT(FTRP(bp), PACK(ptr_size, 1)); 
    }
    
    else if (asize >= 100) { // 할당할 메모리가 73이상이면(64이상이면) 초기 chunksize에서 넘기 때문에 할당할 수 있도록 남겨두고 다음블록으로 찾아간다.
        // Split block
        PUT(HDRP(bp), PACK(remainder, 0));
        PUT(FTRP(bp), PACK(remainder, 0));
        PUT_NOTAG(HDRP(NEXT_BLKP(bp)), PACK(asize, 1));
        PUT_NOTAG(FTRP(NEXT_BLKP(bp)), PACK(asize, 1));
        put_free
    (bp, remainder);
        return NEXT_BLKP(bp);
        
    }
    
    else {
        // Split block
        PUT(HDRP(bp), PACK(asize, 1)); // 73미만(64이하) 상태면 할당가는하고 남기때문에 remainder가 8이상 조건 이기때문에 남는 블럭을 할당해준다.
        PUT(FTRP(bp), PACK(asize, 1)); 
        PUT_NOTAG(HDRP(NEXT_BLKP(bp)), PACK(remainder, 0)); 
        PUT_NOTAG(FTRP(NEXT_BLKP(bp)), PACK(remainder, 0)); 
        put_free
    (NEXT_BLKP(bp), remainder);
    }
    return bp;
}



//////////////////////////////////////// End of Helper functions ////////////////////////////////////////


/*
 * mm_init - initialize the malloc package.
 * Before calling mm_malloc, mm_realloc, or mm_free, 
 * the application program calls mm_init to perform any necessary initializations,
 * such as allocating the initial heap area.
 *
 * Return value : -1 if there was a problem, 0 otherwise.
 */
int mm_init(void)
{
    int list;         // List counter
    char *heap_start; // Pointer to beginning of heap
    
    /* Initialize array of pointers to segregated free lists */
    for (list = 0; list < LISTLIMIT; list++) {
        segregated_free_lists[list] = NULL;
    }
    
    /* Allocate memory for the initial empty heap */
    if ((long)(heap_start = mem_sbrk(4 * WSIZE)) == -1)
        return -1;
    
    PUT_NOTAG(heap_start, 0);                            /* Alignment padding */
    PUT_NOTAG(heap_start + (1 * WSIZE), PACK(DSIZE, 1)); /* Prologue header */
    PUT_NOTAG(heap_start + (2 * WSIZE), PACK(DSIZE, 1)); /* Prologue footer */
    PUT_NOTAG(heap_start + (3 * WSIZE), PACK(0, 1));     /* Epilogue header */
    
    if (extend_heap(INITCHUNKSIZE) == NULL)
        return -1;
    
    return 0;
}

/*
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 *
 * Role : 
 * 1. The mm_malloc routine returns a pointer to an allocated block payload.
 * 2. The entire allocated block should lie within the heap region.
 * 3. The entire allocated block should overlap with any other chunk.
 * 
 * Return value : Always return the payload pointers that are alligned to 8 bytes.
 */
void *mm_malloc(size_t size)
{
    size_t asize;       // Adjusted block size
    size_t extendsize;  // Amount to extend heap if no fit
    void *bp = NULL;   // Pointer
    int list = 0;       // List counter
    
    /* Filter invalid block size */
    if (size == 0)
        return NULL;
    
    /* Adjust block size to include boundary tags and alignment requirements */
    if (size <= DSIZE) {
        asize = 2 * DSIZE;
    } else {
        asize = ALIGN(size+DSIZE);
    }
    
    /* Select a free block of sufficient size from segregated list */
    size_t searchsize = asize;
    while (list < LISTLIMIT) {
        if ((list == LISTLIMIT - 1) || ((searchsize <= 1) && (segregated_free_lists[list] != NULL))) {
            bp = segregated_free_lists[list];
            /* Ignore blocks that are too small or marked with the reallocation bit */
            while ((bp != NULL) && ((asize > GET_SIZE(HDRP(bp))) || (GET_TAG(HDRP(bp)))))
            {
                bp = SUCC(bp);
            }
            if (bp != NULL)
                break;
        }
        
        searchsize >>= 1;
        list++;
    }
    
    /* Extend the heap if no free blocks of sufficient size are found */
    if (bp == NULL) {
        extendsize = MAX(asize, CHUNKSIZE);
        
        if ((bp = extend_heap(extendsize)) == NULL)
            return NULL;
    }
    
    /* Place the block */
    bp = place(bp, asize);
    
    
    /* Return pointer to newly allocated block */
    return bp;
}

/*
 * mm_free - Freeing a block does nothing.
 *
 * Role : The mm_free routine frees the block pointed to by bp
 *
 * Return value : returns nothing
 */
void mm_free(void *bp)
{
    size_t size = GET_SIZE(HDRP(bp));
 
    /* Unset the reallocation tag on the next block */
    REMOVE_RATAG(HDRP(NEXT_BLKP(bp)));

    /* Adjust the allocation status in boundary tags */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    
    /* Insert new block into appropriate list */
    put_free
(bp, size);
    
    /* Coalesce free block */
    coalesce(bp);
    
    return;
}

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 *
 * Role : The mm_realloc routine returns a pointer to an allocated 
 *        region of at least size bytes with constraints.
 *
 *  I used https://github.com/htian/malloc-lab/blob/master/mm.c source idea to maximize utilization
 *  by using reallocation tags
 *  in reallocation cases (realloc-bal.rep, realloc2-bal.rep)
 */
void *mm_realloc(void *bp, size_t size)
{
    void *new_ptr = bp;    /* Pointer to be returned */
    size_t new_size = size; /* Size of new block */
    int remainder;          /* Adequacy of block sizes */
    int extendsize;         /* Size of heap extension */
    int block_buffer;       /* Size of block buffer */
    
    // Ignore size 0 cases
    if (size == 0)
        return NULL;
    
    // Align block size
    if (new_size <= DSIZE) {
        new_size = 2 * DSIZE;
    } else {
        new_size = ALIGN(size+DSIZE);
    }
    
    /* Add overhead requirements to block size */
    new_size += REALLOC_BUFFER;
    
    /* Calculate block buffer */
    block_buffer = GET_SIZE(HDRP(bp)) - new_size;
    
    /* Allocate more space if overhead falls below the minimum */
    if (block_buffer < 0) {
        /* Check if next block is a free block or the epilogue block */
        if (!GET_ALLOC(HDRP(NEXT_BLKP(bp))) || !GET_SIZE(HDRP(NEXT_BLKP(bp)))) {
            remainder = GET_SIZE(HDRP(bp)) + GET_SIZE(HDRP(NEXT_BLKP(bp))) - new_size;
            if (remainder < 0) {
                extendsize = MAX(-remainder, CHUNKSIZE);
                if (extend_heap(extendsize) == NULL)
                    return NULL;
                remainder += extendsize;
            }
            
            remove_free(NEXT_BLKP(bp));
            
            // Do not split block
            PUT_NOTAG(HDRP(bp), PACK(new_size + remainder, 1)); 
            PUT_NOTAG(FTRP(bp), PACK(new_size + remainder, 1)); 
        } else {
            new_ptr = mm_malloc(new_size - DSIZE);
            memcpy(new_ptr, bp, MIN(size, new_size));
            mm_free(bp);
        }
        block_buffer = GET_SIZE(HDRP(new_ptr)) - new_size;
    }
    
    // Tag the next block if block overhead drops below twice the overhead 
    if (block_buffer < 2 * REALLOC_BUFFER)
        SET_RATAG(HDRP(NEXT_BLKP(new_ptr)));
    
    // Return the reallocated block 
    return new_ptr;
}