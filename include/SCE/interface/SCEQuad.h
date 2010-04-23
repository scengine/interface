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
   updated: 02/08/2009 */

#ifndef SCEQUAD_H
#define SCEQUAD_H

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include "SCE/interface/SCEMesh.h"

#ifdef __cplusplus
extern "C" {
#endif

int SCE_Init_Quad (void);
void SCE_Quit_Quad (void);

void SCE_Quad_DrawDefault (void);

void SCE_Quad_MakeMatrix (SCE_TMatrix4, float, float, float, float);
void SCE_Quad_MakeMatrixFromRectangle (SCE_TMatrix4, SCE_SIntRect*);
void SCE_Quad_MakeMatrixFromRectanglef (SCE_TMatrix4, SCE_SFloatRect*);

void SCE_Quad_Draw (float, float, float, float);
void SCE_Quad_DrawFromRectangle (SCE_SIntRect*);
void SCE_Quad_DrawFromRectanglef (SCE_SFloatRect*);

SCE_SMesh* SCE_Quad_GetMesh (void);
SCE_SGeometry* SCE_Quad_GetGeometry (void);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
