#include <ctype.h>   // isprint
#include <stdarg.h>  // va_start, va_end, va_copy, va_list?
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy
#include "8cc.h"     // Buffer type

#define INIT_SIZE 8

Buffer *make_buffer ()
{
	Buffer *r = malloc( sizeof( Buffer ) );

	r->body   = malloc( INIT_SIZE );
	r->nalloc = INIT_SIZE;
	r->len    = 0;

	return r;
}

char *buf_body ( Buffer *b )
{
	return b->body;
}

int buf_len ( Buffer *b )
{
	return b->len;
}

static void realloc_body ( Buffer *b )
{
	int newsize = b->nalloc * 2;  // double size

	char *body = malloc( newsize );

	memcpy( body, b->body, b->len );  // copy existing...

	b->body   = body;
	b->nalloc = newsize;
}

void buf_write ( Buffer *b, char c )
{
	if ( b->nalloc == ( b->len + 1 ) )  // if used up body, alloc more
	{
		realloc_body( b );
	}
	
	b->body[ b->len ] = c;

	b->len += 1;
}

void buf_append ( Buffer *b, char *s, int len )
{
	for ( int i = 0; i < len; i += 1 )
	{
		buf_write( b, s[ i ] );
	}
}

// ?
void buf_printf ( Buffer *b, char *fmt, ... )
{
	va_list args;

	for (;;)  // infinite loop
	{
		int avail = b->nalloc - b->len;

		va_start( args, fmt );
		int written = vsnprintf(  // append to buffer?

			b->body + b->len,     // destination
			avail,                // max bytes can write
			fmt,
			args
		);
		va_end( args );

		if ( avail <= written )   // run out of space?
		{
			realloc_body( b );
			continue;             // alloc and try again
		}

		b->len += written;        // b->len not updated until successfully write all bytes

		return;
	}
}

// ?
char *vformat ( char *fmt, va_list args )
{
	Buffer *b = make_buffer();
	va_list args2;

	for (;;)
	{
		int avail = b->nalloc - b->len;

		va_copy( args2, args );  // ? copy the previously intialized position
		int written = vsnprintf(

			b->body + b->len,
			avail,
			fmt,
			args2
		);
		va_end( args2 );

		if ( avail <= written )
		{
			realloc_body( b );
			continue;
		}

		b->len += written;

		return buf_body( b );
	}
}

char *format ( char *fmt, ... )
{
	va_list args;

	va_start( args, fmt );
	char *r = vformat( fmt, args );
	va_end( args );

	return r;
}

static char *quote ( char c )  // return literal?
{
	switch ( c )
	{
		case '"':  return "\\\"";  // '\"'
		case '\\': return "\\\\";  // '\\'
		case '\b': return "\\b";   // '\b'
		case '\f': return "\\f";
		case '\n': return "\\n";
		case '\r': return "\\r";
		case '\t': return "\\t";
	}
	return NULL;
}

static void print ( Buffer *b, char c )
{
	char *q = quote( c );

	if ( q )                       // print literal...
	{
		buf_printf( b, "%s", q );
	}
	else if ( isprint( c ) )       // printable character
	{
		buf_printf( b, "%c", c );
	}
	else
	{
		buf_printf( b, "\\x%02x", c );  // print as hex, prefixed with 'x' and zfill(2)
	}
}

// ?
char *quote_cstring ( char *p )
{
	Buffer *b = make_buffer();

	while ( *p )  // until reach null termination
	{
		print( b, *p );

		*p += 1;
	}
	return buf_body( b );
}

// ?
char *quote_cstring_len ( char *p, int len )
{
	Buffer *b = make_buffer();

	for ( int i = 0; i < len; i += 1 )
	{
		print( b, p[ i ] );
	}
	return buf_body( b );
}

// ?
char *quote_char ( char c )
{
	if ( c == '\\' ) return "\\\\";
	if ( c == '\'' ) return "\\'";

	return format( "%c", c );
}
