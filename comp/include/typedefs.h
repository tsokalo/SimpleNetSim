#ifndef __TYPEDEFS_H__
#define __TYPEDEFS_H__


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


#endif /* __TYPEDEFS_H__ */
