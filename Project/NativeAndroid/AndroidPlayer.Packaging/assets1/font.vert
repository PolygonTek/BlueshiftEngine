//----------------------------------------------------------------------------------
// File:        native_basic/assets/font.vert
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
// this is set from higher level.  think of it as the upper model matrix
uniform mat4 pixelToClipMat;

uniform vec4 col_uni;

attribute vec2 pos_attr;
attribute vec2 tex_attr;
attribute vec4 col_attr;

varying vec4 col_var;
varying vec2 tex_var;

void main()
{
	// account for translation and rotation of the primitive into [-1,1] spatial default.
    gl_Position = pixelToClipMat * vec4(pos_attr.x, pos_attr.y, 0, 1);

    col_var = col_attr * col_uni; // when shadowing, this multiplies shadowed chars as well!!!
    tex_var = tex_attr;
}
