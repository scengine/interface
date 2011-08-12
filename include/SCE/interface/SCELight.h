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
 
/* created: 13/03/2008
   updated: 12/08/2011 */

#ifndef SCELIGHT_H
#define SCELIGHT_H

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCE_POINT_LIGHT,
    SCE_SPOT_LIGHT,
    SCE_SUN_LIGHT,
    SCE_NUM_LIGHT_TYPES
} SCE_ELightType;

/** \copydoc sce_slight */
typedef struct sce_slight SCE_SLight;
/**
 * \brief A light
 */
struct sce_slight {
    /* TODO: make it as non-pointer */
    SCE_RLight *clight; /* lumiere coeur */
    SCE_ELightType type;
    float intensity;    /* coefficient d'intensite */
    float radius;       /* rayon de portee de la lumiere */
    int activated;      /* defini si la lumiere est active */
    SCE_SBoundingSphere sphere; /**< Bounding sphere (if point) */
    SCE_SCone cone;             /**< Cone (if spot) */
    float attenuation;          /**< Spot edges attenuation */
    SCE_SNode *node;    /* noeud de la lumiere */
    SCE_SListIterator it;
    void *udata;
};

void SCE_Light_Init (SCE_SLight*);

SCE_SLight* SCE_Light_Create (void);
void SCE_Light_Delete (SCE_SLight*);

void SCE_Light_SetData (SCE_SLight*, void*);
void* SCE_Light_GetData (SCE_SLight*);

void SCE_Light_Activate (SCE_SLight*, int);
int SCE_Light_IsActivated (SCE_SLight*);

void SCE_Light_SetType (SCE_SLight*, SCE_ELightType);
SCE_ELightType SCE_Light_GetType (SCE_SLight*);

SCE_SBoundingSphere* SCE_Light_GetBoundingSphere (SCE_SLight*);
SCE_SCone* SCE_Light_GetCone (SCE_SLight*);

SCE_SNode* SCE_Light_GetNode (SCE_SLight*);
SCE_SListIterator* SCE_Light_GetIterator (SCE_SLight*);

void SCE_Light_SetColor (SCE_SLight*, float, float, float);
void SCE_Light_SetColorv (SCE_SLight*, float*);
float* SCE_Light_GetColor (SCE_SLight*);
void SCE_Light_GetColorv (SCE_SLight*, float*);

void SCE_Light_GetPositionv (SCE_SLight*, float*);

void SCE_Light_SetOrientationv (SCE_SLight*, const SCE_TVector3);
void SCE_Light_GetOrientationv (SCE_SLight*, SCE_TVector3);

void SCE_Light_Infinite (SCE_SLight*, int);
int SCE_Light_IsInfinite (SCE_SLight*);

void SCE_Light_SetAngle (SCE_SLight*, float);
float SCE_Light_GetAngle (const SCE_SLight*);

void SCE_Light_SetAttenuation (SCE_SLight*, float);
float SCE_Light_GetAttenuation (const SCE_SLight*);

void SCE_Light_SetIntensity (SCE_SLight*, float);
float SCE_Light_GetIntensity (SCE_SLight*);

void SCE_Light_SetRadius (SCE_SLight*, float);
float SCE_Light_GetRadius (SCE_SLight*);

void SCE_Light_ActivateLighting (int);

void SCE_Light_Use (SCE_SLight*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
