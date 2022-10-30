/*
 * place - 요구 메모리를 할당할 수 있는 가용 블록을 할당한다.(즉 실제로 할당하는 부분이다) 이 때 분할이 가능하다면 분할한다.
 */
static void place(void* bp, size_t asize) {
    size_t csize = GET_SIZE(HDRP(bp));                                      
    // 현재 할당할 수 있는 후보, 즉 실제로 할당할 free 블록의 사이즈
    
    // 분할이 가능한 경우
    // 남은 메모리가 최소한의 free 블록을 만들 수 있는 4 word가 되느냐
    // -> header/footer/payload/정렬을 위한 padding까지 총 4 word 이상이어야 한다.
    if ((csize - asize) >= (2 * DSIZE)) {                                   
        // 2 * DSIZE는 총 4개의 word인 셈이다.
        // csize - asize 부분에 header, footer, payload, 정렬을 위한 padding까지 총 4개가 들어갈 수 있어야 free 블록이 된다.
        // 앞의 블록은 할당시킨다.
        PUT(HDRP(bp), PACK(asize, 1));
        PUT(FTRP(bp), PACK(asize, 1));
        
        // 뒤의 블록은 free시킨다.
        bp = NEXT_BLKP(bp);
        PUT(HDRP(bp), PACK(csize - asize, 0));
        PUT(FTRP(bp), PACK(csize - asize, 0));
        coalesce(bp);                                                       // free 이후 뒤의 것과 또 연결될 수 있으므로 coalesce 실행
        
    // 분할이 불가능한 경우
    // csize - asize가 2 * DSIZE보다 작다는 것은 header, footer, payload, 정렬을 위한 padding 각 1개씩 총 4개로 이루어진 free 블록이 들어갈 공간이 없으므로 어쩔 수 없이 내부 단편화가 될 수 밖에 없다.
    } else {
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
}