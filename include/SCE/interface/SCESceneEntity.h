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
   updated: 12/11/2011 */

#ifndef SCESCENEENTITY_H
#define SCESCENEENTITY_H

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCEGeometryInstance.h"
#include "SCE/interface/SCESceneResource.h"
#include "SCE/interface/SCEShaders.h"
#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEMaterial.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Types of volumes that can be used for frustum culling/other operations */
#define SCE_BOUNDINGBOX 1
#define SCE_BOUNDINGSPHERE 2

/** \copydoc sce_ssceneentityproperties */
typedef struct sce_ssceneentityproperties SCE_SSceneEntityProperties;
struct sce_ssceneentityproperties {
    SCEuint states;      /* scene's states for which the entity should be
                            rendered (default ~0) */
    unsigned int cullface:1;
    int cullmode;
    unsigned int depthtest:1;
    int depthmode;
    unsigned int alphatest:1;
    unsigned int depthscale:1;
    float depthrange[2];
};

/** \copydoc sce_ssceneentityinstance */
typedef struct sce_ssceneentityinstance SCE_SSceneEntityInstance;
/** \copydoc sce_ssceneentity */
typedef struct sce_ssceneentity SCE_SSceneEntity;
/** \copydoc sce_ssceneentitygroup */
typedef struct sce_ssceneentitygroup SCE_SSceneEntityGroup;

/**
 * \brief An instance of a scene entity
 * \sa SCE_SSceneEntity
 */
struct sce_ssceneentityinstance {
    SCE_SNode *node;                 /**< Node */
    SCE_SGeometryInstance *instance; /**< Geometry instance */
    SCE_SLevelOfDetail *lod;         /**< LOD managment structure */
    SCE_SSceneEntity *entity;        /**< Entity used by the instance */
    /* TODO: not very useful */
    SCE_SSceneEntityGroup *group;    /**< Group that contains the instance */
    SCE_SListIterator it;            /**< Own iterators for fast add/remove */
    void *udata;                     /**< User data */
};

/**
 * \brief A scene entity
 *
 * Defines a model which can be rendered via an instance
 * \sa SCE_SSceneEntityInstance
 */
struct sce_ssceneentity {
    SCE_SGeometryInstanceGroup *igroup; /**< Geometry group */

    SCE_SMesh *mesh;              /**< Mesh of the entity */
    SCE_SBoundingBox box;         /**< \c mesh's bounding box */
    SCE_SBoundingSphere sphere;   /**< \c mesh's bounding sphere */
    SCE_SList *textures;          /**< Textures used by the entity */
    SCE_SSceneResource *shader;   /**< Shader used by the entity */
    SCE_SSceneResource *material; /**< Material used by the entity */
    SCE_SSceneEntityProperties props;

    SCE_SSceneEntityGroup *group; /**< Group of the entity */
    /** Used to determine if the instance is in the given frustum */
    int (*isinfrustumfunc)(SCE_SSceneEntityInstance*, SCE_SCamera*);
    SCE_SListIterator it;         /**< Own iterator for group */
    SCE_SListIterator it2;        /**< Public iterator (used in scene manager) */
};

/**
 * \brief Defines a group of entities they represent the same geometry but in
 * a different LOD
 * \sa SCE_SSceneEntity
 */
struct sce_ssceneentitygroup {
    SCE_SList *entities;
    unsigned int n_entities;    /* wtf? */
};


void SCE_SceneEntity_InitInstance (SCE_SSceneEntityInstance*);
SCE_SSceneEntityInstance* SCE_SceneEntity_CreateInstance (void);
void SCE_SceneEntity_DeleteInstance (SCE_SSceneEntityInstance*);
SCE_SSceneEntityInstance*
SCE_SceneEntity_DupInstance (SCE_SSceneEntityInstance*);

void SCE_SceneEntity_InitProperties (SCE_SSceneEntityProperties*);
void SCE_SceneEntity_Init (SCE_SSceneEntity*);
SCE_SSceneEntity* SCE_SceneEntity_Create (void);
void SCE_SceneEntity_Delete (SCE_SSceneEntity*);

void SCE_SceneEntity_InitGroup (SCE_SSceneEntityGroup*);
SCE_SSceneEntityGroup* SCE_SceneEntity_CreateGroup (void);
void SCE_SceneEntity_DeleteGroup (SCE_SSceneEntityGroup*);

void SCE_SceneEntity_AddEntity (SCE_SSceneEntityGroup*, int, SCE_SSceneEntity*);
void SCE_SceneEntity_RemoveEntity (SCE_SSceneEntity*);
#if 0
void SCE_SceneEntity_SortGroup (SCE_SSceneEntityGroup*);
#endif
unsigned int SCE_SceneEntity_GetGroupNumEntities (SCE_SSceneEntityGroup*);
SCE_SList* SCE_SceneEntity_GetGroupEntitiesList (SCE_SSceneEntityGroup*);

void SCE_SceneEntity_SetInstanceDataFromEntity (SCE_SSceneEntityInstance*,
                                                SCE_SSceneEntity*);
void SCE_SceneEntity_AddInstance (SCE_SSceneEntityGroup*,
                                  SCE_SSceneEntityInstance*);
void SCE_SceneEntity_AddInstanceToEntity (SCE_SSceneEntity*,
                                          SCE_SSceneEntityInstance*);
void SCE_SceneEntity_ReplaceInstanceToEntity (SCE_SSceneEntityInstance*);
void SCE_SceneEntity_RemoveInstanceFromEntity (SCE_SSceneEntityInstance*);

void SCE_SceneEntity_Flush (SCE_SSceneEntity*);


SCE_SNode* SCE_SceneEntity_GetInstanceNode (SCE_SSceneEntityInstance*);
SCE_ENodeType SCE_SceneEntity_GetInstanceNodeType (SCE_SSceneEntityInstance*);
SCE_SGeometryInstance*
SCE_SceneEntity_GetInstanceInstance (SCE_SSceneEntityInstance*);
SCE_SOctreeElement*
SCE_SceneEntity_GetInstanceElement (SCE_SSceneEntityInstance*);
SCE_SLevelOfDetail*
SCE_SceneEntity_GetInstanceLOD (SCE_SSceneEntityInstance*);
SCE_SSceneEntityGroup*
SCE_SceneEntity_GetInstanceGroup (SCE_SSceneEntityInstance*);
SCE_SListIterator*
SCE_SceneEntity_GetInstanceIterator1 (SCE_SSceneEntityInstance*);
#if 0
SCE_SListIterator*
SCE_SceneEntity_GetInstanceIterator2 (SCE_SSceneEntityInstance*);
#endif
SCE_SSceneEntity* SCE_SceneEntity_GetInstanceEntity (SCE_SSceneEntityInstance*);
SCE_SBoundingBox* SCE_SceneEntity_GetInstanceBB (SCE_SSceneEntityInstance*);
SCE_SBoundingBox* SCE_SceneEntity_PushInstanceBB (SCE_SSceneEntityInstance*,
                                                  SCE_SBox*);
void SCE_SceneEntity_PopInstanceBB (SCE_SSceneEntityInstance*,
                                    SCE_SBox*);

void* SCE_SceneEntity_GetInstanceData (SCE_SSceneEntityInstance*);
void SCE_SceneEntity_SetInstanceData (SCE_SSceneEntityInstance*, void*);


SCE_SSceneEntityProperties* SCE_SceneEntity_GetProperties (SCE_SSceneEntity*);
void SCE_SceneEntity_SetMesh (SCE_SSceneEntity*, SCE_SMesh*);
SCE_SMesh* SCE_SceneEntity_GetMesh (SCE_SSceneEntity*);

int SCE_SceneEntity_AddTexture (SCE_SSceneEntity*, SCE_STexture*);
void SCE_SceneEntity_RemoveTexture (SCE_SSceneEntity*, SCE_STexture*);
SCE_SList* SCE_SceneEntity_GetTexturesList (SCE_SSceneEntity*);

int SCE_SceneEntity_SetShader (SCE_SSceneEntity*, SCE_SShader*);
SCE_SSceneResource* SCE_SceneEntity_GetShader (SCE_SSceneEntity*);

int SCE_SceneEntity_SetMaterial (SCE_SSceneEntity*, SCE_SMaterial*);
SCE_SSceneResource* SCE_SceneEntity_GetMaterial (SCE_SSceneEntity*);

int SCE_SceneEntity_HasResourceOfType (SCE_SSceneEntity*, int);
int SCE_SceneEntity_HasInstance (SCE_SSceneEntity*);

SCE_SGeometryInstanceGroup*
SCE_SceneEntity_GetInstancesGroup (SCE_SSceneEntity*);
SCE_SListIterator* SCE_SceneEntity_GetIterator (SCE_SSceneEntity*);

void SCE_SceneEntity_SetupBoundingVolume (SCE_SSceneEntity*, int);


void SCE_SceneEntity_DetermineInstanceLOD (SCE_SSceneEntityInstance*,
                                         SCE_SCamera*);
int SCE_SceneEntity_IsInstanceInFrustum (SCE_SSceneEntityInstance*,
                                         SCE_SCamera*);

void SCE_SceneEntity_AddState (SCE_SSceneEntity*, SCEuint);
void SCE_SceneEntity_RemoveState (SCE_SSceneEntity*, SCEuint);
int SCE_SceneEntity_MatchState (SCE_SSceneEntity*, SCEuint);

void SCE_SceneEntity_ApplyProperties (SCE_SSceneEntity*);

void SCE_SceneEntity_UseResources (SCE_SSceneEntity*);
void SCE_SceneEntity_UnuseResources (SCE_SSceneEntity*);
void SCE_SceneEntity_SetDefaultShader (SCE_SShader*);

void SCE_SceneEntity_Lock (void);
void SCE_SceneEntity_Unlock (void);

void SCE_SceneEntity_Render (SCE_SSceneEntity*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
