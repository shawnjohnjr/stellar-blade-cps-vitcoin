/* Minimal Win32 / UE name helpers. CPS SDK1.2.0 UE layouts (MIT; see LICENSE.CPS-SDK). */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <cps_api.h>

int _fltused;
static const cps_api* a;
static cps_client self;
static char dir[1024];
static HANDLE report=INVALID_HANDLE_VALUE;
static void* found[2048];
typedef struct { int32_t index, number; } Name;
static uintptr_t pool, objects;

static unsigned length(const char* s){unsigned n=0;while(s[n])++n;return n;}
static void copy(char* d,const char* s,unsigned cap){unsigned i=0;for(;i+1<cap&&s[i];++i)d[i]=s[i];d[i]=0;}
static void append(char* d,const char* s,unsigned cap){unsigned n=length(d);if(n<cap)copy(d+n,s,cap-n);}
static int equal(const char* s,const char* t){while(*s&&*s==*t){++s;++t;}return *s==*t;}
static int contains(const char* s,const char* t){for(;*s;++s){const char* x=s;const char* y=t;while(*y&&*x==*y){++x;++y;}if(!*y)return 1;}return 0;}
static int readable(const void* p,unsigned n){
    uintptr_t at=(uintptr_t)p,end=at+n;
    if(!p||end<at)return 0;
    while(at<end){MEMORY_BASIC_INFORMATION m;uintptr_t next;
        if(!VirtualQuery((void*)at,&m,sizeof(m))||m.State!=MEM_COMMIT||(m.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return 0;
        next=(uintptr_t)m.BaseAddress+m.RegionSize;if(next<=at)return 0;at=next;
    }return 1;
}
static uintptr_t ptr(const void* p){return readable(p,8)?*(const uintptr_t*)p:0;}
static int32_t i32(const void* p){return readable(p,4)?*(const int32_t*)p:0;}
static void out(const char* s){DWORD n;if(report!=INVALID_HANDLE_VALUE)WriteFile(report,s,length(s),&n,0);}
static void num(int64_t n){char b[32];unsigned i=31;int neg=n<0;uint64_t v=neg?-(uint64_t)n:n;b[i]=0;do{b[--i]=(char)('0'+v%10);v/=10;}while(v);if(neg)b[--i]='-';out(b+i);}
static void hex(uintptr_t n){char b[19];unsigned i=18;b[i]=0;do{b[--i]="0123456789abcdef"[n&15];n>>=4;}while(n);b[--i]='x';b[--i]='0';out(b+i);}
static int name(Name n,char* dest,unsigned cap){
    uint32_t block=(uint32_t)n.index>>16,off=((uint32_t)n.index&65535)*2;
    uintptr_t data,entry;uint16_t h;unsigned len,j;
    dest[0]=0;if(n.index<0||block>=8192||!readable((void*)(pool+8),8)||block>*(uint32_t*)(pool+8))return 0;
    data=ptr((void*)(pool+16+8*block));entry=data+off;if(!data||!readable((void*)entry,2))return 0;
    h=*(uint16_t*)entry;len=h>>6;if(len>1023||!readable((void*)(entry+2),len*((h&1)?2:1)))return 0;
    if(h&1){int size=WideCharToMultiByte(CP_UTF8,0,(WCHAR*)(entry+2),len,dest,cap-1,0,0);if(size<=0)return 0;dest[size]=0;}
    else{for(j=0;j<len&&j+1<cap;++j)dest[j]=*(char*)(entry+2+j);dest[j]=0;}
    return 1;
}
static int objname(void* o,char* dest,unsigned cap){if(!readable(o,0x20)){dest[0]=0;return 0;}return name(*(Name*)((char*)o+0x18),dest,cap);}
