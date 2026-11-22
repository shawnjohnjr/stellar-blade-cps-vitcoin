/* UE layouts: CPS SDK1.2.0 (MIT). Native call ABI independently checked against game1.5.0. */
typedef struct {uintptr_t data,bits;int count;unsigned stride;} Map;
typedef struct {void* object;uint32_t handle,pad;} Delegate;
typedef void (*AddRequest)(void*,int32_t,int32_t,const Name*,int32_t,int32_t,
                          const Delegate*,const Delegate*,uint8_t,uint8_t,uint8_t);
_Static_assert(sizeof(Name)==8,"FName ABI");
_Static_assert(sizeof(Delegate)==16,"Delegate ABI");
static const char target_alias[]="ETC_Item_VendingMachineCoin";
/* Gold alias verified against the in-game display. */
static const char gold_alias[]="BetaCrystal";
static Name vit_name;
static uintptr_t native_add,presentation_global;
static int32_t item_max_amount;



static Delegate empty_delegate;

static int same_bytes(const void* x,const void* y,unsigned n){unsigned i;for(i=0;i<n;++i)if(((uint8_t*)x)[i]!=((uint8_t*)y)[i])return 0;return 1;}
static int map_view(uintptr_t where,unsigned stride,Map* v){
    int n,max,nbits,nfree;uintptr_t data,bits;
    if(!readable((void*)where,56))return 0;
    data=ptr((void*)where);n=i32((void*)(where+8));max=i32((void*)(where+12));
    nbits=i32((void*)(where+40));nfree=i32((void*)(where+52));
    bits=ptr((void*)(where+32));if(!bits)bits=where+16;
    if(n<0||max<n||max>262144||nfree<0||nfree>n||nbits<n)return 0;
    if(n&&(!readable((void*)data,n*stride)||!readable((void*)bits,4*((n+31)/32))))return 0;
    v->data=data;v->bits=bits;v->count=n;v->stride=stride;return 1;
}
static uintptr_t entry(Map* v,int i){
    if(i<0||i>=v->count||!(((uint32_t*)v->bits)[i/32]>>(i%32)&1))return 0;
    return v->data+(unsigned)i*v->stride;
}
static int field_is(uintptr_t type,const char* wanted,const char* kind,int offset,int size){
    uintptr_t f=ptr((void*)(type+0x50));int i=0;char label[256],k[128];
    for(;f&&i++<256;f=ptr((void*)(f+0x20))){uintptr_t c;
        if(!readable((void*)f,0x80))return 0;
        name(*(Name*)(f+0x28),label,sizeof(label));if(!equal(label,wanted))continue;
        c=ptr((void*)(f+8));if(!readable((void*)c,8))return 0;
        name(*(Name*)c,k,sizeof(k));return equal(k,kind)&&i32((void*)(f+0x4c))==offset&&i32((void*)(f+0x3c))==size;
    }return 0;
}
static int identify_item(void){
    int n=a->find_objects("/Script/Engine.DataTable",found,2048),j,k,matches=0;char label[512];
    for(j=0;j<n&&j<2048;++j){uintptr_t table=(uintptr_t)found[j],type=ptr((void*)(table+0x28));Map map;
        if(!objname((void*)type,label,sizeof(label))||!equal(label,"SBItemTableProperty"))continue;
        if(!field_is(type,"ID","UInt32Property",0x10,4)||
           !field_is(type,"ScanDisplayGroup","NameProperty",0x18c,8)||
           !field_is(type,"MaxAmount","IntProperty",0x84,4)||!map_view(table+0x30,24,&map))return 0;
        for(k=0;k<map.count;++k){uintptr_t p=entry(&map,k),row;Name id;
            if(!p)continue;id=*(Name*)p;name(id,label,sizeof(label));if(!equal(label,target_alias)||id.number!=0)continue;
            row=ptr((void*)(p+8));if(!readable((void*)row,0x194)||i32((void*)(row+0x10))!=10||i32((void*)(row+0x84))<1)return 0;
            name(*(Name*)(row+0x18c),label,sizeof(label));if(!equal(label,"DitCoin"))return 0;
            vit_name=id;item_max_amount=i32((void*)(row+0x84));++matches;
        }
    }return matches==1;
}
static int resolve_native(void){
    static const uint8_t prefix[]={0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x56,0x57,0x41,0x55,0x41,0x56};
    cps_scan scan={sizeof(cps_scan),CPS_SCAN_UNIQUE,0,0,0,0,0};
    uintptr_t base=(uintptr_t)GetModuleHandleA(0),getter;
    native_add=a->scan(self,"48 89 5C 24 10 48 89 6C 24 18 56 57 41 55 41 56 41 57 48 83 EC 40 4C 8B BC 24 A8 00 00 00 48 8D 44 24 20 45 33 ED 49 8B D9",&scan);
    if(!native_add||native_add-base!=0x1afa030||!readable((void*)native_add,sizeof(prefix))||!same_bytes((void*)native_add,prefix,sizeof(prefix)))return 0;
    getter=a->scan(self,"48 83 EC 28 48 8B 05 ? ? ? ? 48 85 C0 75 ? 48 8B 0D ? ? ? ? 48 85 C9 75 ? E8 ? ? ? ? 48 8B 0D ? ? ? ? 48 8B 01 45 33 C0 BA 30 22 00 00",&scan);
    if(!getter||getter-base!=0x1a98b50||!readable((void*)getter,11))return 0;
    presentation_global=getter+11+*(int32_t*)(getter+7);
    return presentation_global-base==0x70373d0;
}
static int inventory(int32_t* actor_id,int64_t* vc,int64_t* gold,int log_rows){
    uintptr_t presentation=ptr((void*)presentation_global),manager,actors,actor,inv,bucket=0;
    int n,index,i,j,bucket_id=-1,valid=0;Map outer,inner,list,items;char label[512];
    *vc=0;*gold=0;
    #define REQUIRE(condition,label) do {if(!(condition)){out("INVENTORY CHECK FAILED: " label "\r\n");return 0;}}while(0)
    REQUIRE(presentation,"presentation pointer");manager=ptr((void*)(presentation+0xc8));
    if(log_rows){out("presentation=");hex(presentation);out(" actor manager=");hex(manager);out("\r\n");}
    REQUIRE(manager&&readable((void*)manager,0x40),"actor manager readable");
    actors=ptr((void*)(manager+0x28));n=i32((void*)(manager+0x30));index=i32((void*)(manager+0x3c));
    if(log_rows){out("actor array=");hex(actors);out(" n=");num(n);out(" selected=");num(index);out("\r\n");}
    REQUIRE(n>=1&&n<=100000&&index>=0&&index<n,"actor array index");
    actor=ptr((void*)(actors+8*index));REQUIRE(readable((void*)actor,0x124),"actor record readable");
    *actor_id=i32((void*)(actor+0x30));
    if(log_rows){out("actor record=");hex(actor);out(" id=");num(*actor_id);out(" hp bits=");hex(*(uint32_t*)(actor+0x11c));out(" max hp bits=");hex(*(uint32_t*)(actor+0x120));out("\r\n");}
    REQUIRE(*actor_id>0,"actor ID positive");
    {float hp=*(float*)(actor+0x11c),maximum=*(float*)(actor+0x120);
        /* Exact tolerance from Cheat Menu 1.1.0: max <= 1e7, hp <= 2*max+1. */
        REQUIRE(hp>=0&&maximum>0&&maximum<=10000000&&hp<=maximum*2.0f+1.0f,"HP and maximum sane");
    }
    inv=ptr((void*)(presentation+0xe8));
    if(log_rows){out("inventory manager=");hex(inv);out(" owner map n=");num(i32((void*)(inv+0x60)));out("\r\n");}
    REQUIRE(inv&&map_view(inv+0x58,0x60,&outer),"inventory owner map");
    for(i=0;i<outer.count;++i){uintptr_t p=entry(&outer,i);if(!p||i32((void*)p)!=1)continue;
        REQUIRE(map_view(p+8,0x10,&inner),"owner actor->bucket map");
        for(j=0;j<inner.count;++j){uintptr_t e=entry(&inner,j);if(e&&i32((void*)e)==*actor_id)bucket_id=i32((void*)(e+4));}
    }
    if(log_rows){out("bucket ID=");num(bucket_id);out("\r\n");}
    REQUIRE(bucket_id>=0&&map_view(inv+8,0x18,&list),"bucket list map");
    for(i=0;i<list.count;++i){uintptr_t e=entry(&list,i);if(e&&i32((void*)e)==bucket_id)bucket=ptr((void*)(e+8));}
    if(log_rows){out("bucket=");hex(bucket);out("\r\n");}
    REQUIRE(bucket&&map_view(bucket+0x40,0x168,&items),"bucket item map");
    for(i=0;i<items.count;++i){uintptr_t p=entry(&items,i);Name id;int amount;
        if(!p)continue;
        if(log_rows&&valid==0){out("first item=");hex(p);out(" GUID key=");hex(ptr((void*)p));out(" GUID value=");hex(ptr((void*)(p+0x10)));out("\r\n");}
        REQUIRE(same_bytes((void*)p,(void*)(p+0x10),16),"inventory GUID key equals value");
        amount=i32((void*)(p+0x44));REQUIRE(amount>=0&&amount<=10000000,"inventory amount range");
        id=*(Name*)(p+0x30);REQUIRE(name(id,label,sizeof(label)),"inventory FName readable");
        if(id.index==vit_name.index&&id.number==vit_name.number)*vc+=amount;
        if(equal(label,gold_alias)&&id.number==0)*gold+=amount;
        if(log_rows){out("INVENTORY ");out(label);out(" count=");num(amount);out("\r\n");}
        ++valid;
    }
    REQUIRE(valid>0,"nonempty native inventory");
    #undef REQUIRE
    return 1;
}
