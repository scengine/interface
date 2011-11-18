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
 
/* created: 03/11/2008
   updated: 10/11/2011 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include "SCE/interface/SCESceneEntity.h"

/**
 * \file SCESceneEntity.c
 * \copydoc sceneentity
 * 
 * \file SCESceneEntity.h
 * \copydoc sceneentity
 */

/**
 * \defgroup sceneentity Scene Entity
 * \ingroup interface
 * \internal
 * \brief Scene entities managment
 */

/** @{ */

static int entitylocked = SCE_FALSE;

static SCE_SShader *default_shader = NULL;

void SCE_SceneEntity_InitInstance (SCE_SSceneEntityInstance *einst)
{
    einst->node = NULL;
    einst->instance = NULL;
    einst->lod = NULL;
    einst->entity = NULL;
    einst->group = NULL;
    SCE_List_InitIt (&einst->it);
    SCE_List_SetData (&einst->it, einst);
#if 0
    SCE_List_InitIt (&einst->it2);
    SCE_List_SetData (&einst->it2, einst);
#endif
}

/**
 * \brief Creates an entity instance an initializes all its substructures
 */
SCE_SSceneEntityInstance* SCE_SceneEntity_CreateInstance (void)
{
    SCE_SSceneEntityInstance *einst = NULL;

    SCE_btstart ();
    if (!(einst = SCE_malloc (sizeof *einst)))
        goto fail;
    SCE_SceneEntity_InitInstance (einst);
    if (!(einst->node = SCE_Node_Create ()))
        goto fail;
    if (!(einst->instance = SCE_Instance_Create ()))
        goto fail;
    if (!(einst->lod = SCE_Lod_Create ()))
        goto fail;
    /* see SCE_Scene_OnNodeMoved() */
    SCE_Node_SetData (einst->node, einst);
    /* see SCE_SceneEntity_ForEachInstanceInGroup() */
    SCE_Instance_SetData (einst->instance, einst);
    /* set the matrix pointer for the instance */
    SCE_Instance_SetNode (einst->instance, einst->node);
    goto success;

fail:
    SCE_SceneEntity_DeleteInstance (einst), einst = NULL;
    SCEE_LogSrc ();
success:
    SCE_btend ();
    return einst;
}
void SCE_SceneEntity_DeleteInstance (SCE_SSceneEntityInstance *einst)
{
    if (einst) {
        SCE_List_Remove (&einst->it);
        SCE_Lod_Delete (einst->lod);
        SCE_Instance_Delete (einst->instance);
        SCE_Node_Delete (einst->node);
        SCE_free (einst);
    }
}

/**
 * \brief Duplicates an instance, just copy the node's matrix
 */
SCE_SSceneEntityInstance*
SCE_SceneEntity_DupInstance (SCE_SSceneEntityInstance *einst)
{
    SCE_SSceneEntityInstance *new = NULL;
    if (!(new = SCE_SceneEntity_CreateInstance ())) {
        SCEE_LogSrc ();
        return NULL;
    }
    /* don't copy the final matrix: may cause wrong transformations when adding
       the new instance to the parent's node of einst (for example) */
    SCE_Node_CopyMatrix (new->node, einst->node);
    return new;
}


static int SCE_SceneEntity_IsBSInFrustum (SCE_SSceneEntityInstance*,
                                          SCE_SCamera*);

void SCE_SceneEntity_InitProperties (SCE_SSceneEntityProperties *props)
{
    props->cullface = SCE_TRUE;
    props->cullmode = SCE_BACK;
    props->depthtest = SCE_TRUE;
    props->depthmode = SCE_LESS;
    props->alphatest = SCE_FALSE;
    props->depthscale = SCE_FALSE;
    props->depthrange[0] = 0.0;
    props->depthrange[1] = 1.0;
}

void SCE_SceneEntity_Init (SCE_SSceneEntity *entity)
{
    entity->igroup = NULL;

    entity->mesh = NULL;
    SCE_BoundingBox_Init (&entity->box);
    SCE_BoundingSphere_Init (&entity->sphere);
    entity->textures = NULL;
    entity->shader = NULL;
    entity->material = NULL;
    SCE_SceneEntity_InitProperties (&entity->props);

    entity->group = NULL;
    entity->isinfrustumfunc = SCE_SceneEntity_IsBSInFrustum;
    SCE_List_InitIt (&entity->it);
    SCE_List_SetData (&entity->it, entity);
    SCE_List_InitIt (&entity->it2);
    SCE_List_SetData (&entity->it2, entity);
}
SCE_SSceneEntity* SCE_SceneEntity_Create (void)
{
    SCE_SSceneEntity *entity = NULL;

    SCE_btstart ();
    if (!(entity = SCE_malloc (sizeof *entity)))
        goto failure;
    SCE_SceneEntity_Init (entity);
    if (!(entity->igroup = SCE_Instance_CreateGroup ()))
        goto failure;
    if (!(entity->textures = SCE_List_Create (NULL)))
        goto failure;
    /* NOTE: resources doesn't manage their own iterator (not yet) */
    SCE_List_CanDeleteIterators (entity->textures, SCE_TRUE);
    goto success;
failure:
    SCE_SceneEntity_Delete (entity), entity = NULL;
    SCEE_LogSrc ();
success:
    SCE_btend ();
    return entity;
}
void SCE_SceneEntity_Delete (SCE_SSceneEntity *entity)
{
    if (entity) {
        SCE_SceneEntity_RemoveEntity (entity);
        SCE_List_Remove (&entity->it);
        SCE_List_Remove (&entity->it2);
        SCE_List_Delete (entity->textures);
        SCE_Instance_DeleteGroup (entity->igroup);
        SCE_free (entity);
    }
}


void SCE_SceneEntity_InitGroup (SCE_SSceneEntityGroup *group)
{
    group->entities = NULL;
    group->n_entities = 0;
}

void SCE_SceneEntity_YouDontHaveGroup (void *entity)
{
    ((SCE_SSceneEntity*)entity)->group = NULL;
}

SCE_SSceneEntityGroup* SCE_SceneEntity_CreateGroup (void)
{
    SCE_SSceneEntityGroup *group = NULL;

    SCE_btstart ();
    if (!(group = SCE_malloc (sizeof *group)))
        goto failure;
    SCE_SceneEntity_InitGroup (group);
    if (!(group->entities = SCE_List_Create (SCE_SceneEntity_YouDontHaveGroup)))
        goto failure;
    goto success;
failure:
    SCE_SceneEntity_DeleteGroup (group), group = NULL;
    SCEE_LogSrc ();
success:
    SCE_btend ();
    return group;
}

void SCE_SceneEntity_DeleteGroup (SCE_SSceneEntityGroup *group)
{
    if (group) {
        SCE_List_Delete (group->entities);
        SCE_free (group);
    }
}


/**
 * \brief Adds an entity to a group
 * \param group an entity group
 * \param lod level of detail that the entity represent
 * \param entity the entity to add
 * \sa SCE_SceneEntity_RemoveEntity()
 */
void SCE_SceneEntity_AddEntity (SCE_SSceneEntityGroup *group, int lod,
                                SCE_SSceneEntity *entity)
{
    SCE_SListIterator *it = NULL;
    int i = 0;
    SCE_SceneEntity_RemoveEntity (entity);
    if (!SCE_List_HasElements (group->entities))
        SCE_List_Appendl (group->entities, &entity->it);
    else SCE_List_ForEach (it, group->entities) {
        if (i == lod) {
            SCE_List_Prepend (it, &entity->it);
            break;
        } else if (SCE_List_IsLast (group->entities, it)) {
            SCE_List_Appendl (group->entities, &entity->it);
            break;
        }
        i++;
    }
    entity->group = group;
    group->n_entities++;
}
/**
 * \brief Removes an entity from its group
 * \sa SCE_SceneEntity_AddEntity()
 */
void SCE_SceneEntity_RemoveEntity (SCE_SSceneEntity *entity)
{
    if (entity->group) {
        SCE_List_Removel (&entity->it);
        entity->group->n_entities--;
        entity->group = NULL;
    }
}

#if 0
/**
 * \brief Sorts the list of the entities that the given LOD group contains
 *
 * This function sorts the list of the entities that \p lgroup contains then
 * the first element is the first level of detail based on the number of
 * vertices of each entity's mesh.
 */
void SCE_SceneEntity_SortGroup (SCE_SSceneEntityLODGroup *group)
{
    int changefunc (SCE_SListIterator *it, SCE_SListIterator *it2) {
#define gnf SCE_Mesh_GetNumFaces
        return (gnf (((SCE_SSceneEntity*)SCE_List_GetData (it))->mesh)
                > gnf (((SCE_SSceneEntity*)SCE_List_GetData (it2))->mesh));
#undef gnf
    }
    SCE_List_Sortl (group->entities, changefunc);
}
#endif

unsigned int SCE_SceneEntity_GetGroupNumEntities (SCE_SSceneEntityGroup *g)
{
    return g->n_entities;
}
SCE_SList* SCE_SceneEntity_GetGroupEntitiesList (SCE_SSceneEntityGroup *g)
{
    return g->entities;
}


/**
 * \brief Setup instance informations, like bounding box, from an entity
 *
 * Copies informations from an entity to an instance:
 * - bounding box;
 * - bounding sphere.
 * This function is useful when you want to add an instance to an entity
 * directly, ie. not using an entity group.
 */
void SCE_SceneEntity_SetInstanceDataFromEntity (SCE_SSceneEntityInstance *einst,
                                                SCE_SSceneEntity *entity)
{
    SCE_Node_GetElement (einst->node)->sphere = &entity->sphere;
    SCE_Lod_SetBoundingBox (einst->lod, &entity->box);
}

/**
 * \brief Adds an instance to an entity group
 * \param group the group where add the instance (can be NULL)
 * \param einst the instance to add
 *
 * If \p group is NULL, \p einst is added to the previous group where it was
 * and \p einst->instance is added to the previous geometry group where it was,
 * if \p einst was never added to a group, calling this function with
 * \p group NULL generates a segmentation fault (in the better case).
 * Specific data of the first \p group 's entity are assigned to \p einst by
 * calling SCE_SceneEntity_SetInstanceDataFromEntity().
 * \sa SCE_SceneEntity_RemoveInstance(), SCE_Instance_AddInstance(),
 * SCE_SceneEntity_SelectInstance(), SCE_SceneEntity_SetInstanceDataFromEntity()
 */
void SCE_SceneEntity_AddInstance (SCE_SSceneEntityGroup *group,
                                  SCE_SSceneEntityInstance *einst)
{
    SCE_SSceneEntity *entity =
        SCE_List_GetData (SCE_List_GetFirst (group->entities));

    einst->group = group;
    SCE_SceneEntity_AddInstanceToEntity (entity, einst);
    SCE_SceneEntity_SetInstanceDataFromEntity (einst, entity);
}

/**
 * \brief Defines the entity of the given instance and adds its geometry
 * instance to the geometry group of \p entity
 * \sa SCE_Instance_AddInstance(), SCE_SceneEntity_ReplaceInstanceToEntity(),
 * SCE_SceneEntity_RemoveInstanceFromEntity()
 */
void SCE_SceneEntity_AddInstanceToEntity (SCE_SSceneEntity *entity,
                                          SCE_SSceneEntityInstance *einst)
{
    SCE_Instance_AddInstance (entity->igroup, einst->instance);
    einst->entity = entity;
}
/**
 * \brief Replaces an instance into its previous entity
 * \sa SCE_SceneEntity_AddInstanceToEntity(),
 * SCE_SceneEntity_RemoveInstanceFromEntity()
 */
void SCE_SceneEntity_ReplaceInstanceToEntity (SCE_SSceneEntityInstance *einst)
{
    SCE_Instance_AddInstance (einst->entity->igroup, einst->instance);
}
/**
 * \brief Removes the geometry instance of the given entity instance from its
 * geometry group
 * \sa SCE_SceneEntity_AddInstanceToEntity(),
 * SCE_SceneEntity_ReplaceInstanceToEntity()
 */
void SCE_SceneEntity_RemoveInstanceFromEntity (SCE_SSceneEntityInstance *einst)
{
    SCE_Instance_RemoveInstance (einst->instance);
}

/**
 * \brief Flushs the instances list of the group of \p entity
 * \sa SCE_Instance_FlushInstancesList(), SCE_SSceneEntity::igroup
 */
void SCE_SceneEntity_Flush (SCE_SSceneEntity *entity)
{
    SCE_Instance_FlushInstancesList (entity->igroup);
}

/**
 * \brief Gets the node of the given instance
 */
SCE_SNode* SCE_SceneEntity_GetInstanceNode (SCE_SSceneEntityInstance *einst)
{
    return einst->node;
}
/**
 * \brief Gets the type of the node of an entity instance
 */
SCE_ENodeType
SCE_SceneEntity_GetInstanceNodeType (SCE_SSceneEntityInstance *einst)
{
    return SCE_Node_GetType (einst->node);
}
/**
 * \brief Gets the geometry instance of the given instance
 */
SCE_SGeometryInstance*
SCE_SceneEntity_GetInstanceInstance (SCE_SSceneEntityInstance *einst)
{
    return einst->instance;
}
SCE_SOctreeElement*
SCE_SceneEntity_GetInstanceElement (SCE_SSceneEntityInstance *einst)
{
    return SCE_Node_GetElement (einst->node);
}
/**
 * \brief Gets the "level of detail" structure of the given instance
 */
SCE_SLevelOfDetail*
SCE_SceneEntity_GetInstanceLOD (SCE_SSceneEntityInstance *einst)
{
    return einst->lod;
}
/**
 * \brief Returns the entity group of an instance
 */
SCE_SSceneEntityGroup*
SCE_SceneEntity_GetInstanceGroup (SCE_SSceneEntityInstance *einst)
{
    return einst->group;
}
/**
 * \brief Gets the first iterator of an instance (for scene manager)
 */
SCE_SListIterator*
SCE_SceneEntity_GetInstanceIterator1 (SCE_SSceneEntityInstance *einst)
{
    return &einst->it;
}
#if 0
/**
 * \brief Gets the second iterator of an instance (for models)
 */
SCE_SListIterator*
SCE_SceneEntity_GetInstanceIterator2 (SCE_SSceneEntityInstance *einst)
{
    return &einst->it2;
}
#endif

/**
 * \brief Gets the entity of an instance
 */
SCE_SSceneEntity*
SCE_SceneEntity_GetInstanceEntity (SCE_SSceneEntityInstance *einst)
{
    return einst->entity;
}

/**
 * \brief Gets the bounding box of an instance
 */
SCE_SBoundingBox*
SCE_SceneEntity_GetInstanceBB (SCE_SSceneEntityInstance *einst)
{
    return &einst->entity->box;
}

/**
 * \brief Calls SCE_BoundingBox_Push() and SCE_BoundingBox_MakePlanes()
 * on the bounding box of the given instance
 * \param tmp used for SCE_BoundingBox_Push()
 * \returns the bounding box of \p einst
 * \sa SCE_SceneEntity_PopInstanceBB()
 */
SCE_SBoundingBox*
SCE_SceneEntity_PushInstanceBB (SCE_SSceneEntityInstance *einst, SCE_SBox *tmp)
{
    SCE_SBoundingBox *box = &einst->entity->box;
    /* NOTE: conversion from Matrix4 to Matrix4x3 */
    SCE_BoundingBox_Push (box, SCE_Node_GetFinalMatrix (einst->node), tmp);
    SCE_BoundingBox_MakePlanes (box);
    return box;
}
/**
 * \brief Calls SCE_BoundingBox_Pop() on the bounding box of the given instance
 * \param tmp used for SCE_BoundingBox_Pop()
 * \sa SCE_SceneEntity_PushInstanceBB()
 */
void SCE_SceneEntity_PopInstanceBB (SCE_SSceneEntityInstance *einst,
                                    SCE_SBox *tmp)
{
    SCE_BoundingBox_Pop (&einst->entity->box, tmp);
}

/**
 * \brief Returns user data
 */
void* SCE_SceneEntity_GetInstanceData (SCE_SSceneEntityInstance *einst)
{
    return einst->udata;
}
/**
 * \brief Returns user data
 */
void SCE_SceneEntity_SetInstanceData (SCE_SSceneEntityInstance *einst,
                                      void *data)
{
    einst->udata = data;
}


/**
 * \brief Gets the rendering properties of an entity
 * \sa SCE_SSceneEntityProperties
 */
SCE_SSceneEntityProperties*
SCE_SceneEntity_GetProperties (SCE_SSceneEntity *entity)
{
    return &entity->props;
}
/**
 * \brief Defines the mesh of a scene entity
 * \returns SCE_ERROR on error, SCE_OK otherwise
 *
 * Assigns (without duplicate) \p mesh to \p entity.
 * This function creates a bounding box for \p mesh and store it into
 * \p entity.
 * \sa SCE_SSceneEntity
 */
void SCE_SceneEntity_SetMesh (SCE_SSceneEntity *entity, SCE_SMesh *mesh)
{
    entity->mesh = mesh;
    SCE_Instance_SetGroupMesh (entity->igroup, mesh);
    if (!mesh) {
        SCE_BoundingBox_Init (&entity->box);
        SCE_BoundingSphere_Init (&entity->sphere);
    } else {
        SCE_SGeometry *geom = SCE_Mesh_GetGeometry (mesh);
        SCE_Geometry_GenerateBoundingVolumes (geom);
        SCE_BoundingBox_SetFrom (&entity->box,
                                 SCE_Geometry_GetBox (geom));
        SCE_BoundingSphere_SetFrom (&entity->sphere,
                                    SCE_Geometry_GetSphere (geom));
    }
}

/**
 * \brief Gets the mesh assigned to the given entity
 *
 * This function returns the meshs assigned to \p entity by a previous call of
 * SCE_SceneEntity_SetMesh().
 */
SCE_SMesh* SCE_SceneEntity_GetMesh (SCE_SSceneEntity *entity)
{
    return entity->mesh;
}


/**
 * \brief Adds a texture to an entity
 * \param entity the entity to which add the texture
 * \param tex the texture to add to \p entity
 * \returns SCE_ERROR on error, SCE_OK otherwise
 *
 * When this function is called, the entity \p entity becomes a new owner of
 * \p r and \p r is stored into entity->textures. If \p r is NULL, the
 * comportement of this function is undefined.
 * \sa SCE_SceneEntity_RemoveTexture(), SCE_SceneEntity_SetShader(),
 * SCE_SceneEntity_SetMaterial(), SCE_SceneResource_AddOwner()
 */
int SCE_SceneEntity_AddTexture (SCE_SSceneEntity *entity, SCE_STexture *tex)
{
    SCE_SSceneResource *r = SCE_Texture_GetSceneResource (tex);
    /* TODO: dirty isn't it? */
    if (SCE_List_PrependNewl (entity->textures, r) < 0)
        goto fail;
    if (SCE_SceneResource_AddOwner (r, entity) < 0)
        goto fail;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}
/**
 * \brief Removes a texture from an entity
 * \param entity the entity from which remove the texture
 * \param tex the texture to remove from \p entity
 *
 * Removes a texture already added to \p entity by calling
 * SCE_SceneEntity_AddTexture().
 * \sa SCE_SceneEntity_AddTexture(), SCE_SceneResource_RemoveOwner()
 */
void SCE_SceneEntity_RemoveTexture (SCE_SSceneEntity *entity, SCE_STexture *tex)
{
    SCE_SSceneResource *r = NULL;
    r = SCE_Texture_GetSceneResource (tex);
    SCE_List_EraseFromData (entity->textures, r);
    SCE_SceneResource_RemoveOwner (r, entity);
}
/**
 * \brief Gets the list of the textures used by the given entity
 * \sa SCE_SceneEntity_AddTexture()
 */
SCE_SList* SCE_SceneEntity_GetTexturesList (SCE_SSceneEntity *entity)
{
    return entity->textures;
}

/**
 * \brief Defines the shader used by an entity
 * \param entity the entity that will use the shader
 * \param s the shader to use for \p entity
 * \returns SCE_ERROR on error, SCE_OK otherwise
 *
 * If \p r is NULL, \p entity don't use any shader and this function returns
 * SCE_OK
 * \sa SCE_SceneEntity_SetMaterial(), SCE_SceneEntity_AddTexture(),
 * SCE_SceneResource_AddOwner(), SCE_SceneResource_RemoveOwner(),
 * SCE_SceneEntity_GetShader()
 */
int SCE_SceneEntity_SetShader (SCE_SSceneEntity *entity, SCE_SShader *s)
{
    if (entity->shader)
        SCE_SceneResource_RemoveOwner (entity->shader, entity);
    entity->shader = SCE_Shader_GetSceneResource (s);
    if (entity->shader) {
        if (SCE_SceneResource_AddOwner (entity->shader, entity) < 0) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
    }
    return SCE_OK;
}
/**
 * \brief Gets the shader used by the given entity
 * \sa SCE_SceneEntity_SetShader()
 */
SCE_SSceneResource* SCE_SceneEntity_GetShader (SCE_SSceneEntity *entity)
{
    return entity->shader;
}

/**
 * \brief Defines the material used by an entity
 * \param entity the entity that will use the material
 * \param mat the material to use for \p entity
 * \returns SCE_ERROR on error, SCE_OK otherwise
 *
 * If \p r is NULL, \p entity don't use any material and this function returns
 * SCE_OK
 * \sa SCE_SceneEntity_SetShader(), SCE_SceneEntity_AddTexture(),
 * SCE_SceneResource_AddOwner(), SCE_SceneResource_RemoveOwner(),
 * SCE_SceneEntity_GetMaterial()
 */
int SCE_SceneEntity_SetMaterial (SCE_SSceneEntity *entity, SCE_SMaterial *mat)
{
    if (entity->material)
        SCE_SceneResource_RemoveOwner (entity->material, entity);
    entity->material = SCE_Material_GetSceneResource (mat);
    if (entity->material) {
        if (SCE_SceneResource_AddOwner (entity->material, entity) < 0) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
    }
    return SCE_OK;
}
/**
 * \brief Gets the material used by the given entity
 * \sa SCE_SceneEntity_SetMaterial()
 */
SCE_SSceneResource* SCE_SceneEntity_GetMaterial (SCE_SSceneEntity *entity)
{
    return entity->material;
}

/**
 * \brief Indicates if an entity have a resource of the given type \p type
 * \sa SCE_SceneResource_GetType()
 */
int SCE_SceneEntity_HasResourceOfType (SCE_SSceneEntity *entity, int type)
{
    SCE_SListIterator *it;
    SCE_List_ForEach (it, entity->textures) {
        if (SCE_SceneResource_GetType (SCE_List_GetData (it)) == type)
            return SCE_TRUE;
    }
    if ((entity->shader && SCE_SceneResource_GetType(entity->shader) == type) ||
      (entity->material && SCE_SceneResource_GetType(entity->material) == type))
        return SCE_TRUE;
    return SCE_FALSE;
}

/**
 * \brief Indicates if an entity have any instance
 */
int SCE_SceneEntity_HasInstance (SCE_SSceneEntity *entity)
{
    return SCE_Instance_HasInstances (entity->igroup);
}

/**
 * \brief Gets the geometry instances group of an entity
 */
SCE_SGeometryInstanceGroup*
SCE_SceneEntity_GetInstancesGroup (SCE_SSceneEntity *entity)
{
    return entity->igroup;
}
/**
 * \brief Gets the public iterator of an entity (used in the scene manager)
 */
SCE_SListIterator* SCE_SceneEntity_GetIterator (SCE_SSceneEntity *entity)
{
    return &entity->it2;
}


static int SCE_SceneEntity_IsBBInFrustum (SCE_SSceneEntityInstance *einst,
                                          SCE_SCamera *cam)
{
    int result;
    SCE_SBox saved;
    SCE_SBoundingBox *box = &einst->entity->box;

    /* NOTE: conversion from Matrix4 to Matrix4x3 */
    SCE_BoundingBox_Push (box, SCE_Node_GetFinalMatrix (einst->node), &saved);
    SCE_BoundingBox_MakePlanes (box); /* very important. */
    result = SCE_Frustum_BoundingBoxInBool (SCE_Camera_GetFrustum (cam), box);
    SCE_BoundingBox_Pop (box, &saved);

    return result;
}
static int SCE_SceneEntity_IsBSInFrustum (SCE_SSceneEntityInstance *einst,
                                          SCE_SCamera *cam)
{
    int result;
    SCE_SSphere saved;
    SCE_SBoundingSphere *sphere = &einst->entity->sphere;

    /* NOTE: conversion from Matrix4 to Matrix4x3 */
    /* node matrix will be MAtrix4x3 soon */
    SCE_BoundingSphere_Push (sphere, SCE_Node_GetFinalMatrix (einst->node),
                             &saved);
    result = SCE_Frustum_BoundingSphereInBool (SCE_Camera_GetFrustum (cam),
                                               sphere);
    SCE_BoundingSphere_Pop (sphere, &saved);

    return result;
}
/**
 * \brief Defines the bounding volume to use for frustum culling
 * (frustum culling only for now)
 * \param entity an entity
 * \param volume the bounding volume to use, can be SCE_BOUNDINGBOX or
 * SCE_BOUNDINGSPHERE
 * \todo maybe rename it to SetupFrustumCullingBoundingVolume
 */
void SCE_SceneEntity_SetupBoundingVolume (SCE_SSceneEntity *entity, int volume)
{
    switch (volume) {
    case SCE_BOUNDINGBOX:
        entity->isinfrustumfunc = SCE_SceneEntity_IsBBInFrustum; break;
    case SCE_BOUNDINGSPHERE:
        entity->isinfrustumfunc = SCE_SceneEntity_IsBSInFrustum;
    }
}


/**
 * \brief Computes the LOD for an entity instance and add its geometry instance
 * to the corresponding group
 * 
 * Computes the LOD for an entity instance and add it to the corresponding
 * entity in the \p einst's entity group. \p einst->entity must be in a
 * group structure (SCE_SSceneEntityGroup).
 */
void SCE_SceneEntity_DetermineInstanceLOD (SCE_SSceneEntityInstance *einst,
                                           SCE_SCamera *cam)
{
    /* FIXME: types conflicts together */
    int entityid, lod;
    SCE_SSceneEntity *entity = NULL;
    SCE_SSceneEntityGroup *group = einst->group;

    SCE_Lod_Compute (einst->lod, SCE_Node_GetFinalMatrix (einst->node), cam);
    /* get max LOD */
    lod = SCE_Lod_GetLOD (einst->lod);
    entityid = (lod >= group->n_entities ? group->n_entities - 1 : lod);
    /* add instance to the 'groupid' group */
    entity = SCE_List_GetData (SCE_List_GetIterator (group->entities,entityid));
    SCE_SceneEntity_AddInstanceToEntity (entity, einst);
}
/**
 * \brief Determines if an instance is in the given frustum (returns a boolean)
 * \sa SCE_Frustum_BoundingBoxInBool(), SCE_Frustum_BoundingSphereInBool(),
 * SCE_SceneEntity_SetupBoundingVolume()
 */
int SCE_SceneEntity_IsInstanceInFrustum (SCE_SSceneEntityInstance *einst,
                                         SCE_SCamera *cam)
{
    return einst->entity->isinfrustumfunc (einst, cam);
}


/**
 * \brief Applies the properties of an entity by calling SCE_RSetState()
 */
void SCE_SceneEntity_ApplyProperties (SCE_SSceneEntity *entity)
{
    SCE_RSetState (GL_CULL_FACE, entity->props.cullface);
    SCE_RSetCulledFaces (entity->props.cullmode);
    SCE_RSetState (GL_DEPTH_TEST, entity->props.depthtest);
    SCE_RSetValidPixels (entity->props.depthmode);
    SCE_RSetState (GL_ALPHA_TEST, entity->props.alphatest);
    if (entity->props.depthscale)
        SCE_RDepthRange (entity->props.depthrange[0],
                         entity->props.depthrange[1]);
}

/**
 * \brief Enables the resources that the given entity is using
 * \param entity an entity
 *
 * This function calls SCE_Shader_Use(), SCE_Texture_Use() and
 * SCE_Material_Use(), respectively, for the shader, the textures and the
 * material that \p entity have.
 */
void SCE_SceneEntity_UseResources (SCE_SSceneEntity *entity)
{
    SCE_SListIterator *it = NULL;

    if (entitylocked)
        return;

    SCE_SceneEntity_ApplyProperties (entity);

    if (entity->material)
        SCE_Material_Use (SCE_SceneResource_GetResource (entity->material));
    else
        SCE_Material_Use (NULL);

    if (entity->shader)
        SCE_Shader_Use (SCE_SceneResource_GetResource (entity->shader));
    else
        SCE_Shader_Use (default_shader);

    SCE_Texture_BeginLot ();
    SCE_List_ForEach (it, entity->textures)
        SCE_Texture_Use (SCE_SceneResource_GetResource (SCE_List_GetData (it)));
    SCE_Texture_EndLot ();
}
void SCE_SceneEntity_UnuseResources (SCE_SSceneEntity *entity)
{
    /* reset shitty states */
    if (entity->props.depthscale)
        SCE_RDepthRange (0.0, 1.0);
}
/**
 * \brief Sets the default shader that will be used if none is specified
 * \param shader a shader
 * \sa SCE_SceneEntity_UseResources()
 */
void SCE_SceneEntity_SetDefaultShader (SCE_SShader *shader)
{
    default_shader = shader;
}


/**
 * \brief Any further call to SCE_SceneEntity_UseResources() is ignored
 * \sa SCE_SceneEntity_Unlock()
 */
void SCE_SceneEntity_Lock (void)
{
    entitylocked = SCE_TRUE;
}
/**
 * \brief Further calls to SCE_SceneEntity_UseResources() are no longer ignored
 * \sa SCE_SceneEntity_Lock()
 */
void SCE_SceneEntity_Unlock (void)
{
    entitylocked = SCE_FALSE;
}


/**
 * \brief Render all the instances of the given entity
 */
void SCE_SceneEntity_Render (SCE_SSceneEntity *entity)
{
    SCE_Instance_RenderGroup (entity->igroup);
}

/** @} */
