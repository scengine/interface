/*------------------------------------------------------------------------------
    SCEngine - A 3D real time rendering engine written in the C language
    Copyright (C) 2006-2013  Antony Martin <martin(dot)antony(at)yahoo(dot)fr>

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
   updated: 17/03/2013 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include "SCE/interface/SCEBatch.h"
#include "SCE/interface/SCEQuad.h"
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
    states->state = SCE_SCENE_STANDARD_STATE;
    states->clearcolor = states->cleardepth = SCE_TRUE;
    states->frustum_culling = SCE_FALSE;
    states->lighting = SCE_TRUE;
    states->lod = SCE_FALSE;
    states->deferred = SCE_FALSE;
    states->camera = NULL;
    states->rendertarget = NULL;
    states->cubeface = 0;
    states->skybox = NULL;
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

    for (i = 0; i < SCE_SCENE_STATES_STACK_SIZE; i++)
        SCE_Scene_InitStates (&scene->states[i]);
    scene->state = &scene->states[0];
    scene->state_id = 0;

    scene->rootnode = NULL;
    scene->node_updated = SCE_FALSE;
    scene->n_nodes = 0;
    scene->octree = NULL;
    scene->contribution_size = 0.01f;

    for (i = 0; i < SCE_NUM_SCENE_RESOURCE_GROUPS; i++)
        scene->rgroups[i] = NULL;

    scene->selected = NULL;
    scene->selected_join = NULL;

    SCE_List_Init (&scene->entities);
    SCE_List_Init (&scene->lights);
    SCE_List_SetFreeFunc2 (&scene->lights, SCE_Scene_RemoveLightNode, scene);
    SCE_List_Init (&scene->cameras);
    SCE_List_SetFreeFunc2 (&scene->cameras, SCE_Scene_RemoveCameraNode, scene);
    SCE_List_Init (&scene->sprites);
    /* TODO: remove sprite node? */

    scene->vterrain = NULL;
    scene->voterrain = NULL;

    scene->rclear = scene->gclear = scene->bclear = scene->aclear = 0.5;
    scene->dclear = 1.0;

    scene->bbmesh = NULL;
    scene->bsmesh = NULL;
    scene->bcmesh = NULL;

    scene->deferred = NULL;
    scene->deferred_shader = NULL;
}

#define SCE_SCENE_SPHERE_SUBDIV 16

static int SCE_Scene_MakeBoundingVolumes (SCE_SScene *scene)
{
    SCE_SBox box;
    SCE_SSphere sphere;
    SCE_SCone cone;
    SCE_SGeometry *geom = NULL;
    SCE_TVector3 v;
    float x;

    SCE_Vector3_Set (v, 0.0, 0.0, 0.0);
    SCE_Box_SetFromCenter (&box, v, 1.0, 1.0, 1.0);
    if (!(geom = SCE_BoxGeom_Create (&box, SCE_LINES, SCE_BOX_NONE_TEXCOORD,
                                     SCE_BOX_NONE_NORMALS)))
        goto fail;
    if (!(scene->bbmesh = SCE_Mesh_CreateFrom (geom, SCE_TRUE)))
        goto fail;
    SCE_Mesh_AutoBuild (scene->bbmesh);

    SCE_Sphere_Init (&sphere);
    /* offset so that the mesh wraps the sphere */
    x = 1.0 - cos (M_PI / SCE_SCENE_SPHERE_SUBDIV);
    SCE_Sphere_SetRadius (&sphere, 1.0f + x / (1.0f - x));
    if (!(geom = SCE_SphereGeom_CreateUV (&sphere, SCE_SCENE_SPHERE_SUBDIV,
                                          SCE_SCENE_SPHERE_SUBDIV)))
        goto fail;
    if (!(scene->bsmesh = SCE_Mesh_CreateFrom (geom, SCE_TRUE)))
        goto fail;
    SCE_Mesh_AutoBuild (scene->bsmesh);

    SCE_Cone_Init (&cone);
    SCE_Cone_SetRadius (&cone, 1.0f + x / (1.0f - x));
    SCE_Cone_SetHeight (&cone, 1.0f);
    if (!(geom = SCE_ConeGeom_Create (&cone, SCE_SCENE_SPHERE_SUBDIV)))
        goto fail;
    if (!(scene->bcmesh = SCE_Mesh_CreateFrom (geom, SCE_TRUE)))
        goto fail;
    SCE_Mesh_AutoBuild (scene->bcmesh);

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
    for (i = 0; i < SCE_NUM_SCENE_RESOURCE_GROUPS; i++) {
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
        SCE_Shader_Delete (scene->deferred_shader);
        SCE_List_Clear (&scene->cameras);
        SCE_List_Clear (&scene->lights);
        SCE_List_Clear (&scene->entities);
        for (i = 0; i < SCE_NUM_SCENE_RESOURCE_GROUPS; i++)
            SCE_SceneResource_DeleteGroup (scene->rgroups[i]);
        SCE_Octree_DeleteRecursive (scene->octree);
        SCE_Node_Delete (scene->rootnode);
        SCE_Mesh_Delete (scene->bbmesh);
        SCE_Mesh_Delete (scene->bsmesh);
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

SCE_SMesh* SCE_Scene_GetBoxMesh (SCE_SScene *scene)
{
    return scene->bbmesh;
}
SCE_SMesh* SCE_Scene_GetSphereMesh (SCE_SScene *scene)
{
    return scene->bsmesh;
}
SCE_SMesh* SCE_Scene_GetConeMesh (SCE_SScene *scene)
{
    return scene->bcmesh;
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
 * \todo this function is truely stupid.
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
    SCE_SNode *node = NULL;

    node = SCE_SceneEntity_GetInstanceNode (einst);
    SCE_Node_SetOnMovedCallback (node, NULL, NULL);
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
    if (res && !SCE_SceneResource_IsResourceAdded (res)) {
        SCE_SShader *shader = SCE_SceneResource_GetResource (res);
        if (scene->deferred)
            SCE_Deferred_BuildShader (scene->deferred, shader);
        SCE_Shader_Build (shader);
        SCE_Shader_SetupMatricesMapping (shader);
        SCE_Shader_ActivateMatricesMapping (shader, SCE_TRUE);
        SCE_Scene_AddResource (scene, SCE_SCENE_SHADERS_GROUP, res);
    }
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
 * \brief Adds a sprite to a scene
 * \param scene a scene
 * \param sprite a sprite
 */
void SCE_Scene_AddSprite (SCE_SScene *scene, SCE_SSprite *sprite)
{
    /* only render in standard render mode */
    SCE_SceneEntity_RemoveState (SCE_Sprite_GetEntity (sprite),
                                 SCE_SCENE_SHADOW_MAP_STATE);
    SCE_List_Appendl (&scene->sprites, SCE_Sprite_GetIterator (sprite));
}
/**
 * \brief Removes a sprite from a scene
 * \param scene a scene
 * \param sprite a sprite
 */
void SCE_Scene_RemoveSprite (SCE_SScene *scene, SCE_SSprite *sprite)
{
    SCE_List_Remove (SCE_Sprite_GetIterator (sprite));
}


/**
 * \brief Adds a resource to a scene
 * \param scene a scene
 * \param group resource type of \p res
 * \param res the resource to add
 * \todo remove it, use AddTexture, AddShader, etc.
 * \sa SCE_Scene_RemoveResource()
 */
void SCE_Scene_AddResource (SCE_SScene *scene, SCE_ESceneResourceGroup group,
                            SCE_SSceneResource *res)
{
    SCE_SceneResource_AddResource (scene->rgroups[group], res);
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
    scene->state->skybox = skybox;
    if (skybox) {
        SCE_TMatrix4 mat;
        SCE_SNode *node = SCE_Skybox_GetNode (skybox);
        SCE_Matrix4_Scale (mat, 42.0, 42.0, 42.0);
        SCE_Matrix4_MulTranslate (mat, -0.5, -0.5, -0.5);
        SCE_Node_SetMatrix (node, mat);
        /* only inherits translation from its parent node */
        SCE_Node_TransformTranslation (node);
    }
}

/**
 * \brief Sets the voxel terrain of the scene
 * \param scene a scene
 * \param vt a voxel terrain
 * \sa SCE_SVoxelTerrain, SCE_Scene_SetVoxelOctreeTerrain()
 */
void SCE_Scene_SetVoxelTerrain (SCE_SScene *scene, SCE_SVoxelTerrain *vt)
{
    scene->vterrain = vt;
}
/**
 * \brief Sets the voxel octree terrain of the scene
 * \param scene a scene
 * \param vt a voxel terrain
 * \sa SCE_SVoxelOctreeTerrain, SCE_Scene_SetVoxelTerrain()
 */
void SCE_Scene_SetVoxelOctreeTerrain (SCE_SScene *scene,
                                      SCE_SVoxelOctreeTerrain *vt)
{
    scene->voterrain = vt;
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
 * This function cannot be called once objects have been added to the scene.
 */
int SCE_Scene_MakeOctree (SCE_SScene *scene, unsigned int rec,
                          int loose, float margin)
{
    SCE_Scene_EraseOctreeInternal (scene->octree);
    /* dirty, free memory but keeps size data */
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

static const char *sce_def_vs =
    "varying vec3 normal;"
    "varying vec4 pos;"
    "uniform mat4 sce_projectionmatrix;"
    "uniform mat4 sce_modelviewmatrix;"
    "void main ()"
    "{"
    "  normal = normalize (mat3(sce_modelviewmatrix) * gl_Normal);"
    "  gl_Position = pos = sce_projectionmatrix * (sce_modelviewmatrix * gl_Vertex);"
    "}";


static const char *sce_def_ps =
    "varying vec3 normal;"
    "varying vec4 pos;"
    "void main ()"
    "{"
    "  sce_pack_position (pos);"
    "  sce_pack_normal (normalize (normal));"
    "  sce_pack_color (vec3 (1.0));"
    "}";



int SCE_Scene_SetDeferred (SCE_SScene *scene, SCE_SDeferred *def)
{
    if (!def) {
        scene->deferred = NULL;
        SCE_Shader_Delete (scene->deferred_shader);
        scene->deferred_shader = NULL;
    } else {
        SCE_Shader_Delete (scene->deferred_shader);
        scene->deferred_shader = NULL;

        scene->deferred = def;
        SCE_Scene_AddCamera (scene, def->cam);
        if (!(scene->deferred_shader = SCE_Shader_Create ())) goto fail;
        if (SCE_Shader_AddSource (scene->deferred_shader, SCE_VERTEX_SHADER,
                                  sce_def_vs, SCE_FALSE) < 0) goto fail;
        if (SCE_Shader_AddSource (scene->deferred_shader, SCE_PIXEL_SHADER,
                                  sce_def_ps, SCE_FALSE) < 0) goto fail;
        if (SCE_Deferred_BuildShader (def, scene->deferred_shader) < 0)
            goto fail;
        /* TODO: only if Deferred_BuildShader() doesnt... build the shader. */
        if (SCE_Shader_Build (scene->deferred_shader) < 0) goto fail;
        SCE_Shader_SetupMatricesMapping (scene->deferred_shader);
        SCE_Shader_ActivateMatricesMapping (scene->deferred_shader, SCE_TRUE);
        SCE_Scene_AddResource (scene, SCE_SCENE_SHADERS_GROUP,
                               SCE_Shader_GetSceneResource (scene->deferred_shader));
    }

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


int SCE_Scene_SetupBatching (SCE_SScene *scene, unsigned int n, int *order)
{
    if (SCE_Batch_SortEntities (&scene->entities, SCE_NUM_SCENE_RESOURCE_GROUPS,
                                scene->rgroups, 2, order) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}

int SCE_Scene_SetupDefaultBatching (SCE_SScene *scene)
{
    int order[] = {SCE_SCENE_SHADERS_GROUP, SCE_SCENE_TEXTURES0_GROUP};
    if (SCE_Batch_SortEntities (&scene->entities, SCE_NUM_SCENE_RESOURCE_GROUPS,
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
        if (SCE_SceneEntity_IsInstanceInFrustum (einst, scene->state->camera)) {
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
    size = SCE_Scene_GetOctreeSize (tree, scene->state->camera);
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

/* TODO: add SelectOctreeLights() */
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
                                              scene->state->camera);
    }
}

static void SCE_Scene_DetermineEntities (SCE_SScene *scene)
{
    SCE_SListIterator *it = NULL;
    SCE_SList *instances = scene->selected;
    SCE_List_ForEach (it, instances)
        SCE_SceneEntity_ReplaceInstanceToEntity (SCE_List_GetData (it));
}


static void SCE_Scene_FlushEntities (SCE_SList *entities)
{
    SCE_SListIterator *it = NULL;
    SCE_List_ForEach (it, entities)
        SCE_SceneEntity_Flush (SCE_List_GetData (it));
}

/**
 * \brief Prepares a scene for rendering
 * \param scene the scene to prepare
 * \param camera the camera used for the render, can be NULL then a default
 *        camera (the same one that SCE_Camera_Create() returns) is used
 * \param target the render target
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
                       SCE_STexture *target, SCE_EBoxFace cubeface)
{
    SCE_SFrustum *frustum = NULL;
    int fc;

    scene->state->rendertarget = target;
    scene->state->cubeface = cubeface;
    scene->state->camera = camera;

    fc = scene->state->frustum_culling;
    frustum = SCE_Camera_GetFrustum (scene->state->camera);

    if (scene->state->lod || fc)
        SCE_Scene_FlushEntities (&scene->entities);

    if (fc) {
        /* do it before nodes' update, otherwise the calls of
           List_Removel() can fail */
        SCE_List_BreakAll (scene->selected);
        SCE_List_Flush (scene->selected);
        scene->selected_join = scene->selected;
    }

    if (scene->state->skybox) {
        SCE_Node_Attach (SCE_Camera_GetNode (scene->state->camera),
                         SCE_Skybox_GetNode (scene->state->skybox));
    }

    SCE_Node_UpdateRootRecursive (scene->rootnode);
    SCE_Camera_Update (scene->state->camera);

    if (fc) {
        SCE_Octree_MarkVisibles (scene->octree, frustum);
        SCE_Scene_SelectVisibles (scene);
    }

    if (scene->state->lod)
        SCE_Scene_DetermineEntitiesUsingLOD (scene);
    else if (fc)
        SCE_Scene_DetermineEntities (scene);

    if (scene->vterrain)
        SCE_VTerrain_CullRegions (scene->vterrain, fc ? frustum : NULL);
    else if (scene->voterrain)
        SCE_VOTerrain_CullRegions (scene->voterrain, fc ? frustum : NULL);
}


/**
 * \internal
 * \brief Clears the buffers before the rendering
 */
void SCE_Scene_ClearBuffers (SCE_SScene *scene)
{
    SCEbitfield depthbuffer = 0;

    if (scene->state->cleardepth)
        depthbuffer = SCE_DEPTH_BUFFER_BIT;
    if (scene->state->clearcolor)
        depthbuffer |= SCE_COLOR_BUFFER_BIT;

    SCE_RClearColor (scene->rclear, scene->gclear, scene->bclear,scene->aclear);
    SCE_RClearDepth (scene->dclear);
    /* what does glClear(0)? */
    if (depthbuffer)
        SCE_RClear (depthbuffer);
}


static void SCE_Scene_RenderEntities (SCE_SScene *scene, SCE_SList *entities)
{
    SCE_SSceneEntity *entity = NULL;
    SCE_SListIterator *it;
    SCE_List_ForEach (it, entities) {
        entity = SCE_List_GetData (it);
        if (SCE_SceneEntity_HasInstance (entity) &&
            SCE_SceneEntity_MatchState (entity, scene->state->state)) {
            SCE_SceneEntity_UseResources (entity);
            SCE_SceneEntity_Render (entity);
            SCE_SceneEntity_UnuseResources (entity);
        }
    }
    SCE_Texture_Flush ();
    SCE_Material_Use (NULL);
    SCE_Shader_Use (NULL);
}

static void SCE_Scene_RenderSprites (SCE_SScene *scene, SCE_SList *sprites)
{
    SCE_SListIterator *it;
    SCE_List_ForEach (it, sprites) {
        SCE_Sprite_Render (SCE_List_GetData (it), scene->state->camera,
                           scene->state->state);
    }
    SCE_Texture_Flush ();
    SCE_Material_Use (NULL);
    SCE_Shader_Use (NULL);
}

static void SCE_Scene_RenderSkybox (SCE_SScene *scene, SCE_SCamera *cam)
{
    SCE_SSceneEntity *entity = SCE_Skybox_GetEntity (scene->state->skybox);

    SCE_SceneEntity_UseResources (entity);
    SCE_SceneEntity_Render (entity);
    SCE_SceneEntity_UnuseResources (entity);
    /* because skybox properties disabled it */
    SCE_RSetState (GL_DEPTH_TEST, SCE_TRUE);

    SCE_Texture_Flush ();
    SCE_Shader_Use (NULL);
}

/**
 * \brief Setup matrices according to the given camera
 * \param cam a camera
 * \sa SCE_RLoadMatrix(), SCE_RViewport()
 */
void SCE_Scene_UseCamera (SCE_SCamera *cam)
{
    SCE_SViewport *viewport = SCE_Camera_GetViewport (cam);
    SCE_RViewport (viewport->x, viewport->y, viewport->w, viewport->h);
    SCE_RLoadMatrix (SCE_MAT_PROJECTION, SCE_Camera_GetProj (cam));
    SCE_RLoadMatrix (SCE_MAT_CAMERA, SCE_Camera_GetFinalView (cam));
}


/* TODO: tmp. for some reason the scene disappears when using
   shadows + deferred + alphatest + looking at a kinda special angle... weird */
static void SCE_Scene_ResetEntityProperties (void)
{
    SCE_SSceneEntity a;
    SCE_SceneEntity_Init (&a);
    SCE_SceneEntity_ApplyProperties (&a);
}

static void SCE_Scene_ForwardRender (SCE_SScene *scene, SCE_SCamera *cam,
                                     SCE_STexture *target,
                                     SCE_EBoxFace cubeface)
{
    SCE_SListIterator *it = NULL;

    /* RenderTo() does call glViewport(), as does UseCamera(); in our case
       we want the camera viewport to take over target's */
    SCE_Texture_RenderTo (target, cubeface);
    SCE_Scene_UseCamera (cam);

    if (scene->state->skybox) {
        scene->state->clearcolor = SCE_FALSE;
        scene->state->cleardepth = SCE_TRUE;
    }
    SCE_Scene_ClearBuffers (scene);

    if (scene->state->skybox) {
        SCE_Light_ActivateLighting (SCE_FALSE);
        SCE_Scene_RenderSkybox (scene, cam);
    }

    if (!scene->state->lighting)
        SCE_Light_ActivateLighting (SCE_FALSE);
    else {
        SCE_Light_ActivateLighting (SCE_TRUE);
        /* TODO: only enable lights "close" (which appear big) to the camera */
        SCE_List_ForEach (it, &scene->lights)
            SCE_Light_Use (SCE_List_GetData (it));
    }

    if (scene->vterrain) {
        int mode = scene->state->state & SCE_SCENE_SHADOW_MAP_STATE;
        SCE_VTerrain_ActivateShadowMode (scene->vterrain, mode);
        SCE_VTerrain_Render (scene->vterrain);
    }
    if (scene->voterrain) {
        SCE_Scene_ResetEntityProperties ();
        SCE_VOTerrain_Render (scene->voterrain);
    }
    SCE_Scene_RenderEntities (scene, &scene->entities);
    SCE_Scene_ResetEntityProperties ();

    SCE_Light_Use (NULL);
    SCE_Light_ActivateLighting (SCE_FALSE);

    SCE_Scene_RenderSprites (scene, &scene->sprites);

    if (target)
        SCE_Texture_RenderTo (NULL, 0);

    SCE_RFlush ();
}

static void SCE_Scene_DrawBS (const SCE_SSphere*, const SCE_TMatrix4);

static void
SCE_Deferred_RenderPoint (SCE_SDeferred *def, SCE_SScene *scene,
                          SCE_SCamera *cam, SCE_SLight *light, int flags)
{
    float radius, near, x;
    SCE_TVector3 pos;
    SCE_SBoundingSphere bs;
    SCE_SNode *node = NULL;
    SCE_ELightType type = SCE_POINT_LIGHT;
    SCE_SDeferredLightingShader *shader = NULL;

    if (SCE_Light_GetShader (light))
        shader = &(SCE_Light_GetShader (light)[flags]);
    else
        shader = &def->shaders[type][flags];

    node = SCE_Light_GetNode (light);

    if (!(flags & SCE_DEFERRED_USE_SHADOWS)) {
        SCE_Shader_Use (shader->shader);
    } else {
        SCE_STexture *sm = def->shadowmaps[type]; /* shadow map */
        /* TODO: matrix type */
        float *mat = NULL;

        /* render shadow map */
        SCE_Camera_SetViewport (def->cam, 0, 0, def->sm_w, def->sm_h);
        SCE_Camera_SetProjection (def->cam, M_PI / 2.0, 1.0, 0.001,
                                  SCE_Light_GetRadius (light) + 0.001);

        /* attach the camera to the light :) yes, it's that simple */
        SCE_Node_Attach (node, SCE_Camera_GetNode (def->cam));
        SCE_Node_TransformNoScale (SCE_Camera_GetNode (def->cam));

        /* TODO: setup states */
        SCE_RSetState (GL_BLEND, SCE_FALSE);
        SCE_RActivateColorBuffer (SCE_FALSE); /* ensure depth-only rendering */
        SCE_Deferred_PopStates (def);
        SCE_Scene_PushStates (scene);
        SCE_Shader_Use (def->shadowcube_shader);
        SCE_Shader_Lock ();
        scene->state->state = SCE_SCENE_SHADOW_MAP_STATE;
        scene->state->lighting = SCE_FALSE;
        scene->state->deferred = SCE_FALSE;
        scene->state->skybox = NULL;
        scene->state->clearcolor = SCE_TRUE;
        scene->state->cleardepth = SCE_TRUE;
        scene->state->rendertarget = NULL;
        if (scene->vterrain)
            SCE_VTerrain_ActivatePointShadowMode (scene->vterrain, SCE_TRUE);

        /* rendering each face of the cubemap */
#define SCE_RDR(f) do {                                                 \
            mat = SCE_Node_GetMatrix (SCE_Camera_GetNode (def->cam),    \
                                      SCE_NODE_WRITE_MATRIX);           \
            SCE_Box_FaceOrientation (f, mat);                           \
            SCE_Node_HasMoved (SCE_Camera_GetNode (def->cam));          \
            SCE_Scene_Update (scene, def->cam, sm, f);                  \
            SCE_Scene_Render (scene, def->cam, sm, f);                  \
        } while (0)

        SCE_RDR (SCE_RENDER_POSX);
        SCE_RDR (SCE_RENDER_NEGX);
        SCE_RDR (SCE_RENDER_POSY);
        SCE_RDR (SCE_RENDER_NEGY);
        SCE_RDR (SCE_RENDER_POSZ);
        SCE_RDR (SCE_RENDER_NEGZ);
#undef SCE_RDR
        /* shadow cube map is now filled!1 */

        if (scene->vterrain)
            SCE_VTerrain_ActivatePointShadowMode (scene->vterrain, SCE_FALSE);

        /* reset camera */
        SCE_Node_Detach (SCE_Camera_GetNode (def->cam));
        SCE_Node_SetTransformCallback (SCE_Camera_GetNode (def->cam), NULL);
        SCE_Scene_AddNode (scene, SCE_Camera_GetNode (def->cam));

        SCE_Shader_Unlock ();
        SCE_Scene_PopStates (scene);
        SCE_Deferred_PushStates (def);
        SCE_RActivateColorBuffer (SCE_TRUE);
        /* TODO: LOL glnames + crap set state */
        SCE_RSetState (GL_BLEND, SCE_TRUE);
        SCE_RSetBlending (GL_ONE, GL_ONE);

        SCE_Texture_Use (def->shadowmaps[type]);
        SCE_Shader_Use (shader->shader);
        /* do shadow cube map fetch in object space */
        /* TODO: light's matrix missing: light rotations wont work */
        SCE_Shader_SetMatrix4 (shader->lightviewproj_loc,
                               SCE_Camera_GetFinalViewInverse (cam));
    }

    /* get light's position in view space */
    SCE_Light_GetPositionv (light, pos);
    radius = SCE_Light_GetRadius (light);

    SCE_Matrix4_MulV3Copy (SCE_Camera_GetFinalView (cam), pos);
    SCE_Shader_SetParam3fv (shader->lightpos_loc, 1, pos);
    SCE_Shader_SetParam3fv (shader->lightcolor_loc, 1,
                            SCE_Light_GetColor (light));
    SCE_Shader_SetParamf (shader->lightradius_loc, radius);

    SCE_Light_GetPositionv (light, pos);
    SCE_BoundingSphere_Setv (&bs, pos, radius);
    /* wrapping sphere around geometry (see SCE_Scene_MakeBoundingVolumes()) */
    x = 1.0 - cos (M_PI / SCE_SCENE_SPHERE_SUBDIV);
    radius *= 1.0f + x / (1.0f - x);
    near = SCE_Camera_GetNear (cam) * 2.0;
    radius += near;
    SCE_Sphere_SetRadius (SCE_BoundingSphere_GetSphere (&bs), radius);

    SCE_Camera_GetPositionv (cam, pos);
    if (SCE_Collide_BSWithPointv (&bs, pos)) {
        SCE_RLoadMatrix (SCE_MAT_CAMERA, sce_matrix4_id);
        SCE_RLoadMatrix (SCE_MAT_OBJECT, sce_matrix4_id);
        SCE_RLoadMatrix (SCE_MAT_PROJECTION, sce_matrix4_id);
        SCE_Quad_Draw (-1.0, -1.0, 2.0, 2.0);
    } else {
        SCE_Scene_UseCamera (cam);
        /* TODO: gl keywords */
        SCE_RSetState (GL_CULL_FACE, SCE_TRUE);
        /* using old radius for rendering, otherwise the mesh could be
           clipped by the near plane, thus resulting in an huge artifact */
        SCE_Sphere_SetRadius (SCE_BoundingSphere_GetSphere (&bs),
                              SCE_Light_GetRadius (light));

        SCE_Mesh_Use (scene->bsmesh);
        SCE_Scene_DrawBS (SCE_BoundingSphere_GetSphere (&bs),
                          sce_matrix4_id);
        SCE_Mesh_Unuse ();
        SCE_RSetState (GL_CULL_FACE, SCE_FALSE);
    }
}



/**
 * \brief Achieves CSM translations on a per-pixel basis
 * \param light a light with its last frame position
 * \param w width of a pixel
 * \param h height of a pixel
 * \param mat position matrix of the light for the current frame
 */
static void SCE_Deferred_CSMMoveLight (SCE_SLight *light,
                                       float w, float h, SCE_TMatrix4 mat)
{
    SCE_TVector3 x, y, z;
    SCE_TVector3 origin, pos, new_pos;
    float dw, dh, dd;           /* distances */

    SCE_Vector3_Set (origin, 0.0, 0.0, 0.0);
    SCE_Matrix4_GetBase (mat, x, y, z);
    SCE_Matrix4_GetTranslation (mat, pos);

    dw = SCE_Plane_DistanceAlong (pos, origin, x);
    dh = SCE_Plane_DistanceAlong (pos, origin, y);
    dd = SCE_Plane_DistanceAlong (pos, origin, z);

    dw = w * (int)(dw / w);
    dh = h * (int)(dh / h);

    SCE_Vector3_Operator1v (pos, = -dw *, x);
    SCE_Vector3_Operator1v (pos, -= dh *, y);
    SCE_Vector3_Operator1v (pos, -= dd *, z);

    SCE_Matrix4_SetTranslation (mat, pos);
}

/**
 * \brief Computes splitting planes for Cascaded Shadow Maps
 * \param lambda weight for linear (0.0) and logarithmic (1.0) splits
 * \param near
 * \param far
 * \param splits
 * \param n_splits
 */
static void SCE_Deferred_CSMSplits (float lambda, float near, float far,
                                    float *splits, size_t n_splits)
{
    size_t i;

    for (i = 0; i <= n_splits; i++) {
        /* logarithmic split */
        float slog = near * pow (far / near, (double)i / n_splits);
        /* linear split */
        float slin = near + i * (far - near) / n_splits;
        splits[i] = lambda * slog + (1.0 - lambda) * slin;
    }
}

static void
SCE_Deferred_RenderSun (SCE_SDeferred *def, SCE_SScene *scene,
                        SCE_SCamera *cam, SCE_SLight *light, int flags)
{
    SCE_TVector3 pos;
    SCE_ELightType type = SCE_SUN_LIGHT;
    SCE_SDeferredLightingShader *shader = NULL;

    if (SCE_Light_GetShader (light))
        shader = &(SCE_Light_GetShader (light)[flags]);
    else
        shader = &def->shaders[type][flags];

    if (!(flags & SCE_DEFERRED_USE_SHADOWS)) {
        SCE_Shader_Use (shader->shader);
    } else {
        SCE_TVector3 dir;
        int i;
        SCE_STexture *sm = def->shadowmaps[type]; /* shadow map */
        float dist;
        float splits[SCE_MAX_DEFERRED_CASCADED_SPLITS + 1];
        SCE_TMatrix4 matrices[SCE_MAX_DEFERRED_CASCADED_SPLITS];
        float far;

        dist = 10000.0;  /* TODO: use octree's size to setup the distance */

        SCE_Light_GetPositionv (light, dir);
        SCE_Vector3_Operator1 (dir, *=, -1.0);
        SCE_Vector3_Normalize (dir);

        /* TODO: setup states */
        SCE_RSetState (GL_BLEND, SCE_FALSE);
        SCE_RActivateColorBuffer (SCE_FALSE); /* ensure depth-only rendering */
        SCE_Deferred_PopStates (def);
        SCE_Scene_PushStates (scene);
        SCE_Shader_Use (NULL);
        SCE_Shader_Lock ();
        scene->state->state = SCE_SCENE_SHADOW_MAP_STATE;
        scene->state->lighting = SCE_FALSE;
        scene->state->deferred = SCE_FALSE;
        scene->state->skybox = NULL;
        scene->state->clearcolor = SCE_TRUE; /* wtf? */
        scene->state->cleardepth = SCE_TRUE;
        scene->state->rendertarget = NULL;

        /* setup splits */
        far = def->csm_far > 0.0 ? def->csm_far : SCE_Camera_GetFar (cam);
        /* TODO: make lambda modifiable by the user */
        SCE_Deferred_CSMSplits (0.8, SCE_Camera_GetNear (cam), far, splits,
                                def->cascaded_splits);

        /* rendering each split */
        for (i = 0; i < def->cascaded_splits; i++) {
            /* TODO: matrix type */
            float *proj = NULL;
            SCE_TMatrix4 mat;

            /* render shadow map */
            SCE_Camera_SetViewport (def->cam, def->sm_w * i, 0,
                                    def->sm_w, def->sm_h);

            proj = SCE_Camera_GetProj (def->cam);
            SCE_Frustum_Slice (SCE_Camera_GetFrustum (cam), splits[i],
                               splits[i + 1], dir, dist, proj, mat);

            SCE_Deferred_CSMMoveLight (
                light, SCE_Matrix4_GetOrthoWidth (proj) / def->sm_w,
                SCE_Matrix4_GetOrthoHeight (proj) / def->sm_h, mat);

            SCE_Node_SetMatrix (SCE_Camera_GetNode (def->cam), mat);

            SCE_Node_HasMoved (SCE_Camera_GetNode (def->cam));
            SCE_Scene_Update (scene, def->cam, sm, 0);
            SCE_Scene_Render (scene, def->cam, sm, 0);

            /* dont clear the shadow map for further renders */
            scene->state->clearcolor = SCE_FALSE;
            scene->state->cleardepth = SCE_FALSE;

            /* store modelview projection matrix of the current camera */
            SCE_Matrix4_Copy (matrices[i],
                              SCE_Camera_GetFinalViewProj (def->cam));
            /* unpacked positions are in view space, need to put them back
               in world space */
            SCE_Matrix4_MulCopy (matrices[i],
                                 SCE_Camera_GetFinalViewInverse (cam));
        }
        /* shadow map is now filled!1 */

        SCE_Shader_Unlock ();
        SCE_Scene_PopStates (scene);
        SCE_Deferred_PushStates (def);

        SCE_RActivateColorBuffer (SCE_TRUE);
        /* TODO: LOL glnames + crap set state */
        SCE_RSetState (GL_BLEND, SCE_TRUE);
        SCE_RSetBlending (GL_ONE, GL_ONE);

        SCE_Texture_Use (def->shadowmaps[type]);
        SCE_Shader_Use (shader->shader);

        SCE_Shader_SetMatrix4v (shader->lightviewproj_loc, matrices,
                                def->cascaded_splits);
        SCE_Shader_SetParam (shader->csmnumsplits_loc, def->cascaded_splits);
    }

    /* get light's position in view space */
    SCE_Light_GetPositionv (light, pos);
    SCE_Matrix4_MulV3Copyw (SCE_Camera_GetFinalView (cam), pos, 0.0);
    /* direction is -position */
    SCE_Vector3_Operator1 (pos, *=, -1.0);
    SCE_Vector3_Normalize (pos); /* just in case */
    SCE_Shader_SetParam3fv (shader->lightdir_loc, 1, pos);
    SCE_Shader_SetParam3fv (shader->lightcolor_loc, 1,
                            SCE_Light_GetColor (light));

    SCE_RLoadMatrix (SCE_MAT_CAMERA, sce_matrix4_id);
    SCE_RLoadMatrix (SCE_MAT_OBJECT, sce_matrix4_id);
    SCE_RLoadMatrix (SCE_MAT_PROJECTION, sce_matrix4_id);
    SCE_Quad_Draw (-1.0, -1.0, 2.0, 2.0);

    if (flags & SCE_DEFERRED_USE_SHADOWS)
        SCE_Texture_Use (NULL); /* def->shadowmaps[type] */
}


static void SCE_Scene_DrawBC (const SCE_SCone*, const SCE_TMatrix4);

static void
SCE_Deferred_RenderSpot (SCE_SDeferred *def, SCE_SScene *scene,
                         SCE_SCamera *cam, SCE_SLight *light, int flags)
{
    float near, angle;
    SCE_TVector3 pos, dir;
    SCE_SCone cone;
    SCE_SNode *node = NULL;
    SCE_ELightType type = SCE_SPOT_LIGHT;
    SCE_SDeferredLightingShader *shader = NULL;

    if (SCE_Light_GetShader (light))
        shader = &(SCE_Light_GetShader (light)[flags]);
    else
        shader = &def->shaders[type][flags];

    /* cone angle and height required to make shadow map projection matrix */
    node = SCE_Light_GetNode (light);
    SCE_Cone_Copy (&cone, SCE_Light_GetCone (light));
    SCE_Cone_Push (&cone, SCE_Node_GetFinalMatrix (node), NULL);

    if (!(flags & SCE_DEFERRED_USE_SHADOWS)) {
        SCE_Shader_Use (shader->shader);
    } else {
        /* render shadow map */
        SCE_Camera_SetViewport (def->cam, 0, 0, def->sm_w, def->sm_h);
        SCE_Camera_SetProjectionFromCone (def->cam, &cone, 0.1);

        /* attach the camera to the light :) yes, it's that simple */
        SCE_Node_Attach (node, SCE_Camera_GetNode (def->cam));
        SCE_Node_SetMatrix (SCE_Camera_GetNode (def->cam), sce_matrix4_id);

        /* TODO: setup states */
        SCE_RSetState (GL_BLEND, SCE_FALSE);
        SCE_RActivateColorBuffer (SCE_FALSE); /* ensure depth-only rendering */
        SCE_Deferred_PopStates (def);
        SCE_Scene_PushStates (scene);
        SCE_Shader_Use (NULL);
        SCE_Shader_Lock ();
        scene->state->state = SCE_SCENE_SHADOW_MAP_STATE;
        scene->state->lighting = SCE_FALSE;
        scene->state->deferred = SCE_FALSE;
        scene->state->skybox = NULL;
        scene->state->clearcolor = SCE_TRUE;
        scene->state->cleardepth = SCE_TRUE;
        scene->state->rendertarget = NULL;
        /* rendering */
        SCE_Scene_Update (scene, def->cam, def->shadowmaps[type], 0);
        SCE_Scene_Render (scene, def->cam, def->shadowmaps[type], 0);
        SCE_Shader_Unlock ();
        SCE_Scene_PopStates (scene);
        SCE_Deferred_PushStates (def);
        SCE_RActivateColorBuffer (SCE_TRUE);
        /* TODO: LOL glnames + crap set state */
        SCE_RSetState (GL_BLEND, SCE_TRUE);
        SCE_RSetBlending (GL_ONE, GL_ONE);

        SCE_Texture_Use (def->shadowmaps[type]);
        SCE_Shader_Use (shader->shader);
        {
            SCE_TMatrix4 mat;
            SCE_Matrix4_Copy (mat, SCE_Camera_GetFinalViewProj (def->cam));
            SCE_Matrix4_MulCopy (mat, SCE_Camera_GetFinalViewInverse (cam));
            /* unpacked positions are in view space, need to put them back
               in world space */
            SCE_Shader_SetMatrix4 (shader->lightviewproj_loc, mat);
        }

        /* reset camera */
        SCE_Node_Detach (SCE_Camera_GetNode (def->cam));
        SCE_Scene_AddNode (scene, SCE_Camera_GetNode (def->cam));
    }

    /* get light's position in view space */
    SCE_Light_GetPositionv (light, pos);
    SCE_Matrix4_MulV3Copy (SCE_Camera_GetFinalView (cam), pos);
    /* orientation also in view space */
    SCE_Light_GetOrientationv (light, dir);
    SCE_Matrix4_MulV3Copyw (SCE_Camera_GetFinalView (cam), dir, 0.0);
    SCE_Shader_SetParam3fv (shader->lightpos_loc, 1, pos);
    SCE_Shader_SetParam3fv (shader->lightdir_loc, 1, dir);
    SCE_Shader_SetParam3fv (shader->lightcolor_loc, 1,
                            SCE_Light_GetColor (light));
    SCE_Shader_SetParamf (shader->lightradius_loc,
                          SCE_Light_GetHeight (light));
    /* cosine of the angle, not radians */
    angle = SCE_Math_Cosf (SCE_Light_GetAngle (light));
    SCE_Shader_SetParamf (shader->lightangle_loc, angle);
    SCE_Shader_SetParamf (shader->lightattenuation_loc,
                          1.0 / SCE_Light_GetAttenuation (light));

    near = SCE_Camera_GetNear (cam) * 2.0;
    SCE_Cone_Offset (&cone, near);

    SCE_Camera_GetPositionv (cam, pos);
    if (SCE_Collide_BCWithPointv (&cone, pos)) {
        SCE_RLoadMatrix (SCE_MAT_CAMERA, sce_matrix4_id);
        SCE_RLoadMatrix (SCE_MAT_OBJECT, sce_matrix4_id);
        SCE_RLoadMatrix (SCE_MAT_PROJECTION, sce_matrix4_id);
        SCE_Quad_Draw (-1.0, -1.0, 2.0, 2.0);
    } else {
        /* TODO: use SCE_Deferred_PopStates() ? */
        SCE_Scene_UseCamera (cam);
        /* TODO: gl keywords */
        SCE_RSetState (GL_CULL_FACE, SCE_TRUE);
        /* remove near plane threshold for rendering, otherwise
           the mesh could be clipped by the near plane, thus resulting
           in an huge artifact */
        SCE_Cone_Offset (&cone, -near);

        SCE_Mesh_Use (scene->bcmesh);
        SCE_Scene_DrawBC (&cone, sce_matrix4_id);
        SCE_Mesh_Unuse ();
        SCE_RSetState (GL_CULL_FACE, SCE_FALSE);
        SCE_RLoadMatrix (SCE_MAT_CAMERA, sce_matrix4_id);
        SCE_RLoadMatrix (SCE_MAT_OBJECT, sce_matrix4_id);
        SCE_RLoadMatrix (SCE_MAT_PROJECTION, sce_matrix4_id);
    }

    if (flags & SCE_DEFERRED_USE_SHADOWS)
        SCE_Texture_Use (NULL);
}


void SCE_Deferred_Render (SCE_SDeferred *def, void *scene_,
                          SCE_SCamera *cam, SCE_STexture *target,
                          SCE_EBoxFace cubeface)
{
    int i;
    SCE_SListIterator *it = NULL;
    SCE_SScene *scene = scene_;

    /* Texture_RenderTo() does call glViewport() so setting up the camera
       before forces the viewport to scale to the G-buffer */
    SCE_Scene_UseCamera (cam);

    /* fillup G-buffer */
    SCE_Texture_RenderTo (def->gbuf, 0);
    /* TODO: make it parameterizable, forcing to clear the buffer is bad */
    scene->state->clearcolor = SCE_TRUE;
    scene->state->cleardepth = SCE_TRUE;
    SCE_Scene_ClearBuffers (scene);
    SCE_Light_ActivateLighting (SCE_FALSE);

    /* rendering with default shader */
    SCE_SceneEntity_SetDefaultShader (scene->deferred_shader);
    if (scene->vterrain) {
        int mode = scene->state->state & SCE_SCENE_SHADOW_MAP_STATE;
        SCE_VTerrain_ActivateShadowMode (scene->vterrain, mode);
        SCE_VTerrain_Render (scene->vterrain);
    }
    if (scene->voterrain) {
        SCE_Scene_ResetEntityProperties ();
        SCE_VOTerrain_Render (scene->voterrain);
    }
    SCE_Scene_RenderEntities (scene, &scene->entities);
    SCE_Scene_ResetEntityProperties ();
    SCE_SceneEntity_SetDefaultShader (NULL);

    scene->state->rendertarget = def->targets[SCE_DEFERRED_LIGHT_TARGET];
    scene->state->cubeface = 0;
    SCE_Texture_RenderTo (def->targets[SCE_DEFERRED_LIGHT_TARGET], 0);

    /* setup states */
    SCE_Deferred_PushStates (def);

    SCE_RClearColor (def->amb_color[0], def->amb_color[1],
                     def->amb_color[2], 0.0);
    SCE_RClear (SCE_COLOR_BUFFER_BIT);

    if (scene->state->lighting) {

        /* setup additive blending */
        SCE_RSetState (GL_BLEND, SCE_TRUE);
        SCE_RSetBlending (GL_ONE, GL_ONE);

        /* TODO: overhead for unused shaders */
        /* setup uniform parameters of shaders */
        for (i = 0; i < SCE_NUM_LIGHT_TYPES; i++) {
            int j;
            for (j = 0; j < SCE_NUM_DEFERRED_LIGHT_FLAGS; j++) {
                SCE_Shader_Use (def->shaders[i][j].shader);
                SCE_Shader_SetMatrix4 (def->shaders[i][j].invproj_loc,
                                       SCE_Camera_GetProjInverse (cam));
            }
        }

        /* TODO: tip for shadows:
                 lights inside the view frustum do not need to update the scene
                 whilst those which are outside can cast shadows from
                 invisible objects, removed by frustum culling :> */

        SCE_List_ForEach (it, &scene->lights) {
            int flags = 0;
            SCE_SLight *light = SCE_List_GetData (it);
            SCE_ELightType type = SCE_Light_GetType (light);

            if (SCE_Light_GetShadows (light))
                flags |= SCE_DEFERRED_USE_SHADOWS;
            if (SCE_Light_GetSpecular (light))
                flags |= SCE_DEFERRED_USE_SPECULAR;

            /* apply flags mask from the deferred renderer */
            flags &= def->lightflags_mask;

            if (SCE_Light_GetShader (light)) {
                SCE_SDeferredLightingShader *shd = SCE_Light_GetShader (light);
                SCE_Shader_Use (shd[flags].shader);
                SCE_Shader_SetMatrix4 (shd[flags].invproj_loc,
                                       SCE_Camera_GetProjInverse (cam));
            }

            switch (type) {
            case SCE_POINT_LIGHT:
                SCE_Deferred_RenderPoint (def, scene, cam, light, flags);
                break;
            case SCE_SPOT_LIGHT:
                SCE_Deferred_RenderSpot (def, scene, cam, light, flags);
                break;
            case SCE_SUN_LIGHT:
                SCE_Deferred_RenderSun (def, scene, cam, light, flags);
                break;
            default:            /* onoes */
                break;
            }
        }

        /* reset blending state */
        SCE_RSetState (GL_BLEND, SCE_FALSE);
    }

    /* reset states */
    SCE_Shader_Use (NULL);
    SCE_Deferred_PopStates (def);

    /* final pass */
    SCE_Texture_RenderTo (target, cubeface);

    SCE_RLoadMatrix (SCE_MAT_OBJECT, sce_matrix4_id);
    SCE_RLoadMatrix (SCE_MAT_PROJECTION, sce_matrix4_id);
    SCE_RLoadMatrix (SCE_MAT_CAMERA, sce_matrix4_id);
    SCE_RLoadMatrix (SCE_MAT_TEXTURE, sce_matrix4_id);

    SCE_RSetState (GL_CULL_FACE, SCE_FALSE);
    /* TODO: warning, previous call to ClearBuffers() whether shadows are
       enabled or not may have modified SCE_RClearDepth() too */
    SCE_RClearColor (scene->rclear, scene->gclear, scene->bclear,scene->aclear);
    SCE_RClear (SCE_COLOR_BUFFER_BIT | SCE_DEPTH_BUFFER_BIT);
    SCE_Texture_BeginLot ();
    SCE_Texture_Use (def->targets[SCE_DEFERRED_DEPTH_TARGET]);
    SCE_Texture_Use (def->targets[SCE_DEFERRED_LIGHT_TARGET]);
    SCE_Texture_Use (def->targets[SCE_DEFERRED_COLOR_TARGET]);
    SCE_Texture_EndLot ();
    SCE_Shader_Use (def->final_shader);
    SCE_Quad_Draw (-1.0, -1.0, 2.0, 2.0);
    SCE_Shader_Use (NULL);
    SCE_Texture_Flush ();
    SCE_RSetState (GL_CULL_FACE, SCE_TRUE);

    /* render sprites */
    SCE_Scene_RenderSprites (scene, &scene->sprites);

    /* render skybox */
    if (scene->state->skybox) {
        SCE_SSceneEntity *entity = SCE_Skybox_GetEntity (scene->state->skybox);
        SCE_Scene_UseCamera (cam);
        entity->props.depthtest = SCE_TRUE;
        entity->props.depthscale = SCE_TRUE;
        entity->props.depthrange[0] = 1.0 - SCE_EPSILONF;
        entity->props.depthrange[1] = 1.0 - SCE_EPSILONF;
        SCE_RActivateDepthBuffer (SCE_FALSE);
        SCE_Scene_RenderSkybox (scene, cam);
        SCE_RActivateDepthBuffer (SCE_TRUE);
    }

    if (target)
        SCE_Texture_RenderTo (NULL, 0);
}

/**
 * \brief Renders a scene into a render target
 * \param scene a scene
 * \param cam the camera used for the render or NULL to keep the current one.
 * \param target the render target or NULL to keep the current one. If
 *        both this parameter and the current render targed are NULL, the
 *        default OpenGL's render buffer will be used as the render target.
 * \param cubeface see SCE_Scene_Update()'s cubeface parameter.
 * \see SCE_Scene_Update()
 */
void SCE_Scene_Render (SCE_SScene *scene, SCE_SCamera *cam,
                       SCE_STexture *target, SCE_EBoxFace cubeface)
{
    if (!cam)
        cam = scene->state->camera;
    if (!target)
        target = scene->state->rendertarget;
    if (cubeface < 0)
        cubeface = scene->state->cubeface;

    if (scene->state->deferred)
        SCE_Deferred_Render (scene->deferred, scene, cam, target, cubeface);
    else
        SCE_Scene_ForwardRender (scene, cam, target, cubeface);
}


static void SCE_Scene_CopyStates (SCE_SSceneStates *dst,
                                  const SCE_SSceneStates *src)
{
    memcpy (dst, src, sizeof *src);
}

void SCE_Scene_PushStates (SCE_SScene *scene)
{
    if (scene->state_id < SCE_SCENE_STATES_STACK_SIZE - 1) {
        unsigned int s = scene->state_id;
        scene->state = &scene->states[s + 1];
        SCE_Scene_CopyStates (scene->state, &scene->states[s]);
        scene->state_id++;
    }
}
void SCE_Scene_PopStates (SCE_SScene *scene)
{
    if (scene->state > 0) {
        scene->state_id--;
        scene->state = &scene->states[scene->state_id];
        SCE_Texture_RenderTo (scene->state->rendertarget,
                              scene->state->cubeface);
    }
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

    /* apply scaling */
    SCE_Vector3_Operator1v (a, *=, r->scale);
    SCE_Vector3_Operator1v (b, *=, r->scale);
    SCE_Vector3_Operator1v (c, *=, r->scale);

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
    SCE_Matrix4_GetScale (m, r->scale);
    SCE_Matrix4_NoScaling (m);
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
            SCE_SSceneEntity *entity = NULL;
            inst = SCE_List_GetData (it);
            entity = SCE_SceneEntity_GetInstanceEntity (inst);
            if (SCE_SceneEntity_GetProperties (entity)->pickable) {
                box = SCE_SceneEntity_PushInstanceBB (inst, &tmp);
                if (SCE_Collide_BBWithLine (box, &r->line)) {
                    if (ClosestPointIsCloser (box, r))
                        UpdateClosest (inst, r);
                }
                SCE_SceneEntity_PopInstanceBB (inst, &tmp);
            }
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


static void SCE_Scene_DrawBB (SCE_SBoundingBox *box, const SCE_TMatrix4 m)
{
    SCE_TVector3 center, dim;
    SCE_TMatrix4 mat;
    SCE_SBox *b;
    b = SCE_BoundingBox_GetBox (box);

    SCE_Box_GetCenterv (b, center);
    SCE_Box_GetDimensionsv (b, dim);
    SCE_Matrix4_Translatev (mat, center);
    SCE_Matrix4_MulCopy (mat, m);
    SCE_Matrix4_MulScalev (mat, dim);

    SCE_RLoadMatrix (SCE_MAT_OBJECT, mat);
    SCE_Mesh_Render ();
}

static void SCE_Scene_DrawBS (const SCE_SSphere *s, const SCE_TMatrix4 m)
{
    SCE_TVector3 center;
    float radius;
    SCE_TMatrix4 mat;

    SCE_Sphere_GetCenterv (s, center);
    radius = SCE_Sphere_GetRadius (s);
    SCE_Matrix4_Translatev (mat, center);
    SCE_Matrix4_MulCopy (mat, m);
    SCE_Matrix4_MulScale (mat, radius, radius, radius);

    SCE_RLoadMatrix (SCE_MAT_OBJECT, mat);
    SCE_Mesh_Render ();
}

static void SCE_Scene_DrawBC (const SCE_SCone *c, const SCE_TMatrix4 m)
{
    SCE_TVector3 pos;
    float radius, height;
    SCE_TMatrix4 mat;

    SCE_Cone_GetPositionv (c, pos);
    radius = SCE_Cone_GetRadius (c);
    height = SCE_Cone_GetHeight (c);
    SCE_Matrix4_Translatev (mat, pos);
    SCE_Matrix4_MulCopy (mat, m);
    SCE_Matrix4_MulScale (mat, radius, radius, height);

    SCE_RLoadMatrix (SCE_MAT_OBJECT, mat);
    SCE_Mesh_Render ();
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
