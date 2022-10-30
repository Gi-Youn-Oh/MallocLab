/*
 * find_fit - 힙을 탐색하여 요구하는 메모리 공간보다 큰 가용 블록의 주소를 반환한다.
 */
static void* find_fit(size_t asize) {
    // next-fit
    #ifdef NEXT_FIT
        void* bp;
        void* old_last_freep = last_freep;
        
        // 이전 탐색이 종료된 시점에서부터 다시 시작한다.
        for (bp = last_freep; GET_SIZE(HDRP(bp)); bp = NEXT_BLKP(bp)) {     
            if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
                return bp;
            }
        }
        
        // last_freep부터 찾았는데도 없으면 처음부터 찾아본다. 이 구문이 없으면 앞에 free 블록이 있음에도 extend_heap을 하게 되니 메모리 낭비가 된다.
        for (bp = heap_listp; bp < old_last_freep; bp = NEXT_BLKP(bp)) {
            if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {
                return bp;
            }
        }
        
        // 탐색을 마쳤으니 last_freep도 수정해준다.
        last_freep = bp;
        
        return NULL;                                                        // 못 찾으면 NULL을 리턴한다.
        
    // first-fit
    #else
        void* bp;
        
        for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) { 
        // heap_listp, 즉 prologue부터 탐색한다. 전에 우리는 heap_listp += (2 * WSIZE)를 해두었다.
            if (!GET_ALLOC(HDRP(bp)) && (asize <= GET_SIZE(HDRP(bp)))) {    
                // 할당된 상태가 아니면서 해당 블록의 사이즈가 우리가 할당시키려는 asize보다 크다면 해당 블록에 할당이 가능하므로 곧바로 bp를 반환한다.
                return bp;
            }
        }
        
        return NULL;                                                        // 못 찾으면 NULL을 리턴한다.
        
    #endif
}