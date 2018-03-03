//----------------------------------------------------------------------------------
// File:        jni/nv_ui/nv_ui_rect.h
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

#ifndef __NV_UI_RECT_H__
#define __NV_UI_RECT_H__

#include <nv_global.h>

//=============================================================================
//=============================================================================
/** A class that abstracts an onscreen rectangular region.

    All UI elements use the NvUIRect to a greater or lesser degree.  It is the
    underpinning of onscreen positioning and scaling (and thus animation too),
    and heavily used in input hit-testing routines as well.
*/
class NvUIRect
{
protected:
    friend class NvUIElement;
    // ---------- VARS ----------
    NvF32 m_left, m_top; /**< Generally these are used as a screen-space origin. */
    NvF32 m_width, m_height; /**< top-left + height-width == rect. */
    NvF32 m_zdepth; /**< While not used much yet, z-depth was included for future expansion. */

public:
    // ---------- METHODS ----------
    NvUIRect()
        : m_left(0), m_top(0), m_width(0), m_height(0), m_zdepth(0)
        { /* empty */ }
        
    NvUIRect(NvF32 l, NvF32 t, NvF32 w, NvF32 h, NvF32 z)
        : m_left(l), m_top(t), m_width(w), m_height(h), m_zdepth(z)
        { /* empty */ }

    NvUIRect(NvF32 l, NvF32 t, NvF32 w, NvF32 h)
        : m_left(l), m_top(t), m_width(w), m_height(h), m_zdepth(0.0f)
        { /* empty */ }    

    NvUIRect(NvUIRect &r)
        : m_left(r.m_left), m_top(r.m_top), m_width(r.m_width), m_height(r.m_height), m_zdepth(r.m_zdepth)
        { /* empty */ }
               
    virtual ~NvUIRect()
		{ /*empty*/ }

    virtual void set(NvF32 l, NvF32 t, NvF32 w, NvF32 h, NvF32 z=0.0f)
    {
		m_left = l;
		m_top = t;
		m_width = w;
		m_height = h;
		m_zdepth = z;
	}

    virtual bool inside(NvF32 x, NvF32 y, NvF32 mx=0, NvF32 my=0)
	{
		if (x >= m_left-mx && x <= m_left+m_width+mx)
			if (y >= m_top-my && y <= m_top+m_height+my)
				return true;
		return false;
	}
    
    inline NvF32 top() { return m_top; }
    inline NvF32 left() { return m_left; }
    inline NvF32 width() { return m_width; }
    inline NvF32 height() { return m_height; }

//    friend class NvUIElement;
};

#endif /* __NV_UI_RECT_H__ */
