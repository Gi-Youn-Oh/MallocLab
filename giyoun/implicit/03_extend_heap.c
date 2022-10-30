/*
 *  extend_heap - word 단위의 메모리를 인자로 받아 힙을 늘려준다.  
 */
static void* extend_heap(size_t words) {
    char* bp;
    size_t size;
    
    size = (words % 2 == 1) ? (words + 1) * WSIZE : (words) * WSIZE;        
    // words가 홀수로 들어왔다면 짝수로 바꿔준다. 짝수로 들어왔다면 그대로 WSIZE를 곱해준다. 
    // ex. 5만큼(5개의 워드 만큼) 확장하라고 하면, 6으로 만들고 24바이트로 만든다. 
    // 8바이트(2개 워드, 짝수) 정렬을 위해 짝수로 만들어줘야 한다.
    
    if ((long)(bp = mem_sbrk(size)) == -1) { 
        // 변환한 사이즈만큼 메모리 확보에 실패하면 NULL이라는 주소값을 반환해 실패했음을 알린다. 
        // bp 자체의 값, 즉 주소값이 32bit이므로 long으로 캐스팅한다.
        return NULL;                                                        
        // 그리고 mem_sbrk 함수가 실행되므로 bp는 새로운 메모리의 첫 주소값을 가르키게 된다.
    }              
    
    // 새 free 블록의 header와 footer를 정해준다. 자연스럽게 전 epilogue 자리에는 새로운 header가 자리 잡게 된다. 그리고 epilogue는 맨 뒤로 보내지게 된다.
    PUT(HDRP(bp), PACK(size, 0));    // 새 free 블록의 header로, free 이므로 0을 부여
    PUT(FTRP(bp), PACK(size, 0));    // 새 free 블록의 footer로, free 이므로 0을 부여
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1));    // 앞에서 현재 bp(새롭게 늘어난 메모리의 첫 주소 값으로 역시 payload이다)의 header에 값을 부여해주었다. 따라서 이 header의 사이즈 값을 참조해 다음 블록의 payload를 가르킬 수 있고, 이 payload의 직전인 header는 epilogue가 된다.
    
    return coalesce(bp);     // 앞 뒤 블록이 free 블록이라면 연결하고 bp를 반환한다.
}