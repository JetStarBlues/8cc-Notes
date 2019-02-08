/*
  Does this all simply generate an AST
   which is then turned to assembly by something else??
*/

#include <libgen.h>  // basename
#include <stdlib.h>  // mkstemps, atexit
#include <string.h>  // strdup, strlen, strcmp, strchr
//#include <sys/types.h>
//#include <sys/wait.h>
#include <stdio.h>   // stderr, stdout, perror, fopen, printf, setbuf
#include <unistd.h>  // unlink, getopt, optarg, optind
#include "8cc.h"     // Vector type, EMPTY_VECTOR, vec_len, vec_get
                     //  Buffer type, format, make_buffer, buf_printf
                     //  error, warning_is_error, enable_warning
                     //  add_include_path
                     //

static char   *infile;
static char   *outfile;
static char   *asmfile;

static bool    dumpast;  // abstract syntax tree
static bool    cpponly;
static bool    dumpasm;
static bool    dontlink;

static Buffer *cppdefs;  // ?
static Vector *tmpfiles = &EMPTY_VECTOR;  // ?


static void usage ( int exitcode )
{
	fprintf(

		exitcode ? stderr : stdout,

		"Usage: 8cc [ -E ][ -a ] [ -h ] <file>\n\n"
		"\n"
		"  -I<path>          add to include path\n"
		"  -E                print preprocessed source code\n"
		"  -D name           Predefine name as a macro\n"
		"  -D name=def\n"
		"  -S                Stop before assembly (default)\n"
		"  -c                Do not run linker (default)\n"
		"  -U name           Undefine name\n"
		"  -fdump-ast        print AST\n"
		"  -fdump-stack      Print stacktrace\n"
		"  -fno-dump-source  Do not emit source code as assembly comment\n"
		"  -o filename       Output to the specified file\n"
		"  -g                Do nothing at this moment\n"
		"  -Wall             Enable all warnings\n"
		"  -Werror           Make all warnings into errors\n"
		"  -O<number>        Does nothing at this moment\n"
		"  -m64              Output 64-bit code (default)\n"
		"  -w                Disable all warnings\n"
		"  -h                print this help\n"
		"\n"
		"One of -a, -c, -E or -S must be specified.\n\n"
	);

	exit( exitcode );
}

static void delete_temp_files ()
{
	for ( int i = 0; i < vec_len( tmpfiles ); i += 1 )
	{
		unlink( vec_get( tmpfiles, i ) );  // man unlink - deletes a name from fs. If name was
		                                   //  last link to file, file is deleted.
	}
}

static char *base ( char *path )
{
	return basename( strdup( path ) );
}

static char *replace_suffix ( char *filename, char suffix )
{
	char *r = format( "%s", filename );  // copy filename to a Buffer?

	char *p = r + strlen( r ) - 1;

	if ( *p != 'c' )
	{
		error( "filename suffix is not .c" );
	}

	*p = suffix;

	return r;
}

static FILE *open_asmfile ()
{
	if ( dumpasm )
	{
		asmfile = outfile ? outfile : replace_suffix( base( infile ), 's' );
	}
	else
	{
		asmfile = format( "/tmp/8ccXXXXXX.s" );

		if ( ! mkstemps( asmfile, 2 ) )  // generate unique temporary filename by replacing "XXXXXX"
		{
			perror( "mkstemps" );
		}

		vec_push( tmpfiles, asmfile );
	}

	if ( ! strcmp( asmfile, "-" ) )  // ?
	{
		return stdout;
	}

	FILE *fp = fopen( asmfile, "w" );

	if ( ! fp )
	{
		perror( "fopen" );
	}

	return fp;
}


static void parse_warnings_arg ( char *s )
{
	if ( ! strcmp( s, "error" ) )
	{
		warning_is_error = true;
	}
	else if ( strcmp( s, "all" ) )
	{
		error( "unknown -W option: %s", s );
	}
}

static void parse_f_arg ( char *s )
{
	if ( ! strcmp( s, "dump-ast" ) )
	{
		dumpast = true;
	}
	else if ( ! strcmp( s, "dump-stack" ) )
	{
		dumpstack = true;
	}
	else if ( ! strcmp( s, "no-dump-source" ) )
	{
		dumpsource = false;
	}
	else
	{
		usage( 1 );
	}
}

static void parse_m_arg ( char *s )
{
	if ( strcmp( s, "64" ) )
	{
		error( "Only 64 is allowed for -m, but got %s", s );
	}
}

static void parseopt( int argc, char **argv )
{
	cppdefs = make_buffer();

	for (;;)
	{
		int opt = getopt( argc, argv, "I:ED:O:SU:W:acd:f:gm:o:hw" );

		if ( opt == -1 )
		{
			break;
		}

		switch ( opt )
		{
			case 'I': {

				add_include_path( optarg );
				break;
			}
			case 'E': {

				cpponly = true;
				break;
			}
			case 'D': {

				char *p = strchr( optarg, '=' );

				if ( p )
				{
					*p = ' ';
				}

				buf_printf( cppdefs, "#define %s\n", optarg );
				break;
			}
			case 'O': break;
			case 'S': {

				dumpasm = true;
				break;
			}
			case 'U': {

				buf_printf( cppdefs, "#undef %s\n", optarg );
				break;
			}
			case 'W': {

				parse_warnings_arg( optarg );
				break;
			}
			case 'c': {

				dontlink = true;
				break;
			}
			case 'f': {

				parse_f_arg( optarg );
				break;
			}
			case 'm': {

				parse_m_arg( optarg );
				break;
			}
			case 'g': break;
			case 'o': {

				outfile = optarg;
				break;
			}
			case 'w': {

				enable_warning = false;
				break;
			}
			case 'h': {

				usage( 0 );
				break;
			}
			default: {

				usage( 1 );
				break;
			}
		}
	}

	if ( optind != argc - 1 )  // input file not specified
	{
		usage( 1 );
	}

	if ( ! dumpast && ! cpponly && ! dumpasm && ! dontlink )
	{
		error( "One of -a, -c, -E or -S must be specified" );
	}

	infile = argv[ optind ];
}

char *get_base_file ()
{
	return infile;
}

static void preprocess ()
{
	for (;;)
	{
		Token *tok = read_token();

		if ( tok->kind == TEOF )
		{
			break;
		}

		if ( tok->bol )
		{
			printf( "\n" );
		}

		if ( tok->space )
		{
			printf( " " );
		}

		printf( "%s", tok2s( tok ) );
	}

	printf( "\n" );

	exit( 0 );  // ?
}





