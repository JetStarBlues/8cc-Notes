//#include <sys/types.h>
//#include <unistd.h>
#include <errno.h>     // errno
#include <stdlib.h>    // calloc
#include <stdio.h>     // fileno, getc, ungetc, EOF
#include <string.h>    // strerror
#include <sys/stat.h>  // fstat, stat type?
#include <assert.h>    // assert
#include "8cc.h"       // Vector type, EMPTY_VECTOR, vec_tail, vec_len, vec_pop
                       //  File type
                       //  error

static Vector *files   = &EMPTY_VECTOR;
static Vector *stashed = &EMPTY_VECTOR;  // ?

File *make_file ( FILE *file, char *name )
{
	File *r = calloc( 1, sizeof( File ) );  // allocate space for 1 item of File type

	r->file   = file;
	r->name   = name;
	r->line   = 1;
	r->column = 1;

	struct stat st;

	if ( fstat( fileno( file ), &st ) == -1 )
	{
		error( "fstat failed: %s", strerror( errno ) );  // see 'man errno'
	}

	r->mtime = st.st_mtime;  // last modified time

	return r;
}

File *make_file_string ( char *s )
{
	File *r = calloc( 1, sizeof( File ) );

	r->line   = 1;
	r->column = 1;
	r->p      = s;

	return r;
}

static void close_file ( File *f )
{
	if ( f->file )
	{
		fclose( f->file );
	}
}

static int readc_file ( File *f )
{
	int c = getc( f->file );

	if ( c == EOF )  // ? Something about EOF not immediately following a newline
	{
		if ( f->last == '\n' || f->last == EOF )
		{
			c = EOF;
		}
		else
		{
			c = '\n';
		}
	}

	else if ( c == '\r' )  // ? Something about "\r\n"
	{
		int c2 = getc( f->file );

		if ( c2 != '\n' )
		{
			ungetc( c2, f->file );
		}

		c = '\n';
	}

	f->last = c;

	return c;
}

static int readc_string ( File *f )
{
	int c;

	if ( *f->p == '\0' )
	{
		c = ( f->last == '\n' || f->last == EOF ) ? EOF : '\n';
	}

	else if ( *f->p == '\r' )
	{
		f->p += 1;

		if ( *f->p == '\n' )
		{
			f->p += 1;
		}

		c = '\n';
	}

	else
	{
		c = *f->p

		*f->p += 1;
	}

	f->last = c;

	return c;
}

static int get ()
{
	int c;

	File *f = vec_tail( files );  // ?

	if ( f->buflen > 0 )  // check unread characters buffer
	{
		f->buflen -= 1;

		c = f->buf[ f->buflen ];
	}
	else if ( f->file )
	{
		c = readc_file( f );
	}
	else
	{
		c = readc_string( f );
	}

	if ( c == '\n' )
	{
		f->line   += 1;
		f->column  = 1;
	}
	else if ( c != EOF )
	{
		f->column += 1;
	}

	return c;
}

int readc ()
{
	for (;;)
	{
		int c = get();

		if ( c == EOF )  // ?
		{
			if ( vec_len( files ) == 1 )
			{
				return c;
			}

			close_file( vec_pop( files ) );
			continue;
		}

		if ( c != '\\' )
		{
			return c;
		}

		// ...
		int c2 = get();

		if ( c2 == '\n' )
		{
			continue;  // ? ignore
		}

		unreadc( c2 );

		return c;
	}
}

void unreadc ( int c )
{
	if ( c == EOF )
	{
		return;
	}

	File *f = vec_tail( files );

	assert( f->buflen < ( sizeof( f->buf ) / sizeof( f->buf[ 0 ] ) ) );  // ?

	f->buf[ f->buflen ] = c;  // add c to unread buffer

	f->buflen += 1

	if ( c == '\n' )  // ? undo line count increment
	{
		f->line   -= 1;
		f->column  = 1;
	}
	else
	{
		f->column -= 1;
	}
}

File *current_file ()
{
	return vec_tail( files );
}

void stream_push ( File *f )
{
	vec_push( files, f );
}

int stream_depth ()
{
	return vec_len( files );
}

char *input_position ()
{
	if ( vec_len( files ) == 0 )
	{
		return "(unknown)";
	}

	File *f = vec_tail( files );

	return format( "%s:%d:%d", f->name, f->line, f->column );
}

void stream_stash ( File *f )  // ?
{
	vec_push( stashed, files );  // save current then

	files = make_vector1( f );  // ? make new vector with 'f' as first element
}

void stream_unstash ()
{
	files = vec_pop( stashed );
}
