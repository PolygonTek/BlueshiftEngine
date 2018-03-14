//----------------------------------------------------------------------------------
// File:        jni/nv_bitfont/nv_bitfont.h
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

#ifndef _nvbitfont_h_
#define _nvbitfont_h_

#include <nv_global.h>

#include <nv_color.h>

// fwd decls...
//class NvString;

/** @file nv_bitfont.h
    Library for rendering text strings under GLES, using bitmap fonts.

    BitFont is able to:
    <ul>
        <li> Load multiple bitmap fonts simultaneously, from DDS files, with or without mipmaps </li>
        <li> Handle full 8-bit extended ASCII character codes, or 7-bit restricted ASCII, depending on font file(s) </li>
        <li> Manage 'split' fonts, where top and bottom 'halves' of bitmap are different typefaces or styles </li>
        <li> Allow overriding the default shader to implement custom raster effects </li>
        <li> Allow overriding the default matrix calc to apply custom transformations to text </li>
        <li> Render text aligned to screen edges or any sub-rect 'box' on the screen </li>
        <li> Support multi-line output, including auto-wrapping </li>
        <li> Apply a specific font, size, base color, and multiplied color to each string </li>
        <li> Optionally apply shadowing under the text for more 'pop' </li>
        <li> Optionally embed special escape codes for on-the-fly color changes or style changes </li>
        <li> Given screen size (and rotation), can automatically re-scale and rotate text output </li>
    </ul>
 */


/** Alignment modes for text output.

    BitFont supports standard 'cell' alignment, either to the screen bounds
    or to a specified 'box'.  Note that the center-alignment enumerant is used
    for both horizontal and vertical alignments.
 */
typedef enum
{
    NVBF_ALIGN_LEFT=0, /**< [Default] Left aligned */
    NVBF_ALIGN_RIGHT=1, /**< Right aligned */
    NVBF_ALIGN_CENTER=2, /**< Center aligned */

    NVBF_ALIGN_TOP=0, /**< [Default] Top aligned */
    NVBF_ALIGN_BOTTOM=1 /**< Bottom aligned */
} NvBftAlignment;

/** BitFont supports 'styles' via embedded character codes and string literals.

    You can directly use the following as character values for switching
    between the top and bottom 'halves' of a 'split' font files.  This is
    currently used as 'normal' and 'bold' styles, but the two halves could
    actually be set up as different typeface families.  There are also matching
    string literals for equivalent use directly in C quoted string composition.
    @see NVBF_STYLESTR_NORMAL
    @see NVBF_STYLESTR_BOLD
 */
typedef enum
{
    NVBF_STYLECODE_NORMAL = 0x10, /**< Sets further text to normal style, top half of split texture. */
    NVBF_STYLECODE_BOLD, /**< Sets further text to bold style, bottom half of split texture. */
//    NVBFStyleItalic, // !!!!TBD NYI
//    NVBFStyleBoldItalic, // !!!!TBD NYI
    NVBF_STYLECODE_MAX  /**< Used for programmatic range checking of embedded codes. */
} NvBftStyle;

/** @name Text Style String Literals
*/
/* @{ */
/** BitFont string literal for style reset to 'normal'. */
#define NVBF_STYLESTR_NORMAL     "\020"
/** BitFont string literal to style further characters 'bold'. */
#define NVBF_STYLESTR_BOLD       "\021"
//#define NVBFSTYLEITALIC     "\022"
//#define NVBFSTYLEBOLDITALIC "\023"
/** @see bftStyle */
/* @} */


/** BitFont supports specific color constants embedded in a string.

    You can directly use the following as character values for switching
    color 'runs' in the text.  Note that these embedded color changes will
    completely override the base color specified for a given string.

    There are also string literals for use directly in C quoted string composition.
    @see NVBF_COLORSTR_WHITE
    @see NVBF_COLORSTR_BLACK
 */
typedef enum
{
    NVBF_COLORCODE_WHITE = 0, /**< Sets further text to be white. */
    NVBF_COLORCODE_GRAY, /**< Sets further text to be gray. */
    NVBF_COLORCODE_BLACK, /**< Sets further text to be black. */
    NVBF_COLORCODE_RED, /**< Sets further text to be red. */
    NVBF_COLORCODE_GREEN, /**< Sets further text to be green. */
    NVBF_COLORCODE_BLUE, /**< Sets further text to be blue. */
    NVBF_COLORCODE_MAX /**< Used for programmatic range checking of embedded codes. */
} NvBftColorCode;

/** @name Text Color String Literals
*/
/* @{ */
/** Embedded string literal to change text coloring to white. */
#define NVBF_COLORSTR_WHITE     "\001"
/** Embedded string literal to change text coloring to gray. */
#define NVBF_COLORSTR_GRAY      "\002"
/** Embedded string literal to change text coloring to black. */
#define NVBF_COLORSTR_BLACK     "\003"
/** Embedded string literal to change text coloring to red. */
#define NVBF_COLORSTR_RED       "\004"
/** Embedded string literal to change text coloring to green. */
#define NVBF_COLORSTR_GREEN     "\005"
/** Embedded string literal to change text coloring to blue. */
#define NVBF_COLORSTR_BLUE      "\006"
/** Embedded string literal to restore text coloring to 'normal'. should be 'max' value. */
#define NVBF_COLORSTR_NORMAL    "\007"
/* @} */

/** Helper type to abstract use of unsigned bytes. */
//typedef NvU8 bfuchar; // !!!!!TBD this may be removed?

/** @name BitFont System Creation & Global Property Accessors

    The high level functions for initializing and preparing the global
    state of the BitFont system on a per-app basis.
*/
/* @{ */

/** Base initialization of the font system, once per application.

    Initialize the BitFont system with one or more on-disk fonts.

    @param count total fonts to load
    @param filename array of .DDS font files (and matching .abc spacing data)
    @param split whether each given font is 'split' (full 256 chars, or 128 'normal', 128 'bold')
    @param disableMips whether to force-disable mip use for all specified font files.
    @return zero if initialized fine, one if failed anywhere during init process.
 */
extern NvS32 NVBFInitialize(NvU8 count, const char* filename[], NvBool split[], NvBool disableMips);
/** Clean up NVBF resources -- not yet implemented. */
extern void NVBFCleanup(void); /* !!!!!TBD NYI. */

/** Get the font ID for a named font file.

    Look up the matching font ID for a given font filename previously loaded.
    This is then used as argument to NVBFTextSetFont.  It gives us the ability
    to abstract the order of loaded fonts from the ID/index used to reference
    them later -- most useful when multiple, distinct systems in an app are
    talking to BitFont, and separately loading some same, some different fonts.

    @param filename one of the filenames used in previous NVBFInitialize call.
    @return byte (bfuchar) ID/index that will refer to that font until app exit.
 */
extern NvU8 NVBFGetFontID(const char *filename);

/** Force a different font shader than the default/shared one. */
extern void NVBFPresetFontProgram(GLuint program);

/** Set the destination size/scale to factor coords into. */
extern void NVBFSetScreenRes(NvF32 width, NvF32 height);
/** Get current destination size/scale for safe save/restore. */
extern void NVBFGetScreenRes(NvF32 *width, NvF32 *height);
/** Set the rotation/orientation of text output (in 90-degree increments, preferably). */
extern void NVBFSetScreenRot(NvF32 degrees);
/** Set a starting offset for subsequent drawing calls */
extern void NVBFSetDrawOffset(NvF32 offX, NvF32 offY); // !!!!TBD can be removed?

/* @} */


//============================================================================
// bitfont BFText helper functions
//============================================================================
/** @name "BFText" String Creation & Property Accessors

    BFText 'objects' are an abstraction of a 'text run' or drawable string, and
    contain all information and buffers necessary to quickly draw the text
    using GLES primitives.
*/
/* @{ */

/** Allocate a bftext 'object' (abstracted pointer/reference). */
extern void *NVBFTextAlloc(void);
/** Free an allocated bftext. */
extern void NVBFTextFree(void *bftin);

/** Set whether a given bftext is visible for drawing. */
extern void NVBFTextSetVisibility(void *bftin, NvBool vis);
/** Set the RGBA color (using an @p NvPackedColor) for a line of text. Note that this is
    embedded per-character (per-vertex), and as such requires a rebuild of the string.
    If trying to fade (alpha), or otherwise color-modulate a string on the fly, to avoid
    the rebuilding of the cache you are better off using @p NVBFTextSetMultiplyColor instead.
    Embedded color string literals/escapes take precedence over this string-global value.
*/
extern void NVBFTextSetColor(void *bftin, NvPackedColor color);
/** Set the RGBA color (an @p NvPackedColor) for a line of text.  As this color is multiplied
    in hardware, it doesn't require recaching the optimized vertex data for the string, thus
    allows for easy per-bftext alpha-fades and color modulation. */
extern void NVBFTextSetMultiplyColor(void *bftin, NvPackedColor color);
/** Set the drop-shadow for a bftext.

    Activates a drop-shadow effect on given bftext output.

    @param bftin the bftext to modify
    @param offset the +/- offset of the shadow from the base, in 'texels' (not pixels)
    @param color the shadow color as an @p NvPackedColor
 */
extern void NVBFTextSetShadow(void *bftin, NvS8 offset, NvPackedColor color); // !!!!TBD would be more useful in pixel space
/** Draw less than the full string.

    Switches bftext to draw only first @p num characters in a bftext string.
    Most useful for helping do simple 'type on' effects.
    Note that positioning on the screen is where the chars would be if
    entire text was rendered -- thus left and top alignment are best.
 */
extern void NVBFSetDrawnChars(void *bftin, NvS32 num);

/** Use a subrect of the screen for this bftext's raster operations.

    Sets a subrect in the given screen size, in which processes such as alignment,
    multi-line wrapping, will occur.

    @param bftin the bftext to modify
    @param width left/right alignment 'edges' (and wrap/clip boundary).
    @param height top/bottom alignment 'edges'.
    @param lines if greater than zero, specifies width is used to wrap text, and how many lines to wrap.
    @param dots if greater than zero, specifies a character to repeat three times when needs wrap but out of lines.
 */
extern void NVBFTextSetBox(void *bftin, NvF32 width, NvF32 height, NvS32 lines, NvU32 dots);
/** Helper to quickly update width and height of previously-set text box. */
extern void NVBFTextUpdateBox(void *bftin, NvF32 width, NvF32 height);

/** Select font 'face' to use for this bftext (font ID from NVBFGetFontID). */
extern void NVBFTextSetFont(void *bftin, NvU8 font);
/** Set the output size of text in destination-space pixels, dependent on input font size/scale. */
extern void NVBFTextSetSize(void *bftin, NvF32 size);
/** Set the text string for a given bftext. */
extern void NVBFTextSetString(void *bftin, const char* str);

/** Get the text string for a given bftext. */
extern void NVBFTextGetString(void *bftin, char* str);
/** Get the last calculated output width of the bftext string. */
extern NvF32 NVBFTextGetWidth(void *bftin);
/** Get the last calculated output height of the bftext. */
extern NvF32 NVBFTextGetHeight(void *bftin);

//============================================================================
// positioning the text output
//============================================================================

/** Set the bftext horizontal and vertical alignment.

    Sets the alignment 'edges' for render offset calculation and direction.
    @see bftAlignment
 */
extern void NVBFTextCursorAlign(void *bftin, NvU8 horizontal, NvU8 vertical); // 0==top/left, 1==bottom/right, 2==center/center
/** Set the bftext starting render position.

    Sets an inset/offset from the horizontal and vertical 'edges', chosen by
    @p NVBFTextCursorAlign, in possible combination with @p NVBFTextSetBox.
    (That is, the borders to offset from are "screen edges", unless @p NVBFTextSetBox
    was called, in which case that box/rect is used to determine the 'edges' instead.)
 */
extern void NVBFTextCursorPos(void *bftin, NvF32 horiz, NvF32 vert); // in default mode, top/left offset.

/* @} */


//============================================================================
// bitfont rendering functions
//============================================================================
/** @name BitFont String (BFText) Rasterization Functions
*/
/* @{ */

//============================================================================
/** Prepare to render some bftext objects.

    This sets up necessary bits of the BitFont raster system.
    From this poNvS32 until calling @p NVBFRenderDone, do no other GLES operations.
    It can/should be called once before rendering one or more bftexts, for instance
    looping over an array of text elements on screen.
 */
extern void NVBFTextRenderPrep(void);
/** Conclude rendering of bftexts, making it safe to do other GLES calls. */
extern void NVBFTextRenderDone(void);

/** Set a specific transformation matrix to be used for rendering all text strings,
    until the next call to @p NVBFTextRenderDone. */
extern void NVBFTextMatrix(const NvF32 *mtx);

//============================================================================
/** The main call to actually render a bftext using stored properties. */
extern void NVBFTextRender(void *bftin); //, unsigned NvS32 color);


//============================================================================
/** Force a rebuilding of the cached GLES vertex data for this bftext.

    This function recalculates all cached data related to a given bftext object,
    including GLES vertex data for optimized rendering.  It does NOT normally need
    to be called by the programmer, but is useful in the specific case where you
    have a number of completely static strings, in which case calling this during
    the initialization process can allow the system to preallocate necessary
    structures and not need to recalc/allocate further at runtime.
    
    Many BFText state-changing functions will flag that a given string requires
    its vertex data be rebuilt -- but the system waits until rendering is requested
    so that it only recomputes the cached data once, regardless of the number of
    states changed for that text since the previous render or rebuild call.

    @param bftin the bftext to modify
    @param internal should be set to zero, unless being called by an internal
    function which has already established the VBOs for this bftext.
 */
extern void NVBFTextRebuildCache(void *bftin, NvBool internal);

/* @} */

#endif //_nvbitfont_h_
