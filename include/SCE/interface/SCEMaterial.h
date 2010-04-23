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

#ifndef SCEMATERIAL_H
#define SCEMATERIAL_H

#include <stdarg.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCESceneResource.h"
#include "SCE/interface/SCELight.h"
#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEShaders.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup material
 * @{
 */

/* identifiants des donnes d'une face d'une cubemap */
#define SCE_TEXID_POSX 0
#define SCE_TEXID_NEGX 1
#define SCE_TEXID_POSY 2
#define SCE_TEXID_NEGY 3
#define SCE_TEXID_POSZ 4
#define SCE_TEXID_NEGZ 5

/** \copydoc sce_smaterial */
typedef struct sce_smaterial SCE_SMaterial;
/**
 * \brief A SCE material
 */
struct sce_smaterial
{
    SCE_RMaterial *mat;  /**< Core material */
    /* request that SCE_RCreatePointSprite() doesn't allocs for internal var */
    SCE_RPointSprite ps; /**< Point sprite's model */
    int use_ps;          /**< Are point sprites actived? */
    SCE_SSceneResource s_resource; /**< Scene resource */
};

/** @} */

/* cree un materiau */
SCE_SMaterial* SCE_Material_Create (void);
/* supprime un materiau */
void SCE_Material_Delete (SCE_SMaterial*);

SCE_SSceneResource* SCE_Material_GetSceneResource (SCE_SMaterial*);

/* retourne le materiau coeur d'un materiau */
SCE_RMaterial* SCE_Material_GetCMaterial (SCE_SMaterial*);

/* defini une couleur d'un materiau */
void SCE_Material_SetColor(SCE_SMaterial*, SCEenum, float, float, float, float);
void SCE_Material_SetColorv (SCE_SMaterial*, SCEenum, float*);
/* recupere la couleur d'un materiau */
float* SCE_Material_GetColor (SCE_SMaterial*, SCEenum);
void SCE_Material_GetColorv (SCE_SMaterial*, SCEenum, float*);

/* active/desactive les point sprites */
void SCE_Material_ActivatePointSprite (SCE_SMaterial*, int);
void SCE_Material_EnablePointSprite (SCE_SMaterial*);
void SCE_Material_DisablePointSprite (SCE_SMaterial*);
/* defini un modele de point sprite */
void SCE_Material_SetPointSpriteModel (SCE_SMaterial*, SCE_RPointSprite*);
/* renvoie le modele de point sprite */
SCE_RPointSprite* SCE_Material_GetPointSpriteModel (SCE_SMaterial*);

/* defini le blending */
void SCE_Material_ActivateBlending (SCE_SMaterial*, int);
void SCE_Material_EnableBlending (SCE_SMaterial*);
void SCE_Material_DisableBlending (SCE_SMaterial*);
void SCE_Material_SetBlending (SCE_SMaterial*, SCEenum, SCEenum);

/* defini le materiau actif */
void SCE_Material_Use (SCE_SMaterial*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
