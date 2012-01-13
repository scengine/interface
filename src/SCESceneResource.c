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
 
/* created: 03/11/2008
   updated: 13/01/2012 */

#include <SCE/utils/SCEUtils.h>
#include "SCE/interface/SCESceneResource.h"


void SCE_SceneResource_Init (SCE_SSceneResource *res)
{
    res->resource = NULL;
    SCE_List_Init (&res->owners);
    SCE_List_CanDeleteIterators (&res->owners, SCE_TRUE);
    res->group = NULL;
    res->removed = SCE_TRUE;
    SCE_List_InitIt (&res->it);
    SCE_List_SetData (&res->it, res);
}

SCE_SSceneResource* SCE_SceneResource_Create (void)
{
    SCE_SSceneResource *res = NULL;
    if (!(res = SCE_malloc (sizeof *res)))
        SCEE_LogSrc ();
    else
        SCE_SceneResource_Init (res);
    return res;
}

void SCE_SceneResource_Delete (SCE_SSceneResource *res)
{
    if (res) {
        SCE_SceneResource_RemoveResource (res);
        SCE_List_Clear (&res->owners);
        SCE_free (res);
    }
}


void SCE_SceneResource_InitGroup (SCE_SSceneResourceGroup *group)
{
    group->resources = NULL;
    group->type = 0;
}

/* used for SCE_List_Create() */
static void SCE_SceneResource_YouDontHaveGroup (void *r)
{
    SCE_SSceneResource *res = r;
    res->group = NULL;
    res->removed = SCE_TRUE;
}

SCE_SSceneResourceGroup* SCE_SceneResource_CreateGroup (void)
{
    SCE_SSceneResourceGroup *group = NULL;

    if (!(group = SCE_malloc (sizeof *group)))
        goto fail;
    SCE_SceneResource_InitGroup (group);
    /* don't delete the resources on group deletion */
    if (!(group->resources = SCE_List_Create (
              SCE_SceneResource_YouDontHaveGroup)))
        goto fail;

    return group;
fail:
    SCE_SceneResource_DeleteGroup (group);
    SCEE_LogSrc ();
    return NULL;
}

void SCE_SceneResource_DeleteGroup (SCE_SSceneResourceGroup *group)
{
    if (group) {
        SCE_List_Delete (group->resources);
        SCE_free (group);
    }
}

/**
 * \brief Sets the type of a group
 */
void SCE_SceneResource_SetGroupType (SCE_SSceneResourceGroup *group, int type)
{
    group->type = type;
}

/**
 * \brief Gets the type of the given group
 */
int SCE_SceneResource_GetGroupType (SCE_SSceneResourceGroup *group)
{
    return group->type;
}

/**
 * \brief Gets the type of the group of the given resource
 */
int SCE_SceneResource_GetType (SCE_SSceneResource *res)
{
    return res->group->type;
}


/**
 * \brief Tells whether a resource belongs to a group or not
 * \param res a resource
 * \return SCE_TRUE if \p res belongs to a group, SCE_FALSE otherwise
 */
int SCE_SceneResource_IsResourceAdded (SCE_SSceneResource *res)
{
    return !res->removed;
}

/**
 * \brief Adds a resource into a group
 * \param group the group where add the resource (can be NULL)
 * \param res the resource to add
 *
 * If \p group is NULL, \p res is added to the previous group where it was,
 * if \p res was never added to a group, calling this function with \p group
 * NULL generates a segmentation fault (in the better case).
 * \sa SCE_SceneResource_RemoveResource()
 */
void SCE_SceneResource_AddResource (SCE_SSceneResourceGroup *group,
                                    SCE_SSceneResource *res)
{
    if (!group || group == res->group)
    {
        if (res->removed) /* in this case, res should have a group... */
            SCE_List_Prependl (res->group->resources, &res->it);
    }
    else
    {
        SCE_SceneResource_RemoveResource (res);
        SCE_List_Prependl (group->resources, &res->it);
        res->group = group;
    }
    res->removed = SCE_FALSE;
}
/**
 * \brief Removes a resource from its current group
 * \param res the resource to remove from
 * \sa SCE_SceneResource_AddResource()
 */
void SCE_SceneResource_RemoveResource (SCE_SSceneResource *res)
{
    if (!res->removed)
    {
        SCE_List_Removel (&res->it);
        res->removed = SCE_TRUE;
    }
}

/**
 * \brief Gets the list of the resource of a resources group
 */
SCE_SList* SCE_SceneResource_GetResourcesList (SCE_SSceneResourceGroup *group)
{
    return group->resources;
}

void SCE_SceneResource_SetResource (SCE_SSceneResource *res, void *resource)
{
    res->resource = resource;
}
void* SCE_SceneResource_GetResource (SCE_SSceneResource *res)
{
    return res->resource;
}

int SCE_SceneResource_AddOwner (SCE_SSceneResource *res, void *owner)
{
    if (SCE_List_PrependNewl (&res->owners, owner) < 0)
    {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}
void SCE_SceneResource_RemoveOwner (SCE_SSceneResource *res, void *owner)
{
    SCE_List_EraseFromData (&res->owners, owner);
}

SCE_SList* SCE_SceneResource_GetOwnersList (SCE_SSceneResource *res)
{
    return &res->owners;
}
