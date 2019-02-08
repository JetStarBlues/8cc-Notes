//#include <ctype.h>
//#include <errno.h>
//#include <stdlib.h>
//#include <string.h>
#include "8cc.h"  // Vector type, EMPTY_VECTOR,
                  // Token type
                  // File type, current_file
                  // TSPACE, TNEWLINE, TEOF
                  // errorf, warnf


static Vector *buffers       = &EMPTY_VECTOR;
static Token  *space_token   = &( Token ){ TSPACE };
static Token  *newline_token = &( Token ){ TNEWLINE };
static Token  *eof_token     = &( Token ){ TEOF };

typedef struct
{
	int line;
	int column;
} Pos;

static Pos pos;

static char *pos_string ( Pos *p )
{
	File *f = current_file();

	return format(

		"%s:%d:%d",
		f ? f->name : "(unknown)",
		p->line,
		p->column
	);
}

#define errorp ( p, ... ) errorf( __FILE__ ":" STR( __LINE__ ), pos_string( &p ), __VA_ARGS__ )
#define warnp  ( p, ... )  warnf( __FILE__ ":" STR( __LINE__ ), pos_string( &p ), __VA_ARGS__ )

static void skip_block_comment ( void );

