/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/imports.h"
#include "main/mtypes.h"
#include "main/macros.h"

#include "swrast_setup/swrast_setup.h"
#include "math/m_translate.h"
#include "tnl/tnl.h"

#include "radeon_context.h"
#include "radeon_ioctl.h"
#include "radeon_state.h"
#include "radeon_swtcl.h"
#include "radeon_maos.h"
#include "radeon_tcl.h"

static void emit_vecfog(GLcontext *ctx, struct radeon_aos *aos,
			GLvoid *data, int stride, int count)
{
   int i;
   uint32_t *out;
   int size = 1;
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if (RADEON_DEBUG & RADEON_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   if (stride == 0) {
      radeonAllocDmaRegion( rmesa, &aos->bo, &aos->offset, size * 4, 32 );
      count = 1;
      aos->stride = 0;
   }
   else {
      radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, size * 4, 32);
      aos->stride = size;
   }

   aos->components = size;
   aos->count = count;


   /* Emit the data
    */
   out = (uint32_t*)((char*)aos->bo->ptr + aos->offset);
   for (i = 0; i < count; i++) {
      out[0] = radeonComputeFogBlendFactor( ctx, *(GLfloat *)data );
      out++;
      data += stride;
   }
}

static void emit_s0_vec(uint32_t *out, GLvoid *data, int stride, int count)
{
   int i;
   if (RADEON_DEBUG & RADEON_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   for (i = 0; i < count; i++) {
      out[0] = *(int *)data;
      out[1] = 0;
      out += 2;
      data += stride;
   }
}

static void emit_stq_vec(uint32_t *out, GLvoid *data, int stride, int count)
{
   int i;

   if (RADEON_DEBUG & RADEON_VERTS)
      fprintf(stderr, "%s count %d stride %d\n",
	      __FUNCTION__, count, stride);

   for (i = 0; i < count; i++) {
      out[0] = *(int *)data;
      out[1] = *(int *)(data+4);
      out[2] = *(int *)(data+12);
      out += 3;
      data += stride;
   }
}




static void emit_tex_vector(GLcontext *ctx, struct radeon_aos *aos,
			    GLvoid *data, int size, int stride, int count)
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   int emitsize;
   uint32_t *out;

   if (RADEON_DEBUG & RADEON_VERTS)
      fprintf(stderr, "%s %d/%d\n", __FUNCTION__, count, size);

   switch (size) {
   case 4: emitsize = 3; break;
   case 3: emitsize = 3; break;
   default: emitsize = 2; break;
   }


   if (stride == 0) {
      radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, emitsize * 4, 32);
      count = 1;
      aos->stride = 0;
   }
   else {
      radeonAllocDmaRegion(rmesa, &aos->bo, &aos->offset, emitsize * count * 4, 32);
      aos->stride = emitsize;
   }

   aos->components = emitsize;
   aos->count = count;

   /* Emit the data
    */
   out = (uint32_t*)((char*)aos->bo->ptr + aos->offset);
   switch (size) {
   case 1:
      emit_s0_vec( out, data, stride, count );
      break;
   case 2:
      radeonEmitVec8( out, data, stride, count );
      break;
   case 3:
      radeonEmitVec12( out, data, stride, count );
      break;
   case 4:
      emit_stq_vec( out, data, stride, count );
      break;
   default:
      assert(0);
      exit(1);
      break;
   }
}




/* Emit any changed arrays to new GART memory, re-emit a packet to
 * update the arrays.  
 */
void radeonEmitArrays( GLcontext *ctx, GLuint inputs )
{
   r100ContextPtr rmesa = R100_CONTEXT( ctx );
   struct vertex_buffer *VB = &TNL_CONTEXT( ctx )->vb;
   GLuint nr = 0;
   GLuint vfmt = 0;
   GLuint count = VB->Count;
   GLuint vtx, unit;
   
#if 0
   if (RADEON_DEBUG & RADEON_VERTS)
      _tnl_print_vert_flags( __FUNCTION__, inputs );
#endif

   if (1) {
      if (!rmesa->tcl.obj.buf) 
	rcommon_emit_vector( ctx, 
			     &(rmesa->tcl.aos[nr]),
			     (char *)VB->ObjPtr->data,
			     VB->ObjPtr->size,
			     VB->ObjPtr->stride,
			     count);

      switch( VB->ObjPtr->size ) {
      case 4: vfmt |= RADEON_CP_VC_FRMT_W0;
      case 3: vfmt |= RADEON_CP_VC_FRMT_Z;
      case 2: vfmt |= RADEON_CP_VC_FRMT_XY;
      default:
         break;
      }
      nr++;
   }
   

   if (inputs & VERT_BIT_NORMAL) {
      if (!rmesa->tcl.norm.buf)
	 rcommon_emit_vector( ctx, 
			      &(rmesa->tcl.aos[nr]),
			      (char *)VB->NormalPtr->data,
			      3,
			      VB->NormalPtr->stride,
			      count);

      vfmt |= RADEON_CP_VC_FRMT_N0;
      nr++;
   }

   if (inputs & VERT_BIT_COLOR0) {
      int emitsize;
      if (VB->ColorPtr[0]->size == 4 &&
	  (VB->ColorPtr[0]->stride != 0 ||
	   VB->ColorPtr[0]->data[0][3] != 1.0)) {
	 vfmt |= RADEON_CP_VC_FRMT_FPCOLOR | RADEON_CP_VC_FRMT_FPALPHA;
	 emitsize = 4;
      }

      else {
	 vfmt |= RADEON_CP_VC_FRMT_FPCOLOR;
	 emitsize = 3;
      }

      if (!rmesa->tcl.rgba.buf)
	rcommon_emit_vector( ctx,
			     &(rmesa->tcl.aos[nr]),
			     (char *)VB->ColorPtr[0]->data,
			     emitsize,
			     VB->ColorPtr[0]->stride,
			     count);

      nr++;
   }


   if (inputs & VERT_BIT_COLOR1) {
      if (!rmesa->tcl.spec.buf) {

	rcommon_emit_vector( ctx,
			     &(rmesa->tcl.aos[nr]),
			     (char *)VB->SecondaryColorPtr[0]->data,
			     3,
			     VB->SecondaryColorPtr[0]->stride,
			     count);
      }

      vfmt |= RADEON_CP_VC_FRMT_FPSPEC;
      nr++;
   }

/* FIXME: not sure if this is correct. May need to stitch this together with
   secondary color. It seems odd that for primary color color and alpha values
   are emitted together but for secondary color not. */
   if (inputs & VERT_BIT_FOG) {
      if (!rmesa->tcl.fog.buf)
	 emit_vecfog( ctx,
		      &(rmesa->tcl.aos[nr]),
		      (char *)VB->FogCoordPtr->data,
		      VB->FogCoordPtr->stride,
		      count);

      vfmt |= RADEON_CP_VC_FRMT_FPFOG;
      nr++;
   }


   vtx = (rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] &
	  ~(RADEON_TCL_VTX_Q0|RADEON_TCL_VTX_Q1|RADEON_TCL_VTX_Q2));
      
   for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
      if (inputs & VERT_BIT_TEX(unit)) {
	 if (!rmesa->tcl.tex[unit].buf)
	    emit_tex_vector( ctx,
			     &(rmesa->tcl.aos[nr]),
			     (char *)VB->TexCoordPtr[unit]->data,
			     VB->TexCoordPtr[unit]->size,
			     VB->TexCoordPtr[unit]->stride,
			     count );
	 nr++;

	 vfmt |= RADEON_ST_BIT(unit);
         /* assume we need the 3rd coord if texgen is active for r/q OR at least
	    3 coords are submitted. This may not be 100% correct */
         if (VB->TexCoordPtr[unit]->size >= 3) {
	    vtx |= RADEON_Q_BIT(unit);
	    vfmt |= RADEON_Q_BIT(unit);
	 }
	 if ( (ctx->Texture.Unit[unit].TexGenEnabled & (R_BIT | Q_BIT)) )
	    vtx |= RADEON_Q_BIT(unit);
	 else if ((VB->TexCoordPtr[unit]->size >= 3) &&
	          ((ctx->Texture.Unit[unit]._ReallyEnabled & (TEXTURE_CUBE_BIT)) == 0)) {
	    GLuint swaptexmatcol = (VB->TexCoordPtr[unit]->size - 3);
	    if (((rmesa->NeedTexMatrix >> unit) & 1) &&
		 (swaptexmatcol != ((rmesa->TexMatColSwap >> unit) & 1)))
	       radeonUploadTexMatrix( rmesa, unit, swaptexmatcol ) ;
	 }
      }
   }

   if (vtx != rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT]) {
      RADEON_STATECHANGE( rmesa, tcl );
      rmesa->hw.tcl.cmd[TCL_OUTPUT_VTXFMT] = vtx;
   }

   rmesa->tcl.nr_aos_components = nr;
   rmesa->tcl.vertex_format = vfmt;
}
