/*------------------------------------------------------------------------------
    SCEngine - A 3D real time rendering engine written in the C language
    Copyright (C) 2006-2010  Antony Martin <martin(dot)antony(at)yahoo(dot)fr>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 -----------------------------------------------------------------------------*/
 
/* created: 06/04/2008
   updated: 08/07/2008 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCEQuad.h"

/**
 * \file SCEQuad.c
 * \copydoc quad
 * \file SCEQuad.h
 * \copydoc quad
 */

/**
 * \defgroup quad Offers quad mesh
 * \ingroup interface
 * \brief
 */

/** @{ */

static SCE_SMesh *mesh = NULL;

int SCE_Init_Quad (void)
{
    SCE_SGeometry *geom = NULL;
    /*SCEvertices vertices[] = {-1., -1.,  1., -1.,  1., 1.,  -1., 1.};*/
    /* hax, there is no need for a 3rd component but Geometry_SetData()
       requires it */
    SCEvertices vertices[] = {0., 0., 0.,  1., 0., 0.,  1., 1., 0.,  0., 1., 0.};
    SCEvertices texcoord[] = {0., 0.,  1., 0.,  1., 1.,  0., 1.};
    SCEvertices normals[] =  {0.,0.,1.,  0.,0.,1., 0.,0.,1., 0.,0.,1.};
    SCEvertices colors[] = {1., 1., 1., 1.,  1., 1., 1., 1.,
                            1., 1., 1., 1.,  1., 1., 1., 1.};
    SCEindices indices[] = {1, 0, 2, 2, 0, 3};

    if (!(geom = SCE_Geometry_Create ()))
        goto fail;
    if (!(mesh = SCE_Mesh_Create ()))
        goto fail;
    SCE_Geometry_SetPrimitiveType (geom, SCE_TRIANGLES);
    if (SCE_Geometry_SetDataDup (geom, vertices, normals, texcoord, indices,
                                 4, 6) < 0)
        goto fail;
    if (SCE_Mesh_SetGeometry (mesh, geom, SCE_TRUE) < 0)
        goto fail;
    SCE_Mesh_Build (mesh, SCE_GLOBAL_VERTEX_BUFFER, NULL);

    return SCE_OK;
fail:
    SCE_Mesh_Delete (mesh);
    SCEE_LogSrc ();
    SCEE_LogSrcMsg ("failed to initialize quads manager");
    return SCE_ERROR;
}
void SCE_Quit_Quad (void)
{
    SCE_Mesh_Delete (mesh), mesh = NULL;
}

void SCE_Quad_DrawDefault (void)
{
    SCE_Mesh_Use (mesh);
    SCE_Mesh_Render ();
    SCE_Mesh_Unuse ();
}

void SCE_Quad_MakeMatrix (SCE_TMatrix4 mat, float x, float y, float w, float h)
{
    SCE_Matrix4_Scale (mat, w, h, 1.);
    SCE_Matrix4_MulTranslate (mat, x/w, y/h, 0.);
}
void SCE_Quad_MakeMatrixFromRectangle (SCE_TMatrix4 mat, SCE_SIntRect *r)
{
    SCE_Quad_MakeMatrix (mat, r->p1[0], r->p1[1], SCE_Rectangle_GetWidth (r),
                         SCE_Rectangle_GetHeight (r));
}
void SCE_Quad_MakeMatrixFromRectanglef (SCE_TMatrix4 mat, SCE_SFloatRect *r)
{
    SCE_Quad_MakeMatrix (mat, r->p1[0], r->p1[1], SCE_Rectangle_GetWidthf (r),
                         SCE_Rectangle_GetHeightf (r));
}

void SCE_Quad_Draw (float x, float y, float w, float h)
{
    SCE_TMatrix4 mat;
    SCE_RPushMatrix ();
    SCE_Quad_MakeMatrix (mat, x, y, w, h);
    SCE_RMultMatrix (mat);
    SCE_Quad_DrawDefault ();
    SCE_RPopMatrix ();
}

void SCE_Quad_DrawFromRectangle (SCE_SIntRect *r)
{
    SCE_Quad_Draw (r->p1[0], r->p1[1], SCE_Rectangle_GetWidth (r),
                   SCE_Rectangle_GetHeight (r));
}
void SCE_Quad_DrawFromRectanglef (SCE_SFloatRect *r)
{
    SCE_Quad_Draw (r->p1[0], r->p1[1], SCE_Rectangle_GetWidthf (r),
                   SCE_Rectangle_GetHeightf (r));
}

SCE_SMesh* SCE_Quad_GetMesh (void)
{
    return mesh;
}

SCE_SGeometry* SCE_Quad_GetGeometry (void)
{
    return SCE_Mesh_GetGeometry (mesh);
}

/** @} */
