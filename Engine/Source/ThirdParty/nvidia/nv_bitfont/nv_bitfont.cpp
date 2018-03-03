//----------------------------------------------------------------------------------
// File:        jni/nv_bitfont/nv_bitfont.cpp
// SDK Version: v10.14 
// Email:       tegradev@nvidia.com
// Site:        http://developer.nvidia.com/
//
// Copyright (c) 2007-2012, NVIDIA CORPORATION.  All rights reserved.
//
// TO  THE MAXIMUM  EXTENT PERMITTED  BY APPLICABLE  LAW, THIS SOFTWARE  IS PROVIDED
// *AS IS*  AND NVIDIA AND  ITS SUPPLIERS DISCLAIM  ALL WARRANTIES,  EITHER  EXPRESS
// OR IMPLIED, INCLUDING, BUT NOT LIMITED  TO, IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL  NVIDIA OR ITS SUPPLIERS
// BE  LIABLE  FOR  ANY  SPECIAL,  INCIDENTAL,  INDIRECT,  OR  CONSEQUENTIAL DAMAGES
// WHATSOEVER (INCLUDING, WITHOUT LIMITATION,  DAMAGES FOR LOSS OF BUSINESS PROFITS,
// BUSINESS INTERRUPTION, LOSS OF BUSINESS INFORMATION, OR ANY OTHER PECUNIARY LOSS)
// ARISING OUT OF THE  USE OF OR INABILITY  TO USE THIS SOFTWARE, EVEN IF NVIDIA HAS
// BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
//
//
//----------------------------------------------------------------------------------


#include <nv_global.h>

#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <nv_shader/nv_shader.h>
#include <nv_hhdds/nv_hhdds.h>

//#include <nv_memory/nv_memory.h>

#include "nv_bitfont.h"
//#include <nv_string/nv_string.h>
#include <nv_file/nv_file.h>
//#include "nv_glesutil/nv_statefilter.h"


#include <android/log.h>
#define MODULE "nv_bitfont"
#define LOG_LEVEL_ERROR
#define LOG_LEVEL_DEBUG
//#define LOG_LEVEL_VERBOSE
#ifdef LOG_LEVEL_ERROR
#define ERROR_LOG(s, args...) __android_log_print(ANDROID_LOG_ERROR, MODULE, s, ##args)
#else
#define ERROR_LOG(s, args...)
#endif
#ifdef LOG_LEVEL_DEBUG
#define DEBUG_LOG(s, args...) __android_log_print(ANDROID_LOG_DEBUG, MODULE, s, ##args)
#else
#define DEBUG_LOG(s, args...)
#endif
#ifdef LOG_LEVEL_VERBOSE
#define VERBOSE_LOG(s, args...) __android_log_print(ANDROID_LOG_VERBOSE, MODULE, s, ##args)
#else
#define VERBOSE_LOG(s, args...)
#endif


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
static int TestPrintGLError(const char* str)
{
    int err;
    err = glGetError();
    if (err)
        VERBOSE_LOG(str, err);
    return err;
}

//========================================================================
// datatypes & defines
//========================================================================

#define IND_PER_QUAD    6
#define VERT_PER_QUAD    4
#define MAX_FILENAME    1024
#define BF_NUMCHARS        256

typedef float Point2[2];

static NvPackedColor s_charColorTable[16] =
{
    NV_PACKED_COLOR(0xFF, 0xFF, 0xFF, 0xFF), //white
    NV_PACKED_COLOR(0x99, 0x99, 0x99, 0xFF), //medium-gray
    NV_PACKED_COLOR(0x00, 0x00, 0x00, 0xFF), //black
    NV_PACKED_COLOR(0xFF, 0x33, 0x33, 0xFF), //brightened red
    NV_PACKED_COLOR(0x11, 0xFF, 0x11, 0xFF), //brighter green
    NV_PACKED_COLOR(0x33, 0x33, 0xFF, 0xFF) //brightened blue
};

/* windows ABC font metrics... */
typedef struct 
{ 
   short l;
   short a;
   short b;
   short c;
} ABCStruct;

typedef struct
{
    float top, left, bottom, right;
} fRect;

typedef struct _NvBitFont
{
    NvU8        id;        // no need for more than 255, right??
    char        alpha;
    char        rgb;
    char        isSplit;

    char        filename[1024];

    GLuint       tex;
    float        cell;
    ABCStruct    abc[BF_NUMCHARS];
    fRect        rect[BF_NUMCHARS];

    // FIXME TODO!!!!
    //NvHHDDSImage *data; // don't NEED to keep this cached
    // ... BUT on some platforms, we may need to 'survive' sleep or pause or suspend.
    // is Android such a platform???

    struct _NvBitFont *next;
} NvBitFont;

//========================================================================
// static vars
//========================================================================

static NvBool s_bfInitialized = NV_FALSE;

// these track the bitmap font loading.
static NvBitFont *bitFontLL = NULL;
static NvU8 bitFontID = 1; // NOT ZERO, start at 1 so zero is 'invalid'...

// for ES2, always vbos, but we need to track shader program...
static GLuint fontProg = 0;
static GLboolean fontProgAllocInternal = 1;
static GLint fontProgLocMat;
static GLint fontProgLocColor, fontProgLocTex;
static GLint fontProgAttribPos, fontProgAttribCol, fontProgAttribTex;

//========================================================================
static float dispW = 0, dispH = 0;
static float dispAspect = 0;
static float dispRotation = 0;
static float dispOffX = 0, dispOffY = 0;

//========================================================================
//========================================================================
static GLuint lastColorArray = 0;
static GLuint lastFontTexture = 0;
static NvU8 lastTextMode = 111;

//========================================================================
//static float lastTextHigh = 0;

// the master index VBO for rendering ANY text, since they're all sequences of rects.
static int maxIndexChars = 0;
static short *masterTextIndexList = NULL;
static GLuint masterTextIndexVBO = 0;

static NvF32 s_pixelToClipMatrix[4][4];
static NvF32 s_pixelScaleFactorX = 2.0f / 640.0f;
static NvF32 s_pixelScaleFactorY = 2.0f / 480.0f;

const GLfloat *m_matrixOverride = NV_NULL;


//========================================================================
// these are for DrawString, for setting up some high-water-mark buffers
// so the function isn't reallocating constantly.
static short *textIndices = NULL;
static Point2 *textVertCoords = NULL;
static Point2 *textTexCoords = NULL;
static int maxBatchSize = 0;
static int currBatchSize = 0;

#define NvMalloc malloc
#define NvFree free
#define NvMemset memset
#define NvMemcpy memcpy

#define NvFilterBindBuffer		glBindBuffer
#define NvFilterBufferData		glBufferData
#define NvFilterDisable			glDisable
#define NvFilterEnable			glEnable
#define NvFilterIsEnabled		glIsEnabled
#define NvFilterUseProgram		glUseProgram
#define NvFilterGetVertexAttribiv			glGetVertexAttribiv
#define NvFilterGetVertexAttribPointerv		glGetVertexAttribPointerv


const static char s_fontVertShader[] =
"// this is set from higher level.  think of it as the upper model matrix\n"
"uniform mat4 pixelToClipMat;\n"
"uniform vec4 col_uni;\n"
"attribute vec2 pos_attr;\n"
"attribute vec2 tex_attr;\n"
"attribute vec4 col_attr;\n"
"varying vec4 col_var;\n"
"varying vec2 tex_var;\n"
"void main() {\n"
"	// account for translation and rotation of the primitive into [-1,1] spatial default.\n"
"    gl_Position = pixelToClipMat * vec4(pos_attr.x, pos_attr.y, 0, 1);\n"
"    col_var = col_attr * col_uni; // when shadowing, this multiplies shadowed chars as well!!!\n"
"    tex_var = tex_attr;\n"
"}\n";

const static char s_fontFragShader[] =
"precision mediump float;\n"
"uniform sampler2D fontTex;\n"
"varying vec4 col_var;\n"
"varying vec2 tex_var;\n"
"void main() {\n"
"    float alpha = texture2D(fontTex, tex_var).a;\n"
"    gl_FragColor = col_var * vec4(1.0, 1.0, 1.0, alpha);\n"
"}\n";


//========================================================================
//========================================================================
static NvBitFont *BitFontAlloc(void)
{
    NvBitFont *bf = NULL;
    bf = (NvBitFont*)NvMalloc(sizeof(NvBitFont));
    if (bf==NULL) return (NULL);
    // clear the structure
    NvMemset(bf, 0, sizeof(NvBitFont));
    // hook into ll
    bf->next = bitFontLL;
    bitFontLL = bf;
    return(bf);
}


#if 0
//========================================================================
//========================================================================
static void BitFontFree(NvBitFont *bf)
{
    NvBitFont *head, *follow;
    // need to walk the ll so we can remove.
    head = bitFontLL;
    follow = NULL;
    if (bf==bitFontLL) // the top of the list
    {
        bitFontLL = bitFontLL->next; // shift head up..
    }
    else
    while (head->next)
    {
        follow = head;
        head = head->next;
        if (bf==head)
        {
            follow->next = head->next; // remove from list
            break; // done.
        }
    }

    // then clean up and NvFree.
    //if (bf->dds) NVHHDDSNvFree(bf->dds);
    NvFree(bf);
}
#endif


//========================================================================
//========================================================================
static NvBitFont *BitFontFromID(NvU8 fontnum)
{
    NvBitFont *bitfont = bitFontLL;
    while (bitfont)
    {
        if (bitfont->id==fontnum) // matched
            return bitfont;
        bitfont = bitfont->next;
    }
    
    return NULL;
}


//========================================================================
// this is the ONLY externalized access to the bitfont list
// used to match up a filename with the internal id/fontnum it got.
//========================================================================
NvU8 NVBFGetFontID(const char* filename)
{
    NvBitFont *bitfont = bitFontLL;
    while (bitfont)
    {
        if (0==strcmp(filename, bitfont->filename))
            return bitfont->id;
        bitfont = bitfont->next;
    }
    
    return 0;
}


static void NVBFFontProgramPrecache(void)
{
    NvFilterUseProgram(fontProg);
    
    // grab new uniform locations.
    fontProgLocMat = glGetUniformLocation(fontProg, "pixelToClipMat");
    fontProgLocColor = glGetUniformLocation(fontProg, "col_uni");
    fontProgLocTex = glGetUniformLocation(fontProg, "fontTex");

    fontProgAttribPos = glGetAttribLocation(fontProg, "pos_attr");
    fontProgAttribCol = glGetAttribLocation(fontProg, "col_attr");
    fontProgAttribTex = glGetAttribLocation(fontProg, "tex_attr");

    // and bind the uniform for the sampler
    // as it never changes.
    glUniform1i(fontProgLocTex, 0);
}


//========================================================================
// if called before init, this will keep BF from trying to load its default
// program 'font.vert' and 'font.frag'.
// if called AFTER init, this will delete the old program, and put ours in
// its place.
// Note that any replacement program must AT LEAST support the attribs
// and uniforms the canonical font shader references, so code here won't fail.
//========================================================================
void NVBFPresetFontProgram(GLuint program)
{
    if (fontProg && fontProgAllocInternal)
        glDeleteProgram(fontProg);
    fontProg = program;
    fontProgAllocInternal = 0;
    NVBFFontProgramPrecache();
}


//========================================================================
// !!!!TBD needs a lot more error handling with finding the files...
// should also allow a method for being handed off the data from the app,
// rather than opening the files here.  split this into two funcs!!!!!TBD
//========================================================================
int NVBFInitialize(NvU8 count, const char* filename[], NvBool split[], NvBool disableMips)
                 //, int mipmap) // the mipmap setting is needed only if we add back RAW support.
{
    int i,j;
    NvFile *file;
    NvSize n;
    int texw;
    const float one16 = 1.0f/16.0f; // 16 cells across...
    float halftexel, onetexel;
    char abcFilename[1024];
    GLubyte tmpabc[BF_NUMCHARS][8];
    NVHHDDSImage *dds = NULL;
    NvBitFont *bitfont = NULL;
//    int colorBytes = 4;
    int fontsLoaded = 0;

    if (TestPrintGLError("> Caught GL error 0x%x @ top of NVBFInitialize...\n"))
    {
        //return(1);
    }
    
    if (fontProg == 0)
    { // then not one set already, load one...
    // this loads from a file
        fontProg = nv_load_program_from_strings(s_fontVertShader, s_fontFragShader);  
        if (!fontProg)
        {
            ERROR_LOG("!!> NVBFInitialize failure: couldn't load shader program...\n");
            return(1);
        }

        fontProgAllocInternal = 1;
        NVBFFontProgramPrecache();

        // The following entries are const
        // so we set them up now and never change
        s_pixelToClipMatrix[2][0] = 0.0f;
        s_pixelToClipMatrix[2][1] = 0.0f;

        // Bitfont obliterates Z right now
        s_pixelToClipMatrix[0][2] = 0.0f;
        s_pixelToClipMatrix[1][2] = 0.0f;
        s_pixelToClipMatrix[2][2] = 0.0f;
        s_pixelToClipMatrix[3][2] = 0.0f;

        s_pixelToClipMatrix[0][3] = 0.0f;
        s_pixelToClipMatrix[1][3] = 0.0f;
        s_pixelToClipMatrix[2][3] = 0.0f;
        s_pixelToClipMatrix[3][3] = 1.0f;
    }

    // since this is our 'system initialization' function, allocate the master index VBO here.
    if (masterTextIndexVBO==0)
    {
        glGenBuffers(1, &masterTextIndexVBO);
        if (TestPrintGLError("Error 0x%x NVBFInitialize master index vbo...\n"))
            return(1);
    }

    for (j=0; j<count; j++)
    {
        if (NVBFGetFontID(filename[j]))
        {
            fontsLoaded++;
            continue; // already have this one.
        }
            
        dds = NULL;

        NvS32 namelen = strlen(filename[j]);
        if (0==strcmp(filename[j]+namelen-3, "dds"))
        {
            dds = NVHHDDSLoad(filename[j], 0);
            if (dds==NULL)
            {
//                ERROR_LOG("Font [%s] couldn't be loaded by the NVHHDDS library.\n", filename[j]);
                continue;
            }
            texw = dds->width;
        }
        else
        {
//            ERROR_LOG("Font [%s] wasn't a .DDS file, the only supported format.\n", filename[j]);
            continue;            
        }

        // build the abc filename...
        strcpy(abcFilename, filename[j]);
        i = strlen(abcFilename);
        abcFilename[i-3] = 0; // null term
        strcat(abcFilename, "abc");

        file = NvFOpen(abcFilename);
        if (!file)
        {
            ERROR_LOG(">> FAILED TO LOAD font ABC file: %s...\n", abcFilename);
            continue;
        }
        i = NvFSize(file);
        n = NvFRead(tmpabc, 1, i, file);
        NvFClose(file);

        // okay, at this point we're ready to go, so flag we're okay, allocate a font struct, setup GL stuff.
        // !!!!TBD not sure if we should wait until uploaded, definitely wait
        // until the files are both opened and read in!!
        fontsLoaded++;

        bitfont = BitFontAlloc();
        bitfont->id = bitFontID++; // store our 'index' away
        bitfont->alpha = (char)(dds && dds->alpha); // !!!!TBD or ==2 for LumaAlpha tex?
        bitfont->rgb = (char)(dds && dds->components>=3);
        bitfont->isSplit = 0;
        if (split && split[j]) 
            bitfont->isSplit = 1;
        //kdStrcpy_s(bitfont->filename, kdStrlen(filename[j]) + 1, filename[j]);
        strcpy(bitfont->filename, filename[j]);

        // copy over the abc data.
        NvMemcpy(bitfont->abc, tmpabc, sizeof(tmpabc));

        TestPrintGLError("Error 0x%x NVBFInitialize before texture gen...\n");

        // GL initialization...
        glGenTextures( 1, &(bitfont->tex) );
        glBindTexture( GL_TEXTURE_2D, bitfont->tex );
        TestPrintGLError("Error 0x%x NVBFInitialize after texture bind...\n");
    
        // set up the font Rect structure and the spacing
        bitfont->cell = texw * one16; // cell size.
        onetexel = 1.0f/texw;
        halftexel = 0.5f/texw;
        for (i=0; i<BF_NUMCHARS; i++)
        {
            int x = i%16;
            int y = i/16;
            bitfont->rect[i].left = x * one16;
#ifdef no_bfb_adjust
            bitfont->rect[i].right = (x+1) * one16;
#else // bfb has the glyphs rendered shifted -- we must compensate.
#define SRC_RECT_ADJ    1
            if (bitfont->abc[i].a != 0)
                bitfont->rect[i].left += (bitfont->abc[i].a * onetexel);
            bitfont->rect[i].right = bitfont->rect[i].left +
                        ((bitfont->abc[i].b + SRC_RECT_ADJ) * onetexel);
#endif
            bitfont->rect[i].top = y * one16;
            bitfont->rect[i].bottom = (y+1) * one16;

#if 1 // avoid bilinear or mip bleeding by a half-texel inward shift?  could add problems...
            bitfont->rect[i].top += onetexel;
//            bitfont->rect[i].left += halftexel;
//            bitfont->rect[i].right -= halftexel;
//            bitfont->rect[i].top += halftexel;
//            bitfont->rect[i].bottom -= halftexel;
#endif
        }

        {    
            int numMips = dds->numMipmaps;
            int upw = dds->width, uph = dds->height;
            if (disableMips)
                numMips = 1;

            //DEBUG_LOG("Uploading font [%s], size %dx%d, miplevels %d...\n", filename[j], dds->mipwidth[0], dds->mipheight[0], dds->numMipmaps);
            for (i=0; i<numMips; i++) // run through and upload base+mips
            {
                //VERBOSE_LOG(">>> mip #%d, %dx%d...\n", i, dds->mipwidth[i], dds->mipheight[i]);
                if (dds->compressed) // the dxt formats...
                {
                    glCompressedTexImage2D( GL_TEXTURE_2D, i, dds->format, upw, uph, 0, dds->size[i], dds->data[i] );
                }
                else // rgba textures... !!!!TBD no pal handler yet.
                    glTexImage2D( GL_TEXTURE_2D, i, dds->format, upw, uph, 0, dds->format, dds->componentFormat, dds->data[i] );
                if (TestPrintGLError("Error 0x%x NVBFInitialize after DDS image upload...\n"))
                {
                    //printf(">> mip #%d, %dx%d)...\n", i, upw, uph);
                }
                // only 'shrink' to 1, NOT ZERO.
                if (upw>1) upw>>=1;
                if (uph>1) uph>>=1;
            }

            if (TestPrintGLError("Error 0x%x NVBFInitialize REALLY after DDS image upload...\n"))
            {
                //printf(">> mip #%d, %dx%d)...\n", i, upw, uph);
            }
            
            if (numMips>1)
            {
                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR ); // quality, not speed!!! !!!!TBD
#ifdef XX
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 4);
#endif
//                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST ); // speed, not quality!!! !!!!TBD
            }
            else
                glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
            glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
        }

        // !!!!TBD do we want to hold onto the dds in our structure?
        if (dds)
            NVHHDDSFree(dds); // clean up
    }

    // turn off texture bind.
    glBindTexture( GL_TEXTURE_2D, 0 );

    if (fontsLoaded!=count)
        ERROR_LOG("!!> couldn't load all requested fonts\n");

    s_bfInitialized = NV_TRUE;

    return (fontsLoaded!=count);
}


//========================================================================
// this function has a multi-tiered job
// not only should it clean up the fonts themselves
// but it should also make sure that the BFText system is cleaned up
// including the LL objects, the master index ptr & VBO, etc.
//========================================================================
void NVBFCleanup()
{
    if (s_bfInitialized)
    {
    // free the shader
        if (fontProg)
        {
            glDeleteProgram(fontProg);
            fontProg = 0;
        }
    // NvFree each font in the fontLL
        NvBitFont *bitfont = bitFontLL, *currFont;
        while (bitfont)
        {
            currFont = bitfont;
            bitfont = bitfont->next;
            glDeleteTextures( 1, &(currFont->tex) );
            NvFree(currFont);
        }
        bitFontLL = NULL;
		bitFontID = 1;
    // NvFree the master index vbo
        if (masterTextIndexVBO)
        {
            glDeleteBuffers(1, &masterTextIndexVBO);
            masterTextIndexVBO = 0;
        }
        NvFree(masterTextIndexList);
        masterTextIndexList = NV_NULL;
		maxIndexChars = 0;
    // !!!!TBD

		// for safety, we're going to clear _everything_ here to init's
		bitFontLL = NULL;
		bitFontID = 1; // NOT ZERO, start at 1 so zero is 'invalid'...
		fontProg = 0;
		fontProgAllocInternal = 1;
		fontProgLocMat = 0;
		fontProgLocColor = 0;
		fontProgLocTex = 0;
		fontProgAttribPos = 0;
		fontProgAttribCol = 0;
		fontProgAttribTex = 0;
		dispW = 0;
		dispH = 0;
		dispAspect = 0;
		dispRotation = 0;
		dispOffX = 0;
		dispOffY = 0;
		lastColorArray = 0;
		lastFontTexture = 0;
		lastTextMode = 111;
		maxIndexChars = 0;
		masterTextIndexList = NULL;
		masterTextIndexVBO = 0;
		//s_pixelToClipMatrix[4][4];
		s_pixelScaleFactorX = 2.0f / 640.0f;
		s_pixelScaleFactorY = 2.0f / 480.0f;
		m_matrixOverride = NV_NULL;
		textIndices = NULL;
		textVertCoords = NULL;
		textTexCoords = NULL;
		maxBatchSize = 0;
		currBatchSize = 0;
    }
    
    s_bfInitialized = NV_FALSE;
}


//========================================================================
void NVBFSetScreenRes(float width, float height)
{
    dispW = width;
    dispH = height;
    dispAspect = dispW/dispH;

    s_pixelScaleFactorX = 2.0f / dispW;
    s_pixelScaleFactorY = 2.0f / dispH;
}

//========================================================================
void NVBFGetScreenRes(float *width, float *height)
{
    if (width)
        *width = dispW;
    if (height)
        *height = dispH;
}

//========================================================================
void NVBFSetScreenRot(float degrees)
{
    dispRotation = degrees;
}


//========================================================================
void NVBFSetDrawOffset(float offX, float offY)
{
    dispOffX = offX;
    dispOffY = offY;
}


//========================================================================
//========================================================================
//========================================================================

typedef struct
{
    float pos[2]; // 8 or 12 bytes, depending on how we add Z support. !!!!TBD
    float uv[2]; // 8 bytes => 16 or 20 total.
    unsigned int color; // packed 4 byte color => 20 or 24 total.  ABGR with A << 24...
} BFVert;

typedef unsigned int NvUTF32Char;

typedef struct _BFText
{
    int stringChars;
    int stringMax;
    char *string;  // !! ascii for now
    // !!!!TBD not sure how much these features will be allowed in unicode... hmmm...
    int stringCharsOut; // since string can have escape codes, need a sep count of REAL to output.
    int drawnChars; // allowing 'clamping' the number of chars to actually draw.

    BFVert *data;
    GLuint vbo;
    
    int numLines;
    int calcLinesMax; // size of buffers allocated.
    int *calcLineChars; // output char count per line
    float *calcLineWidth; // line width

    NvPackedColor charColor; // base color.  set in vertices, can override with escape codes.
    NvPackedColor multColor; // multiplied color.  set in uniform, will multiply on the fly -- AFFECTS SHADOW TOO THOUGH!

    float ppt;
    NvBool cached; // all vbo burn-in bits ready.
    NvBool visible;
    NvU8 bpad;
    NvU8 fontNum;
    float fontSize;
    NvBitFont *font;
    
    int hMode, vMode;
    float hPos, vPos; // the 0-1 coordinates
    float textLeft, textTop; // the screen coordinates

    float boxWidth; // max width before cap/wrap, zero if none.
    float boxHeight;
    int boxLines;
    
    NvBool hasBox; // do we have a box set that we are going to adjust to?
    NvBool doWrap;
    NvBool doScissor; // !!!!TBD!!!!TBD
    NvBool posCached; // position has alignment applied.
    
    NvU32 truncChar; // if a ... or other char.  zero if none.
    
    NvS8 shadowDir; // signed so we can use it as shift amount
    NvPackedColor shadowColor;
    
    float pixelsWide, pixelsHigh; // size of text.

    struct _BFText *next;
} BFText;

static BFText *bfTextLL = NULL;


//========================================================================
// !!!!TBD handle memory error on main alloc or any sub buffers.
//========================================================================
void *NVBFTextAlloc(void)
{
    BFText *bft = NULL;
    bft = new BFText;
    if (bft==NULL) return (NULL);
    // clear the structure to all ZEROS.
    NvMemset(bft, 0, sizeof(BFText));
    // hook into ll
    bft->next = bfTextLL;
    bfTextLL = bft;
    glGenBuffers(1, &(bft->vbo));
        
    // set any default state...
    bft->visible = 1; // should be visible to start!
    bft->hMode = -1;
    bft->vMode = -1;
    bft->hPos = -1111;
    bft->vPos = -1111;   
    bft->drawnChars = -1; // no clamping.
    bft->charColor = NV_PC_PREDEF_WHITE; // pure white, no alpha.
    bft->multColor = NV_PC_PREDEF_WHITE; // pure white, no alpha.
    
    return(bft);
}


//========================================================================
//========================================================================
void NVBFTextFree(void *bftin)
{
	if (!bftin)
		return;
    BFText *bft = (BFText *)bftin, *head, *follow;
    // need to walk the ll so we can remove.
    head = bfTextLL;
    follow = NULL;
    if (bft==bfTextLL) // the top of the list
    {
        bfTextLL = bfTextLL->next; // shift head up..
    }
    else
    while (head->next)
    {
        follow = head;
        head = head->next;
        if (bft==head)
        {
            follow->next = head->next; // remove from list
            break; // done.
        }
    }

    // then clean up and NvFree.
	if (bft->calcLineChars)
		NvFree(bft->calcLineChars);
	if (bft->calcLineWidth)
		NvFree(bft->calcLineWidth);
    if (bft->vbo)
        glDeleteBuffers(1, &(bft->vbo));
    if (bft->string)
        NvFree(bft->string);
    if (bft->data)
        NvFree(bft->data);

    delete bft;
}


//========================================================================
//========================================================================
void NVBFTextSetVisibility(void *bftin, NvBool vis)
{
    BFText *bft = (BFText *)bftin;
    bft->visible = vis;    
    // visibility shouldn't affect caching!
}


//========================================================================
//========================================================================
void NVBFTextSetColor(void *bftin, NvPackedColor color)
{
    BFText *bft = (BFText *)bftin;
    if (NV_PC_EQUAL(bft->charColor, color))
        return; // no change occurred.
    bft->charColor = color;
    // flag we need to recache this bftext
    bft->cached = 0;
}


//========================================================================
//========================================================================
void NVBFTextSetShadow(void *bftin, NvS8 dir, NvPackedColor color)
{
    BFText *bft = (BFText *)bftin;
    if (bft->shadowDir==dir && NV_PC_EQUAL(bft->shadowColor, color))
        return; // no change.
    bft->shadowDir = dir;
    bft->shadowColor = color;
    // flag we need to recache this bftext
    bft->cached = 0;
}


//========================================================================
//========================================================================
void NVBFTextSetMultiplyColor(void *bftin, NvPackedColor color)
{
    BFText *bft = (BFText *)bftin;
    if (NV_PC_EQUAL(bft->multColor, color))
        return; // no change occurred.
    bft->multColor = color;
    // no need to recache.
}


//========================================================================
//========================================================================
void NVBFSetDrawnChars(void *bftin, int num)
{
    BFText *bft = (BFText *)bftin;
    bft->drawnChars = num;    
//    bft->cached = 1; // flag that we cached this.
}


//========================================================================
// the cases here...
// lines is 0 for single line, no cropping.
// lines is 1 for single line, cropped.  dots non-zero to append when crop.
// lines is >1 for multiline, space wrapped, never cropped (even at final line? !!!!TBD)
//========================================================================
void NVBFTextSetBox(void *bftin, float width, float height, int lines, NvU32 dots)
{
    BFText *bft = (BFText *)bftin;
    
    if (bft->hasBox
    &&  bft->boxWidth == width
    &&  bft->boxHeight == height
    &&  bft->boxLines == lines
    &&  bft->truncChar == dots)
        return; // no changes.
    
    bft->hasBox = 1;
    bft->boxWidth = width;
    bft->boxHeight = height;
    bft->boxLines = lines;
    
    bft->doWrap = 0;
    if (lines>1)
        bft->doWrap = 1;
        
    bft->truncChar = 0;
    if (0!=dots)
        bft->truncChar = dots;       
        
    // flag we need to recache this bftext
    bft->cached = 0;
}


//========================================================================
// this is ONLY called to UPDATE an existing box.
// I have it check hasBox to ensure that's the case!
//========================================================================
void NVBFTextUpdateBox(void *bftin, float width, float height)
{
    BFText *bft = (BFText *)bftin;
    if (bft->hasBox)
    {
        if (bft->boxWidth != width
        ||  bft->boxHeight != height)
        {
            bft->boxWidth = width;
            bft->boxHeight = height;
            // flag we need to recache this bftext
            bft->cached = 0;
        }
    }
}


//========================================================================
//========================================================================
void NVBFTextSetFont(void *bftin, NvU8 fontnum)
{
    BFText *bft = (BFText *)bftin;
    if (fontnum == bft->fontNum) // same, we're done before we've even started.
        return;

    // flag we need to recache this bftext
    bft->cached = 0;

    // just cache the values.  we'll use them in the recache function.
    if (fontnum==0) // oops...
    {
        bft->fontNum = 0; // tag this.
        bft->font = NULL;
    }
    else
        bft->font = BitFontFromID(fontnum); // find our font and hold onto the ptr, for ease of coding.    
    if (bft->font!=NULL)
           bft->fontNum = fontnum;
    else // handle the case that font wasn't loaded...
    {
        // !!!!TBD ASSERT????
        bft->font = bitFontLL; // whatever the first font is.
        if (bft->font)
            bft->fontNum = bft->font->id;
        else
            bft->fontNum = 0; // try and have 0 to test later!!!
    }
       
    if (bft->fontNum==0) // something went wrong.
    {
        // what can we do? !!!!TBD
        bft->cached = 1; // so we try to do no more work!
    }   
}


//========================================================================
//========================================================================
void NVBFTextSetSize(void *bftin, float size)
{
    BFText *bft = (BFText *)bftin;
    if (size == bft->fontSize) // same, we're done before we've even started.
        return;

    // flag we need to recache this bftext
    bft->cached = 0;
    bft->fontSize = size;
}


//========================================================================
//========================================================================
void NVBFTextSetString(void *bftin, const char *str)
{
    BFText *bft = (BFText *)bftin;
    if (bft==NULL)
    {
    	ERROR_LOG("Null bitfont.");
		return;
	}
    if (str==NULL)
    	DEBUG_LOG("bitfont sent a null string ptr...");
//    if (bft->string==NULL)
//    	DEBUG_LOG("bitfont has no string allocated yet...");
    
    if (bft->stringMax) // allocated
    	if (0==strcmp(bft->string, str)) // same, we're done before we've even started.
    		return;

    // flag we need to recache this bftext
    bft->cached = 0;
    // clear other computed values.
    bft->pixelsWide = 0;
    bft->pixelsHigh = 0;

    // check that we have storage enough for the string, and for calc'd data
    // then, check that we have enough space in our bftext-local storage...
    // !!!!TBD GetNvU8Length isn't definitively what I want, I don't think... might be multi-word chars in there.
    bft->stringChars = strlen(str);
    NvS32 charsToAlloc = 2*(bft->stringChars+1); // doubled for shadow text... !!!!TBD remove if we do shadow in shader.
    if (charsToAlloc > bft->stringMax-1) // need to account for null termination and for doubled shadow text
    {
        if (bft->stringMax) // allocated, NvFree structs.
        {
            NvFree(bft->string);
            NvFree(bft->data);
        }
        // reset max to base chars padded to 16 boundary, PLUS another 16 (8+8) for minor growth.
        bft->stringMax = charsToAlloc + 16-((charsToAlloc)%16) + 16;
        bft->string = (char*)NvMalloc(bft->stringMax*sizeof(char)); // !!!!TBD should use TCHAR size here??
        NvMemset(bft->string, 0, bft->stringMax*sizeof(char));
        bft->data = (BFVert*)NvMalloc(bft->stringMax*sizeof(BFVert)*VERT_PER_QUAD);
    }

    memcpy(bft->string, str, bft->stringChars+1); // include the null.
}

//========================================================================
// !!!!TBD to remove?
//========================================================================
void NVBFTextGetString(void *bftin, char* str)
{
    BFText *bft = (BFText *)bftin;
    strcpy(str, (const char*)bft->string);
}


//========================================================================
//========================================================================
static int UpdateMasterIndexBuffer(int stringMax, NvBool internalCall)
{
	if (stringMax>maxIndexChars) // reallocate...
	{
		if (maxIndexChars) // delete first..
			NvFree(masterTextIndexList);
	
		maxIndexChars = stringMax; // easy solution, keep these aligned!
		int n = sizeof(short) * IND_PER_QUAD * maxIndexChars;
		masterTextIndexList = (short*)NvMalloc(n);
		if (masterTextIndexList==NULL)
			return -1;
	
		// re-init the index buffer.
		for (int c=0; c<maxIndexChars; c++) // triangle list indices... three per triangle, six per quad.
		{
			masterTextIndexList[c*6+0] = (short)(c*4+0);
			masterTextIndexList[c*6+1] = (short)(c*4+2);
			masterTextIndexList[c*6+2] = (short)(c*4+1);
			masterTextIndexList[c*6+3] = (short)(c*4+0);
			masterTextIndexList[c*6+4] = (short)(c*4+3);
			masterTextIndexList[c*6+5] = (short)(c*4+2);
		}
	
		if (!internalCall)
			NvFilterBindBuffer(GL_ELEMENT_ARRAY_BUFFER, masterTextIndexVBO);
		NvFilterBufferData(GL_ELEMENT_ARRAY_BUFFER, n, masterTextIndexList, GL_STATIC_DRAW);
		if (!internalCall)
			NvFilterBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // !!!!TBD resetting here, if we're INSIDE the render call, is wasteful... !!!!TBD
	}
	
	return 0;
}


//========================================================================
//========================================================================
static void AdjustGlyphsForAlignment(const BFText *bft)
{
    if (bft->hMode==NVBF_ALIGN_LEFT)
    	return; // nothing to do.

	DEBUG_LOG("Adjusting glyphs for alignment...");
    const bool center = (bft->hMode==NVBF_ALIGN_CENTER);
    BFVert *vp = bft->data;
    // loop over the lines in this bftext...
	for (int i=0; i<bft->numLines; i++)
	{
		const int max = bft->calcLineChars[i];
		DEBUG_LOG("line %d, char count %d.", i, max);
		float w = bft->calcLineWidth[i];
		if (center)
			w *= 0.5f;
		// loop through chars on this line, shifting X coords
		// note char count covers shadow glyphs properly.
		for (int j=0; j<max; j++)
		{
			for (int k=0; k<4; k++)
			{
				vp->pos[0] -= w; // shift back by half or full linewidth.
				vp++; // next vertex;
			}
		}
	}
}


//========================================================================
//========================================================================
static void AddOutputGlyph(
                            NvU32 vc,
                            const NvBitFont *bitfont,
                            BFVert **vp, // so explicit we're modding it!
                            float *left,
                            float t,
                            float b,
                            float hsizepertex,
                            NvPackedColor color )
{                    
    // generate vertex positions
    float l = (*left) + (bitfont->abc[vc].a * hsizepertex); // start offset
    float r = l + (bitfont->abc[vc].b * hsizepertex); // glyph width

    // then, update the offset based on the total ABC width (which for a monospaced font should == fontCell size...)
    //left += fullglyphwidth;
    *left = r + (bitfont->abc[vc].c * hsizepertex); // right pad
    
    // generate texture offsets/coords
    // match ordering!
//        vp->pos[2] = 0; // !!!TBD z positioning?
    (*vp)->pos[0] = l;
    (*vp)->pos[1] = b;
    (*vp)->uv[0] = bitfont->rect[vc].left;
    (*vp)->uv[1] = bitfont->rect[vc].bottom;
    (*vp)->color = NV_PC_PACK_UINT(color);
    (*vp)++;
    (*vp)->pos[0] = l;
    (*vp)->pos[1] = t;
    (*vp)->uv[0] = bitfont->rect[vc].left;
    (*vp)->uv[1] = bitfont->rect[vc].top;
    (*vp)->color = NV_PC_PACK_UINT(color);
    (*vp)++;
    (*vp)->pos[0] = r;
    (*vp)->pos[1] = t;
    (*vp)->uv[0] = bitfont->rect[vc].right;
    (*vp)->uv[1] = bitfont->rect[vc].top;
    (*vp)->color = NV_PC_PACK_UINT(color);
    (*vp)++;
    (*vp)->pos[0] = r;
    (*vp)->pos[1] = b;
    (*vp)->uv[0] = bitfont->rect[vc].right;
    (*vp)->uv[1] = bitfont->rect[vc].bottom;
    (*vp)->color = NV_PC_PACK_UINT(color);
    (*vp)++;

//    bft->stringCharsOut++;
}


//========================================================================
//========================================================================
static NvU32 CharToVirtualIndex(const NvBitFont *bitfont, NvBftStyle bfs, NvU32 inChar)
{
	NvU32 vc = inChar;
    if (bitfont->isSplit) // okay, hack time.
    { // we only have a few choices.  shift the entire range, lose off the end, shift part, lose off the middle.
    	vc -= 32; // shift back 32 chars. // we skip 32 at end of first set, to get second set w/o translation
//      if (vc<128) // gets us through alpha, past tilde of first set
//          vc -= 32; // shift back 32 chars. // we skip 32 at end of first set, to get second set w/o translation
//      else // tricky... shift into second group properly for shared stuff.
//          vc += 96;
        if (bfs==NVBF_STYLECODE_BOLD) // we assume bold or ital is second half -- only get one dual-font right now !!!!TBD
        	vc += 128;
    }
    
    return vc;
}


//========================================================================
//========================================================================
static void TrackOutputLines(BFText *bft, float lineWidth)
{
	const int charCount = bft->stringCharsOut; // it's in bf, no need to pass.
	
	// then we want to track lines so we can align
	if (bft->calcLinesMax==0)
	{
		bft->calcLinesMax = 8; // something that seems decent for single lines
							// or small boxes, we'll grow later.
		bft->calcLineChars = (int *)NvMalloc(sizeof(int) * bft->calcLinesMax);
		if (bft->calcLineChars==NULL)
			return; // TODO FIXME
		bft->calcLineWidth = (float *)NvMalloc(sizeof(float) * bft->calcLinesMax);
		if (bft->calcLineWidth==NULL)
			return; // TODO FIXME
	}
	
	if (bft->numLines > bft->calcLinesMax)
	{ // then resizing.
		int newMax = (bft->calcLinesMax*3)/2; // add 50% of current.  reasonable.
		int *newLineChars = (int *)NvMalloc(sizeof(int) * newMax);
		if (newLineChars==NULL)
			return; // TODO FIXME
		float *newLineWidth = (float *)NvMalloc(sizeof(float) * newMax);
		if (newLineWidth==NULL)
			return; // TODO FIXME\
		// copy the line data.
		for (int i=0; i < bft->calcLinesMax; i++)
		{
			newLineChars[i] = bft->calcLineChars[i];
			newLineWidth[i] = bft->calcLineWidth[i];
		}
		// free old data
		NvFree(bft->calcLineChars);
		NvFree(bft->calcLineWidth);
		// swap in new data.
		bft->calcLineChars = newLineChars;
		bft->calcLineWidth = newLineWidth;
		bft->calcLinesMax = newMax;
	}
	
	// NOW, we actually get around to the tracking. :)
	int lineNum = 0;
	int prevChars = 0;
	if (bft->numLines>0)
		lineNum = bft->numLines - 1;
	for (int i=0; i<lineNum; i++)
		prevChars += bft->calcLineChars[i];
	bft->calcLineChars[lineNum] = charCount-prevChars;
	bft->calcLineWidth[lineNum] = lineWidth;
}

//========================================================================
// this function rebuilds the VBO/rendercache based on a simplistic
// ENGLISH char-walk of the string.
// !!!!TBD handle the actual unicode chars we might get properly
// !!!!TBD handle complex script layouts and break rules of non-roman lang
//========================================================================
void NVBFTextRebuildCache(void *bftin, NvBool internalCall)
{
    NvBftStyle bfs = NVBF_STYLECODE_NORMAL;
    BFText *bft = (BFText *)bftin;
    float vsize, hsize, hsizepertex, fullglyphwidth;
    float left, t, b;
    //float l,r;
    int n,j,c;
    BFVert *vp, *lastvp;
    GLuint vbmode = GL_STATIC_DRAW /*GL_DYNAMIC_DRAW*/;
    const NvBitFont *bitfont = bft->font;
    NvPackedColor color;
    int linesign = 1;
    int lineheightdelta = 0; // !!!!TBD
    int lastlinestart = 0, lastwhitespacein = 0, lastwhitespaceout = 0; // so we can reset positions for wrappable text.
    float lastwhitespaceleft = 0;
    NvU32 realcharindex;
    float extrawrapmargin = 0;

    if (bft->cached) // then no work to do here, move along.
        return;
    if (!bft->fontNum)
        return;
    if (!bitfont)
    	return;

    // first, check that our master index buffer is big enough.
    if (UpdateMasterIndexBuffer(bft->stringMax, internalCall))
    	return; // TODO FIXME error output/handling.

    // recalc size in terms of the screen res...
    j = bft->fontNum-1; // fontnum is 1-based, other stuff is 0-based
    vsize = bft->fontSize;// *(high/((dispAspect<1)?640.0f:480.0f)); // need the texel-factor for the texture at the end...
    hsize = vsize;
    hsizepertex = hsize / bft->font->cell;

    // calc extra margin for wraps...
    if (bft->hasBox && bft->truncChar)
    {
        // calculate the approx truncChar size needed.  Note we don't have
    	// style info at this point, so this could be off by a bunch.  !!!!TBD FIXME
        const NvU32 tvc = CharToVirtualIndex(bitfont, NVBF_STYLECODE_NORMAL, bft->truncChar);
        extrawrapmargin = (bitfont->abc[tvc].a + bitfont->abc[tvc].b + bitfont->abc[tvc].c) * hsizepertex;
        extrawrapmargin *= 3; // for ...
    }
   
    // loop over the string, building up the output chars...
    left = 0;
	int maxWidth = 0;
    t = 0;
    b = t + (linesign * vsize);
    vp = bft->data;
    color = bft->charColor; // default to set color;
    bft->stringCharsOut = 0;
    
    lastvp = vp;
    lastlinestart = 0;
    lastwhitespacein = 0;
    lastwhitespaceout = 0;
    n=0;
    bft->numLines = 1;
    while (n<bft->stringChars)
    {
        // !!!!TBD THIS ISN'T UNICODE-READY!!!!
        realcharindex = (NvU32)(bft->string[n]); // need to cast unsigned for accented chars...        
        if (realcharindex==0) // null.  done.
            break;

        if ((realcharindex=='\n') //==0x0A == linefeed.
        ||  (realcharindex=='\r')) //==0x0D == return.
        {
            if (bft->hasBox && (bft->boxLines > 0) &&
            		((bft->numLines + 1) > bft->boxLines)) 
				break; // exceeded line cap, break from cache-chars loop.
            n++;
            TrackOutputLines(bft, left);
            lastlinestart = n; // where we broke and restarted.
            lastwhitespacein = n; // so we can rollback input position..
            lastwhitespaceout = bft->stringCharsOut; // so we can reset output position.
            lastvp = vp; // so we can reset output position.
            bft->numLines++; // count lines!

            t = b + lineheightdelta; // move to next line.
            b = t + (linesign * vsize); // proper calc for bottom.
            left = 0; // reset to left edge.

            lastwhitespaceleft = 0; // on return, reset

            continue; // loop again.
        }

        // !!!!!TBD handling of unicode/multibyte at some point.
        if (realcharindex < 0x20) // embedded commands under 0x20, color table code under 0x10...
        {
            // first check any chars we want excluded!
            if (realcharindex=='\t')
            {
                // do nothing here.  fall through, forward into processing.
            }
            else
            {
                if (realcharindex < 0x10) // color table index
                { // colorcodes are 1-based, table indices are 0-based
                    if (NVBF_COLORCODE_MAX == realcharindex-1)
                        color = bft->charColor; // default to set color;
                    else
                        color = s_charColorTable[realcharindex-1];
                }
                else // escape codes
                {
                    if (realcharindex < NVBF_STYLECODE_MAX)
                        bfs = (NvBftStyle)realcharindex;
                }
                n++; // now proceed to next char.
                continue; // loop again.
            }
        }
        
        // okay, now shift to virtual index, as it will be different from input.
        const NvU32 vc = CharToVirtualIndex(bitfont, bfs, realcharindex);

        // precalc the full glyph spacing, to optimize some of this processing.
        fullglyphwidth = (bitfont->abc[vc].a + bitfont->abc[vc].b + bitfont->abc[vc].c) * hsizepertex;

        if (realcharindex==' ' || realcharindex=='\t') // hmmm, optimization to skip space/tab characters, since we encode the 'space' into the position.
        {
            // then, update the offset based on the total ABC width (which for a monospaced font should == fontCell size...)
            lastwhitespaceleft = left;
            left += fullglyphwidth;
            n++; // now proceed to next char.
            if (lastwhitespacein!=n-1) // then cache state
            {
                lastwhitespacein = n; // so we can rollback input position..
                lastwhitespaceout = bft->stringCharsOut; // so we can reset output position.
                lastvp = vp; // so we can reset output position.
            }
            // one more check+update
            if (lastwhitespacein==lastlinestart+1) { // was first char of our new line, reset linestart num
                lastlinestart = n;
			}
            continue; // loop again.
        }
        
        // !!!!!TBD linewrap... should actually look ahead for space/lf, OR
        // needs to do a 'roll back' when it hits edge... but for extra long strings,
        // we need to realize we don't have a whitespace character we can pop back to,
        // and need to just immed character wrap.
        
        // check to see if we'd go off the 'right' edge (with spacing...)
        if ( bft->hasBox && ((left + fullglyphwidth) > (bft->boxWidth - extrawrapmargin)) )
        {
            if (!bft->doWrap) // then character truncate!
            {
            }
            else // word wrapping, jump back IF it's sane to.
            if (lastwhitespacein!=lastlinestart)
            {
                n = lastwhitespacein; // go back some chars.
                bft->stringCharsOut = lastwhitespaceout; // undo output buffering.
                vp = lastvp; // undo output buffering.
                left = lastwhitespaceleft; // undo word positioning.
            }
            else
            {
                // !!!!TBD some update to outputs here.
                // not resetting n, sco, or vp... keep going forward from HERE.
            }
            
            TrackOutputLines(bft, left);
            lastlinestart = n; // where we broke and restarted.
            lastwhitespacein = n; // so we can rollback input position..
            lastwhitespaceout = bft->stringCharsOut; // so we can reset output position.
            lastvp = vp; // so we can reset output position.
            bft->numLines++; // count lines!

            // !!!!TBD truncChar handling...
            // !!!!TBD how to insert truncChar's into output stream.  need sub-function to add char.
            
            if ((bft->boxLines > 0) && (bft->numLines > bft->boxLines)) // FIXME lines+1????
            {
                if (bft->truncChar)
                {
                    int i;
                    const NvU32 tvc = CharToVirtualIndex(bitfont, bfs, bft->truncChar);
                    if (bft->doWrap) // if wrapping, shift to ... position.
                        left = lastwhitespaceleft;
                    for (i=0; i<3; i++) // for ellipses style
                    {
                        if (bft->shadowDir)
                        {
                            float soff = bft->shadowDir * hsizepertex;
                            float tmpleft = left+soff; // so we don't really change position.
                            AddOutputGlyph( tvc, bitfont, &vp, &tmpleft, t+soff, b+soff, hsizepertex, bft->shadowColor );
                            bft->stringCharsOut++; // update number of output chars.
                        }        
                        AddOutputGlyph( tvc, bitfont, &vp, &left, t, b, hsizepertex, color );
                        bft->stringCharsOut++; // update number of output chars.
                    }
                    
                    // update char count and line width since we're going to break out.
                    TrackOutputLines(bft, left);
                }
                break; // out of the output loop, we're done.
            }
           
            // if doing another line, reset variables.
            t = b + lineheightdelta; // move to next line.
            b = t + (linesign * vsize); // proper calc for bottom.
			if (maxWidth < left)
				maxWidth = left;
            left = 0; // reset to left edge.
            lastwhitespaceleft = 0; // on return, reset
            
            continue; // restart this based on new value of n!
        }

        if (bft->shadowDir)
        {
            float soff = bft->shadowDir * hsizepertex;
            float tmpleft = left+soff; // so we don't really change position.
            AddOutputGlyph( vc, bitfont, &vp, &tmpleft, t+soff, b+soff, hsizepertex, bft->shadowColor );
            bft->stringCharsOut++; // update number of output chars.
        }        
        AddOutputGlyph( vc, bitfont, &vp, &left, t, b, hsizepertex, color );
        bft->stringCharsOut++; // update number of output chars.
        
        // now proceed to next char.
        n++; 
    }

    TrackOutputLines(bft, left);

    for (int i=0; i<bft->numLines; i++)
		if (maxWidth < bft->calcLineWidth[i])
			maxWidth = bft->calcLineWidth[i];

    // if alignment is not left, we shift each line based on linewidth.
    AdjustGlyphsForAlignment(bft);
    
	//DEBUG_LOG(">> output glyph count = %d, stringMax = %d.", bft->stringCharsOut, bft->stringMax);
	
    if (!internalCall)
        NvFilterBindBuffer(GL_ARRAY_BUFFER, bft->vbo);
    NvFilterBufferData(GL_ARRAY_BUFFER, bft->stringCharsOut*sizeof(BFVert)*VERT_PER_QUAD, bft->data, vbmode);
    if (!internalCall)
        NvFilterBindBuffer(GL_ARRAY_BUFFER, 0); // !!!!TBD resetting here, if we're INSIDE the render call, is wasteful... !!!!TBD

    bft->ppt = hsizepertex; // cache this result for rendering shadows, other offsets.
    bft->pixelsWide = maxWidth; // cache the total width in output pixels, for justification and such.
    bft->pixelsHigh = vsize * bft->numLines;
    bft->cached = 1; // flag that we cached this.
    bft->posCached = 0; // flag that position needs recache.  FIXME could optimize...
}


//========================================================================
float NVBFTextGetWidth(void *bftin)
{
    BFText *bft = (BFText *)bftin;
    return(bft->pixelsWide);
}


//========================================================================
float NVBFTextGetHeight(void *bftin)
{
    BFText *bft = (BFText *)bftin;
    return(bft->pixelsHigh);
}


#define SAVED_ATTRIBS_MAX   16 /* something for now */

typedef struct
{
    GLboolean enabled;
    GLint size;
    GLint stride;
    GLint type;
    GLint norm;
    GLvoid *ptr;    
} NVBFGLAttribInfo;

typedef struct
{
    GLint               programBound;
    NVBFGLAttribInfo    attrib[SAVED_ATTRIBS_MAX];

    GLboolean           depthMaskEnabled;    
    GLboolean           depthTestEnabled;
    GLboolean           cullFaceEnabled;
    GLboolean           blendEnabled;
    //gStateBlock.blendFunc // !!!!TBD
    //blendFuncSep // tbd
    
    GLint               vboBound;
    GLint               iboBound;
    GLint               texBound;
    GLint               texActive;
    
    // stencil?  viewport? // !!!!TBD
    
} NVBFGLStateBlock;

static NVBFGLStateBlock gStateBlock;
static bool gSaveRestoreState = false; // default was true, let's try false.

//========================================================================
void NVBFSaveGLState()
{
    int i;
    int tmpi;
    
    TestPrintGLError("Error 0x%x in SaveState @ start...\n");

    glGetIntegerv(GL_CURRENT_PROGRAM, &(gStateBlock.programBound));
    for (i=0; i<SAVED_ATTRIBS_MAX; i++)
    {    
        NvFilterGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &tmpi);
        gStateBlock.attrib[i].enabled = (GLboolean)tmpi;
        if (tmpi)
        {
            NvFilterGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_SIZE, &(gStateBlock.attrib[i].size));
            NvFilterGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_STRIDE, &(gStateBlock.attrib[i].stride));
            NvFilterGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_TYPE, &(gStateBlock.attrib[i].type));
            NvFilterGetVertexAttribiv(i, GL_VERTEX_ATTRIB_ARRAY_NORMALIZED, &(gStateBlock.attrib[i].norm));
            NvFilterGetVertexAttribPointerv(i, GL_VERTEX_ATTRIB_ARRAY_POINTER, &(gStateBlock.attrib[i].ptr));
       }
    }    

    glGetBooleanv(GL_DEPTH_WRITEMASK, &(gStateBlock.depthMaskEnabled));
    gStateBlock.depthTestEnabled = glIsEnabled(GL_DEPTH_TEST);
    gStateBlock.blendEnabled = NvFilterIsEnabled(GL_BLEND);
    gStateBlock.cullFaceEnabled = glIsEnabled(GL_CULL_FACE);
//    glGetIntegerv(GL_CULL_FACE_MODE, &(gStateBlock.cullMode));

    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &(gStateBlock.vboBound));
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &(gStateBlock.iboBound));
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &(gStateBlock.texBound));
    glGetIntegerv(GL_ACTIVE_TEXTURE, &(gStateBlock.texActive));

    TestPrintGLError("Error 0x%x in SaveState @ end...\n");
}

//========================================================================
void NVBFRestoreGLState()
{
    int i;
    
    nv_flush_tracked_attribs(); // turn ours off...

    TestPrintGLError("Error 0x%x in RestoreState @ start...\n");

    NvFilterUseProgram(gStateBlock.programBound);

    // set buffers first, in case attribs bound to them...
    NvFilterBindBuffer(GL_ARRAY_BUFFER, gStateBlock.vboBound);
    NvFilterBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gStateBlock.iboBound);

    if (gStateBlock.programBound)
    { // restore program stuff..
        for (i=0; i<SAVED_ATTRIBS_MAX; i++)
        {    
            if (gStateBlock.attrib[i].enabled) // only restore enabled ones.. ;)
            {
                nv_set_attrib_by_index(i,
                                    gStateBlock.attrib[i].size,
                                    gStateBlock.attrib[i].type,
                                    (GLboolean)(gStateBlock.attrib[i].norm),
                                    gStateBlock.attrib[i].stride,
                                    gStateBlock.attrib[i].ptr);
           }
        }    
    }
    
    if (gStateBlock.depthMaskEnabled)
        glDepthMask(GL_TRUE); // we turned off.
    if (gStateBlock.depthTestEnabled)
        glEnable(GL_DEPTH_TEST); // we turned off.
    if (!gStateBlock.blendEnabled)
        NvFilterDisable(GL_BLEND); // we turned ON.
    if (gStateBlock.cullFaceEnabled)
        glEnable(GL_CULL_FACE); // we turned off.
//    glGetIntegerv(GL_CULL_FACE_MODE, &(gStateBlock.cullMode));

    // restore tex BEFORE switching active state...
    glBindTexture(GL_TEXTURE_2D, gStateBlock.texBound);
    if (gStateBlock.texActive != GL_TEXTURE0)
        glActiveTexture(gStateBlock.texActive); // we set to 0

    TestPrintGLError("Error 0x%x in RestoreState @ end...\n");
}


//========================================================================
void NVBFTextRenderPrep()
{   
    if (gSaveRestoreState)
        NVBFSaveGLState();

    NvFilterUseProgram(fontProg);

    // set up master rendering state
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    
    // blending...
    NvFilterEnable(GL_BLEND);
    
    // texture to base.
    glActiveTexture(GL_TEXTURE0);
    // do we need to loop over TUs and disable any we're not using!?!?! !!!!TBD

    // any master buffer...
    NvFilterBindBuffer(GL_ELEMENT_ARRAY_BUFFER, masterTextIndexVBO);

    TestPrintGLError("Error 0x%x in NVBFTextRenderPrep...\n");

    // reset state caching!
    lastFontTexture = 0;
    lastTextMode = 111; // won't ever match... ;)
}


//========================================================================
//========================================================================
void NVBFTextRenderDone()
{
    if (gSaveRestoreState)
        NVBFRestoreGLState();
    else
    {
        glBindTexture( GL_TEXTURE_2D, 0 );
        lastFontTexture = 0;

        NvFilterBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        NvFilterBindBuffer(GL_ARRAY_BUFFER, 0);
        
        nv_flush_tracked_attribs(); // clear any attrib binds.
        
        NvFilterUseProgram((GLuint)0);
    }
    
    NVBFTextMatrix(NV_NULL); // explicitly clear each Done.
}


//========================================================================
// 0==top/left, 1==bottom/right, 2==center/center
//========================================================================
void NVBFTextCursorAlign(void *bftin, NvU8 h, NvU8 v)
{
    BFText *bft = (BFText *)bftin;
    if (bft->vMode!=v || bft->hMode!=h) // reset
    {
		bft->vMode = v;
		bft->hMode = h;
		bft->posCached = false; // FIXME for now, force recalc.
    }
}

//========================================================================
// in default mode, top/left offset.
//========================================================================
void NVBFTextCursorPos(void *bftin, float h, float v)
{
    BFText *bft = (BFText *)bftin;
    
    if (bft->hPos!=h || bft->vPos!=v)
    {
    	bft->hPos = h;
		bft->vPos = v;    
		bft->posCached = false; // we need to adjust
//    bft->hModeVBO = -1;
    }
}


//========================================================================
//========================================================================
void NVBFTextMatrix(const GLfloat *mtx)
{
    m_matrixOverride = mtx;
    if (m_matrixOverride!=NV_NULL)
        glUniformMatrix4fv(fontProgLocMat, 1, GL_FALSE, m_matrixOverride);
}


//========================================================================
//========================================================================
void UpdateTextPosition(BFText *bft)
{
	if (bft->posCached) return;
	
    bft->textLeft = bft->hPos;
    bft->textTop = bft->vPos;
    
    // horizontal adjustments
    if (bft->hasBox)
    {
		if (bft->hMode==NVBF_ALIGN_CENTER)
			bft->textLeft += (bft->boxWidth*0.5f); // align box to point, center content within.
		else if (bft->hMode==NVBF_ALIGN_RIGHT)
			bft->textLeft += bft->boxWidth; // align to right of positioned box.
    }

    // vertical adjustments
    if (bft->vMode==NVBF_ALIGN_CENTER)
    {
    	if (bft->hasBox)
    		bft->textTop += (bft->boxHeight*0.5f); // shift down half box size to center
    	bft->textTop -= (bft->pixelsHigh*0.5f); // shift up half text height to center
    }
    else if (bft->vMode==NVBF_ALIGN_BOTTOM)
    {
    	if (bft->hasBox)
    		bft->textTop += bft->boxHeight; // shift down by box height
    	bft->textTop -= bft->pixelsHigh; // shift up by text height
    }
    
    bft->posCached = 1;
}


//========================================================================
//========================================================================
void NVBFTextRender(void *bftin)
{
    BFText *bft = (BFText *)bftin;
    NvBitFont *bitfont = bft->font;
    void *p = 0;
    int count = bft->drawnChars;

    if (!bft->visible || !bft->fontNum || !bitfont) // nothing we should output...
        return;

    if (count<0)
        count = bft->stringChars; // !!!TBD maybe negative means something else.
    else
    if (count>bft->stringChars)
        count = bft->stringChars;
    if (count==0)
        return; // done...
    if (bft->shadowDir)
        count *= 2; // so we draw char+shadow equally...

    // set up master rendering state
    {
        NvU8 *offset = NULL;
        NvFilterBindBuffer(GL_ARRAY_BUFFER, bft->vbo);

        nv_set_attrib_by_index(fontProgAttribPos, 2, GL_FLOAT, 0, sizeof(BFVert), (void *)offset);
        offset += sizeof(float) * 2; // jump ahead the two floats
        nv_set_attrib_by_index(fontProgAttribTex, 2, GL_FLOAT, 0, sizeof(BFVert), (void *)offset); // !!!!TBD update this to use a var if we do 2 or 3 pos verts...
        offset += sizeof(float) * 2; // jump ahead the two floats.
        nv_set_attrib_by_index(fontProgAttribCol, 4, GL_UNSIGNED_BYTE, 1, sizeof(BFVert), (void *)offset); // !!!!TBD update this to use a var if we do 2 or 3 pos verts...
        offset += sizeof(GLuint);
    }

    // since buffer state is now set, can rebuild cache now without extra calls.
    if (!bft->cached) // need to recache BEFORE we do anything using textwidth, etc.
        NVBFTextRebuildCache(bftin, 1);
    if (count > bft->stringCharsOut) // recheck count against CharsOut after rebuilding cache
        count = bft->stringCharsOut;
    if (!bft->posCached) // AFTER we may have rebuilt the cache, we check if we recalc pos.
    	UpdateTextPosition(bft);

    // set the model matrix offset for rendering this text based on position & alignment
    // first, do any pre-render position updates

    // we apply any global screen orientation/rotation so long as
    // caller hasn't specified their own transform matrix.
    if (m_matrixOverride==NV_NULL)
    {
        float rad = (float)(3.14159f/180.0f);  // deg->rad
        float cosfv;
        float sinfv;
        const float wNorm = s_pixelScaleFactorX;
        const float hNorm = s_pixelScaleFactorY;

        rad = (dispRotation * rad); // [-1,2]=>[-90,180] in radians...
        cosfv = (float)cos(rad);
        sinfv = (float)sin(rad);

        s_pixelToClipMatrix[0][0] = wNorm * cosfv;
        s_pixelToClipMatrix[1][0] = hNorm * sinfv;
        s_pixelToClipMatrix[0][1] = wNorm * sinfv;
        s_pixelToClipMatrix[1][1] = hNorm * -cosfv;

        s_pixelToClipMatrix[3][0] = wNorm * (dispOffX + bft->textLeft) * cosfv
                                    - cosfv - sinfv
                                  + hNorm * (dispOffY + bft->textTop)  * sinfv;
        s_pixelToClipMatrix[3][1] = wNorm * (dispOffX + bft->textLeft) * sinfv
                                    - sinfv + cosfv
                                  - hNorm * (dispOffY + bft->textTop)  * cosfv;

        glUniformMatrix4fv(fontProgLocMat, 1, GL_FALSE, &(s_pixelToClipMatrix[0][0]));
    }

    // bind texture... now with simplistic state caching
    if (lastFontTexture != bitfont->tex)
    {
        glBindTexture( GL_TEXTURE_2D, bitfont->tex );
        lastFontTexture = bitfont->tex;
    }

    // now, switch blend mode to work for our luma-based text texture.
    if (bitfont->alpha)
    {
        // We need to have the alpha make the destination alpha
        // so that text doesn't "cut through" existing opaque
        // destination alpha
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
            GL_ONE, GL_ONE);
        // glBlendAmout(0.5??) !!!!!TBD
    }

    glUniform4f(fontProgLocColor, 
            ((NvF32)NV_PC_RED(bft->multColor))/255.0f,
            ((NvF32)NV_PC_GREEN(bft->multColor))/255.0f,
            ((NvF32)NV_PC_BLUE(bft->multColor))/255.0f,
            ((NvF32)NV_PC_ALPHA(bft->multColor))/255.0f);

    // draw it already!
    //DEBUG_LOG("printing %d chars...", count);
    glDrawElements(GL_TRIANGLES, IND_PER_QUAD * count, GL_UNSIGNED_SHORT, p);

    TestPrintGLError("Error 0x%x NVBFTextRender drawels...\n");
}
