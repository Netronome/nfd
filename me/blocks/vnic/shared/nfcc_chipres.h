#define alloc_mem(x) _alloc_mem(#x)
#define declare_resource(x) _declare_resource(#x)
#define _alloc_resource(x) __alloc_resource(#x)

#define EMEM_QUEUE_ALLOC(_name_, _scope_, _resourcepool_) \
    _alloc_resource(_name_ _resourcepool_ _scope_ 1)

#if _nfp_has_island("emem0")
    _declare_resource("emem0_queues global 1024")

    #define EMEM0_QUEUE_ALLOC(_name_, _scope_) \
        EMEM_QUEUE_ALLOC(_name_, _scope_, emem0_queues)

#endif

#if _nfp_has_island("emem1")
    _declare_resource("emem1_queues global 1024")

    #define EMEM1_QUEUE_ALLOC(_name_, _scope_) \
        EMEM_QUEUE_ALLOC(_name_, _scope_, emem1_queues)
#endif

#if _nfp_has_island("emem2")
    _declare_resource("emem2_queues global 1024")

    #define EMEM2_QUEUE_ALLOC(_name_, _scope_) \
        EMEM_QUEUE_ALLOC(_name_, _scope_, emem2_queues)
#endif

_declare_resource("cls_rings island 16")

#define CLS_RING_ALLOC(_name_, _scope_) \
    _alloc_resource(_name_ cls_rings _scope_ 1)

_declare_resource("cls_apsignals island 16")

#define CLS_APSIGNAL_ALLOC(_name_, _scope_) \
    _alloc_resource(_name_ cls_apsignals _scope_ 1)

_declare_resource("cls_apfilters island 16")

#define CLS_APFILTER_ALLOC(_name_, _scope_) \
    _alloc_resource(_name_ cls_apfilters _scope_ 1)
