#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__


#include <stdio.h>


typedef unsigned char   u8_t;       ///< Unsigned byte.
typedef unsigned short  u16_t;      ///< Unsigned word.
typedef unsigned int    u32_t;      ///< Unsigned dword.

typedef unsigned char   yesno_t;    ///< C-style boolean.
#define YES     1
#define NO      0

typedef unsigned char yesnoneither_t;
#define NEITHER 2

/**
 * @brief C-style return codes.
 *
 * Used when invoking the yacc-lex parser.
 */
typedef enum
{
    RC_OK = 0,      ///< RC_OK              Successful operation.
    RC_SYNTAX_ERR,  ///< RC_SYNTAX_ERR      Syntax error during parsing.
    RC_ALLOC_ERR,   ///< RC_ALLOC_ERR       Malloc error in parser.
    RC_IO_ERR,      ///< RC_IO_ERR          File reading/writing error.
    RC_PREPROC_ERR  ///< RC_PREPROC_ERR     (Syntax) error in preprocessor.
} rc_t;

/*#define DEBUG*/
#ifdef DEBUG_MESSAGES
#define dbgMsg printf
#else
#define dbgMsg
#endif

#define PROFILE_CNT 9

inline void writePacketDump(const u8_t *buf, u16_t bufSz)
{
    if (buf != 0 && bufSz != 0)
    {
        int i = 0;

        for (i = 0; i < bufSz; i += 8)
        {
            int j;

            printf("%04x ", i);
            for (j = 0; j < 8 && (i + j) < bufSz; j++)
            {
                printf("%02x ", buf[i + j]);
            }
            printf("\n");
        }
    }
}

#define SWAP16(x) ((u16_t)(((((u16_t)(x)) >> 8) & 0xffU)    \
                   | ((((u16_t)(x)) & 0xffU) << 8)))

#define SWAP32(x) (((((u32_t)(x)) >> 24) & 0xffU)           \
                   | ((((u32_t)(x)) >>  8) & 0xff00U)       \
                   | ((((u32_t)(x)) <<  8) & 0xff0000U)     \
                   | ((((u32_t)(x)) << 24) & 0xff000000U))

#if defined(BIG_ENDIAN_MACHINE)
#    define HTONS(x) (x)
#    define NTOHS(x) (x)
#    define HTONL(x) (x)
#    define NTOHL(x) (x)
#elif defined(LITTLE_ENDIAN_MACHINE)
#    define HTONS(x) (SWAP16(x))
#    define NTOHS(x) (SWAP16(x))
#    define HTONL(x) (SWAP32(x))
#    define NTOHL(x) (SWAP32(x))
#endif


#endif /* __TYPEDEFS_H__ */
