int mm_init(void)
{
    /*Create the initial empty heap*/
    if ((heap_listp = mem_sbrk(4*WSIZE == (void*)-1)
        return -1;
    PUT(heap_listp, 0);    // Alignment padding
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); // Prologue header
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); // Prologue footer
    PUT(heap_listp + (3*WSIZE), PACK(0, 1)); // Epilogue hearder
    heap_listp += (2*WSIZE);

    /* Extend the empty heap with a free block of CHUNKSIZE bytes*/
    if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
        return -1;
    return 0; 
}

/* 
 * mm_init - initialize the malloc package.
 */

int mm_init(void) {
    if ((heap_listp = mem_sbrk(4 * WSIZE)) == (void *)-1)  // 이때의 heap_listp 는 처음을 가리킨다. (proloque 생성전)
    // memlib.c를 살펴보면 할당 실패시 (void *)-1을 반환하고 있다. 정상 포인터를 반환하는 것과는 달리, 
    // 오류 시 이와 구분 짓기 위해 mem_sbrk는 (void *)-1을 반환하고 있다.
        return -1;                                                          
        // 할당에 실패하면 -1을 리턴한다.
        
    PUT(heap_listp, 0);                                                   
    // Alignment padding으로 unused word이다. 맨 처음 메모리를 8바이트 정렬(더블 워드)을 위해 사용하는 미사용 패딩이다.
    
    /* 프롤로그의 헤더와 푸터는 각각 8바이트를 갖고있다고 내용을 담고 있다.*/
    PUT(heap_listp + (1 * WSIZE), PACK(DSIZE, 1));                          
    // prologue header로, 맨 처음에서 4바이트 뒤에 header가 온다. 
    // 이 header에 사이즈(프롤로그는 8바이트)와 allocated 1(프롤로그는 사용하지 말라는 의미)을 통합한 값을 부여한다.
    PUT(heap_listp + (2 * WSIZE), PACK(DSIZE, 1));                         
    // prologue footer로, 값은 header와 동일해야 한다.
    PUT(heap_listp + (3 * WSIZE), PACK(0, 1));                              
    // epilogue header
    
    /* 여기부터 heap_lsitp 가 프롤로그 footer 를 가리킨다 */
    heap_listp += (2 * WSIZE);                                              
    // heap_listp는 prologue footer를 가르키도록 만든다.
    
    #ifdef NEXT_FIT
        last_freep = heap_listp;                                            
        // next_fit 사용 시 마지막으로 탐색한 free 블록을 가리키는 포인터이다.
    #endif
    
    // CHUCKSIZE만큼 힙을 확장해 초기 free 블록을 생성한다. 이 때 CHUCKSIZE는 2^12으로 4kB 정도였다.(4096 bytes)
    if (extend_heap(CHUNKSIZE / WSIZE) == NULL) {   // 곧바로 extend_heap이 실행된다.
        return -1;
    }
    
    return 0;
}
