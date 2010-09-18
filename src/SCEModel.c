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

/* created: 27/06/2009
   updated: 06/09/2009 */

#include <SCE/utils/SCEUtils.h>
#include "SCE/interface/SCEModel.h"


void SCE_Model_InitInstance (SCE_SModelInstance *minst)
{
    minst->n = 0;
    minst->inst = NULL;
    SCE_List_InitIt (&minst->it);
    SCE_List_SetData (&minst->it, minst);
}
SCE_SModelInstance* SCE_Model_CreateInstance (void)
{
    SCE_SModelInstance *minst = SCE_malloc (sizeof *minst);
    if (!minst)
        SCEE_LogSrc ();
    else
        SCE_Model_InitInstance (minst);
    return minst;
}
SCE_SModelInstance* SCE_Model_CreateInstance2 (void)
{
    SCE_SModelInstance *minst = SCE_malloc (sizeof *minst);
    if (!minst)
        SCEE_LogSrc ();
    else {
        SCE_Model_InitInstance (minst);
        if (!(minst->inst = SCE_SceneEntity_CreateInstance ())) {
            SCE_Model_DeleteInstance (minst), minst = NULL;
            SCEE_LogSrc ();
        }
    } 
    return minst;
}
void SCE_Model_DeleteInstance (SCE_SModelInstance *minst)
{
    if (minst) {
        SCE_List_Remove (&minst->it);
        SCE_SceneEntity_DeleteInstance (minst->inst);
        SCE_free (minst);
    }
}
SCE_SModelInstance* SCE_Model_DupInstance (SCE_SModelInstance *src)
{
    SCE_SModelInstance *minst = NULL;
    minst = SCE_Model_CreateInstance ();
    if (!minst)
        SCEE_LogSrc ();
    else {
        minst->n = src->n;
        if (!(minst->inst = SCE_SceneEntity_DupInstance (src->inst))) {
            SCE_Model_DeleteInstance (minst), minst = NULL;
            SCEE_LogSrc ();
        }
    }
    return minst;
}


static void SCE_Model_InitEntity (SCE_SModelEntity *entity)
{
    entity->entity = NULL;
    entity->is_instance = SCE_TRUE;
    SCE_List_InitIt (&entity->it);
    SCE_List_SetData (&entity->it, entity);
}
static void SCE_Model_DeleteEntity (SCE_SModelEntity*);
static SCE_SModelEntity* SCE_Model_CreateEntity (SCE_SSceneEntity *e)
{
    SCE_SModelEntity *entity = NULL;
    if (!(entity = SCE_malloc (sizeof *entity)))
        goto fail;
    SCE_Model_InitEntity (entity);
    if (!e) {
        if (!(e = SCE_SceneEntity_Create ()))
            goto fail;
        entity->is_instance = SCE_FALSE;
    }
    entity->entity = e;
    SCE_btend ();
    return entity;
fail:
    SCE_Model_DeleteEntity (entity);
    SCEE_LogSrc ();
    SCE_btend ();
    return NULL;
}
static void SCE_Model_DeleteEntity (SCE_SModelEntity *entity)
{
    if (entity) {
        if (!entity->is_instance)
            SCE_SceneEntity_Delete (entity->entity);
        SCE_free (entity);
    }
}

static void SCE_Model_InitEntityGroup (SCE_SModelEntityGroup *group)
{
    group->group = NULL;
    group->is_instance = SCE_TRUE;
    group->num = 0;
    SCE_List_InitIt (&group->it);
    SCE_List_SetData (&group->it, group);
}
static void SCE_Model_DeleteEntityGroup (SCE_SModelEntityGroup*);
static SCE_SModelEntityGroup*
SCE_Model_CreateEntityGroup (SCE_SSceneEntityGroup *g)
{
    SCE_SModelEntityGroup *group = NULL;
    if (!(group = SCE_malloc (sizeof *group)))
        goto fail;
    SCE_Model_InitEntityGroup (group);
    if (!g) {
        if (!(g = SCE_SceneEntity_CreateGroup ()))
            goto fail;
        group->is_instance = SCE_FALSE;
    }
    group->group = g;
    return group;
fail:
    SCE_Model_DeleteEntityGroup (group);
    SCEE_LogSrc ();
    return NULL;
}
static void SCE_Model_DeleteEntityGroup (SCE_SModelEntityGroup *group)
{
    if (group) {
        if (!group->is_instance) {
            SCE_SceneEntity_DeleteGroup (group->group);
            SCE_free (group);
        }
    }
}

static void SCE_Model_Init (SCE_SModel *mdl)
{
    unsigned int i;
    SCE_List_Init (&mdl->entities);
    SCE_List_SetFreeFunc (&mdl->entities,
                          (SCE_FListFreeFunc)SCE_Model_DeleteEntity);
    SCE_List_Init (&mdl->groups);
    SCE_List_SetFreeFunc (&mdl->groups,
                          (SCE_FListFreeFunc)SCE_Model_DeleteEntityGroup);
    SCE_List_Init (&mdl->instances);
    SCE_List_SetFreeFunc (&mdl->instances,
                          (SCE_FListFreeFunc)SCE_Model_DeleteInstance);
    mdl->root_node = NULL;
    mdl->root_node_instance = SCE_FALSE; /* SCE_TRUE.. ? */
    mdl->type = SCE_MODEL_ROOT;
    mdl->udata = NULL;
}
SCE_SModel* SCE_Model_Create (void)
{
    SCE_SModel *mdl = NULL;
    if (!(mdl = SCE_malloc (sizeof *mdl)))
        SCEE_LogSrc ();
    else {
        SCE_Model_Init (mdl);
        if (!(mdl->root_node = SCE_Node_Create ())) {
            SCEE_LogSrc ();
            SCE_free (mdl);
            return NULL;
        }
    }
    return mdl;
}
void SCE_Model_Delete (SCE_SModel *mdl)
{
    if (mdl) {
        SCE_List_Clear (&mdl->instances);
        if (mdl->type != SCE_MODEL_HARD_INSTANCE) {
            SCE_List_Clear (&mdl->entities);
            SCE_List_Clear (&mdl->groups);
        }
        if (!mdl->root_node_instance)
            SCE_Node_Delete (mdl->root_node);
        SCE_free (mdl);
    }
}


/**
 * \brief Specify user data
 */
void SCE_Model_SetData (SCE_SModel *mdl, void *data)
{
    mdl->udata = data;
}
/**
 * \brief Returns user data
 */
void* SCE_Model_GetData (SCE_SModel *mdl)
{
    return mdl->udata;
}

static void SCE_Model_BuildEntityv (SCE_SModelEntity *entity, SCE_SMesh *mesh,
                                    SCE_SShader *shader, SCE_STexture **texs)
{
    SCE_SceneEntity_SetMesh (entity->entity, mesh);
    if (shader)
        SCE_SceneEntity_SetShader (entity->entity, shader);
    if (texs) {
        unsigned int i = 0;
        for (i = 0; texs[i]; i++)
            SCE_SceneEntity_AddTexture (entity->entity, texs[i]);
    }
}


static SCE_SModelEntityGroup*
SCE_Model_LocateEntityGroup (SCE_SModel *mdl, int num)
{
    SCE_SModelEntityGroup *group = NULL;
    SCE_SListIterator *it = NULL;
    SCE_List_ForEach (it, &mdl->groups) {
        group = SCE_List_GetData (it);
        if (group->num == num)
            return group;
    }
    return NULL;
}

#define SCE_CHECK_LEVEL(level) do {                                     \
        if (level >= SCE_MAX_MODEL_ENTITIES) {                          \
            SCEE_Log (SCE_INVALID_ARG);                                 \
            SCEE_LogMsg ("parameter 'level' is too high (%d), maximum is %u",\
                         level, SCE_MAX_MODEL_ENTITIES);                \
            return SCE_ERROR;                                           \
        }                                                               \
    } while (0)

/**
 * \brief Creates and adds a entity to a model
 * \returns SCE_ERROR on error, SCE_OK otherwise
 *
 * If \p level is lesser than 0, then using the latest level of detail.
 */
int SCE_Model_AddNewEntityv (SCE_SModel *mdl, SCEuint num, SCEuint level,
                             SCE_SMesh *mesh, SCE_SShader *shader,
                             SCE_STexture **tex)
{
    SCE_SModelEntity *entity = NULL;
    SCE_SModelEntityGroup *mgroup = NULL;

    if (!(mgroup = SCE_Model_LocateEntityGroup (mdl, num))) {
        if (!(mgroup = SCE_Model_CreateEntityGroup (NULL)))
            goto fail;
        mgroup->num = num;
        SCE_List_Appendl (&mdl->groups, &mgroup->it);
    }
    if (!(entity = SCE_Model_CreateEntity (NULL)))
        goto fail;
    SCE_Model_BuildEntityv (entity, mesh, shader, tex);
    SCE_List_Appendl (&mdl->entities, &entity->it);
    SCE_SceneEntity_AddEntity (mgroup->group, level, entity->entity);

    return SCE_OK;
fail:
    SCE_Model_DeleteEntity (entity);
    SCEE_LogSrc ();
    return SCE_ERROR;
}
/**
 * \brief Creates and adds a new entity to a model
 * \sa SCE_Model_AddNewEntityv() SCE_Model_AddEntity()
 */
int SCE_Model_AddNewEntity (SCE_SModel *mdl, SCEuint num, SCEuint level,
                            SCE_SMesh *mesh, SCE_SShader *shader, ...)
{
    va_list args;
    int code;
    unsigned int i = 0;
    SCE_STexture *tex = NULL;
    SCE_STexture *textures[42];

    va_start (args, shader);
    tex = va_arg (args, SCE_STexture*);
    while (tex) {
        textures[i] = tex;
        tex = va_arg (args, SCE_STexture*);
        i++;
    }
    va_end (args);
    textures[i] = NULL;

    code = SCE_Model_AddNewEntityv (mdl, num, level, mesh, shader, textures);
    return code;
}

/**
 * \brief Adds an existing entity to a model
 * \param mdl a model
 * \param num identifier of the entity
 * \param level LOD level of the entity
 * \param entity the entity
 * \param is_instance set to SCE_TRUE if \p entity is already
 * used somewhere else
 * \returns SCE_ERROR on error, SCE_OK otherwise
 * \sa SCE_Model_AddNewEntity()
 */
int SCE_Model_AddEntity (SCE_SModel *mdl, SCEuint num, SCEuint level,
                         SCE_SSceneEntity *entity, int is_instance)
{
    SCE_SModelEntity *mentity = NULL;
    SCE_SModelEntityGroup *mgroup = NULL;

    if (!(mgroup = SCE_Model_LocateEntityGroup (mdl, num))) {
        if (!(mgroup = SCE_Model_CreateEntityGroup (NULL)))
            goto fail;
        mgroup->num = num;
        SCE_List_Appendl (&mdl->groups, &mgroup->it);
    }
    if (!(mentity = SCE_Model_CreateEntity (entity)))
        goto fail;
    mentity->is_instance = is_instance;
    SCE_List_Appendl (&mdl->entities, &mentity->it);
    SCE_SceneEntity_AddEntity (mgroup->group, level, mentity->entity);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

/**
 * \brief Defines a root node and join all the instances to it, further added
 * instances will also be added to it
 * \sa SCE_Model_GetRootNode()
 */
void SCE_Model_SetRootNode (SCE_SModel *mdl, SCE_SNode *root)
{
    SCE_Node_Delete (mdl->root_node);
    mdl->root_node = root;
    if (root) {
        SCE_SListIterator *it = NULL;
        SCE_List_ForEach (it, &mdl->instances) {
            SCE_SModelInstance *minst = SCE_List_GetData (it);
            SCE_SNode *node = SCE_SceneEntity_GetInstanceNode (minst->inst);
            if (node != root)
                SCE_Node_Attach (root, node);
        }
    }
    mdl->root_node_instance = SCE_FALSE;
}
/**
 * \brief Gets the root node of a model
 * \sa SCE_Model_SetRootNode()
 */
SCE_SNode* SCE_Model_GetRootNode (SCE_SModel *mdl)
{
    return mdl->root_node;
}
/**
 * \brief Is the root node an instance node?
 */
int SCE_Model_RootNodeIsInstance (SCE_SModel *mdl)
{
    return mdl->root_node_instance;
}


void SCE_Model_AddModelInstance (SCE_SModel *mdl, SCE_SModelInstance *minst,
                                 int root)
{
    SCE_List_Appendl (&mdl->instances, &minst->it);
    if (root) {
        SCE_Model_SetRootNode (mdl,
                               SCE_SceneEntity_GetInstanceNode (minst->inst));
        mdl->root_node_instance = SCE_TRUE;
    } else if (mdl->root_node) {
        SCE_Node_Attach (mdl->root_node,
                         SCE_SceneEntity_GetInstanceNode (minst->inst));
    }
    /* default data of the instances of a model */
    SCE_SceneEntity_SetInstanceData (minst->inst, mdl);
}
int SCE_Model_AddInstance (SCE_SModel *mdl, SCEuint n,
                           SCE_SSceneEntityInstance *einst, int root)
{
    SCE_SModelInstance *minst = NULL;
    if (!(minst = SCE_Model_CreateInstance ())) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    minst->n = n;
    minst->inst = einst;
    SCE_Model_AddModelInstance (mdl, minst, root);
    return SCE_OK;
}
/**
 * \brief Adds a new instance
 * \param matrix if non NULL, copies this matrix into the matrix of the
 * new instance's node
 * \todo using float* for SCE_TMatrix, bad.
 */
int SCE_Model_AddNewInstance (SCE_SModel *mdl, SCEuint n, int root,
                              float *mat)
{
    SCE_SSceneEntityInstance *einst = NULL;
    SCE_SModelInstance *minst = SCE_Model_CreateInstance2 ();
    if (!minst)
        goto fail;
    minst->n = n;
    einst = minst->inst;
    SCE_Model_AddModelInstance (mdl, minst, root);
    if (mat) {
        SCE_Node_SetMatrix (SCE_SceneEntity_GetInstanceNode (einst), mat);
    }
    return SCE_OK;
fail:
    SCE_SceneEntity_DeleteInstance (einst);
    SCEE_LogSrc ();
    return SCE_ERROR;
}


/**
 * \brief Gets the number of LOD for a given the entity group
 * \param num entity group
 */
unsigned int SCE_Model_GetNumLOD (SCE_SModel *mdl, SCEuint num)
{
    SCE_SModelEntityGroup *mgroup = NULL;
    if (!(mgroup = SCE_Model_LocateEntityGroup (mdl, num)))
        return 0;
    return SCE_List_GetLength
        (SCE_SceneEntity_GetGroupEntitiesList (mgroup->group));
}

/**
 * \brief Gets the required entity
 * \param num the number of the required entity
 * \param level LOD of the required entity
 * \returns the scene entity required, if no on have been found, returns NULL
 *
 * If \p level is lesser than 0, then using the latest level of detail.
 */
SCE_SSceneEntity* SCE_Model_GetEntity (SCE_SModel *mdl, SCEuint num,
                                       SCEuint level)
{
    SCE_SListIterator *it = NULL;
    SCE_SModelEntityGroup *mgroup = NULL;
    SCE_SSceneEntity *entity = NULL;

    if (!(mgroup = SCE_Model_LocateEntityGroup (mdl, num)))
        return NULL;
    it = SCE_List_GetIterator
        (SCE_SceneEntity_GetGroupEntitiesList (mgroup->group), num);
    if (!it)
        return NULL;
    entity = SCE_List_GetData (it);
    return entity;
}
/**
 * \brief Gets the list of all the entities of a model
 * \returns a list of pointers to SCE_SModelEntity structures
 */
SCE_SList* SCE_Model_GetEntitiesList (SCE_SModel *mdl)
{
    return &mdl->entities;
}

/**
 * \brief Gets the scene entity of a model entity
 * \returns \p entity::entity
 */
SCE_SSceneEntity* SCE_Model_GetEntityEntity (SCE_SModelEntity *entity)
{
    return entity->entity;
}


/**
 * \brief Adds the entity instances of a model to its instances groups
 *
 * You must merge your instances before add the model to a scene! Otherwise
 * you'll enjoy a beautiful segmentation fault.
 */
int SCE_Model_MergeInstances (SCE_SModel *mdl)
{
    SCE_SModelEntityGroup *mgroup = NULL;
    SCE_SListIterator *it = NULL;

    SCE_List_ForEach (it, &mdl->instances) {
        SCE_SModelInstance *minst = SCE_List_GetData (it);
        mgroup = SCE_Model_LocateEntityGroup (mdl, minst->n);
        if (!mgroup) {
            SCEE_Log (SCE_INVALID_ARG);
            SCEE_LogMsg ("no group number %u in this model", minst->n);
            return SCE_ERROR;
        }
        SCE_SceneEntity_AddInstance (mgroup->group, minst->inst);
    }
    return SCE_OK;
}

static SCE_SModelEntity* SCE_Model_CopyDupEntity (SCE_SModelEntity *in)
{
    SCE_SModelEntity *entity = NULL;
    if (!(entity = SCE_Model_CreateEntity (in->entity))) {
        SCEE_LogSrc ();
        return NULL;
    }
    return entity;
}
static int SCE_Model_InstanciateHard (SCE_SModel *mdl, SCE_SModel *mdl2)
{
    unsigned int i;
    mdl2->entities = mdl->entities;
    mdl2->groups = mdl->groups;
    mdl2->type = SCE_MODEL_HARD_INSTANCE;
    return SCE_OK;
}
static int SCE_Model_InstanciateSoft (SCE_SModel *mdl, SCE_SModel *mdl2)
{
    SCE_SListIterator *it = NULL;
    unsigned int i;

    /* NOTE: mdl's groups will be added to those of mdl2 */
    /* duplicate SCE_SModelEntityGroup */
    SCE_List_ForEach (it, &mdl->groups) {
        SCE_SModelEntityGroup *mgroup = NULL, *newg = NULL;
        mgroup = SCE_List_GetData (it);
        if (!(newg = SCE_Model_CreateEntityGroup (mgroup->group)))
            goto fail;
        SCE_List_Appendl (&mdl2->groups, &newg->it);
    }

    SCE_List_ForEach (it, &mdl->entities) {
        SCE_SModelEntity *entity = NULL;
        if (!(entity = SCE_Model_CopyDupEntity (SCE_List_GetData (it))))
            goto fail;
        SCE_List_Appendl (&mdl2->entities, &entity->it);
    }

    mdl2->type = SCE_MODEL_SOFT_INSTANCE;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}
static int SCE_Model_InstanciateInstances (SCE_SModel *mdl, SCE_SModel *mdl2)
{
    SCE_SListIterator *it = NULL;
    SCE_List_ForEach (it, &mdl->instances) {
        SCE_SNode *node = NULL;
        SCE_SModelInstance *minst = NULL, *newinst = NULL;
        minst = SCE_List_GetData (it);
        if (!(newinst = SCE_Model_DupInstance (minst)))
            goto fail;
        node = SCE_SceneEntity_GetInstanceNode (minst->inst);
        SCE_Model_AddModelInstance (mdl2, newinst, (node == mdl->root_node));
    }
    /* automatically merge the instances to make a ready-to-use model! */
    if (SCE_Model_MergeInstances (mdl2) < 0)
        goto fail;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}
/**
 * \brief Instanciates a model (can only instanciate a built model)
 * \param mode the instance type, cannot be SCE_MODEL_ROOT
 * \param dup_inst duplicate instances too?
 *
 * The instances in the new instanced model will be automatically merged.
 * Moreover, \p mdl2 must has been allocated by SCE_Model_Create().
 * \sa SCE_Model_CreateInstanciate()
 */
int SCE_Model_Instanciate (SCE_SModel *mdl, SCE_SModel *mdl2,
                           SCE_EModelType mode, int dup_inst)
{
    int code = SCE_OK;
    switch (mode) {
    case SCE_MODEL_SOFT_INSTANCE:
        code = SCE_Model_InstanciateSoft (mdl, mdl2);
        break;
    case SCE_MODEL_HARD_INSTANCE:
        code = SCE_Model_InstanciateHard (mdl, mdl2);
    }

    if (code < 0)
        goto fail;
    if (dup_inst) {
        if (SCE_Model_InstanciateInstances (mdl, mdl2) < 0)
            goto fail;
    }
    if (!mdl2->root_node && mdl->root_node) {
        SCE_SNode *node = NULL;
        /* TODO: multi-usage idea, but is a tree node really adapted to
           any situation? */
        if (!(node = SCE_Node_Create ()))
            goto fail;
        SCE_Model_SetRootNode (mdl2, node);
    }

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}
/**
 * \brief Instanciates a model
 *
 * This function does like SCE_Model_Instanciate() except that it first creates
 * a new model.
 * \sa SCE_Model_Instanciate()
 */
SCE_SModel* SCE_Model_CreateInstanciate (SCE_SModel *mdl, SCE_EModelType mode,
                                         int dup_inst)
{
    SCE_SModel *instance = NULL;
    if (!(instance = SCE_Model_Create ())) {
        SCEE_LogSrc ();
        return NULL;
    }
    SCE_Model_Instanciate (mdl, instance, mode, dup_inst);
    return instance;
}

/**
 * \brief Gets the mode of \p mdl (type of the model)
 *
 * If \p mdl is an instance created by SCE_Model_Instanciate(), this function
 * returns the following:
 * - SCE_MODEL_SOFT_INSTANCE: the data of the model structure have been
 *   duplicated;
 * - SCE_MODEL_HARD_INSTANCE: the content of the model structure have just been
 *   copied, like a simple '=' on C struct.
 * If \p mdl isn't an instance, this functions returns SCE_MODEL_ROOT.
 */
SCE_EModelType SCE_Model_GetType (SCE_SModel *mdl)
{
    return mdl->type;
}


SCE_SList* SCE_Model_GetInstancesList (SCE_SModel *mdl)
{
    return &mdl->instances;
}

SCE_SSceneEntityGroup* SCE_Model_GetSceneEntityGroup (SCE_SModel *mdl,
                                                      unsigned int n)
{
    SCE_SListIterator *it = SCE_List_GetIterator (&mdl->groups, n);
    if (!it)
        return NULL;
    else
        return ((SCE_SModelEntityGroup*)SCE_List_GetData (it))->group;
}
