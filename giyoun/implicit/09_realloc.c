/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size) {
    // 블록의 크기를 줄이는 것이면 줄이려는 size만큼으로 줄인다.
    // 블록의 크기를 늘리는 것이면 
    // 핵심은, 이미 할당된 블록의 사이즈를 직접 건드리는 것이 아니라, 요청한 사이즈 만큼의 블록을 새로 메모리 공간에 만들고 현재의 블록을 반환하는 것이다.
    // 해당 블록의 사이즈가 이 정도로 변경되었으면 좋겠다는 것이 size, 
    void *oldptr = ptr;                                                     // 크기를 조절하고 싶은 힙의 시작 포인터
    void *newptr;                                                           // 크기 조절 뒤의 새 힙의 시작 포인터
    size_t copySize;                                                        // 복사할 힙의 크기
    
    newptr = mm_malloc(size);                                               // place를 통해 header, footer가 배정된다.
    if (newptr == NULL) {
        return NULL;
    }
    
    copySize = GET_SIZE(HDRP(oldptr));                                      // 원래 블록의 사이즈
    
    if (size < copySize) {                                                  // 만약 블록의 크기를 줄이는 것이라면 size만큼으로 줄이면 된다. copySize - size 공간의 데이터는 잘리게 된다. 밑의 memcpy에서 잘린 만큼의 데이터는 복사되지 않는다.
        copySize = size;
    }
    
    memcpy(newptr, oldptr, copySize);                                       // oldptr부터 copySize까지의 데이터를, newptr부터 심겠다.
    mm_free(oldptr);                                                        // 기존 oldptr은 반환한다.
    return newptr;
}