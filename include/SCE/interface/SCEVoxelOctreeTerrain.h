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

/* created: 16/03/2013
   updated: 18/03/2013 */

#ifndef SCEVOXELOCTREETERRAIN_H
#define SCEVOXELOCTREETERRAIN_H

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEVoxelRenderer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    /* region is unused */
    SCE_VOTERRAIN_REGION_POOL,
    /* region is being processed in the pipeline */
    SCE_VOTERRAIN_REGION_PIPELINE,
    /* region has been processed, now ready to be rendered */
    SCE_VOTERRAIN_REGION_READY,
    /* region is being used for rendering */
    SCE_VOTERRAIN_REGION_RENDER,
    /* region could be rendered but higher LOD covers this area */
    SCE_VOTERRAIN_REGION_HIDDEN
} SCE_EVOTerrainRegionStatus;

/** \copydoc sce_svoterrainlevel */
typedef struct sce_svoterrainlevel SCE_SVOTerrainLevel;

/** \copydoc sce_svoterrainregion */
typedef struct sce_svoterrainregion SCE_SVOTerrainRegion;
/**
 * \brief A single sub-region of a terrain level
 */
struct sce_svoterrainregion {
    SCE_EVOTerrainRegionStatus status;
    SCE_SVoxelMesh vm;          /**< Abstract voxel mesh */
    SCE_SMesh mesh;             /**< Pointer to the mesh */
    SCE_TMatrix4 matrix;        /**< World transform matrix */
    int draw;                   /**< Whether this region should be rendered */
    SCE_SVoxelOctreeNode *node; /**< Node associated with this region */
    SCE_SVOTerrainLevel *level; /**< Owner of this region */
    SCE_SListIterator it;       /* pipeline */
    SCE_SListIterator it2;      /* level->to_render,ready,hidden */
    SCE_SListIterator it3;      /* level->regions */
    SCE_SListIterator it4;      /* terrain->to_render */
};


struct sce_svoterrainlevel {
    SCEuint level;
    long x, y, z;               /* position of the viewer */
    /* current coordinates of the origin (in level's space) */
    long origin_x, origin_y, origin_z;

    SCE_SList regions;
    SCE_SList ready;            /* regions ready to be rendered */
    SCE_SList to_render;        /* regions to render */
    SCE_SList hidden;
};

#define SCE_VOTERRAIN_NUM_PIPELINE_STAGES 3

typedef struct sce_svoterrainpipeline SCE_SVOTerrainPipeline;
struct sce_svoterrainpipeline {
    SCE_SVoxelTemplate template;
    SCE_SGrid grid;
    SCE_STexData *tc, *tc2;
    SCE_STexture *tex, *tex2;
    SCE_RBufferPool *vertex_pool;
    SCE_RBufferPool *index_pool;
    SCE_RBufferPool default_vertex_pool;
    SCE_RBufferPool default_index_pool;

    /* pipeline stages */
    SCE_SList stages[SCE_VOTERRAIN_NUM_PIPELINE_STAGES];

    SCE_SQEMMesh qmesh;
    SCEvertices *vertices;
    SCEvertices *normals;
    SCEindices *indices;
    SCEindices *anchors;
    void *interleaved;
    SCEuint n_vertices;
    SCEuint n_indices;
    SCEuint n_anchors;
};


#define SCE_VOTERRAIN_MAX_LEVELS 10

typedef struct sce_svoxeloctreeterrain SCE_SVoxelOctreeTerrain;
struct sce_svoxeloctreeterrain {
    SCE_SVoxelWorld *vw;
    SCEuint n_levels;
    SCEuint w, h, d;            /* dimensions of a region */
    SCEuint n_regions;          /* width of a level */

    long x, y, z;               /* position of the viewer */
    /* current coordinates of the origin */
    long origin_x, origin_y, origin_z;
    float scale;
    int comp_pos, comp_nor;     /* TODO: move to pipe */
    SCE_SVOTerrainPipeline pipe;

    SCE_SList pool;             /* global pool of available regions */
    SCE_SList to_render;
    SCE_SVOTerrainLevel levels[SCE_VOTERRAIN_MAX_LEVELS];
    SCE_SShader *shader;
};

void SCE_VOTerrain_Init (SCE_SVoxelOctreeTerrain*);
void SCE_VOTerrain_Clear (SCE_SVoxelOctreeTerrain*);
SCE_SVoxelOctreeTerrain* SCE_VOTerrain_Create (void);
void SCE_VOTerrain_Delete (SCE_SVoxelOctreeTerrain*);

void SCE_VOTerrain_SetVoxelWorld (SCE_SVoxelOctreeTerrain*, SCE_SVoxelWorld*);
SCE_SVoxelWorld* SCE_VOTerrain_GetVoxelWorld (SCE_SVoxelOctreeTerrain*);
void SCE_VOTerrain_SetNumRegions (SCE_SVoxelOctreeTerrain*, SCEuint);
SCEuint SCE_VOTerrain_GetNumRegions (const SCE_SVoxelOctreeTerrain*);
void SCE_VOTerrain_SetUnit (SCE_SVoxelOctreeTerrain*, float);
void SCE_VOTerrain_SetShader (SCE_SVoxelOctreeTerrain*, SCE_SShader*);

int SCE_VOTerrain_Build (SCE_SVoxelOctreeTerrain*);

int SCE_VOTerrain_SetPosition (SCE_SVoxelOctreeTerrain*, long, long, long);
void SCE_VOTerrain_GetRectangle (const SCE_SVoxelOctreeTerrain*, SCEuint,
                                 SCE_SLongRect3*);
void SCE_VOTerrain_GetCurrentRectangle (const SCE_SVoxelOctreeTerrain*, SCEuint,
                                        SCE_SLongRect3*);
size_t SCE_VOTerrain_GetUsedVRAM (const SCE_SVoxelOctreeTerrain*);

void SCE_VOTerrain_CullRegions (SCE_SVoxelOctreeTerrain*, const SCE_SFrustum*);

int SCE_VOTerrain_Update (SCE_SVoxelOctreeTerrain*);
void SCE_VOTerrain_Render (SCE_SVoxelOctreeTerrain*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
