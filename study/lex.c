//#include <ctype.h>
#include <errno.h>  // errno
//#include <stdlib.h>
#include <string.h>  // strerror, strcmp
#include <stdio.h>   // fopen
#include "8cc.h"     // Vector type, EMPTY_VECTOR, format
                     //  Token type, TSPACE, TNEWLINE, TEOF, TIDENT, TSTRING, TKEYWORD, TNUMBER, TINVALID, TCHAR
                     //  File type, current_file, make_file, stream_push
                     //  TSPACE, TNEWLINE, TEOF
                     //  errorf, warnf, error
                     //  format


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

void lex_init ( char *filename )
{
	vec_push( buffers, make_vector() );

	if ( ! strcmp( filename, "-" ) )
	{
		stream_push( make_file( stdin, "-" ) );
		return;
	}

	FILE *fp = fopen( filename, "r" );

	if ( ! fp )
	{
		error( "Cannot open %s: %s", filename, strerror( errno ) );
	}

	stream_push( make_file( fp, filename ) );
}

static Pos get_pos ( int delta )
{
	File *f = current_file();

	return ( Pos ){ f->line, f->column + delta };  // ?
}

static void mark ()
{
	pos = get_pos( 0 );
}

static Token *make_token ( Token *tmpl )
{
	Token *r = malloc( sizeof( Token ) );

	*r = *tmpl;  // ? why malloc 'r' if going to use 'tmpl'?

	r->hideset = NULL;  // ?

	File *f = current_file();

	r->file   = f;
	r->line   = pos.line;
	r->column = pos.column;
	r->count  = f->ntok;

	f->ntok += 1;

	return r;
}

static Token *make_ident ( char *p )
{
	return make_token( &( Token ){

		TIDENT,
		.sval = p
	} );
}

static Token *make_strtok ( char *s, int len, int enc )
{
	return make_token( &( Token ){

		TSTRING,
		.sval = s,
		.slen = len,
		.enc  = enc
	} );
}

static Token *make_keyword ( int id )
{
	return make_token( &( Token ){

		TKEYWORD,
		.id = id
	} );
}

static Token *make_number ( char *s )
{
	return make_token( &( Token ){

		TNUMBER,
		.sval = s
	} );
}

static Token *make_invalid ( char c )
{
	return make_token( &( Token ){

		TINVALID,
		.c = c
	} );
}

static Token *make_char ( int c, int enc )
{
	return make_token( &( Token ){

		TCHAR,
		.c   = c,
		.enc = enc
	} );
}

static bool iswhitespace ( int c )
{
	return c == ' ' || c == '\t' || c == '\f' || c == '\v';
}

static int peek ()
{
	int r = readc();

	unreadc( r );

	return r;
}

static bool next ( int expect )
{
	int c = readc();

	if ( c == expect )
	{
		return true;
	}
	
	unreadc( c );
	
	return false;
}

static void skip_line ()
{
	for ( ;; )
	{
		int c = readc();

		if ( c == EOF )
		{
			return;
		}

		if ( c == '\n' )
		{
			unreadc( c );
			return;
		}
	}
}


















