/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */

void * mm_malloc (size_t size)
{
    size_t asize; /*조정된 블록사이즈*/
    size_t extendsize; /* 알맞은 크기의 free 블록이 없을 시 확장하는 사이즈*/
    char *bp;

    /* Ignore spurious reqeusts*/
    if ( size == 0)  // 0이면 요청무시
    retrun NULL;

    /* A블럭의 최소크기를 맞추기 위해 size를 조정 */
    if (size <= DSIZE) 
        asize = 2 * DSIZE; 
        //블럭의 최소 크기는 16byte(header(4byte) + footer(4byte) + data(1byte 이상) = 9byte -> 8의 배수로 만들기 = 16byte
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

    // 적절한 공간을 가진 블록을 찾으면 할당(혹은 분할까지) 진행한다.
    // bp는 계속 free 블록을 가리킬 수 있도록 한다.
    if ((bp = find_fit(aszie)) ! = NULL) { 
        place (bp, asize);
        return bp;
    }

    // 적절한 공간을 찾지 못했다면 힙을 늘려주고, 그 늘어난 공간에 할당시켜야 한다.
    extendsize = MAX(asize,CHUNKSIZE); // 둘 중 더 큰 값을 선택한다.
    if ((bp= extend_heap(extendsize/WSIZE))==NULL) // 실패 시 bp로는 NULL을 반환한다.
        return NULL;

    place (bp, asize); // 힙을 늘리는 데에 성공했다면, 그 늘어난 공간에 할당시킨다.
    return bp;
}