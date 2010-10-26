#ifndef __ES_H__
#define __ES_H__

#include "types.h"

typedef char sig_issuer[0x40];

typedef u32 sigtype;
typedef sigtype sig_header;
typedef sig_header signed_blob;


typedef u8 sha1[20];
typedef u8 aeskey[16];

typedef struct _tmd_content {
        u32 cid;
        u16 index;
        u16 type;
        u64 size;
        sha1 hash;
}  __attribute__ ( ( packed ) ) tmd_content;

typedef struct _tmd {
        sig_issuer issuer;              //0x140
        u8 version;                             //0x180
        u8 ca_crl_version;              //0x181
        u8 signer_crl_version;  //0x182
        u8 fill2;                               //0x183
        u64 sys_version;                //0x184
        u64 title_id;                   //0x18c
        u32 title_type;                 //0x194
        u16 group_id;                   //0x198
        u16 zero;                               //0x19a
        u16 region;                             //0x19c
        u8 ratings[16];                 //0x19e
        u8 reserved[12];                //0x1ae
        u8 ipc_mask[12];
        u8 reserved2[18];
        u32 access_rights;
        u16 title_version;
        u16 num_contents;
        u16 boot_index;
        u16 fill3;
        // content records follow
        // C99 flexible array
        tmd_content contents[];
}  __attribute__ ( ( packed ) ) tmd;

typedef struct _sig_rsa2048 {
        sigtype type;
        u8 sig[256];
        u8 fill[60];
} __attribute__ ( ( packed ) ) sig_rsa2048;

typedef struct _sig_rsa4096 {
        sigtype type;
        u8 sig[512];
        u8 fill[60];
}  __attribute__ ( ( packed ) ) sig_rsa4096;



#define ES_SIG_RSA4096          0x00000100   // 0x00010000
#define ES_SIG_RSA2048          0x01000100   // 0x00010001
#define ES_SIG_ECC              0x02000100   // 0x00010002

#define SIGNATURE_PAYLOAD(x) ((void *)(((u8*)(x)) + SIGNATURE_SIZE(x)))
#define SIGNATURE_SIZE(x) (\
        ((*(x))==ES_SIG_RSA2048) ? sizeof(sig_rsa2048) : ( \
        ((*(x))==ES_SIG_RSA4096) ? sizeof(sig_rsa4096) : 0 ))

#endif
