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

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>

#include "SCE/interface/SCESprite.h"

void SCE_Sprite_Init (SCE_SSprite *sprite)
{
    sprite->entity = NULL;
    sprite->shader = NULL;
    sprite->texture = NULL;
    sprite->node = NULL;
    SCE_List_InitIt (&sprite->it);
    SCE_List_SetData (&sprite->it, sprite);
}
void SCE_Sprite_Clear (SCE_SSprite *sprite)
{
    SCE_List_Remove (&sprite->it);
    SCE_SceneEntity_Delete (sprite->entity);
    SCE_Shader_Delete (sprite->shader);
    SCE_Texture_Delete (sprite->texture);
    SCE_Node_Delete (sprite->node);
}

SCE_SSprite* SCE_Sprite_Create (void)
{
    SCE_SSprite *sprite = NULL;
    if (!(sprite = SCE_malloc (sizeof *sprite)))
        goto fail;
    else {
        SCE_Sprite_Init (sprite);
        if (!(sprite->entity = SCE_SceneEntity_Create ())) goto fail;
        if (!(sprite->node = SCE_Node_Create ())) goto fail;
        sprite->entity->props.cullface = SCE_FALSE;
        sprite->entity->props.alphatest = SCE_TRUE;
        sprite->entity->props.alphafunc = SCE_GREATER;
        sprite->entity->props.alpharef = 0.99;
        sprite->entity->props.depthtest = SCE_FALSE;
    }
    return sprite;
fail:
    SCE_Sprite_Delete (sprite);
    SCEE_LogSrc ();
    return NULL;
}
void SCE_Sprite_Delete (SCE_SSprite *sprite)
{
    if (sprite) {
        SCE_Sprite_Clear (sprite);
        SCE_free (sprite);
    }
}

void SCE_Sprite_SetShader (SCE_SSprite *sprite, SCE_SShader *shader)
{
    sprite->shader = shader;
    SCE_SceneEntity_SetShader (sprite->entity, shader);
}
void SCE_Sprite_SetTexture (SCE_SSprite *sprite, SCE_STexture *texture)
{
    if (sprite->texture) {
        SCE_SceneEntity_RemoveTexture (sprite->entity, sprite->texture);
        SCE_Texture_Delete (sprite->texture);
        sprite->texture = NULL;
    }
    sprite->texture = texture;
    SCE_SceneEntity_AddTexture (sprite->entity, texture);
}

SCE_SSceneEntity* SCE_Sprite_GetEntity (SCE_SSprite *sprite)
{
    return sprite->entity;
}
SCE_SShader* SCE_Sprite_GetShader (SCE_SSprite *sprite)
{
    return sprite->shader;
}
SCE_STexture* SCE_Sprite_GetTexture (SCE_SSprite *sprite)
{
    return sprite->texture;
}
SCE_SNode* SCE_Sprite_GetNode (SCE_SSprite *sprite)
{
    return sprite->node;
}
SCE_SListIterator* SCE_Sprite_GetIterator (SCE_SSprite *sprite)
{
    return &sprite->it;
}

/* TODO: two types of "occlusion" functions:
         - "will the sprite be culled by other objects?" (this one)
         - "will the sprite cull other objects?" */
void SCE_Sprite_ActivateOcclusion (SCE_SSprite *sprite, int a)
{
    if (a) {
        sprite->entity->props.depthtest = SCE_TRUE;
    } else {
        sprite->entity->props.depthtest = SCE_FALSE;
    }
}


void SCE_Sprite_Render (const SCE_SSprite *sprite, const SCE_SCamera *cam,
                        SCEuint state)
{
    if (SCE_SceneEntity_MatchState (sprite->entity, state)) {
        SCE_TVector3 pos;
        int screen_w, screen_h, w, h;
        SCE_SFloatRect r;

        screen_w = SCE_Camera_GetViewport ((SCE_SCamera*)cam)->w;
        screen_h = SCE_Camera_GetViewport ((SCE_SCamera*)cam)->h;
        w = SCE_Texture_GetWidth (sprite->texture, 0, 0);
        h = SCE_Texture_GetHeight (sprite->texture, 0, 0);
        SCE_Matrix4_GetTranslation (SCE_Node_GetFinalMatrix(sprite->node),pos);
        SCE_Camera_Project ((SCE_SCamera*)cam, pos);

        pos[0] = pos[0] * 0.5 + 0.5;
        pos[1] = pos[1] * 0.5 + 0.5;
        SCE_Rectangle_SetFromCenterfv (&r, pos,
                                       (float)w/screen_w, (float)h/screen_h);

        SCE_SceneEntity_UseResources (sprite->entity);
        /* FIXME: redundant call to SCE_Texture_Use() */
        SCE_Texture_Blitf (&r, NULL, NULL, sprite->texture);
        SCE_SceneEntity_UnuseResources (sprite->entity);
    }
}
