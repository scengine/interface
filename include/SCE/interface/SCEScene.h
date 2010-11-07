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
 
/* created: 19/01/2007
   updated: 06/11/2009 */

#ifndef SCESCENE_H
#define SCESCENE_H

/* external dependencies */
#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

/* internal dependencies */
#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEMaterial.h"
#include "SCE/interface/SCESceneEntity.h"
#include "SCE/interface/SCEModel.h"
#include "SCE/interface/SCESkybox.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup scene
 * @{
 */

#define SCE_SCENE_SHADERS_GROUP 0
#define SCE_SCENE_MATERIALS_GROUP 1
#define SCE_SCENE_TEXTURES0_GROUP 2
#define SCE_SCENE_NUM_RESOURCE_GROUP 3

/* default octree size */
#define SCE_SCENE_OCTREE_SIZE (16384.0)

typedef int (*SCE_FSceneForEachEntityGroupFunc)(SCE_SSceneEntityGroup *g,
                                                void *p);

/** \copydoc sce_sscenestates */
typedef struct sce_sscenestates SCE_SSceneStates;
/**
 * \brief States of a scene
 */
struct sce_sscenestates
{
    int clearcolor, cleardepth; /**< Clear the buffers? */
    int frustum_culling; /**< Use frustum culling? */
    int lighting;        /**< Use lighting? */
    int lod;             /**< Use LOD? */
};

/** \copydoc sce_sscene */
typedef struct sce_sscene SCE_SScene;
/**
 * \brief Scene description structure
 */
struct sce_sscene
{
    SCE_SSceneStates states;    /**< Scene's states */
    SCE_SNode *rootnode;        /**< Root node */
    int node_updated;           /**< Defines the root node's status */
    unsigned int n_nodes;       /**< Number of nodes attached to \c rootnode */

    SCE_SOctree *octree;        /**< Scene's octree */

    float contribution_size;    /**< Contribution culling size */

    /** All the resources of the scene */
    SCE_SSceneResourceGroup *rgroups[SCE_SCENE_NUM_RESOURCE_GROUP];

    SCE_SList *instances;       /**< Instances */
    SCE_SList *selected;        /**< Selected instances */
    SCE_SList *selected_join;   /**< Where to join lists */

    SCE_SList entities;         /**< Scene's entities */
    SCE_SList lights;           /**< Scene's lights list */
    SCE_SList cameras;          /**< Cameras in the scene */

    SCE_SSkybox *skybox;        /**< Scene skybox */

    /** Color buffer clear values */
    float rclear, gclear, bclear, aclear;
    float dclear;               /**< Depth buffer clear value */
    SCE_STexture *rendertarget; /**< Scene's render target */
    SCEuint cubeface;           /**< Face of the cubemap (used if
                                 * \c rendertarget is a cubemap) */
    SCE_SCamera *camera;        /**< The camera of the current render */

    SCE_SMesh *bbmesh;
    SCE_SMesh *bsmesh;
};

/** @} */

int SCE_Init_Scene (void);
void SCE_Quit_Scene (void);

SCE_SScene* SCE_Scene_Create (void);
void SCE_Scene_Delete (SCE_SScene*);

SCE_SNode* SCE_Scene_GetRootNode (SCE_SScene*);

void SCE_Scene_OnNodeMoved (SCE_SNode*, void*);

void SCE_Scene_AddNode (SCE_SScene*, SCE_SNode*);
void SCE_Scene_AddNodeRecursive (SCE_SScene*, SCE_SNode*);
void SCE_Scene_RemoveNode (SCE_SScene*, SCE_SNode*);

void SCE_Scene_AddInstance (SCE_SScene*, SCE_SSceneEntityInstance*);
void SCE_Scene_RemoveInstance (SCE_SScene*, SCE_SSceneEntityInstance*);

void SCE_Scene_AddEntityResources (SCE_SScene*, SCE_SSceneEntity*);
void SCE_Scene_RemoveEntityResources (SCE_SScene*, SCE_SSceneEntity*);
void SCE_Scene_AddEntity (SCE_SScene*, SCE_SSceneEntity*);
void SCE_Scene_RemoveEntity (SCE_SScene*, SCE_SSceneEntity*);

void SCE_Scene_AddModel (SCE_SScene*, SCE_SModel*);
void SCE_Scene_RemoveModel (SCE_SScene*, SCE_SModel*);
/*int SCE_Scene_AddEntityGroup (SCE_SScene*, SCE_SSceneEntityGroup*);*/
void SCE_Scene_AddLight (SCE_SScene*, SCE_SLight*);
void SCE_Scene_AddCamera (SCE_SScene*, SCE_SCamera*);

void SCE_Scene_AddResource (SCE_SScene*, int, SCE_SSceneResource*);
void SCE_Scene_RemoveResource (SCE_SSceneResource*);

void SCE_Scene_SetSkybox (SCE_SScene*, SCE_SSkybox*);

SCE_SList* SCE_Scene_GetSelectedInstancesList (SCE_SScene*);

void SCE_Scene_SetOctreeSize (SCE_SScene*, float, float, float);
void SCE_Scene_SetOctreeSizev (SCE_SScene*, SCE_TVector3);
int SCE_Scene_MakeOctree (SCE_SScene*, unsigned int, int, float);

int SCE_Scene_SetupBatching (SCE_SScene*, unsigned int, int*);
int SCE_Scene_SetupDefaultBatching (SCE_SScene*);

void SCE_Scene_ClearBuffers (SCE_SScene*);

void SCE_Scene_Update (SCE_SScene*, SCE_SCamera*, SCE_STexture*, SCEuint);
void SCE_Scene_Render (SCE_SScene*, SCE_SCamera*, SCE_STexture*, int);

typedef struct sce_spickresult SCE_SPickResult;
struct sce_spickresult {
    SCE_SLine3 line, line2;
    SCE_SPlane plane, plane2;
    SCE_SSceneEntityInstance *inst;
    SCEindices index;
    SCE_TVector3 point;
    float distance;
    int sup;
    SCE_TVector3 a, b, c;
};

void SCE_Pick_Init (SCE_SPickResult*);
void SCE_Scene_Pick (SCE_SScene*, SCE_SCamera*, SCE_TVector2, SCE_SPickResult*);

void SCE_Scene_DrawBoundingBoxes (SCE_SScene*);
void SCE_Scene_DrawInstanceOctrees (SCE_SScene*, SCE_SSceneEntityInstance*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
