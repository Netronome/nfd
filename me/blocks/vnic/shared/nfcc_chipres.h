#ifndef _NFCC_CHIPRES_H_
#define _NFCC_CHIPRES_H_

#define _link_sym(x) __link_sym(#x)
#define ASM(_str) __asm{_str}

#define EMEM_QUEUE_ALLOC(_name_, _scope_, _resourcepool_) \
    ASM(.declare_resource _resourcepool_ global 1024)     \
    ASM(.alloc_resource _name_ _resourcepool_ _scope_ 1)

#if _nfp_has_island("emem0")
    #define emem0_queues_DECL                              \
        ASM(.declare_resource emem0_queues global 1024)

    #define EMEM0_QUEUE_ALLOC(_name_, _scope_)             \
        EMEM_QUEUE_ALLOC(_name_, _scope_, emem0_queues)
#endif

#if _nfp_has_island("emem1")
    #define emem1_queues_DECL                              \
        ASM(.declare_resource emem1_queues global 1024)

    #define EMEM1_QUEUE_ALLOC(_name_, _scope_)             \
        EMEM_QUEUE_ALLOC(_name_, _scope_, emem1_queues)
#endif

#if _nfp_has_island("emem2")
    #define emem2_queues_DECL                              \
        ASM(.declare_resource emem2_queues global 1024)

    #define EMEM2_QUEUE_ALLOC(_name_, _scope_)             \
        EMEM_QUEUE_ALLOC(_name_, _scope_, emem2_queues)
#endif


#define CLS_RING_DECL                           \
    ASM(.declare_resource cls_rings island 16)

#define CLS_RING_ALLOC(_name_, _scope_)             \
    ASM(.alloc_resource _name_ cls_rings _scope_ 1)

#define CLS_APSIGNAL_DECL                           \
    ASM(.declare_resource cls_apsignals island 16)

#define CLS_APSIGNAL_ALLOC(_name_, _scope_)             \
    ASM(.alloc_resource _name_ cls_apsignals _scope_ 1)

#define CLS_APFILTER_DECL                           \
    ASM(.declare_resource cls_apfilters island 16)

#define CLS_APFILTER_ALLOC(_name_, _scope_)             \
    ASM(.alloc_resource _name_ cls_apfilters _scope_ 1)


#endif /* !_NFCC_CHIPRES_H_ */
