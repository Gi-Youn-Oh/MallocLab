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
    "5조",
    /* First member's full name */
    "Giyoun Oh",
    /* First member's email address */
    "dhrldbs@google.com",
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
#define SET_PTR(p, ptr) (*(unsigned int *)(p) = (unsigned int)(ptr))

/* Adjust the reallocation tag */
#define SET_RATAG(p)   (GET(p) |= 0x2) // realloc 태그 달기
#define REMOVE_RATAG(p) (GET(p) &= ~0x2)

/* Read the size and allocation bit from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)
#define GET_ALLOC(p) (GET(p) & 0x1)  // allocate 여부 확인
#define GET_TAG(p)   (GET(p) & 0x2)  // realloc tag 여부 확인


/* Address of block's header and footer */
#define HDRP(ptr) ((char *)(ptr) - WSIZE)
#define FTRP(ptr) ((char *)(ptr) + GET_SIZE(HDRP(ptr)) - DSIZE)

/* Address of next and previous blocks */
#define NEXT_BLKP(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr) - WSIZE))
#define PREV_BLKP(ptr) ((char *)(ptr) - GET_SIZE((char *)(ptr) - DSIZE))

/* Address of free block's predecessor and successor entries */
#define SUCC_PTR(ptr) ((char *)(ptr))
#define PRED_PTR(ptr) ((char *)(ptr) + WSIZE)

/* Address of free block's predecessor and successor on the segregated list */
#define SUCC(ptr) (*(char **)(ptr))
#define PRED(ptr) (*(char **)(PRED_PTR(ptr)))


/*
 * Global variables
 */
void *segregated_free_lists[LISTLIMIT]; 


/*
 * Function prototypes
 */
static void *extend_heap(size_t size);
static void *coalesce(void *ptr);
static void *place(void *ptr, size_t asize);
static void insert_node(void *ptr, size_t size);
static void delete_node(void *ptr);

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
    void *ptr;                   
    size_t asize;                // Adjusted size 
    
    asize = ALIGN(size);
    
    if ((ptr = mem_sbrk(asize)) == (void *)-1)
        return NULL;
    
    // Set headers and footer 
    PUT_NOTAG(HDRP(ptr), PACK(asize, 0));  
    PUT_NOTAG(FTRP(ptr), PACK(asize, 0));   
    PUT_NOTAG(HDRP(NEXT_BLKP(ptr)), PACK(0, 1)); 
    insert_node(ptr, asize);

    return coalesce(ptr);
}

static void insert_node(void *ptr, size_t size) {
    int list = 0;
    void *search_ptr = ptr;
    void *insert_ptr = NULL;
    
    // Select segregated list 
    // list 0~18 까지만 쓰는 이유 : 마지막 list에서 Null 값을 표시해야 하기 때문입니다.
    while ((list < LISTLIMIT - 1) && (size > 1)) { // list_num [0,1,2,3...] size = [64,16,8,4,2,1,0] 탈출
        size >>= 1;
        list++;
    }
    
    // Keep size ascending order and search
    search_ptr = segregated_free_lists[list];
    while ((search_ptr != NULL) && (size > GET_SIZE(HDRP(search_ptr)))) { // 가리키는 포인터가 만약 넣으려는 사이즈 보다 작으면 다음 리스트를 찾는다.
        insert_ptr = search_ptr;
        search_ptr = SUCC(search_ptr);
    }
    
    // Set predecessor and successor  ptr = 현재 , search_ptr 다음 / insert_prt 이전 
    if (search_ptr != NULL) { // 1. 찾으려는 리스트가 null 이 아닐때
        if (insert_ptr != NULL) { // 1-1) insert_ptr이 null 이 아닐 때 
            SET_PTR(SUCC_PTR(ptr), search_ptr);
            SET_PTR(PRED_PTR(search_ptr), ptr);
            SET_PTR(PRED_PTR(ptr), insert_ptr);
            SET_PTR(SUCC_PTR(insert_ptr), ptr);
        } else {                    // 1-2) insert_ptr이 null 일때 
            SET_PTR(SUCC_PTR(ptr), search_ptr);
            SET_PTR(PRED_PTR(search_ptr), ptr);
            SET_PTR(PRED_PTR(ptr), NULL);
            segregated_free_lists[list] = ptr;
        }
    } else {                 //2.  찾으려는 리스트가 null 일때 
        if (insert_ptr != NULL) {  // 2-1)insert_ptr이 null이 아닐 때
            SET_PTR(SUCC_PTR(ptr), NULL);
            SET_PTR(PRED_PTR(ptr), insert_ptr);
            SET_PTR(SUCC_PTR(insert_ptr), ptr);
        } else {                  // 2-2)insert_ptr이 null 일때
            SET_PTR(SUCC_PTR(ptr), NULL);
            SET_PTR(PRED_PTR(ptr), NULL);
            segregated_free_lists[list] = ptr;
        }
    }
    
    return;
}


static void delete_node(void *ptr) {
    int list = 0;
    size_t size = GET_SIZE(HDRP(ptr));
    
    // Select segregated list 
    while ((list < LISTLIMIT - 1) && (size > 1)) {
        size >>= 1;
        list++;
    }
    
    if (SUCC(ptr) != NULL) {  // 1.다음 블록이 있을때
        if (PRED(ptr) != NULL) { //1-1) 이전 블록 또한 있을때
            SET_PTR(PRED_PTR(SUCC(ptr)), PRED(ptr));
            SET_PTR(SUCC_PTR(PRED(ptr)), SUCC(ptr));
        } else {  // 1-2) 이전블록이 없을 때
            SET_PTR(PRED_PTR(SUCC(ptr)), NULL);
            segregated_free_lists[list] = SUCC(ptr);
        }
    } else { //2. 다음 블록이 없을 때
        if (PRED(ptr) != NULL) { //2-1) 이전블록은 있다
            SET_PTR(SUCC_PTR(PRED(ptr)), NULL); // 이전블록의 다음블록은 없다.
        } else { // 2-2) 이전 블록도 없다.
            segregated_free_lists[list] = NULL;
        }
    }
    
    return;
}


static void *coalesce(void *ptr)
{
    size_t prev_alloc = GET_ALLOC(HDRP(PREV_BLKP(ptr)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(ptr)));
    size_t size = GET_SIZE(HDRP(ptr));
    

    // Do not coalesce with previous block if the previous block is tagged with Reallocation tag
    if (GET_TAG(HDRP(PREV_BLKP(ptr))))
        prev_alloc = 1;




    if (prev_alloc && next_alloc) {                         // Case 1
        return ptr;
    }
    else if (prev_alloc && !next_alloc) {                   // Case 2
        delete_node(ptr);
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(ptr), PACK(size, 0));
        PUT(FTRP(ptr), PACK(size, 0));
    } else if (!prev_alloc && next_alloc) {                 // Case 3 
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr)));
        PUT(FTRP(ptr), PACK(size, 0));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    } else {                                                // Case 4
        delete_node(ptr);
        delete_node(PREV_BLKP(ptr));
        delete_node(NEXT_BLKP(ptr));
        size += GET_SIZE(HDRP(PREV_BLKP(ptr))) + GET_SIZE(HDRP(NEXT_BLKP(ptr)));
        PUT(HDRP(PREV_BLKP(ptr)), PACK(size, 0));
        PUT(FTRP(NEXT_BLKP(ptr)), PACK(size, 0));
        ptr = PREV_BLKP(ptr);
    }
    
    insert_node(ptr, size);
    
    return ptr;
}

static void *place(void *ptr, size_t asize) // ptr_size = segregated list 에서 들고온 가용블록
{
    size_t ptr_size = GET_SIZE(HDRP(ptr)); // ptr_size 크기 정보 획득
    size_t remainder = ptr_size - asize;  // 남는 공간 = ptr_size(가져온 가용블록)-(사용할 크기)
    
    delete_node(ptr);
    
    
    if (remainder <= DSIZE * 2) {
        // Do not split block 
        PUT(HDRP(ptr), PACK(ptr_size, 1)); 
        PUT(FTRP(ptr), PACK(ptr_size, 1)); 
    }
    
    else if (asize >= 100) {
        // Split block
        PUT(HDRP(ptr), PACK(remainder, 0));
        PUT(FTRP(ptr), PACK(remainder, 0));
        PUT_NOTAG(HDRP(NEXT_BLKP(ptr)), PACK(asize, 1));
        PUT_NOTAG(FTRP(NEXT_BLKP(ptr)), PACK(asize, 1));
        insert_node(ptr, remainder);
        return NEXT_BLKP(ptr);
        
    }
    
    else {
        // Split block
        PUT(HDRP(ptr), PACK(asize, 1)); 
        PUT(FTRP(ptr), PACK(asize, 1)); 
        PUT_NOTAG(HDRP(NEXT_BLKP(ptr)), PACK(remainder, 0)); 
        PUT_NOTAG(FTRP(NEXT_BLKP(ptr)), PACK(remainder, 0)); 
        insert_node(NEXT_BLKP(ptr), remainder);
    }
    return ptr;
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
    void *ptr = NULL;   // Pointer
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
            ptr = segregated_free_lists[list];
            /* Ignore blocks that are too small or marked with the reallocation bit */
            while ((ptr != NULL) && ((asize > GET_SIZE(HDRP(ptr))) || (GET_TAG(HDRP(ptr))))) // tag 1 표시시 안된다는 의미 
            {
                ptr = SUCC(ptr);
            }
            if (ptr != NULL)
                break;
        }
        
        searchsize >>= 1;
        list++;
    }
    
    /* Extend the heap if no free blocks of sufficient size are found */
    if (ptr == NULL) {
        extendsize = MAX(asize, CHUNKSIZE);
        
        if ((ptr = extend_heap(extendsize)) == NULL)
            return NULL;
    }
    
    /* Place the block */
    ptr = place(ptr, asize);
    
    
    /* Return pointer to newly allocated block */
    return ptr;
}










/*
 * mm_free - Freeing a block does nothing.
 *
 * Role : The mm_free routine frees the block pointed to by ptr
 *
 * Return value : returns nothing
 */
void mm_free(void *ptr)
{
    size_t size = GET_SIZE(HDRP(ptr));
 
    /* Unset the reallocation tag on the next block */
    REMOVE_RATAG(HDRP(NEXT_BLKP(ptr)));

    /* Adjust the allocation status in boundary tags */
    PUT(HDRP(ptr), PACK(size, 0));
    PUT(FTRP(ptr), PACK(size, 0));
    
    /* Insert new block into appropriate list */
    insert_node(ptr, size);
    
    /* Coalesce free block */
    coalesce(ptr);
    
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
void *mm_realloc(void *ptr, size_t size)
{
    void *new_ptr = ptr;    /* Pointer to be returned */
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
    block_buffer = GET_SIZE(HDRP(ptr)) - new_size;
    
    /* Allocate more space if overhead falls below the minimum */
    if (block_buffer < 0) {
        /* Check if next block is a free block or the epilogue block */
        if (!GET_ALLOC(HDRP(NEXT_BLKP(ptr))) || !GET_SIZE(HDRP(NEXT_BLKP(ptr)))) { // 다음 블록이 가용블록이거나, 에필로그 블록일 경우
            remainder = GET_SIZE(HDRP(ptr)) + GET_SIZE(HDRP(NEXT_BLKP(ptr))) - new_size;
            if (remainder < 0) {
                extendsize = MAX(-remainder, CHUNKSIZE);
                if (extend_heap(extendsize) == NULL)
                    return NULL;
                remainder += extendsize;
            }
            
            delete_node(NEXT_BLKP(ptr));
            
            // Do not split block
            PUT_NOTAG(HDRP(ptr), PACK(new_size + remainder, 1)); 
            PUT_NOTAG(FTRP(ptr), PACK(new_size + remainder, 1)); 
        } else { //다음블록이 가용블록이 아니거나, 에필로그 블록이 아닐때
            new_ptr = mm_malloc(new_size - DSIZE);
            memcpy(new_ptr, ptr, MIN(size, new_size));
            mm_free(ptr);
        }
        block_buffer = GET_SIZE(HDRP(new_ptr)) - new_size;
    }
    
    // Tag the next block if block overhead drops below twice the overhead 
    if (block_buffer < 2 * REALLOC_BUFFER)
        SET_RATAG(HDRP(NEXT_BLKP(new_ptr)));
    
    // Return the reallocated block 
    return new_ptr;
}            

// 1. Reallocte tag 를 new_ptr 다음에 달아주는이유
// 2. coalesce 할때 realloc tag는 건드리지 않는 이유
