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
 
/* created: 03/11/2008
   updated: 12/07/2009 */

#ifndef SCESCENERESOURCE_H
#define SCESCENERESOURCE_H

#include <SCE/utils/SCEUtils.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sce_ssceneresourcegroup SCE_SSceneResourceGroup;

typedef struct sce_ssceneresource SCE_SSceneResource;
struct sce_ssceneresource {
    void *resource;                 /**< Resource */
    SCE_SList owners;               /**< Objects using this resource */
    SCE_SSceneResourceGroup *group; /**< Group if this resource */
    int removed;                    /**< Is it removed from its group? */
    SCE_SListIterator it;
};

struct sce_ssceneresourcegroup {
    SCE_SList *resources;       /**< Resources of this group */
    int type;                   /**< Type of the resources */
};

void SCE_SceneResource_Init (SCE_SSceneResource*);
SCE_SSceneResource* SCE_SceneResource_Create (void);
void SCE_SceneResource_Delete (SCE_SSceneResource*);

void SCE_SceneResource_InitGroup (SCE_SSceneResourceGroup*);
SCE_SSceneResourceGroup* SCE_SceneResource_CreateGroup (void);
void SCE_SceneResource_DeleteGroup (SCE_SSceneResourceGroup*);

void SCE_SceneResource_SetGroupType (SCE_SSceneResourceGroup*, int);
int SCE_SceneResource_GetGroupType (SCE_SSceneResourceGroup*);

int SCE_SceneResource_GetType (SCE_SSceneResource*);

void SCE_SceneResource_AddResource (SCE_SSceneResourceGroup*,
                                    SCE_SSceneResource*);
void SCE_SceneResource_RemoveResource (SCE_SSceneResource*);

SCE_SList* SCE_SceneResource_GetResourcesList (SCE_SSceneResourceGroup*);

void SCE_SceneResource_SetResource (SCE_SSceneResource*, void*);
void* SCE_SceneResource_GetResource (SCE_SSceneResource*);

int SCE_SceneResource_AddOwner (SCE_SSceneResource*, void*);
void SCE_SceneResource_RemoveOwner (SCE_SSceneResource*, void*);

SCE_SList* SCE_SceneResource_GetOwnersList (SCE_SSceneResource*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
