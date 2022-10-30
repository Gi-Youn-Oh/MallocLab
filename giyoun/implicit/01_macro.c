// 기본 상수와 매크로
#define WSIZE 4 //word size
#define DSIZE 8 // double word size
#define CHUNKSIZE (1<<12) // heap을 확장할 떄 확장할 최소 크기 처음 4kb할당, 초기 free블록 // << 비트 쉬프트 연산자 2^12
#define MAX(x, y) ((x) > (y)? (x) : (y)) // 삼항 연산자, 최대값을 구하는 매크로

// free 리스트에서 header와 footer를 조작하는 데에는, 많은 양의 캐스팅 변환과 포인터 연산이 사용되기에 애초에 매크로로 만든다.
// size 와 alloc을 or 비트 연산시킨다.
// 애초에 size의 오른쪽 3자리는 000으로 비어져 있다.
// 왜? -> 메모리가 더블 워드 크기로 정렬되어 있다고 전제하기 때문이다. 따라서 size는 무조건 8바이트보다 큰 셈이다.
#define PACK(size, alloc)((size)|(alloc)) //header에 들어갈 값 ?

/* 포인터 p가 가르키는 워드의 값을 읽거나, p가 가르키는 워드에 값을 적는 매크로 */
#define GET(p) (*(unsigned int*)(p))
// 보통 p는 void 포인터라고 한다면 곧바로 *p(* 자체가 역참조)를 써서 참조할 수 없기 때문에, 
// 그리고 우리는 4바이트(1워드)씩 주소 연산을 한다고 전제하기에 unsigned int로 캐스팅 변환을 한다. 
// p가 가르키는 곳의 값을 불러온다.

#define PUT(p, val) (*(unsigned int*)(p) = (val))
// p가 가르키는 곳에 val를 넣는다.

/* header 혹은 footer의 값인 size or allocated 여부를 가져오는 매크로 */
#define GET_SIZE(p)  (GET(p) & ~0x7)   //0x1 = 2 진수로 1, 0x2 = 2 진수로 10
// 블록의 사이즈만 가지고 온다. ex. 1011 0111 & 1111 1000 = 1011 0000으로 사이즈만 읽어옴을 알 수 있다.

#define GET ALLOC(p) (GET(p)& 0x1)
// 블록이 할당되었는지 free인지를 나타내는 flag를 읽어온다. ex. 1011 0111 & 0000 0001 = 0000 0001로 allocated임을 알 수 있다.


/* 블록 포인터 bp(payload를 가르키고 있는 포인터)를 바탕으로 블록의 header와 footer의 주소를 반환하는 매크로 */
#define HDRP(bp)  ((char *)(bp) - WSIZE)              
// header는 payload보다 앞에 있으므로 4바이트(워드)만큼 빼줘서 앞으로 1칸 전진하게 한다.

#define FTRP(bp)  ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)        
 // footer는 payload에서 블록의 크기만큼 뒤로 간 다음 8바이트(더블 워드)만큼 빼줘서 앞으로 2칸 전진하게 해주면 footer가 나온다.
// 이 때 포인터는 char형이어야 4 혹은 8바이트, 즉 정확히 1바이트씩 움직일 수 있다. 
// 만약 int형으로 캐스팅 해주면 - WSIZE 했을 때 16바이트 만큼 움직일 수도 있다.

/* 블록 포인터 bp를 바탕으로, 이전과 다음 블록의 payload를 가르키는 주소를 반환하는 매크로 */
#define NEXT_BLKP(bp)   ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))   
//다음 블록 위치 = 지금 위치에서 지금 블록의 크기 만큼 뒤로 가면 다음 블록 header 바로 뒤
//(본인 블록 header에서 블럭 크기 확인)
#define PREV_BLKP(bp)   ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))   
//이전 블록 위치 = 지금 위치에서 이전 블록 크기 만큼 앞으로 가면 이전 블록 header 바로 뒤
//(이전 블록의 footer에서 이전 블록의 크기 확인)

/* define searching method for find suitable free blocks to allocate */
// #define NEXT_FIT                                                            
// define하면 next_fit, 안하면 first_fit으로 탐색한다.

/* global variable & functions */
static char* heap_listp;                                                    
// 항상 prologue block을 가리키는 정적 전역 변수 설정
// static 변수는 함수 내부(지역)에서도 사용이 가능하고 함수 외부(전역)에서도 사용이 가능하다.
                                               
#ifdef NEXT_FIT                                                            
 // #ifdef ~ #endif를 통해 조건부로 컴파일이 가능하다. NEXT_FIT이 선언되어 있다면 밑의 변수를 컴파일 할 것이다.
    static void* last_freep;                                                
    // next_fit 사용 시 마지막으로 탐색한 free 블록을 가리키는 포인터이다.
#endif

/* 코드 순서상, implicit declaration of function(warning)을 피하기 위해 미리 선언해주는 부분? */
static void* extend_heap(size_t words);
static void* coalesce(void* bp);
static void* find_fit(size_t asize);
static void place(void* bp, size_t newsize);

int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *bp);
void *mm_realloc(void *ptr, size_t size);
