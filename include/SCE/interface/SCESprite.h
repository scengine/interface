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

/* created: 20/11/2011
   updated: 20/11/2011 */

#ifndef SCESPRITE_H
#define SCESPRITE_H

#include <SCE/core/SCECore.h>   /* SCE_SNode, SCE_SCamera */
#include "SCE/interface/SCESceneEntity.h"
#include "SCE/interface/SCEShaders.h"
#include "SCE/interface/SCETexture.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sce_ssprite SCE_SSprite;
struct sce_ssprite {
    SCE_SSceneEntity *entity;
    SCE_SShader *shader;
    SCE_STexture *texture;
    SCE_SNode *node;
    SCE_SListIterator it;
};

void SCE_Sprite_Init (SCE_SSprite*);
void SCE_Sprite_Clear (SCE_SSprite*);
SCE_SSprite* SCE_Sprite_Create (void);
void SCE_Sprite_Delete (SCE_SSprite*);

void SCE_Sprite_SetShader (SCE_SSprite*, SCE_SShader*);
void SCE_Sprite_SetTexture (SCE_SSprite*, SCE_STexture*);

SCE_SSceneEntity* SCE_Sprite_GetEntity (SCE_SSprite*);
SCE_SShader* SCE_Sprite_GetShader (SCE_SSprite*);
SCE_STexture* SCE_Sprite_GetTexture (SCE_SSprite*);
SCE_SNode* SCE_Sprite_GetNode (SCE_SSprite*);
SCE_SListIterator* SCE_Sprite_GetIterator (SCE_SSprite*);

void SCE_Sprite_ActivateOcclusion (SCE_SSprite*, int);

void SCE_Sprite_Render (const SCE_SSprite*, const SCE_SCamera*, SCEuint);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
