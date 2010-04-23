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
 
/* created: 19/05/2007
   updated: 12/07/2009 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCEMaterial.h"

static SCE_SMaterial *used = NULL;

static void SCE_Material_Init (SCE_SMaterial *m)
{
    m->mat = NULL;
    SCE_RInitPointSprite (&m->ps);
    m->use_ps = SCE_FALSE;
    SCE_SceneResource_Init (&m->s_resource);
    SCE_SceneResource_SetResource (&m->s_resource, m);
}

SCE_SMaterial* SCE_Material_Create (void)
{
    SCE_SMaterial *mat = NULL;
    if (!(mat = SCE_malloc (sizeof *mat)))
        goto failure;
    SCE_Material_Init (mat);
    if (!(mat->mat = SCE_RCreateMaterial ()))
        goto failure;
    return mat;
failure:
    SCE_Material_Delete (mat);
    SCEE_LogSrc ();
    return NULL;
}

void SCE_Material_Delete (SCE_SMaterial *mat)
{
    if (mat) {
        SCE_SceneResource_RemoveResource (&mat->s_resource);
        SCE_RDeleteMaterial (mat->mat);
        SCE_free (mat);
    }
}

SCE_SSceneResource* SCE_Material_GetSceneResource (SCE_SMaterial *mat)
{
    return &mat->s_resource;
}

SCE_RMaterial* SCE_Material_GetCMaterial (SCE_SMaterial *mat)
{
    return mat->mat;
}


void SCE_Material_SetColor (SCE_SMaterial *mat, SCEenum type,
                            float r, float g, float b, float a)
{
    SCE_RSetMaterialColor (mat->mat, type, r, g, b, a);
}
void SCE_Material_SetColorv (SCE_SMaterial *mat, SCEenum type, float *color)
{
    SCE_RSetMaterialColorv (mat->mat, type, color);
}

float* SCE_Material_GetColor (SCE_SMaterial *mat, SCEenum type)
{
    return SCE_RGetMaterialColor (mat->mat, type);
}
void SCE_Material_GetColorv (SCE_SMaterial *mat, SCEenum type, float *color)
{
    SCE_RGetMaterialColorv (mat->mat, type, color);
}


void SCE_Material_ActivatePointSprite (SCE_SMaterial *mat, int actived)
{
    mat->use_ps = actived;
}
void SCE_Material_EnablePointSprite (SCE_SMaterial *mat)
{
    mat->use_ps = SCE_TRUE;
}
void SCE_Material_DisablePointSprite (SCE_SMaterial *mat)
{
    mat->use_ps = SCE_FALSE;
}
void SCE_Material_SetPointSpriteModel (SCE_SMaterial *mat, SCE_RPointSprite *ps)
{
    mat->ps = *ps;
}
SCE_RPointSprite* SCE_Material_GetPointSpriteModel (SCE_SMaterial *mat)
{
    return &mat->ps;
}


void SCE_Material_ActivateBlending (SCE_SMaterial *mat, int use)
{
    SCE_RActiveMaterialBlending (mat->mat, use);
}
void SCE_Material_EnableBlending (SCE_SMaterial *mat)
{
    SCE_REnableMaterialBlending (mat->mat);
}
void SCE_Material_DisableBlending (SCE_SMaterial *mat)
{
    SCE_RDisableMaterialBlending (mat->mat);
}
void SCE_Material_SetBlending (SCE_SMaterial *mat, SCEenum src, SCEenum dst)
{
    SCE_RSetMaterialBlending (mat->mat, src, dst);
}

void SCE_Material_Use (SCE_SMaterial *mat)
{
    if (mat) {
        used = mat;
        SCE_RUseMaterial (mat->mat);
        SCE_RUsePointSprite ((mat->use_ps) ? &mat->ps : NULL);
    } else if (used) {
        used = NULL;
        SCE_RUsePointSprite (NULL);
        SCE_RUseMaterial (NULL);
    }
}
