#include "Precompiled.h"
#pragma hdrstop

B_NAMESPACE_BEGIN

// image command function type
enum icFuncType_t {
	FUNC_HEIGHTMAP,			// heightmap ( image, bumpiness )
	FUNC_ADDNORMALS,		// addnormals ( image, image )
	FUNC_GRAYSCALE			// grayscale ( image )
};

// image command argument type
enum icArgumentType_t {
	ARGUMENT_IMAGEFILE,
	ARGUMENT_NUMBER
};

// image command parse state
enum icParseState_t {
	PARSE_STATE_EMPTY		= BIT(0),
	PARSE_STATE_FUNC		= BIT(1),
	PARSE_STATE_LPAREN		= BIT(2),
	PARSE_STATE_RPAREN		= BIT(3),
	PARSE_STATE_PATH		= BIT(4),
	PARSE_STATE_NUMBER		= BIT(5),
	PARSE_STATE_COMMA		= BIT(6)
};

// image command function
struct icFunc_t {
	icFuncType_t		type;
	char *				name;
	int					numArgs;
};

// image command argument
struct icArgument_t {
	icArgumentType_t	type;
	Image				image;
	float				number;
};


static const icFunc_t g_icFuncs[] = {
	{ FUNC_HEIGHTMAP,	"heightmap",	2 },
	{ FUNC_ADDNORMALS,	"addnormals",	2 },
	{ FUNC_GRAYSCALE,	"grayscale",	1 }
};

static icFunc_t			g_funcStack[16];
static int				g_funcStackPointer;

static icArgument_t		g_argumentStack[16];
static int				g_argumentStackPointer;

static const icFunc_t *Image_FindImageCommandFunc(const char *name) {
	for (int i = 0; i < COUNT_OF(g_icFuncs); i++) {
		if (!Str::Icmp(g_icFuncs[i].name, name)) {
			return &g_icFuncs[i];
		}
	}

	return NULL;
}

static bool Image_ExecuteImageCommandFunc() {
	icFunc_t *			func;
	icArgument_t		dest;
	icArgument_t *		args[16];
	byte *				src0;
	byte *				src1;
	byte *				tmp;
	int					i;

	assert(g_funcStackPointer > -1);
	func = &g_funcStack[g_funcStackPointer--];

	assert(g_argumentStackPointer >= func->numArgs - 1);
	for (i = func->numArgs - 1; i >= 0; i--) {
		args[i] = &g_argumentStack[g_argumentStackPointer--];
	}

	switch (func->type) {
	case FUNC_HEIGHTMAP:
		if (args[0]->type != ARGUMENT_IMAGEFILE || args[1]->type != ARGUMENT_NUMBER) {
			return false;
		}

		if (args[0]->image.GetFormat() != IMAGE_FORMAT_RGBA_8_8_8_8) {
			args[0]->image.ConvertFormatSelf(IMAGE_FORMAT_RGBA_8_8_8_8);
		}

		Image *normapMap = args[0]->image.MakeNormalMapRGBA8888(args[1]->number);
		
		dest.type = ARGUMENT_IMAGEFILE;
		dest.image = normalMap;
		break;
	case FUNC_ADDNORMALS:
		if (args[0]->type != ARGUMENT_IMAGEFILE || args[1]->type != ARGUMENT_IMAGEFILE) {
			return false;
		}

		if (args[0]->width != args[1]->width || args[0]->height != args[1]->height) {
			int maxwidth = Max(args[0]->width, args[1]->width);
			int maxheight = Max(args[0]->height, args[1]->height);

			if (args[0]->width < maxwidth || args[0]->height < maxheight) {
				tmp = (byte *)Mem_Alloc16(Image_MemRequired(maxwidth, maxheight, args[0]->format, false));
				Image_Scale(args[0]->pic, args[0]->width, args[0]->height, tmp, maxwidth, maxheight, args[0]->format, IMAGE_BICUBIC);
				Mem_AlignedFree(args[0]->pic);
				args[0]->pic = tmp;
			}

			if (args[1]->width < maxwidth || args[1]->height < maxheight) {
				tmp = (byte *)Mem_Alloc16(Image_MemRequired(maxwidth, maxheight, args[1]->format, false));
				Image_Scale(args[1]->pic, args[1]->width, args[1]->height, tmp, maxwidth, maxheight, args[1]->format, IMAGE_BICUBIC);
				Mem_AlignedFree(args[1]->pic);
				args[1]->pic = tmp;
			}			
		}

		if (args[0]->format != IMAGE_FORMAT_RGBA_8_8_8_8) {
			src0 = (byte *)Mem_Alloc16(args[0]->width * args[0]->height * 4);
			Image_ConvertFormat(args[0]->pic, args[0]->format, src0, IMAGE_FORMAT_RGBA_8_8_8_8, args[0]->width, args[0]->height);
			Mem_AlignedFree(args[0]->pic);
		} else {
			src0 = args[0]->pic;
		}

		if (args[1]->format != IMAGE_FORMAT_RGBA_8_8_8_8) {
			src1 = (byte *)Mem_Alloc16(args[1]->width * args[1]->height * 4);
			Image_ConvertFormat(args[1]->pic, args[1]->format, src1, IMAGE_FORMAT_RGBA_8_8_8_8, args[1]->width, args[1]->height);
			Mem_AlignedFree(args[1]->pic);
		} else {
			src1 = args[1]->pic;
		}

		Image_AddNormalMapsRGBA8888(src0, src1, args[0]->width, args[0]->height, &tmp);
		Mem_AlignedFree(src0);
		Mem_AlignedFree(src1);

		dest.type = ARGUMENT_IMAGEFILE;
		dest.width = args[0]->width;
		dest.height = args[0]->height;
		dest.format = IMAGE_FORMAT_RGBA_8_8_8_8;
		dest.pic = tmp;
		break;	
	case FUNC_GRAYSCALE:
		break;
	default:
		return false;
	}

	assert(g_argumentStackPointer + 1 < COUNT_OF(g_argumentStack));
	g_argumentStack[++g_argumentStackPointer] = dest;

	return true;
}

void Image::CommandLoad(const char *command) {
	int					tokenType;
	int					puncType;
	icParseState_t		lastParseState;
	const icFunc_t *	func;
	icArgument_t		arg;
	Str				path;
	int					i;

	*pic = NULL;		

	Lexer lexer;
	lexer.Init(LEXFL_NOERRORS);
	lexer.Load(command, Str::Length(command), command);

	Str token;
	if (!lexer.ReadToken(&token, false)) {
		return;
	}

	if (!Image_FindImageCommandFunc(token.c_str())) {
		return;
	}

	g_funcStackPointer = -1;
	g_argumentStackPointer = -1;

	lastParseState = PARSE_STATE_EMPTY;

	do {
		if (token.IsEmpty()) {
			goto END_OF_FUNCTION;
		}

		tokenType = lexer.GetTokenType();
		if (tokenType & TT_PUNCTUATION) {
			puncType = lexer.GetPunctuationType();
			switch (puncType) {
			case P_PARENTHESESOPEN:
				if (!(lastParseState & PARSE_STATE_FUNC)) {
					goto SYNTAX_ERROR;
				}

				lastParseState = PARSE_STATE_LPAREN;
				break;
			case P_PARENTHESESCLOSE:
				if (!(lastParseState & (PARSE_STATE_PATH | PARSE_STATE_NUMBER | PARSE_STATE_RPAREN))) {
					goto SYNTAX_ERROR;
				}

				if (lastParseState & PARSE_STATE_PATH) {
					arg.type = ARGUMENT_IMAGEFILE;

					arg.image.Load(path, false);
					if (arg.image.IsEmpty()) {
						goto IMAGE_FILE_NOT_FOUND;
					}

					assert(g_argumentStackPointer + 1 < COUNT_OF(g_argumentStack));
					g_argumentStack[++g_argumentStackPointer] = arg;

					path.Empty();
				}

				Image_ExecuteImageCommandFunc();

				lastParseState = PARSE_STATE_RPAREN;
				break;
			case P_COMMA:
				if (!(lastParseState & (PARSE_STATE_PATH | PARSE_STATE_NUMBER | PARSE_STATE_RPAREN)))
					goto SYNTAX_ERROR;

				if (lastParseState & PARSE_STATE_PATH) {
					arg.type = ARGUMENT_IMAGEFILE;

					arg.image.Load(path, false);
					if (arg.image.IsEmpty()) {
						goto IMAGE_FILE_NOT_FOUND;
					}

					assert(g_argumentStackPointer + 1 < COUNT_OF(g_argumentStack));
					g_argumentStack[++g_argumentStackPointer] = arg;

					path.Empty();
				}

				lastParseState = PARSE_STATE_COMMA;
				break;
			case P_REF:				
			case P_DIV:
			case P_BACKSLASH:
				if (!(lastParseState & PARSE_STATE_PATH)) {
					goto SYNTAX_ERROR;
				}

				path += token;

				lastParseState = PARSE_STATE_PATH;
				break;
			default:
				goto SYNTAX_ERROR;
			}
		} else if (tokenType & TT_IDENTIFIER) {
			if (!(lastParseState & (PARSE_STATE_EMPTY | PARSE_STATE_LPAREN | PARSE_STATE_COMMA | PARSE_STATE_PATH))) {
				goto SYNTAX_ERROR;
			}

			func = Image_FindImageCommandFunc(token.c_str());
			if (func) {
				assert(g_funcStackPointer + 1 < COUNT_OF(g_funcStack));
				g_funcStack[++g_funcStackPointer] = *func;

				lastParseState = PARSE_STATE_FUNC;
			} else {
				path += token;

				lastParseState = PARSE_STATE_PATH;
			}
		} else if (tokenType & TT_NUMBER) {
			if (!(lastParseState & (PARSE_STATE_LPAREN | PARSE_STATE_COMMA))) {
				goto SYNTAX_ERROR;
			}

			arg.type = ARGUMENT_NUMBER;
			arg.number = atof(token.c_str());

			assert(g_argumentStackPointer + 1 < COUNT_OF(g_argumentStack));
			g_argumentStack[++g_argumentStackPointer] = arg;

			lastParseState = PARSE_STATE_NUMBER;
		}
	} while (lexer.ReadToken(&token, false));

SYNTAX_ERROR:
	Core::Log(LOG_WARNING, L"syntax error: '%hs'\n", token.c_str());

	for (i = 0; i <= g_argumentStackPointer; i++) {
		if (g_argumentStack[i].type == ARGUMENT_IMAGEFILE) {
			Mem_AlignedFree(g_argumentStack[i].pic);
		}
	}
	return;

IMAGE_FILE_NOT_FOUND:
	Core::Log(LOG_WARNING, L"image file not found: '%hs'\n", path.c_str());

	for (i = 0; i <= g_argumentStackPointer; i++) {
		if (g_argumentStack[i].type == ARGUMENT_IMAGEFILE) {
			Mem_AlignedFree(g_argumentStack[i].pic);
		}
	}
	return;

END_OF_FUNCTION:
	if (lastParseState == PARSE_STATE_RPAREN) {
		if (g_argumentStackPointer == 0 && g_argumentStack[0].type == ARGUMENT_IMAGEFILE) {
			*pic = g_argumentStack[0].pic;
			*width = g_argumentStack[0].width;
			*height = g_argumentStack[0].height;
			*format = g_argumentStack[0].format;
		}
	}

	for (i = 1; i <= g_argumentStackPointer; i++) {
		if (g_argumentStack[i].type == ARGUMENT_IMAGEFILE) {
			Mem_AlignedFree(g_argumentStack[i].pic);
		}
	}
}

B_NAMESPACE_END