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
 
/* created: 19/01/2008
   updated: 29/02/2010 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include "SCE/interface/SCEBatch.h"
#include "SCE/interface/SCEScene.h"


/**
 * \file SCEScene.c
 * \copydoc scene
 * 
 * \file SCEScene.h
 * \copydoc scene
 */

/**
 * \defgroup scene Scene
 * \ingroup interface
 * \internal
 * \brief Scene manager
 */

/** @{ */

/*static SCE_SCamera *default_camera = NULL;*/

/* dat iz omg coef u no??? */
static float omg_coeffs[2] = {0.3f, 0.02f};

typedef struct sce_ssceneoctree SCE_SSceneOctree;
struct sce_ssceneoctree {
    SCE_SList *instances[3];       /* TODO: pkeu 3 laiveuls ser coul (oupa?) */
    SCE_SList *lights;
    SCE_SList *cameras;
    /* add nodes type here */
};

/** \internal */
int SCE_Init_Scene (void)
{
#if 0
    if (!(default_camera = SCE_Camera_Create ())) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    omg_coeffs[0] = 0.3f;
    omg_coeffs[1] = 0.02f;
#endif
    return SCE_OK;
}
/** \internal */
void SCE_Quit_Scene (void)
{
/*    SCE_Camera_Delete (default_camera), default_camera = NULL;*/
}


static void SCE_Scene_InitOctree (SCE_SSceneOctree *tree)
{
    unsigned int i;
    for (i = 0; i < 3; i++)
        tree->instances[i] = NULL;
    tree->lights = NULL;
    tree->cameras = NULL;
}

static void SCE_Scene_DeleteOctree (SCE_SSceneOctree*);

static SCE_SSceneOctree* SCE_Scene_CreateOctree (void)
{
    unsigned int i;
    SCE_SSceneOctree *tree = NULL;
    tree = SCE_malloc (sizeof *tree);
    if (!tree)
        goto failure;
    SCE_Scene_InitOctree (tree);
    for (i = 0; i < 3; i++) {   /* TODO: use macro for the value 3 */
        if (!(tree->instances[i] = SCE_List_Create (NULL)))
            goto failure;
    }
    if (!(tree->lights = SCE_List_Create (NULL)))
        goto failure;
    if (!(tree->cameras = SCE_List_Create (NULL)))
        goto failure;
    goto success;
failure:
    SCE_Scene_DeleteOctree (tree), tree = NULL;
    SCEE_LogSrc ();
success:
    return tree;
}

static void SCE_Scene_DeleteOctree (SCE_SSceneOctree *tree)
{
    if (tree) {
        unsigned int i;
        for (i = 0; i < 3; i++)
            SCE_List_Delete (tree->instances[i]);
        SCE_List_Delete (tree->lights);
        SCE_List_Delete (tree->cameras);
    }
}


static void SCE_Scene_InitStates (SCE_SSceneStates *states)
{
    states->clearcolor = states->cleardepth = SCE_TRUE;
    states->frustum_culling = SCE_FALSE;
    states->lighting = SCE_TRUE;
    states->lod = SCE_FALSE;
}

static void SCE_Scene_RemoveLightNode (void *scene, void *light)
{
    SCE_Scene_RemoveNode (scene, SCE_Light_GetNode (light));
}
static void SCE_Scene_RemoveCameraNode (void *scene, void *cam)
{
    SCE_Scene_RemoveNode (scene, SCE_Camera_GetNode (cam));
}
static void SCE_Scene_Init (SCE_SScene *scene)
{
    unsigned int i;

    SCE_Scene_InitStates (&scene->states);

    scene->rootnode = NULL;
    scene->node_updated = SCE_FALSE;
    scene->n_nodes = 0;
    scene->octree = NULL;
    scene->contribution_size = 0.01f;

    for (i = 0; i < SCE_SCENE_NUM_RESOURCE_GROUP; i++)
        scene->rgroups[i] = NULL;

    scene->selected = NULL;
    scene->selected_join = NULL;

    SCE_List_Init (&scene->entities);
    SCE_List_Init (&scene->lights);
    SCE_List_SetFreeFunc2 (&scene->lights, SCE_Scene_RemoveLightNode, scene);
    SCE_List_Init (&scene->cameras);
    SCE_List_SetFreeFunc2 (&scene->cameras, SCE_Scene_RemoveCameraNode, scene);

    scene->skybox = NULL;
    scene->rclear = scene->gclear = scene->bclear = scene->aclear = 0.5;
    scene->dclear = 1.0;
    scene->rendertarget = NULL;
    scene->cubeface = 0;
    scene->camera = NULL;

    scene->bbmesh = NULL;
    scene->bsmesh = NULL;
}

static int SCE_Scene_MakeBoundingVolumes (SCE_SScene *scene)
{
    SCE_SBox box;
    SCE_SSphere sphere;
    SCE_SGeometry *geom = NULL;
    SCE_TVector3 v;

    SCE_Vector3_Set (v, 0.0, 0.0, 0.0);
    SCE_Box_SetFromCenter (&box, v, 1.0, 1.0, 1.0);
    if (!(geom = SCE_BoxGeom_Create (&box, SCE_LINES, SCE_BOX_NONE_TEXCOORD)))
        goto fail;
    if (!(scene->bbmesh = SCE_Mesh_CreateFrom (geom, SCE_TRUE)))
        goto fail;
    SCE_Mesh_AutoBuild (scene->bbmesh);

    return SCE_TRUE;
fail:
    SCE_Geometry_Delete (geom);
    SCEE_LogSrc ();
    return SCE_ERROR;
}
/**
 * \brief Creates a new scene
 */
SCE_SScene* SCE_Scene_Create (void)
{
    unsigned int i;
    SCE_SScene *scene = NULL;
    SCE_SSceneOctree *stree = NULL;
    SCE_SOctreeElement *el = NULL;

    if (!(scene = SCE_malloc (sizeof *scene)))
        goto failure;
    SCE_Scene_Init (scene);
    if (!(scene->rootnode = SCE_Node_Create ()))
        goto failure;
    if (!(scene->octree = SCE_Octree_Create ()))
        goto failure;
    for (i = 0; i < SCE_SCENE_NUM_RESOURCE_GROUP; i++) {
        if (!(scene->rgroups[i] = SCE_SceneResource_CreateGroup ()))
            goto failure;
        SCE_SceneResource_SetGroupType (scene->rgroups[i], i);
    }
    /* TODO: sucks (use static structures) */
    if (!(scene->selected = SCE_List_Create (NULL)))
        goto failure;

    SCE_Octree_SetSize (scene->octree, SCE_SCENE_OCTREE_SIZE,
                        SCE_SCENE_OCTREE_SIZE, SCE_SCENE_OCTREE_SIZE);
    if (!(stree = SCE_Scene_CreateOctree ()))
        goto failure;
    SCE_Octree_SetData (scene->octree, stree);

    if (SCE_Scene_MakeBoundingVolumes (scene) < 0)
        goto failure;

    return scene;
failure:
/*    SCE_Scene_DeleteOctree (stree);*/
    SCE_Scene_Delete (scene);
    SCEE_LogSrc ();
    return NULL;
}

/**
 * \brief Deletes a scene
 * \param scene the scene you want to delete
 */
void SCE_Scene_Delete (SCE_SScene *scene)
{
    if (scene) {
        unsigned int i;
        SCE_List_Clear (&scene->cameras);
        SCE_List_Clear (&scene->lights);
        SCE_List_Clear (&scene->entities);
        for (i = 0; i < SCE_SCENE_NUM_RESOURCE_GROUP; i++)
            SCE_SceneResource_DeleteGroup (scene->rgroups[i]);
        SCE_Octree_DeleteRecursive (scene->octree);
        SCE_Node_Delete (scene->rootnode);
        SCE_free (scene);
    }
}


/**
 * \brief Gets the root node of a scene
 * \param scene a scene
 * \returns the scene's root node
 */
SCE_SNode* SCE_Scene_GetRootNode (SCE_SScene *scene)
{
    return scene->rootnode;
}


/* called when a node has moved */
void SCE_Scene_OnNodeMoved (SCE_SNode *node, void *param)
{
    /* TODO: tmp */
    SCE_SOctreeElement *el = NULL;
    SCE_SBoundingSphere *bs = NULL;
    SCE_SSphere old;

    el = SCE_Node_GetElement (node);
    bs = el->sphere;

    SCE_BoundingSphere_Push (bs, SCE_Node_GetFinalMatrix (node), &old);
    SCE_Octree_ReinsertElement (el);
    SCE_BoundingSphere_Pop (bs, &old);
}


static unsigned int SCE_Scene_DetermineElementList (SCE_SOctreeElement *el,
                                                    SCE_SOctree *tree)
{
    float d;
    float c;
    d = SCE_BoundingSphere_GetRadius (el->sphere) * 2.0f;
    c = d / SCE_Box_GetWidth (SCE_BoundingBox_GetBox (
                                        SCE_Octree_GetBoundingBox (tree)));
    /* TODO: ptdr rofl mdr? */
    if (c < omg_coeffs[1]){
        return 2;
    } else if (c < omg_coeffs[0]) {
        return 1;
    }
    return 0;
}
/* inserts an instance into an octree */
static void SCE_Scene_InsertInstance (SCE_SOctree *tree, SCE_SOctreeElement *el)
{
    unsigned int id = 0;
    SCE_SSceneOctree *stree = NULL;
    stree = SCE_Octree_GetData (tree);
    id = SCE_Scene_DetermineElementList (el, tree);
    SCE_List_Prependl (stree->instances[id], &el->it);
}
/* inserts a light into an octree */
static void SCE_Scene_InsertLight (SCE_SOctree *tree, SCE_SOctreeElement *el)
{
    SCE_SSceneOctree *stree = NULL;
    stree = SCE_Octree_GetData (tree);
    SCE_List_Prependl (stree->lights, &el->it);
}
/* inserts a camera into an octree */
static void SCE_Scene_InsertCamera (SCE_SOctree *tree, SCE_SOctreeElement *el)
{
    SCE_SSceneOctree *stree = NULL;
    stree = SCE_Octree_GetData (tree);
    SCE_List_Prependl (stree->cameras, &el->it);
}

/**
 * \internal
 * \brief Bound to SCE_Octree_InsertElement()
 */
static void SCE_Scene_AddElement (SCE_SScene *scene, SCE_SOctreeElement *el)
{
    SCE_Octree_InsertElement (scene->octree, el);
}
/**
 * \internal
 * \brief Bound to SCE_Octree_RemoveElement()
 */
static void SCE_Scene_RemoveElement (SCE_SOctreeElement *el)
{
    SCE_Octree_RemoveElement (el);
}
/**
 * \internal
 * \brief Adds the node's element to a scene
 *
 * Bound to SCE_Scene_AddElement(), but calls
 * SCE_BoundingSphere_Push() before and SCE_BoundingSphere_Pop() after.
 * \sa SCE_Scene_AddNode()
 */
static void SCE_Scene_AddNodeElement (SCE_SScene *scene, SCE_SNode *node)
{
    SCE_SSphere old;
    SCE_SOctreeElement *el = SCE_Node_GetElement (node);
    SCE_BoundingSphere_Push (el->sphere, SCE_Node_GetFinalMatrix (node), &old);
    SCE_Scene_AddElement (scene, el);
    SCE_BoundingSphere_Pop (el->sphere, &old);
}
/**
 * \internal
 * \brief Removes a node's element from a scene
 *
 * Just calls SCE_Octree_RemoveElement().
 * \sa SCE_Octree_RemoveElement(), SCE_Scene_RemoveElement()
 */
static void SCE_Scene_RemoveNodeElement (SCE_SNode *node)
{
    SCE_Octree_RemoveElement (SCE_Node_GetElement (node));
}
/**
 * \brief Adds a node to a scene
 * \param scene a scene
 * \param node the node to add
 * 
 * Adds a node to a scene.
 * Inserts the octree element of \p node by calling SCE_Octree_InsertElement().
 * \sa SCE_Scene_RemoveNode(), SCE_Octree_InsertElement()
 * \warning this function will be marked as internal in a future release,
 * so do not use it anyway.
 */
void SCE_Scene_AddNode (SCE_SScene *scene, SCE_SNode *node)
{
    /* let the user make his own nodes tree */
    if (!SCE_Node_HasParent (node)) {
        SCE_Node_Attach (scene->rootnode, node);
        scene->n_nodes++;
    }
}
/**
 * \brief Adds a node and all its children to a scene
 *
 * This function calls SCE_Scene_AddNode() under \p node and also calls
 * SCE_Scene_AddNodeRecursive() for each child of \p node.
 * \sa SCE_Scene_AddNode()
 */
void SCE_Scene_AddNodeRecursive (SCE_SScene *scene, SCE_SNode *node)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *children = NULL;
    SCE_Scene_AddNode (scene, node);
    children = SCE_Node_GetChildrenList (node);
    SCE_List_ForEach (it, children)
        SCE_Scene_AddNodeRecursive (scene, SCE_List_GetData (it));
}
/**
 * \brief Removes a node from a scene
 * \param scene the scene from which remove \p node
 * \param node the node to remove
 * \warning the node \p node HAVE to be previously added to the scene \p scene,
 * the behavior is undefined otherwise.
 */
void SCE_Scene_RemoveNode (SCE_SScene *scene, SCE_SNode *node)
{
    if (SCE_Node_GetParent (node) == scene->rootnode) {
        scene->n_nodes--;
        SCE_Node_Detach (node);
    }
}

/**
 * \brief Adds an instance to a scene
 * \param scene the scene into which add \p einst
 * \param einst the instance to add
 *
 * Adds an instance to a scene, adds its node by calling SCE_Scene_AddNode().
 * \sa SCE_Scene_RemoveInstance(), SCE_Scene_AddNode(), SCE_Scene_MakeOctree()
 */
void SCE_Scene_AddInstance (SCE_SScene *scene, SCE_SSceneEntityInstance *einst)
{
    SCE_SOctreeElement *el = NULL;
    SCE_SNode *node = NULL;

    node = SCE_SceneEntity_GetInstanceNode (einst);
    el = SCE_SceneEntity_GetInstanceElement (einst);
    el->insert = SCE_Scene_InsertInstance;

    SCE_Scene_AddNode (scene, node);
    SCE_Scene_AddNodeElement (scene, node);
    /* NOTE: not relative to the (future) parent (gne?) */
    SCE_Node_SetOnMovedCallback (node, SCE_Scene_OnNodeMoved, scene);
}
/**
 * \brief Removes an instance from a scene
 * \param scene the scene from which remove \p einst
 * \param einst the instance to remove
 * \sa SCE_Scene_AddInstance()
 */
void SCE_Scene_RemoveInstance (SCE_SScene *scene,
                               SCE_SSceneEntityInstance *einst)
{
    SCE_List_Remove (SCE_SceneEntity_GetInstanceIterator1 (einst));
    SCE_Scene_RemoveNodeElement (SCE_SceneEntity_GetInstanceNode (einst));
    SCE_Scene_RemoveNode (scene, SCE_SceneEntity_GetInstanceNode (einst));
}

/**
 * \brief Adds the resources of a scene entity
 * \sa SCE_Scene_AddResource()
 */
void SCE_Scene_AddEntityResources (SCE_SScene *scene, SCE_SSceneEntity *entity)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *textures = NULL;
    SCE_SSceneResource *res = NULL;

    textures = SCE_SceneEntity_GetTexturesList (entity);
    SCE_List_ForEach (it, textures) {
        SCE_Scene_AddResource (scene, SCE_SCENE_TEXTURES0_GROUP,
                               SCE_List_GetData (it));
    }
    res = SCE_SceneEntity_GetShader (entity);
    if (res)
        SCE_Scene_AddResource (scene, SCE_SCENE_SHADERS_GROUP, res);
    res = SCE_SceneEntity_GetMaterial (entity);
    if (res)
        SCE_Scene_AddResource (scene, SCE_SCENE_MATERIALS_GROUP, res);
}
/**
 * \brief Removes the resources of a scene entity
 * \sa SCE_Scene_RemoveResource()
 */
void SCE_Scene_RemoveEntityResources (SCE_SScene *scene,
                                      SCE_SSceneEntity *entity)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *textures = NULL;
    SCE_SSceneResource *res = NULL;

    textures = SCE_SceneEntity_GetTexturesList (entity);
    SCE_List_ForEach (it, textures) {
        SCE_Scene_RemoveResource (SCE_List_GetData (it));
    }
    res = SCE_SceneEntity_GetShader (entity);
    if (res)
        SCE_Scene_RemoveResource (res);
    res = SCE_SceneEntity_GetMaterial (entity);
    if (res)
        SCE_Scene_RemoveResource (res);
}

/**
 * \brief Adds an entity to a scene
 * \param scene the scene into which add \p entity
 * \param entity the entity to add into the scene
 */
void SCE_Scene_AddEntity (SCE_SScene *scene, SCE_SSceneEntity *entity)
{
    /* TODO: why it doesn't add the resources too?? */
    SCE_List_Remove (SCE_SceneEntity_GetIterator (entity));
    SCE_List_Prependl (&scene->entities, SCE_SceneEntity_GetIterator (entity));
}
/**
 * \brief Removes an entity from a scene
 * \param scene the scene from which remove \p entity
 * \param entity the entity to remove from the scene
 */
void SCE_Scene_RemoveEntity (SCE_SScene *scene, SCE_SSceneEntity *entity)
{
    SCE_List_Remove (SCE_SceneEntity_GetIterator (entity));
}


static void SCE_Scene_AddModelEntities (SCE_SScene *scene, SCE_SModel *mdl)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *entities = SCE_Model_GetEntitiesList (mdl);
    if (!entities)
        return;
    SCE_List_ForEach (it, entities) {
        SCE_SSceneEntity *entity = NULL;
        entity = ((SCE_SModelEntity*)SCE_List_GetData (it))->entity;
        SCE_Scene_AddEntityResources (scene, entity);
        SCE_Scene_AddEntity (scene, entity);
    }
}
/**
 * \brief Adds a model to a scene
 * \param scene the scene into which add \p mdl
 * \param mdl the model to add into the scene
 */
void SCE_Scene_AddModel (SCE_SScene *scene, SCE_SModel *mdl)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *instances = NULL;

    /* we are now protected against double-add of entities! */
    SCE_Scene_AddModelEntities (scene, mdl);

    instances = SCE_Model_GetInstancesList (mdl);
    SCE_List_ForEach (it, instances) {
        SCE_SModelInstance *minst = SCE_List_GetData (it);
        SCE_Scene_AddInstance (scene, minst->inst);
    }

    if (SCE_Model_GetRootNode (mdl) && !SCE_Model_RootNodeIsInstance (mdl))
        SCE_Scene_AddNode (scene, SCE_Model_GetRootNode (mdl));
}


static void SCE_Scene_RemoveModelEntities (SCE_SScene *scene, SCE_SModel *mdl)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *entities = SCE_Model_GetEntitiesList (mdl);
    if (!entities)
        return;
    SCE_List_ForEach (it, entities) {
        SCE_SSceneEntity *entity = NULL;

        entity = ((SCE_SModelEntity*)SCE_List_GetData (it))->entity;
        SCE_Scene_RemoveEntityResources (scene, entity);
        SCE_Scene_RemoveEntity (scene, entity);
    }
}
/**
 * \brief Removes a model from a scene
 * \param scene the scene from which remove \p mdl
 * \param mdl the model to remove from the scene
 */
void SCE_Scene_RemoveModel (SCE_SScene *scene, SCE_SModel *mdl)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *instances = NULL;

    if (SCE_Model_GetType (mdl) == SCE_MODEL_ROOT)
        SCE_Scene_RemoveModelEntities (scene, mdl);

    instances = SCE_Model_GetInstancesList (mdl);
    SCE_List_ForEach (it, instances) {
        SCE_SModelInstance *minst = SCE_List_GetData (it);
        SCE_Scene_RemoveInstance (scene, minst->inst);
    }
}


/**
 * \brief Adds a light to a scene
 * \param scene the scene into which add \p light
 * \param light the light to add into the scene
 */
void SCE_Scene_AddLight (SCE_SScene *scene, SCE_SLight *light)
{
    SCE_SNode *node = NULL;
    SCE_List_Appendl (&scene->lights, SCE_Light_GetIterator (light));
    node = SCE_Light_GetNode (light);
    SCE_Node_GetElement (node)->insert = SCE_Scene_InsertLight;
    SCE_Scene_AddNode (scene, node);
}

/**
 * \brief Adds a camera to a scene
 * \param scene the scene into which add \p camera
 * \param camera the camera to add into the scene
 */
void SCE_Scene_AddCamera (SCE_SScene *scene, SCE_SCamera *camera)
{
    SCE_SNode *node = NULL;
    SCE_List_Appendl (&scene->cameras, SCE_Camera_GetIterator (camera));
    node = SCE_Camera_GetNode (camera);
    SCE_Node_GetElement (node)->insert = SCE_Scene_InsertCamera;
    SCE_Scene_AddNode (scene, node);
}

/**
 * \brief Adds a resource to a scene
 * \param scene the scene into which add the resource
 * \param id ID of the scene group into which add the resource (e.g.
 *           SCE_SCENE_MATERIALS_GROUP)
 * \param res the resource to add
 * \todo remove it, use AddTexture, AddShader, etc.
 * \sa SCE_Scene_RemoveResource()
 */
void SCE_Scene_AddResource (SCE_SScene *scene, int id, SCE_SSceneResource *res)
{
    SCE_SceneResource_AddResource (scene->rgroups[id], res);
}
/**
 * \brief Removes a resource from a scene (except that no scene parameter is
 * needed.)
 * \param res the ressource to remove
 * \todo see SCE_Scene_AddResource()
 */
void SCE_Scene_RemoveResource (SCE_SSceneResource *res)
{
    SCE_SceneResource_RemoveResource (res);
}

/**
 * \brief Defines the skybox of a scene
 * \param scene the scene
 * \param skybox the skybox
 */
void SCE_Scene_SetSkybox (SCE_SScene *scene, SCE_SSkybox *skybox)
{
    if (scene->skybox) {
/*        SCE_Scene_RemoveEntityGroup (scene, SCE_Skybox_GetEntityGroup
          (scene->skybox));*/
/*        SCE_Scene_RemoveInstance (scene, SCE_Skybox_GetInstance(scene->skybox));*/
    }
    scene->skybox = skybox;
    if (skybox) {
/*        SCE_Scene_AddEntityGroup (scene, SCE_Skybox_GetEntityGroup (skybox));*/
/*        SCE_Scene_AddInstance (scene, SCE_Skybox_GetInstance(scene->skybox));*/
    }
}

/**
 * \brief Gets the list of the selected instances (after culling and other
 * tests) for rendering
 * \param scene a scene
 */
SCE_SList* SCE_Scene_GetSelectedInstancesList (SCE_SScene *scene)
{
    return scene->selected;
}


/**
 * \brief Defines the size of the octree of a scene
 * \param scene a scene
 * \param w,h,d the new dimensions of the octree
 * \sa SCE_Octree_SetSize()
 */
void SCE_Scene_SetOctreeSize (SCE_SScene *scene, float w, float h, float d)
{
    SCE_Octree_SetSize (scene->octree, w, h, d);
}
/**
 * \brief Defines the size of the octree of a scene
 * \param scene a scene
 * \param d the new dimensions of the octree
 * \sa SCE_Scene_SetOctreeSize(), SCE_Octree_SetSize()
 */
void SCE_Scene_SetOctreeSizev (SCE_SScene *scene, SCE_TVector3 d)
{
    SCE_Octree_SetSizev (scene->octree, d);
}

/* builds internal structure for an octree */
static int SCE_Scene_MakeOctreeInternal (SCE_SOctree *tree)
{
    SCE_SSceneOctree *stree = SCE_Scene_CreateOctree ();
    if (!tree) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    SCE_Octree_SetData (tree, stree);
    if (SCE_Octree_HasChildren (tree)) {
        unsigned int i;
        SCE_SOctree **children = SCE_Octree_GetChildren (tree);
        for (i = 0; i < 8; i++) {
            if (SCE_Scene_MakeOctreeInternal (children[i]) < 0) {
                SCEE_LogSrc ();
                return SCE_ERROR;
            }
        }
    }
    return SCE_OK;
}
static void SCE_Scene_EraseOctreeInternal (SCE_SOctree *tree)
{
    SCE_Scene_DeleteOctree (SCE_Octree_GetData (tree));
    if (SCE_Octree_HasChildren (tree)) {
        unsigned int i;
        SCE_SOctree **children = SCE_Octree_GetChildren (tree);
        for (i = 0; i < 8; i++)
            SCE_Scene_EraseOctreeInternal (children[i]);
    }
}
/**
 * \brief Makes an octree for the given scene
 * \returns SCE_ERROR on error, SCE_OK otherwise
 *
 * Makes an octree for the given scene and clears the previous one.
 * \sa SCE_Octree_RecursiveMake(), SCE_Octree_Clear()
 */
int SCE_Scene_MakeOctree (SCE_SScene *scene, unsigned int rec,
                          int loose, float margin)
{
    SCE_Scene_EraseOctreeInternal (scene->octree);
    SCE_Octree_Clear (scene->octree);
    if (!loose)                 /* shield! */
        margin = 0.0;
    if (SCE_Octree_RecursiveMake (scene->octree, rec, NULL,
                                  NULL, loose, margin) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    /* creates the internal structures */
    if (SCE_Scene_MakeOctreeInternal (scene->octree) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}

int SCE_Scene_SetupBatching (SCE_SScene *scene, unsigned int n, int *order)
{
    if (SCE_Batch_SortEntities (&scene->entities, SCE_SCENE_NUM_RESOURCE_GROUP,
                                scene->rgroups, 2, order) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}

int SCE_Scene_SetupDefaultBatching (SCE_SScene *scene)
{
    int order[] = {SCE_SCENE_SHADERS_GROUP, SCE_SCENE_TEXTURES0_GROUP};
    if (SCE_Batch_SortEntities (&scene->entities, SCE_SCENE_NUM_RESOURCE_GROUP,
                                scene->rgroups, 2, order) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}


static float SCE_Scene_GetOctreeSize (SCE_SOctree *tree, SCE_SCamera *cam)
{
    return SCE_Lod_ComputeBoundingBoxSurface (SCE_Octree_GetBox (tree), cam);
}

static void SCE_Scene_SelectAllInstances (SCE_SScene *scene,
                                          SCE_SSceneOctree *stree,
                                          unsigned int id)
{
    if (SCE_List_HasElements (stree->instances[id])) {
        SCE_List_Join (scene->selected_join, stree->instances[id]);
        scene->selected_join = stree->instances[id];
    }
}
static void SCE_Scene_SelectVisibleInstances (SCE_SScene *scene,
                                              SCE_SSceneOctree *stree,
                                              unsigned int id)
{
    SCE_SListIterator *it = NULL;
    SCE_SSceneEntityInstance *einst = NULL;
    SCE_List_ForEach (it, stree->instances[id]) {
        einst = SCE_List_GetData (it);
        if (SCE_SceneEntity_IsInstanceInFrustum (einst, scene->camera)) {
            SCE_List_Prependl (scene->selected,
                               SCE_SceneEntity_GetInstanceIterator1 (einst));
        }
    }
}

static void SCE_Scene_SelectOctreeInstances(SCE_SScene *scene,
                                            SCE_SOctree *tree,
                                            void (*selectfun)(SCE_SScene*,
                                                              SCE_SSceneOctree*,
                                                              unsigned int))
{
    unsigned int i = 0;
    float size = 0.0f;
    SCE_SSceneOctree *stree = SCE_Octree_GetData (tree);
    size = SCE_Scene_GetOctreeSize (tree, scene->camera);
    selectfun (scene, stree, 0);
    for (i = 0; i < 2; i++) {
        if (omg_coeffs[i] * size < scene->contribution_size)
            break; /* the next are smaller, so stop */
        else
            selectfun (scene, stree, i + 1);
    }
}

static void SCE_Scene_SelectAllOctreeInstancesRec (SCE_SScene *scene,
                                                   SCE_SOctree *tree)
{
    SCE_Scene_SelectOctreeInstances (scene, tree, SCE_Scene_SelectAllInstances);
    if (SCE_Octree_HasChildren (tree)) {
        unsigned int i;
        SCE_SOctree **children = SCE_Octree_GetChildren (tree);
        for (i = 0; i < 8; i++)
            SCE_Scene_SelectAllOctreeInstancesRec (scene, children[i]);
    }
}


static void SCE_Scene_SelectVisibleOctrees (SCE_SScene *scene,
                                            SCE_SOctree *tree)
{
    if (!SCE_Octree_IsVisible (tree))
        return;
    if (!SCE_Octree_IsPartiallyVisible (tree))
        SCE_Scene_SelectAllOctreeInstancesRec (scene, tree);
    else {
        /* TODO: using SelectAllInstances() if the octree is too far */
        SCE_Scene_SelectOctreeInstances (scene, tree,
                                         SCE_Scene_SelectVisibleInstances);
        if (SCE_Octree_HasChildren (tree)) {
            unsigned int i;
            SCE_SOctree **children = SCE_Octree_GetChildren (tree);
            for (i = 0; i < 8; i++)
                SCE_Scene_SelectVisibleOctrees (scene, children[i]);
        }
    }
}

static void SCE_Scene_SelectVisibles (SCE_SScene *scene)
{
    SCE_Scene_SelectVisibleOctrees (scene, scene->octree);
}


static void SCE_Scene_DetermineEntitiesUsingLOD (SCE_SScene *scene)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *instances = scene->selected;
    SCE_List_ForEach (it, instances) {
        SCE_SceneEntity_DetermineInstanceLOD (SCE_List_GetData (it),
                                              scene->camera);
    }
}

static void SCE_Scene_DetermineEntities (SCE_SScene *scene)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *instances = scene->selected;
    SCE_List_ForEach (it, instances)
        SCE_SceneEntity_ReplaceInstanceToEntity (SCE_List_GetData (it));
}


static void SCE_Scene_FlushEntities (SCE_SScene *scene)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *entities = &scene->entities;
    SCE_List_ForEach (it, entities)
        SCE_SceneEntity_Flush (SCE_List_GetData (it));
}

/**
 * \brief Prepares a scene for rendering
 * \param scene the scene to prepare
 * \param camera the camera used for the render, can be NULL then a default
 *        camera (the same one that SCE_Camera_Create() returns) is used
 * \param rendertarget the render target
 * \param cubeface if the render target is a cubemap, it is used to determine
 *        which face is the render target. Otherwise, if the render target is
 *        not a cubmap, this parameter is useless.
 *        This can be one of SCE_TEX_POSX, SCE_TEX_NEGX, SCE_TEX_POSY,
 *        SCE_TEX_NEGY, SCE_TEX_POSZ or SCE_TEX_NEGZ.
 * 
 * This function prepares the given scene's active renderer for render. It
 * assignes a camera and a render target to the active scene's renderer.
 * 
 * \note cubeface is saved even if the render target is not a cubemap.
 * \sa SCE_Scene_Render(), SCE_Texture_RenderTo()
 */
void SCE_Scene_Update (SCE_SScene *scene, SCE_SCamera *camera,
                       SCE_STexture *rendertarget, SCEuint cubeface)
{
    int fc;

    scene->rendertarget = rendertarget;
    scene->cubeface = cubeface;
    scene->camera = camera;

    fc = scene->states.frustum_culling;

    if (scene->states.lod || fc)
        SCE_Scene_FlushEntities (scene);

    if (fc) {
        /* do it before nodes' update, otherwise the calls of
           List_Removel() can fail */
        SCE_List_BreakAll (scene->selected);
        SCE_List_Flush (scene->selected);
        scene->selected_join = scene->selected;
    }

    SCE_Node_UpdateRootRecursive (scene->rootnode);
    SCE_Camera_Update (scene->camera);

    if (fc) {
        SCE_Octree_MarkVisibles (scene->octree,
                                 SCE_Camera_GetFrustum (scene->camera));
        SCE_Scene_SelectVisibles (scene);
    }

    if (scene->states.lod)
        SCE_Scene_DetermineEntitiesUsingLOD (scene);
    else if (fc)
        SCE_Scene_DetermineEntities (scene);
}


/**
 * \internal
 * \brief Clears the buffers before the rendering
 */
void SCE_Scene_ClearBuffers (SCE_SScene *scene)
{
    SCEbitfield depthbuffer = 0;

    if (scene->states.cleardepth)
        depthbuffer = GL_DEPTH_BUFFER_BIT;
    if (scene->states.clearcolor)
        depthbuffer |= GL_COLOR_BUFFER_BIT;

    SCE_RClearColor (scene->rclear, scene->gclear, scene->bclear,scene->aclear);
    SCE_RClearDepth (scene->dclear);
    /* what does glClear(0)? */
    if (depthbuffer)
        SCE_RClear (depthbuffer);
}


static void SCE_Scene_RenderEntities (SCE_SScene *scene)
{
    SCE_SSceneEntity *entity = NULL;
    SCE_SListIterator *it;
    SCE_List_ForEach (it, &scene->entities) {
        entity = SCE_List_GetData (it);
        if (SCE_SceneEntity_HasInstance (entity)) {
            SCE_SceneEntity_UseResources (entity);
            SCE_SceneEntity_Render (entity);
        }
    }
    SCE_Texture_Flush ();
    SCE_Material_Use (NULL);
    SCE_Shader_Use (NULL);
}


static void SCE_Scene_RenderSkybox (SCE_SScene *scene, SCE_SCamera *cam)
{
    SCE_TVector3 pos;
    float *mat = NULL;
    SCE_SSceneEntity *entity = SCE_Skybox_GetEntity (scene->skybox);
    SCE_SSceneEntityInstance *einst = SCE_Skybox_GetInstance (scene->skybox);
    SCE_SNode *node = SCE_SceneEntity_GetInstanceNode (einst);

    mat = SCE_Camera_GetFinalViewInverse (cam);
    SCE_Matrix4_GetTranslation (mat, pos);
    mat = SCE_Node_GetFinalMatrix (node); /* hax */
    SCE_Matrix4_Translatev (mat, pos);
    SCE_Matrix4_MulScale (mat, 42.0, 42.0, 42.0);
    SCE_Matrix4_MulTranslate (mat, -0.5, -0.5, -0.5);

    SCE_SceneEntity_UseResources (entity);
    SCE_SceneEntity_Render (entity);
    /* because skybox properties disabled it */
    SCE_RActivateDepthBuffer (GL_TRUE);

    SCE_Texture_Flush ();
    SCE_Shader_Use (NULL);
}
/* TODO: cheat. */
static void SCE_Scene_UseCamera (SCE_SCamera *cam)
{
    SCE_RViewport (cam->viewport.x, cam->viewport.y,
                   cam->viewport.w, cam->viewport.h);
    SCE_RSetActiveMatrix (SCE_MAT_PROJECTION);
    SCE_RLoadMatrix (cam->proj);  /* NOTE: Load ou Mult ? */
    SCE_RSetActiveMatrix (SCE_MAT_MODELVIEW);
    SCE_RLoadMatrix (cam->finalview);  /* Load ou Mult ? */
}

/**
 * \brief Renders a scene into a render target
 * \param scene a scene
 * \param cam the camera used for the render or NULL to keep the current one.
 * \param rendertarget the render target or NULL to keep the current one. If
 *        both this parameter and the current render targed are NULL, the
 *        default OpenGL's render buffer will be used as the render target.
 * \param cubeface see SCE_Scene_Update()'s cubeface parameter.
 * \see SCE_Scene_Update()
 */
void SCE_Scene_Render (SCE_SScene *scene, SCE_SCamera *cam,
                       SCE_STexture *rendertarget, int cubeface)
{
    SCE_SListIterator *it = NULL;

    if (!cam)
        cam = scene->camera;
    if (!rendertarget)
        rendertarget = scene->rendertarget;
    if (cubeface < 0)
        cubeface = scene->cubeface;

    /* mise en place du render target */
    SCE_Texture_RenderTo (rendertarget, cubeface);

    /* preparation des tampons */
    if (scene->skybox) {
        scene->states.clearcolor = SCE_FALSE;
        scene->states.cleardepth = SCE_TRUE;
    }
    SCE_Scene_ClearBuffers (scene);

    /* activation de la camera et mise en place des matrices */
    SCE_RSetActiveMatrix (SCE_MAT_MODELVIEW);
    SCE_RPushMatrix ();
    SCE_Scene_UseCamera (cam);

    /* render skybox (if any) */
    if (scene->skybox) {
        SCE_Light_ActivateLighting (SCE_FALSE);
        SCE_Scene_RenderSkybox (scene, cam);
    }

    if (!scene->states.lighting)
        SCE_Light_ActivateLighting (SCE_FALSE);
    else {
        SCE_Light_ActivateLighting (SCE_TRUE);
        /* TODO: activation de toutes les lumières (bourrin & temporaire) */
        SCE_List_ForEach (it, &scene->lights)
            SCE_Light_Use (SCE_List_GetData (it));
    }

    SCE_Scene_RenderEntities (scene);

    /* restauration des parametres par defaut */
    SCE_Light_Use (NULL);
    SCE_Light_ActivateLighting (SCE_FALSE);

    SCE_RSetActiveMatrix (SCE_MAT_MODELVIEW);
    SCE_RPopMatrix ();

    if (rendertarget)
        SCE_Texture_RenderTo (NULL, 0);

    SCE_RFlush ();
}


void SCE_Pick_Init (SCE_SPickResult *r)
{
    SCE_Line3_Init (&r->line);
    SCE_Line3_Init (&r->line2);
    SCE_Plane_Init (&r->plane);
    SCE_Plane_Init (&r->plane2);
    r->inst = NULL;
    r->index = 0;
    SCE_Vector3_Set (r->point, 0.0, 0.0, 0.0);
    r->distance = -1.0f;
    r->sup = SCE_FALSE;
    SCE_Vector3_Set (r->a, 0.0, 0.0, 0.0);
    SCE_Vector3_Set (r->b, 0.0, 0.0, 0.0);
    SCE_Vector3_Set (r->c, 0.0, 0.0, 0.0);
}

static int ProcessTriangle (SCE_TVector3 a, SCE_TVector3 b, SCE_TVector3 c,
                            SCEindices index, void *data)
{
    SCE_TVector3 p;
    SCE_SPickResult *r = data;

    if (SCE_Plane_TriangleLineIntersection (a, b, c, &r->line2, p)) {
        /* check if it is behind the view point */
        if (SCE_Plane_DistanceToPointv (&r->plane2, p) > 0.0f) {
            float d = SCE_Vector3_Distance (p, r->line2.o);
            if (d < r->distance || r->distance < 0.0) {
                r->distance = d;
                SCE_Vector3_Copy (r->point, p);
                r->sup = SCE_TRUE;
                r->index = index;
                SCE_Vector3_Copy (r->a, a);
                SCE_Vector3_Copy (r->b, b);
                SCE_Vector3_Copy (r->c, c);
            }
        }
    }

    return SCE_FALSE;
}

static int ProcessGeom (SCE_SGeometry *geom, SCE_SPickResult *r)
{
    r->sup = SCE_FALSE;
    SCE_Geometry_ForEachTriangle (geom, ProcessTriangle, r);
    return r->sup;
}

static void UpdateClosest (SCE_SSceneEntityInstance *inst, SCE_SPickResult *r)
{
    SCE_SGeometry *geom = NULL;
    SCE_SMesh *mesh = NULL;
    SCE_SSceneEntity *entity = NULL;
    SCE_SNode *node = NULL;
    SCE_TMatrix4 m;

    node = SCE_SceneEntity_GetInstanceNode (inst);
    SCE_Node_GetFinalMatrixv (node, m);
    SCE_Matrix4_InverseCopy (m);

    entity = SCE_SceneEntity_GetInstanceEntity (inst);
    mesh = SCE_SceneEntity_GetMesh (entity);
    geom = SCE_Mesh_GetGeometry (mesh);
    SCE_Line3_Mul (&r->line2, &r->line, m);
    SCE_Plane_SetFromPointv (&r->plane2, r->line2.n, r->line2.o);

    if (ProcessGeom (geom, r))
        r->inst = inst;
}

static int ClosestPointIsCloser (SCE_SBoundingBox *box, SCE_SPickResult *r)
{
    float *points = NULL;
    size_t i;

    /* initial value is lesser than 0 */
    if (r->distance < 0.0)
        return SCE_Collide_PlanesWithBBBool (&r->plane, 1, box);

    points = SCE_BoundingBox_GetPoints (box);

    for (i = 0; i < 8; i++) {
        /* ignore points behind the point of view */
        if (SCE_Plane_DistanceToPointv (&r->plane, &points[i * 3]) > 0.0f) {
            /* one point of the bounding box is closer from the view point
               than the actual collision position, we'll have to
               process a mesh/line intersection calculus */
            if (SCE_Vector3_Distance (r->line.o, &points[i * 3]) < r->distance)
                return SCE_TRUE;
        }
    }
    return SCE_FALSE;
}

static void SCE_Scene_PickClosest (SCE_SSceneOctree *stree, SCE_SPickResult *r)
{
    SCE_SBox tmp;
    SCE_SBoundingBox *box = NULL;
    SCE_SSceneEntityInstance *inst = NULL;
    SCE_SListIterator *it = NULL;
    SCE_SList *insts = NULL;
    size_t i;

    /* TODO: warning, 3 ! */
    for (i = 0; i < 3; i++) {
        insts = stree->instances[i];
        SCE_List_ForEach (it, insts) {
            inst = SCE_List_GetData (it);
            box = SCE_SceneEntity_PushInstanceBB (inst, &tmp);
            if (SCE_Collide_BBWithLine (box, &r->line)) {
                if (ClosestPointIsCloser (box, r))
                    UpdateClosest (inst, r);
            }
            SCE_SceneEntity_PopInstanceBB (inst, &tmp);
        }
    }
}

static void SCE_Scene_PickRec (SCE_SOctree *octree, SCE_SPickResult *r)
{
    SCE_SBoundingBox *box = NULL;

    box = SCE_Octree_GetBoundingBox (octree);

    SCE_BoundingBox_MakePlanes (box);
    /* check if it is behind the view point */
    if (!SCE_Collide_PlanesWithBBBool (&r->plane, 1, box) ||
        /* now check if the line intersects the box */
        !SCE_Collide_BBWithLine (box, &r->line) ||
        /* check if the octree is close enough */
        !ClosestPointIsCloser (box, r))
        return;

    SCE_Scene_PickClosest (SCE_Octree_GetData (octree), r);

    if (SCE_Octree_HasChildren (octree)) {
        size_t i;
        SCE_SOctree **children = SCE_Octree_GetChildren (octree);
        for (i = 0; i < 8; i++)
            SCE_Scene_PickRec (children[i], r);
    }
}

void SCE_Scene_Pick (SCE_SScene *scene, SCE_SCamera *cam, SCE_TVector2 point,
                     SCE_SPickResult *r)
{
    SCE_Camera_Line (cam, point, &r->line);
    SCE_Plane_SetFromPointv (&r->plane, r->line.n, r->line.o);
    SCE_Scene_PickRec (scene->octree, r);
}


static void SCE_Scene_DrawBB (SCE_SBoundingBox *box, SCE_TMatrix4 m)
{
    SCE_TVector3 center, dim;
    SCE_TMatrix4 mat;
    SCE_SBox *b;
    b = SCE_BoundingBox_GetBox (box);

    SCE_Box_GetCenterv (b, center);
    SCE_Box_GetDimensionsv (b, &dim[0], &dim[1], &dim[2]);
    SCE_Matrix4_Translatev (mat, center);
    SCE_Matrix4_MulCopy (mat, m);
    SCE_Matrix4_MulScalev (mat, dim);

    SCE_RPushMatrix ();
    SCE_RMultMatrix (mat);
    SCE_Mesh_Render ();
    SCE_RPopMatrix ();
}

static void SCE_Scene_DrawInstBB (SCE_SSceneEntityInstance *inst)
{
    SCE_Scene_DrawBB (SCE_SceneEntity_GetInstanceBB (inst),
                      SCE_Node_GetFinalMatrix (inst->node));
}

void SCE_Scene_DrawBoundingBoxes (SCE_SScene *scene)
{
    SCE_SSceneEntityInstance *inst = NULL;
    SCE_SListIterator *it = NULL;

    SCE_Mesh_Use (scene->bbmesh);
    SCE_List_ForEach (it, scene->selected) {
        inst = SCE_List_GetData (it);
        SCE_Scene_DrawInstBB (inst);
    }
    SCE_Mesh_Unuse ();
}


void SCE_Scene_DrawInstanceOctrees (SCE_SScene *scene,
                                    SCE_SSceneEntityInstance *inst)
{
    SCE_TMatrix4 id;
    SCE_SOctree *tree = NULL;
    SCE_SOctreeElement *el = NULL;

    el = SCE_Node_GetElement (SCE_SceneEntity_GetInstanceNode (inst));
    tree = el->octree;
    SCE_Matrix4_Identity (id);

    SCE_Mesh_Use (scene->bbmesh);
    while (tree) {
        SCE_Scene_DrawBB (SCE_Octree_GetBoundingBox (tree), id);
        tree = SCE_Octree_GetParent (tree);
    }
    SCE_Mesh_Unuse ();
}

/** @} */
