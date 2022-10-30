/*
 * coalesce - 앞 혹은 뒤 블록이 free 블록이고, 현재 블록도 free 블록이라면 연결시키고 연결된 free 블록의 주소를 반환한다.
 */ 
static void* coalesce(void* bp) {
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));                     // 이전 블록의 free 여부
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));                     // 다음 블록의 free 여부
    size_t size = GET_SIZE(HDRP(bp));                                       // 현재 블록의 사이즈
    
    // 경우 1. 이전 블록 할당, 다음 블록 할당 - 연결시킬 수 없으니 그대로 bp를 반환한다.
    if (prev_alloc && next_alloc) {
        return bp;
    }
    
    // 경우 2. 이전 블록 할당, 다음 블록 free - 다음 블록과 연결시키고 현재 bp를 반환하면 된다.
    else if (prev_alloc && !next_alloc) {
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));                              
        // 다음 블록의 header에 저장된 다음 블록의 사이즈를 더해주면 연결된 만큼의 사이즈가 나온다.
        PUT(HDRP(bp), PACK(size, 0));                                       
        // 현재 블록의 header에 새로운 header가 부여된다.
        PUT(FTRP(bp), PACK(size, 0));                                       
        // 연결된 새로운 블록의 footer에 새로운 footer가 부여된다.
    }
    
    // 경우 3. 이전 블록 free, 다음 블록 할당 - 이전 블록과 연결시키고 이전 블록을 가리키도록 bp를 바꾼다.
    else if (!prev_alloc && next_alloc) {
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(FTRP(bp), PACK(size, 0));                                       // 현재 블록의 footer에 새로운 footer를 부여한다.
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));                            // 이전 블록의 header에 새로운 header를 부여한다.
        bp = PREV_BLKP(bp);                                                 // bp가 이전 블록을 가리키도록 한다.
    }
    
    // 경우 4. 이전 블록 free, 다음 블록 free - 모두 연결한 후 이전 블록을 가리키도록 bp를 바꾼다.
    else {
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));  // 이전 블록의 header에서 사이즈를, 다음 블록의 footer에서 사이즈를 읽어와 size를 더한다.
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));                            // 이전 블록의 header에 새로운 header를 부여한다.
        PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));                            // 다음 블록의 footer에 새로운 footer를 부여한다.
        bp = PREV_BLKP(bp);
    }
    
    #ifdef NEXT_FIT
        last_freep = bp;
    #endif
    
    return bp;
}