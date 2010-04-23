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
 
/* created: 11/04/2010
   updated: 16/04/2010 */

#ifndef SCEINTERFACE_H
#define SCEINTERFACE_H

/* external dependencies */
#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

/* internal dependencies */
#include "SCE/interface/SCEQuad.h"
#include "SCE/interface/SCEMesh.h"
#include "SCE/interface/SCELight.h"
#include "SCE/interface/SCEGeometryInstance.h"
#include "SCE/interface/SCESceneResource.h"
#include "SCE/interface/SCESceneEntity.h"
#include "SCE/interface/SCEBatch.h"
#include "SCE/interface/SCEModel.h"
#include "SCE/interface/SCESkybox.h"
#include "SCE/interface/SCEScene.h"

#ifdef __cplusplus
extern "C" {
#endif

int SCE_Init_Interface (FILE*, SCEbitfield);
void SCE_Quit_Interface (void);

#if 0
const char* SCE_GetVersionString (void);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
