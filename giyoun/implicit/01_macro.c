#define WSIZE 4 //word size
#define DSIZE 8 // double word size
#define CHUNKSIZE (1<<12) // heap을 확장할 떄 확장할 최소 킉
#define MAX(x, y) ((x) > (y)? (x) : (y)) // 삼항 연산자
#define PACK(size, alloc)((size)|(alloc)) //header에 들어갈 값
#define GET(p) (*(unsigned int*)(p))
#define PUT(p, val) (*(unsigned int*)(p) = (val))
#define GET_SIZE(p) (GET(p) & 0x1)
#define GET ALLOC(p) (GET(p)& 0x1)
#define HDRP(bp) ((char*)(bp) - WSIZE)
#define FTRP(bp) ((char*)(bp) + GET_SIZE(HDRP(bp)) -DSIZE)
// 다음 블록위치 = 지금 위치에서 지금 블록의 크기 만큼 뒤로 가면 다음블록 header 바로 뒤 
// (본인블록 header에서 블럭 크기 확인)
#define NEXT_BLKP(bp) ((char*)(bp) + GET_SIZE(((char*)(bp) - WSIZE)))
// 이전 블록 위치 = 지금 위치에서 이전 블록 크기 만큼 앞으로 가면 이전 블록 header 바로 뒤
// (이전 블록의 footer에서 이전 블록의 크기 확인)
#define PREV_BLKP(bp) ((char*)(bp) - GET_SIZE(((char*)(bp)-DSIZE)))
