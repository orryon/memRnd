// memrnd.cpp : fills memory array of int32 or int64 and goes over it all in random order.

#include <windows.h>
#include <timeapi.h>
#include <random>
using namespace std;
using int64 = long long;

#pragma comment (lib, "winmm.lib")

void usage(void) 
{
    printf("usage : memrnd [-l] [-64] [-tNN] <test-size-in-MB>\n");
    printf("\t-l (-L)     -- use large pages\n");
    printf("\t-64         -- use 64-bit integers (selected automatically when size exceeds 16383)\n");
    printf("\t-tNN (-Tnn) -- use multiple threads, number immediately after T|t, 0 used for detected logical cpu number\n");
    exit(1);
}

size_t nPageSize = 4096;

#pragma warning( push )
#pragma warning( disable : 6385 )
#pragma warning( disable : 6386 )
int64 * makeArray64(size_t nCount, BOOL useLargePages) {
    int64 n, nMax, nRnd, nNext;
    size_t nSize = nCount * sizeof( int64);
    int64* a;
    default_random_engine gen(2019);
    if (useLargePages) {
        size_t nMinSize = GetLargePageMinimum() - 1;
        nSize = (nSize + nMinSize) & ~nMinSize;
        a = (int64*)VirtualAlloc(NULL, nSize, MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
    }
    else
      a = (int64*) malloc( nSize);
    if (!a) {
        printf("error allocating %zd KB : error %ld", nSize >> 10, GetLastError());
        exit(1);
    }
    a[0] = 1;
    a[1] = 0;
    nMax = (int64) nCount;
    for (n = 2; n < nMax; n++) {
        nNext = abs(nRnd = gen()) % n;
        a[n] = a[nNext];
        a[nNext] = n;
    }
    return a ;
}

DWORD* makeArray32(size_t nCount, BOOL useLargePages) {
    DWORD n, nNext;
    int64 nRnd;
    size_t nSize = nCount * sizeof(DWORD);
    DWORD* a;
    default_random_engine gen(2019);
    if (useLargePages) {
        size_t nMinSize = GetLargePageMinimum() - 1;
        nSize = (nSize + nMinSize) & ~nMinSize;
        a = (DWORD*)VirtualAlloc(NULL, nSize, MEM_COMMIT | MEM_LARGE_PAGES, PAGE_READWRITE);
    }
    else
        a = (DWORD*)malloc(nSize);
    if (!a) {
        printf("error allocating %zd KB : error %ld", nSize >> 10, GetLastError());
        exit(1);
    }
    a[0] = 1;
    a[1] = 0;
    for (n = 2; n < nCount; n++) {
        nNext = (DWORD) (abs(nRnd = gen()) % n) ;
        a[n] = a[nNext];
        a[nNext] = n;
    }
    return a;
}
#pragma warning( pop )

void testArray32(DWORD* a) {
    DWORD n = 0;
    while (n = a[n]);
}

void testArray64(int64 * a) {
    int64 n = 0;
    while (n = a[n]);
}

BOOL allowLargePagesUsing (void){
    BOOL result;
    HANDLE hToken = INVALID_HANDLE_VALUE;
    TOKEN_PRIVILEGES item;
    item.PrivilegeCount = 1;
    item.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (result = LookupPrivilegeValueW(NULL, /*L"SeLockMemoryPrivilege"*/SE_LOCK_MEMORY_NAME, &item.Privileges[0].Luid) &&
            OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken)) {
        result = AdjustTokenPrivileges(hToken, FALSE, &item, 0, NULL, NULL) ;
        CloseHandle(hToken);
    }
    return result;
}

size_t nSize = 0;
BOOL use64 = FALSE, useLargePages = FALSE;

typedef struct {
    DWORD id;
    DWORD result;
    HANDLE eventPaused;
    HANDLE eventRun;
    DWORD tStarted;
} TRunData;
TRunData* results;
HANDLE* threadHandles;
HANDLE* threadPaused;
DWORD* threadIds;

DWORD WINAPI run(_In_ LPVOID lpParameter) {
    TRunData* result;
    DWORD t;
    DWORD* a32 = NULL;
    int64* a64 = NULL;
    result = (TRunData*)lpParameter;
    if (use64) {
        if (!(a64 = makeArray64( nSize << 17, useLargePages))) {
            printf("error allocating array : error code %ld", GetLastError());
            exit(1);
        }
    }
    else {
        if (!(a32 = makeArray32( nSize << 18, useLargePages))) {
            printf("error allocating array : error code %ld", GetLastError());
            exit(1);
        }
    }
    t = timeGetTime() - result->tStarted;
    printf("thread %ld : array filled in %ld ms\n", result -> id, t);
    SetEvent(result->eventPaused);
    WaitForSingleObject(result->eventRun, INFINITE);
    if (use64)
        testArray64(a64);
    else
        testArray32(a32);
    t = timeGetTime() - result->tStarted;
    if (useLargePages)
        VirtualFree(use64 ? (void*)a64 : (void*)a32, 0, MEM_RELEASE);
    else
        free(use64 ? (void*)a64 : (void*)a32);
    printf("thread %ld : array walked in %ld ms\n", result->id, t);
    result->result = t;
    return 0;
}

int main( int argv, char** argc)
{
    char* s ;
    double x;
    DWORD nWait;
    DWORD t;
    TRunData* threadData;
    size_t tSum = 0;
    DWORD n, nThreads = 1;
    int i;
    if (argv < 2)
        usage();
    for (i = 1; i < argv; i++) {
        s = argc[i];
        if (((s[0] == '-') || (s[0] == '/')) && s[1]) {
            if ((s[1] == 'l') || (s[1] == 'L'))
                useLargePages = TRUE;
            else
            if ((s[1] == '6') && (s[2] == '4'))
                use64 = TRUE;
            else 
            if ((s[1] == 't') || (s[1] == 'T')) {
                nThreads = 0;
                sscanf_s( s + 2, "%ld", &nThreads);
                if (nThreads > 128) {
                    printf("too big thread count, using 2 threads");
                    nThreads = 2;
                }
                else
                if (!nThreads)
                {
                    SYSTEM_INFO sysinfo;
                    GetSystemInfo( &sysinfo);
                    nThreads = sysinfo.dwNumberOfProcessors;
                    printf("%ld logical CPUs found, using %ld threads", nThreads, nThreads);
                }
            }
        }
        else
        if (s[0])
            try {
                sscanf_s(s, "%zd", &nSize);
            }
            catch(...) {
                nSize = 0;
            }
    }
    if ((nSize == 0) || (nSize > 100000))
        usage();
    if (useLargePages and !allowLargePagesUsing()) {
        printf("error allowing large pages using : error code %ld", GetLastError());
        exit(1);
    }
    use64 = use64 || (nSize >= ((int64) 1) << 14);
    if (useLargePages)
        nPageSize = GetLargePageMinimum();
    printf("using int%s array [%zdM] : %zd MB ; page size : %zd KB\n", 
        use64 ? "64" : "32", 
        use64 ? (nSize > 8 ? nSize >> 3 : nSize << 7) : (nSize > 4 ? nSize >> 2 : nSize << 8), 
        nSize, nPageSize >> 10);

    results = (TRunData*) malloc(nThreads * sizeof(TRunData));
    threadHandles = (HANDLE*) malloc(nThreads * sizeof(HANDLE));
    threadPaused = (HANDLE*)malloc(nThreads * sizeof(HANDLE));
    threadIds = (DWORD*) malloc(nThreads * sizeof(DWORD));

    t = timeGetTime();
    for (n = 0; n < nThreads; n++) {
        (threadData = results + n) -> id = n;
        threadData->eventPaused = (threadPaused[n] = CreateEvent(NULL, TRUE, FALSE, NULL));
        threadData->eventRun = CreateEvent(NULL, TRUE, FALSE, NULL);
        threadData->tStarted = t;
        try {
            threadHandles[n] = CreateThread(NULL, 0, run, results + n, 0, threadIds + n);
        }
        catch (...) {
            printf("error creating thread #%ld : last error code : %ld", n, GetLastError());
            exit(1);
        }
        SetThreadPriority(threadHandles[n], /*THREAD_PRIORITY_IDLE*/ THREAD_PRIORITY_BELOW_NORMAL);
    }
    while ((nWait = WaitForMultipleObjects(nThreads, threadPaused, TRUE, INFINITE)) == WAIT_TIMEOUT);
    if (nWait == WAIT_FAILED) {
        printf("thread waiting call failed : error code : %ld", GetLastError());
        exit(1);
    }

    t = timeGetTime();
    for (n = 0; n < nThreads; n++) {
        results[n].tStarted = t;
        SetEvent(results[n].eventRun);
    }

    while ((nWait = WaitForMultipleObjects(nThreads, threadHandles, TRUE, INFINITE)) == WAIT_TIMEOUT);
    if (nWait == WAIT_FAILED) {
        printf("thread waiting call failed : error code : %ld", GetLastError());
        exit(1);
    }
    for (n = 0; n < nThreads; n++) {
        CloseHandle(threadHandles[n]);
        CloseHandle(threadPaused[n]);
        CloseHandle(results[n].eventRun);
    }
    free(threadHandles);
    free(threadPaused);
    free(threadIds);

    for (n = 0; n < nThreads; tSum += results[n++].result);

    printf("\nspeed results :\n");
    x = (double) (nSize) ;
    x = x * 1000 * (double) nThreads / tSum / (use64 ? 8 : 4) ;
    if (nThreads > 1) 
        printf("average per thread :\n");
    if (use64)
        printf("\t%10.2f M int64 per second\n", x);
    else
        printf("\t%10.2f M int32 per second\n", x);
    printf("\t%10.2f MBps\n", x * (use64 ? 8 : 4));
    printf("\t%10.2f MBps into cache\n", x * 64);
    if (nThreads > 1) {
        x = x * nThreads;
        printf("total :\n");
        if (use64)
            printf("\t%10.2f M int64 per second\n", x);
        else
            printf("\t%10.2f M int32 per second\n", x);
        printf("\t%10.2f MBps\n", x * (use64 ? 8 : 4));
        printf("\t%10.2f MBps into cache\n", x * 64);
    }
    free(results);
    return 0;
}
