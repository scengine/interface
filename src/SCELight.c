/*------------------------------------------------------------------------------
    SCEngine - A 3D real time rendering engine written in the C language
    Copyright (C) 2006-2012  Antony Martin <martin(dot)antony(at)yahoo(dot)fr>

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
   updated: 11/01/2012 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCELight.h"

/* is lighting activated? */
static int use_lighting = SCE_TRUE;

void SCE_Light_Init (SCE_SLight *light)
{
    light->clight = NULL;       /* TODO: bouh */
    light->type = SCE_POINT_LIGHT;
    light->intensity = 1.0f;
    light->activated = SCE_TRUE;
    light->node = NULL;
    SCE_BoundingSphere_Init (&light->sphere);
    SCE_BoundingSphere_GetSphere (&light->sphere)->radius = 1.0;
    SCE_Cone_Init (&light->cone);
    light->attenuation = 1.0;
    light->cast_shadows = SCE_FALSE;
    light->specular = SCE_FALSE;
    light->shader = NULL;
    SCE_List_InitIt (&light->it);
    SCE_List_SetData (&light->it, light);
    light->udata = NULL;
}

SCE_SLight* SCE_Light_Create (void)
{
    SCE_SLight *light = NULL;
    if (!(light = SCE_malloc (sizeof *light)))
        goto fail;
    SCE_Light_Init (light);
    if (!(light->node = SCE_Node_Create ()))
        goto fail;
    if (!(light->clight = SCE_RCreateLight ()))
        goto fail;
    SCE_Node_SetData (light->node, light);
    SCE_Node_GetElement (light->node)->sphere = &light->sphere;
    return light;
fail:
    SCE_Light_Delete (light);
    SCEE_LogSrc ();
    return NULL;
}

void SCE_Light_Delete (SCE_SLight *light)
{
    if (light) {
        SCE_RDeleteLight (light->clight);
        SCE_List_Remove (&light->it);
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

void SCE_Light_SetType (SCE_SLight *light, SCE_ELightType type)
{
    light->type = type;
}
SCE_ELightType SCE_Light_GetType (SCE_SLight *light)
{
    return light->type;
}

SCE_SBoundingSphere* SCE_Light_GetBoundingSphere (SCE_SLight *light)
{
    return &light->sphere;
}
SCE_SCone* SCE_Light_GetCone (SCE_SLight *light)
{
    return &light->cone;
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

void SCE_Light_SetMatrix (SCE_SLight *light, const SCE_TMatrix4 mat)
{
    SCE_Node_SetMatrix (light->node, mat);
}
void SCE_Light_SetPosition (SCE_SLight *light, float x, float y, float z)
{
    SCE_TVector3 pos;
    SCE_Vector3_Set (pos, x, y, z);
    SCE_Light_SetPositionv (light, pos);
}
void SCE_Light_SetPositionv (SCE_SLight *light, const SCE_TVector3 pos)
{
    float *mat = SCE_Node_GetMatrix (light->node, SCE_NODE_WRITE_MATRIX);
    SCE_Matrix4_SetTranslation (mat, pos);
}
void SCE_Light_GetPositionv (SCE_SLight *light, SCE_TVector3 pos)
{
    SCE_Matrix4_GetTranslation (SCE_Node_GetFinalMatrix (light->node), pos);
}

void SCE_Light_GetOrientationv (SCE_SLight *light, SCE_TVector3 dir)
{
    float *mat = SCE_Node_GetFinalMatrix (light->node);
    SCE_GetMat4C3 (2, mat, dir); /* retrieve 3rd column */
    SCE_Vector3_Operator1 (dir, *=, -1.0); /* reverse! */
    SCE_Vector3_Normalize (dir);
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
    SCE_Cone_SetAngle (&light->cone, a);
}
float SCE_Light_GetAngle (const SCE_SLight *light)
{
    return SCE_Cone_GetAngle (&light->cone);
}

void SCE_Light_SetAttenuation (SCE_SLight *light, float att)
{
    light->attenuation = att;
}
float SCE_Light_GetAttenuation (const SCE_SLight *light)
{
    return light->attenuation;
}

/**
 * \brief Sets 'cast shadows' state of a light, disabled by default
 * \param light a light
 * \param cast whether \p light will cast shadows
 * \sa SCE_Light_GetShadows()
 */
void SCE_Light_SetShadows (SCE_SLight *light, int cast)
{
    light->cast_shadows = cast;
}
/**
 * \brief Gets a light's 'cast shadows' status
 * \param light a light
 * \return SCE_TRUE if \p light is casting shadows, SCE_FALSE otherwise
 * \sa SCE_Light_SetShadows()
 */
int SCE_Light_GetShadows (const SCE_SLight *light)
{
    return light->cast_shadows;
}

/**
 * \brief Toggles specular, disabled by default
 * \param light a light
 * \param specular whether \p light will produce specular lighting
 * \sa SCE_Light_GetSpecular()
 */
void SCE_Light_SetSpecular (SCE_SLight *light, int specular)
{
    light->specular = specular;
}
/**
 * \brief Gets a light's 'specular' status
 * \param light a light
 * \returns SCE_TRUE if \p is producing specular lighting, SCE_FALSE otherwise
 * \sa SCE_Light_SetSpecular()
 */
int SCE_Light_GetSpecular (const SCE_SLight *light)
{
    return light->specular;
}


void SCE_Light_SetShader (SCE_SLight *light,
                          SCE_SDeferredLightingShader *shader)
{
    light->shader = shader;
}
SCE_SDeferredLightingShader* SCE_Light_GetShader (const SCE_SLight *light)
{
    return light->shader;
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

void SCE_Light_SetHeight (SCE_SLight *light, float height)
{
    SCE_Cone_SetHeight (&light->cone, height);
}
float SCE_Light_GetHeight (SCE_SLight *light)
{
    return SCE_Cone_GetHeight (&light->cone);
}

void SCE_Light_SetRadius (SCE_SLight *light, float radius)
{
    float *mat = SCE_Node_GetMatrix (light->node, SCE_NODE_WRITE_MATRIX);
    SCE_Matrix4_SetScale (mat, radius, radius, radius);
    SCE_BoundingSphere_GetSphere (&light->sphere)->radius = radius;
}
float SCE_Light_GetRadius (SCE_SLight *light)
{
    SCE_TVector3 scale;
    float s;
    SCE_Matrix4_GetScale (SCE_Node_GetFinalMatrix (light->node), scale);
    s = MAX (scale[0], scale[1]);
    s = MAX (s, scale[2]);
    return s;
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
        float radius = SCE_Light_GetRadius (light);

        if (radius > 1.0f)                         /* 3 gives good results */
            SCE_RSetLightQuadraticAtt (light->clight, 3.0f/radius);
        else if (radius <= 0.0f) {
            SCE_RSetLightQuadraticAtt (light->clight, 0.0f);
            SCE_RSetLightLinearAtt (light->clight, 0.0f);
        } else
            SCE_RSetLightLinearAtt (light->clight, 12.0f/radius);

        SCE_Light_GetPositionv (light, pos);
        SCE_Light_GetOrientationv (light, dir);
        SCE_RSetLightPositionv (light->clight, pos);
        SCE_RSetLightDirectionv (light->clight, dir);
        SCE_RUseLight (light->clight);
    } else {
        SCE_RUseLight (NULL);
    }
}
