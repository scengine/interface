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
 
/* created: 13/03/2008
   updated: 15/03/2009 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCELight.h"

/* is lighting activated? */
static int use_lighting = SCE_TRUE;

void SCE_Light_Init (SCE_SLight *light)
{
    light->clight = NULL;       /* TODO: bouh */
    light->intensity = 1.0f;
    light->radius = 16.0f;
    light->activated = SCE_TRUE;
    light->node = NULL;
    SCE_BoundingSphere_Init (&light->sphere);
    SCE_BoundingSphere_GetSphere (&light->sphere)->radius = light->radius;
    SCE_List_InitIt (&light->it);
    SCE_List_SetData (&light->it, light);
    light->udata = NULL;
}

SCE_SLight* SCE_Light_Create (void)
{
    SCE_SLight *light = NULL;
    SCE_btstart ();
    if (!(light = SCE_malloc (sizeof *light)))
        goto failure;
    SCE_Light_Init (light);
    if (!(light->node = SCE_Node_Create ()))
        goto failure;
    if (!(light->clight = SCE_RCreateLight ()))
        goto failure;
    SCE_Node_SetData (light->node, light);
    SCE_Node_GetElement (light->node)->sphere = &light->sphere;
    goto success;
failure:
    SCE_Light_Delete (light), light = NULL;
    SCEE_LogSrc ();
success:
    SCE_btend ();
    return light;
}

void SCE_Light_Delete (SCE_SLight *light)
{
    if (light) {
        SCE_RDeleteLight (light->clight);
        SCE_Node_Delete (light->node);
        SCE_free (light);
    }
}


void SCE_Light_SetData (SCE_SLight *light, void *data)
{
    light->udata = data;
}
void* SCE_Light_GetData (SCE_SLight *light)
{
    return light->udata;
}


void SCE_Light_Activate (SCE_SLight *light, int activated)
{
    light->activated = activated;
}
int SCE_Light_IsActivated (SCE_SLight *light)
{
    return light->activated;
}

SCE_SNode* SCE_Light_GetNode (SCE_SLight *light)
{
    return light->node;
}
SCE_SListIterator* SCE_Light_GetIterator (SCE_SLight *light)
{
    return &light->it;
}

void SCE_Light_SetColor (SCE_SLight *light, float r, float g, float b)
{
    SCE_RSetLightColor (light->clight, r, g, b);
}
void SCE_Light_SetColorv (SCE_SLight *light, float *c)
{
    SCE_RSetLightColorv (light->clight, c);
}

float* SCE_Light_GetColor (SCE_SLight *light)
{
    return SCE_RGetLightColor (light->clight);
}
void SCE_Light_GetColorv (SCE_SLight *light, float *c)
{
    SCE_RGetLightColorv (light->clight, c);
}

void SCE_Light_GetPositionv (SCE_SLight *light, float *pos)
{
    SCE_Vector3_Set (pos, 0.0f, 0.0f, 0.0f);
    SCE_Matrix4_MulV3Copy (SCE_Node_GetFinalMatrix (light->node), pos);
}

void SCE_Light_GetDirectionv (SCE_SLight *light, float *dir)
{
    SCE_TMatrix3 mat;
    SCE_Vector3_Set (dir, 0.0f, 0.0f, -1.0f);
    SCE_Matrix3_CopyM4 (mat, SCE_Node_GetFinalMatrix (light->node));
    SCE_Matrix3_MulV3Copy (mat, dir);
}

void SCE_Light_Infinite (SCE_SLight *light, int inf)
{
    SCE_RInfiniteLight (light->clight, inf);
}
int SCE_Light_IsInfinite (SCE_SLight *light)
{
    return SCE_RIsInfiniteLight (light->clight);
}

void SCE_Light_SetAngle (SCE_SLight *light, float a)
{
    SCE_RSetLightAngle (light->clight, a);
}
float SCE_Light_GetAngle (SCE_SLight *light)
{
    return SCE_RGetLightAngle (light->clight);
}

void SCE_Light_SetIntensity (SCE_SLight *light, float intensity)
{
    float *color = SCE_RGetLightColor (light->clight);
    SCE_Vector4_Operator1 (color, /=, light->intensity);
    SCE_Vector4_Operator1 (color, *=, intensity);
    light->intensity = intensity;
}
float SCE_Light_GetIntensity (SCE_SLight *light)
{
    return light->intensity;
}

void SCE_Light_SetRadius (SCE_SLight *light, float radius)
{
    light->radius = radius;
    SCE_BoundingSphere_GetSphere (&light->sphere)->radius = light->radius;
    if (radius > 1.0f)                      /* 3 gives good results */
        SCE_RSetLightQuadraticAtt (light->clight, 3.0f/radius);
    else if (radius <= 0.0f) {
        SCE_RSetLightQuadraticAtt (light->clight, 0.0f);
        SCE_RSetLightLinearAtt (light->clight, 0.0f);
    } else
        SCE_RSetLightLinearAtt (light->clight, 12.0f/radius);
}
float SCE_Light_GetRadius (SCE_SLight *light)
{
    return light->radius;
}


void SCE_Light_ActivateLighting (int actived)
{
    use_lighting = actived;
    SCE_RActivateLighting (actived);
}

void SCE_Light_Use (SCE_SLight *light)
{
    if (!use_lighting)
        return;

    if (light && light->activated) {
        SCE_TVector3 pos, dir;
        SCE_Light_GetPositionv (light, pos);
        SCE_Light_GetDirectionv (light, dir);
        SCE_RSetLightPositionv (light->clight, pos);
        SCE_RSetLightDirectionv (light->clight, dir);
        SCE_RUseLight (light->clight);
    } else {
        SCE_RUseLight (NULL);
    }
}
