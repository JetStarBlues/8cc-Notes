//#include <stdarg.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/stat.h>
//#include <sys/types.h>
//#include <unistd.h>
//#include "8cc.h"

bool dumpstack = false;
bool dumpsource = true;

static char *REGS [] = {
	"rdi",
	"rsi",
	"rdx",
	"rcx",
	"r8",
	"r9"
};
static char *SREGS [] = {
	"dil",
	"sil",
	"dl",
	"cl",
	"r8b",
	"r9b"
};
static char *MREGS [] = {
	"edi",
	"esi",
	"edx",
	"ecx",
	"r8d",
	"r9d"
};

static int TAB = 8;

static Vector *functions = &EMPTY_VECTOR;
static int     stackpos;
static int     numgp;
static int     numfp;
static FILE   *outputfp;
static Map    *source_files = &EMPTY_MAP;
static Map    *source_lines = &EMPTY_MAP;
static char   *last_loc = "";

static void emit_addr      ( Node *node );
static void emit_expr      ( Node *node );
static void emit_decl_init ( Vector *inits, int off, int totalsize );
static void do_emit_data   ( Vector *inits, int size, int off, int depth );
static void emit_data      ( Node *v, int off, int depth );

#define REGAREA_SIZE 176

#define emit          ( ... ) emitf( __LINE__, "\t" __VA_ARGS__ )  // ?
#define emit_noindent ( ... ) emitf( __LINE__, __VA_ARGS__      )

// ?
#ifdef __GNUC__
	#define SAVE                                                             \
                                                                             \
		int save_hook __attribute__ ( ( unused, cleanup( pop_function ) ) ); \
                                                                             \
		if ( dumpstack )                                                     \
		{                                                                    \
			vec_push( functions, ( void * ) __func__ );                      \
		}                                                                    \

	static void pop_function ( void *ignore )
	{
		if ( dumpstack )
		{
			vec_pop( functions );
		}
	}
#else
	#define SAVE
#endif


... TODO, SKIPPED A BUNCH OF FXS ...


static char *get_int_reg ( Type *ty, char r )
{
	assert( r == 'a' || r == 'c' );

	switch ( ty->size )
	{
		case 1: return ( r == 'a' ) ? "al"  : "cl";
		case 2: return ( r == 'a' ) ? "ax"  : "cx";
		case 4: return ( r == 'a' ) ? "eax" : "ecx";
		case 8: return ( r == 'a' ) ? "rax" : "rcx";

		default:
			error( "Unknown data size: %s: %d", ty2s( ty ), ty->size );
	}
}

static char *get_load_inst ( Type *ty )
{
	switch ( ty->size )
	{
		case 1: return "movsbq";  // move sign extended byte ?
		case 2: return "movswq";  // move sign extended word
		case 4: return "movslq";  // move sign extended double word
		case 8: return "mov";

		default:
			error( "Unknown data size: %s: %d", ty2s( ty ), ty->size );
	}
}


... TODO, SKIPPED A BUNCH OF FXS ...


// ?
static void push_xmm ( int reg )
{
	SAVE;
	emit( "sub $8, #rsp" );
	emit( "movsd #xmm%d, (#rsp)", reg );
	stackpos += 8;
}

// ?
static void pop_xmm ( int reg )
{
	SAVE;
	emit( "movsd (#rsp), #xmm%d", reg );
	emit( "add $8, #rsp" );
	stackpos -= 8;

	assert( stackpos >= 0 );
}

// ?
static void push ( char *reg )
{
	SAVE;
	emit( "push #%s", reg );
	stackpos += 8;
}

// ?
static void pop ( char *reg )
{
	SAVE;
	emit( "pop #%s", reg );
	stackpos -= 8;

	assert( stackpos >= 0 );
}

// ?
static int push_struct ( int size )
{
	SAVE;

	int aligned = align( size, 8 );

	emit( "sub $%d, #rsp", aligned );
	emit( "mov #rcx, -8(#rsp)" );
	emit( "mov #r11, -16(#rsp)" );
	emit( "mov #rax, #rcx" );

	int i = 0;
	for ( ; i < size; i += 8 )
	{
		emit( "movq %d(#rcx), #r11", i );
		emit( "mov #r11, %d(#rsp)", i );
	}
	for ( ; i < size; i += 4 )
	{
		emit( "movl %d(#rcx), #r11", i );
		emit( "movl #r11d, %d(#rsp)", i );
	}
	for ( ; i < size; i++ )
	{
		emit( "movb %d(#rcx), #r11", i );
		emit( "movb #r11b, %d(#rsp)", i );
	}

	emit( "mov  -8(#rsp), #rcx" );
	emit( "mov -16(#rsp), #r11" );

	stackpos += aligned;

	return aligned;
}


... TODO, SKIPPED A BUNCH OF FXS ...


// ?
static void emit_intcast ( Type *ty )
{
	switch( ty->kind )
	{
		case KIND_BOOL:
		case KIND_CHAR: {

			ty->usig ? emit( "movzbq #al, #rax" ) : emit( "movsbq #al, #rax" );
			return;
		{
		case KIND_SHORT: {

			ty->usig ? emit( "movzwq #ax, #rax" ) : emit( "movswq #ax, #rax" );
			return;
		}
		case KIND_INT: {

			ty->usig ? emit( "mov #eax, #eax" ) : emit( "cltq" );
			return;
		}
		case KIND_LONG:
		case KIND_LLONG: return;
	}
}

// ?
/*
	int foo ( double x )
	{
		return ( int ) x;  // <--  cvttsd2si %xmm0, %eax
	}
*/
static void emit_toint ( Type *ty )
{
	SAVE;

	if ( ty->kind == KIND_FLOAT )
	{
		emit( "cvttss2si #xmm0, #eax" );
	}
	else if ( ty->kind == KIND_DOUBLE )
	{
		emit( "cvttsd2si #xmm0, #eax" );
	}
}

// ?
static void emit_lload ( Type *ty, char *base, int off )
{
	SAVE;

	if ( ty->kind == KIND_ARRAY )
	{
		emit( "lea %d(#%s), #rax", off, base );
	}
	else if ( ty->kind == KIND_FLOAT )
	{
		emit( "movss %d(#%s), #xmm0", off, base );
	}
	else if ( ty->kind == KIND_DOUBLE || ty->kind == KIND_LDOUBLE )
	{
		emit( "movsd %d(#%s), #xmm0", off, base );
	}
	else
	{
		char *inst = get_load_inst( ty );
		emit( "%s %d(#%s), #rax", inst, off, base );
		maybe_emit_bitshift_load( ty );
	}
}




