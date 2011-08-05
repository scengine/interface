/*------------------------------------------------------------------------------
    SCEngine - A 3D real time rendering engine written in the C language
    Copyright (C) 2006-2011  Antony Martin <martin(dot)antony(at)yahoo(dot)fr>

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

/* created: 04/08/2011
   updated: 04/08/2011 */

#ifndef SCEDEFERRED_H
#define SCEDEFERRED_H

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEShaders.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCE_DEFERRED_COLOR_TARGET = 0,
    SCE_DEFERRED_DEPTH_TARGET,
    SCE_DEFERRED_NORMAL_TARGET,
    SCE_DEFERRED_SPECULAR_TARGET,
    SCE_DEFERRED_EMISSIVE_TARGET,
    SCE_NUM_DEFERRED_TARGETS
} SCE_EDeferredTarget;

typedef struct sce_sdeferred SCE_SDeferred;
struct sce_sdeferred {
    SCE_STexture *gbuf;
    SCE_STexture *targets[SCE_NUM_DEFERRED_TARGETS]; /**< Render buffers */
    int n_targets;                                   /**< Used targets */
    SCEuint w, h;               /**< targets' dimensions */

    SCE_SShader *point_shader;
    SCE_SShader *spot_shader;
    SCE_SShader *sun_shader;
};

SCE_SDeferred* SCE_Deferred_Create (void);
void SCE_Deferred_Delete (SCE_SDeferred*);

void SCE_Deferred_SetDimensions (SCE_SDeferred*, SCEuint, SCEuint);

int SCE_Deferred_Build (SCE_SDeferred*, const char*);
int SCE_Deferred_BuildShader (SCE_SDeferred*, SCE_SShader*);

void SCE_Deferred_Render (SCE_SDeferred*, void*, SCE_SCamera*,
                          SCE_STexture*, int);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
