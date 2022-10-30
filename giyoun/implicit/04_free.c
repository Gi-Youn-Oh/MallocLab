/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp) {
    size_t size = GET_SIZE(HDRP(bp));                                       // bp가 가리키는 블록의 사이즈만 들고 온다.
    
    // header, footer 둘 다 flag를 0으로 바꿔주면 된다.
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));                      
    
    coalesce(bp);                                                           // 앞 뒤 블록이 free 블록이라면 연결한다.                   
}