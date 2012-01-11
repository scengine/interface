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
 
/* created: 25/10/2008
   updated: 10/01/2012 */

#ifndef SCEINSTANCING_H
#define SCEINSTANCING_H

#include <SCE/utils/SCEUtils.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCEMesh.h"

#ifdef __cplusplus
extern "C" {
#endif

enum sce_einstancingtype {
    SCE_SIMPLE_INSTANCING = 0,
    SCE_PSEUDO_INSTANCING,
    SCE_HARDWARE_INSTANCING
};
typedef enum sce_einstancingtype SCE_EInstancingType;

/** \copydoc sce_sgeometryinstance */
typedef struct sce_sgeometryinstance SCE_SGeometryInstance;
/** \copydoc sce_sgeometryinstancegroup */
typedef struct sce_sgeometryinstancegroup SCE_SGeometryInstanceGroup;

/**
 * \brief Geometry instance
 *
 * This structure stores informations about one instance of a group of
 * instances.
 * \sa SCE_SGeometryInstanceGroup
 */
struct sce_sgeometryinstance {
    SCE_SNode *node;                   /**< Instance's node */
    SCE_SListIterator it;              /**< Own iterator, used by the groups */
    void *data;                        /**< Used defined data */
    SCE_SGeometryInstanceGroup *group; /**< Group of the instance */
};

typedef void (*SCE_FInstanceGroupRenderFunc)(SCE_SGeometryInstanceGroup*);
typedef void (*SCE_FInstanceGroupHwFunc)(SCE_SListIterator**, unsigned int);

/**
 * \brief A group of instances
 *
 * This structure defines a group of multiple instances of one geometry
 * \sa SCE_SGeometryInstance
 */
struct sce_sgeometryinstancegroup {
    SCE_SMesh *mesh;            /**< Mesh of this group (common data) */
    SCE_SList instances;        /**< Instances of this group */
    unsigned int n_instances;   /**< Number of instances in the group */
    SCE_FInstanceGroupRenderFunc renderfunc; /**< Render function */
    /* vertex attributes for pseudo instancing */
    int attrib1, attrib2, attrib3;
    SCE_FInstanceGroupHwFunc hwfunc; /**< Called before each
                                      * SCE_Mesh_RenderInstanced() in
                                      * hardware instancing */
    unsigned int batch_count; /**< Instances per batch in hardware instancing */
};

void SCE_Instance_Init (SCE_SGeometryInstance*);
SCE_SGeometryInstance* SCE_Instance_Create (void);
void SCE_Instance_Delete (SCE_SGeometryInstance*);

void SCE_Instance_InitGroup (SCE_SGeometryInstanceGroup*);
SCE_SGeometryInstanceGroup* SCE_Instance_CreateGroup (void);
void SCE_Instance_DeleteGroup (SCE_SGeometryInstanceGroup*);

void SCE_Instance_SetInstancingType (SCE_SGeometryInstanceGroup*,
                                     SCE_EInstancingType);
void SCE_Instance_SetAttribIndices (SCE_SGeometryInstanceGroup*, int, int, int);
void SCE_Instance_SetBatchCount (SCE_SGeometryInstanceGroup*, unsigned int);
void SCE_Instance_SetHwCallback (SCE_SGeometryInstanceGroup*,
                                 SCE_FInstanceGroupHwFunc);

void SCE_Instance_AddInstance (SCE_SGeometryInstanceGroup*,
                               SCE_SGeometryInstance*);
void SCE_Instance_RemoveInstance (SCE_SGeometryInstance*);
void SCE_Instance_RemoveInstanceSafe (SCE_SGeometryInstance*);

void SCE_Instance_FlushInstancesList (SCE_SGeometryInstanceGroup*);
int SCE_Instance_HasInstances (SCE_SGeometryInstanceGroup*);

void SCE_Instance_SetGroupMesh (SCE_SGeometryInstanceGroup*, SCE_SMesh*);
SCE_SMesh* SCE_Instance_GetGroupMesh (SCE_SGeometryInstanceGroup*);

void SCE_Instance_SetNode (SCE_SGeometryInstance*, SCE_SNode*);
float* SCE_Instance_GetMatrix (SCE_SGeometryInstance*);

void SCE_Instance_SetData (SCE_SGeometryInstance*, void*);
void* SCE_Instance_GetData (SCE_SGeometryInstance*);

void SCE_Instance_RenderGroup (SCE_SGeometryInstanceGroup*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
