//#include <ctype.h>
//#include <libgen.h>
//#include <locale.h>
//#include <stdlib.h>
//#include <string.h>
//#include <time.h>
//#include <unistd.h>
#include "8cc.h"  // Token type, EMPTY_VECTOR
                  // Vector type
                  // Map type, EMPTY_MAP
                  // TNUMBER


static Map    *macros           = &EMPTY_MAP;
static Map    *once             = &EMPTY_MAP;
static Map    *keywords         = &EMPTY_MAP;
static Map    *include_guard    = &EMPTY_MAP;
static Vector *cond_incl_stack  = &EMPTY_VECTOR;
static Vector *std_include_path = &EMPTY_VECTOR;

static Token  *cpp_token_zero   = &( Token ){ .kind = TNUMBER, .sval = "0" };
static Token  *cpp_token_one    = &( Token ){ .kind = TNUMBER, .sval = "1" };

static struct tm now;





