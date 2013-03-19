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

/* created: 30/01/2012
   updated: 18/03/2013 */

#ifndef SCEVOXELTERRAIN_H
#define SCEVOXELTERRAIN_H

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>   /* SCE_SGrid, SCE_SCamera */
#include "SCE/interface/SCEShaders.h"
#include "SCE/interface/SCEVoxelRenderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/** \copydoc sce_svoxelterrainlevel */
typedef struct sce_svoxelterrainlevel SCE_SVoxelTerrainLevel;

/** \copydoc sce_svoxelterrainregion */
typedef struct sce_svoxelterrainregion SCE_SVoxelTerrainRegion;
/**
 * \brief A single sub-region of a terrain level
 */
struct sce_svoxelterrainregion {
    int x, y, z;                /**< Coordinates of this region (constant) */
    int wx, wy, wz;             /**< Wrapped coordinates (dynamic) */
    SCE_SVoxelMesh vm;          /**< Abstract voxel mesh */
    SCE_SMesh *mesh;            /**< Pointer to the mesh */
    SCE_TMatrix4 matrix;        /**< World transform matrix */
    int draw;                   /**< Whether this region should be rendered */
    SCE_SListIterator it, it2, it3;
    int need_update;               /**< Hehe. */
    SCE_SList *level_list;         /**< Update list the region is in */
    SCE_SVoxelTerrainLevel *level; /**< Owner of this region */
};

/**
 * \brief Single level (in terms of LOD) of a voxel terrain
 */
struct sce_svoxelterrainlevel {
    int level;               /**< Level. */
    SCE_SGrid grid;          /**< Density data */
    SCE_SGrid grid2;         /**< Material data */
    int wrap[3];             /**< Texture wrapping */
    SCE_STexture *tex;       /**< Density texture */
    SCE_STexture *mat;       /**< Material texture */
    SCEuint subregions;      /**< Number of sub-regions per side */
    SCE_SVoxelTerrainRegion *regions;  /**< Level sub-regions */
    SCE_SMesh *mesh;         /**< Final meshes */
    int wrap_x, wrap_y, wrap_z; /**< Sub-regions wrapping */
    int enabled;             /**< Is this level enabled? */
    int x, y, z;             /**< Position of the origin of the regions */

    long map_x, map_y, map_z;/**< Origin of the grid in the map */

    int need_update;         /**< Does the texture need to be updated? */
    SCE_SIntRect3 update_zone;
    SCE_SList list1, list2;
    SCE_SList *updating, *queue;

    SCE_SList to_render;     /**< Regions to render */

    SCE_SListIterator it;
};


typedef struct sce_svoxelterrainshader SCE_SVoxelTerrainShader;
struct sce_svoxelterrainshader {
    SCE_SShader *shd;
    int regions_loc;
    int current_loc;
    int wrapping0_loc, wrapping1_loc;
    int tcorigin_loc;
    int hightex_loc;
    int lowtex_loc;
    int enabled_loc;
    int topdiffuse_loc;
    int sidediffuse_loc;
    int noise_loc;
    int material_loc;
};

/* shader flags */
#define SCE_VTERRAIN_USE_LOD (0x00000001)
#define SCE_VTERRAIN_USE_SHADOWS (0x00000002)
#define SCE_VTERRAIN_USE_POINT_SHADOWS (0x00000004)
/* number of flags combinations (2^3 = 8) */
#define SCE_NUM_VTERRAIN_SHADERS (SCE_VTERRAIN_USE_POINT_SHADOWS << 1)

#define SCE_VTERRAIN_USE_LOD_NAME "SCE_VTERRAIN_USE_LOD"
#define SCE_VTERRAIN_USE_SHADOWS_NAME "SCE_VTERRAIN_USE_SHADOWS"
#define SCE_VTERRAIN_USE_POINT_SHADOWS_NAME "SCE_VTERRAIN_USE_POINT_SHADOWS"

typedef struct sce_svoxelterrainhybridgenerator
SCE_SVoxelTerrainHybridGenerator;
struct sce_svoxelterrainhybridgenerator {
    SCE_SList queue;
    int dim;
    SCE_SGrid grid;
    SCE_SMCGenerator mc_gen;
    SCEuint mc_step;
    int query;                /* whether we are waiting for data */
    int grid_ready;           /* whether we have the data we wanted */
    SCEvertices *vertices;
    SCEvertices *normals;
    SCEindices *indices;
    SCEindices *anchors;
    void *interleaved;
    SCEuint n_vertices;
    SCEuint n_indices;
    SCEuint n_anchors;
    int x, y, z;
    SCEuint level;
    SCE_SQEMMesh qmesh;
};

/* maximum number of terrain levels */
#define SCE_MAX_VTERRAIN_LEVELS 16

/** \copydoc sce_svoxelterrain */
typedef struct sce_svoxelterrain SCE_SVoxelTerrain;
/**
 * \brief A voxel terrain
 * 
 * This structure doesn't hold a whole terrain, it only keeps the visible
 * part of the terrain in memory. You may update this structure manually if
 * you plan to move on your terrain. The module can tell you what parts of
 * the terrain are missing depending on your position, but you're in charge
 * of being able to provide them. This includes generating terrain sub-levels'
 * grids.
 */
struct sce_svoxelterrain {
    SCE_SVoxelTemplate template;
    int comp_pos, comp_nor;     /**< Compress positions? normals? */
    SCE_EVoxelRenderPipeline rpipeline;
    SCEuint cut;                /**< cut for the hybrid generation method */
    SCE_SVoxelTerrainHybridGenerator hybrid;
    SCE_RBufferPool *vertex_pool;
    SCE_RBufferPool *index_pool;

    SCEuint subregion_dim;      /**< Dimensions of one subregion */
    SCEuint n_subregions;       /**< Number of subregions per side */
    SCE_SVoxelTerrainLevel levels[SCE_MAX_VTERRAIN_LEVELS];
    size_t n_levels;
    long x, y, z;               /**< Position of the theoretical viewer */
    int width, height, depth;   /**< Dimensions of one level */
    float unit;                 /**< Distance between two consecutive voxels */
    float scale;                /**< Scale to apply to the terrain */
    int built;                  /**< Is the terrain built? */

    SCE_SList to_update;        /**< List of levels to update */
    SCE_SVoxelTerrainLevel *update_level; /**< Level being updated */
    SCEuint max_updates;        /**< Maximum number of updated regions
                                     per frame */

    int trans_enabled;
    int shadow_mode;                 /**< Whether we are filling shadow maps */
    int point_shadow;                /**< If the shadow is a point light */
    int use_materials;

    SCE_SShader *pipeline;           /**< Pipeline of all shaders below */
    SCE_SVoxelTerrainShader shaders[SCE_NUM_VTERRAIN_SHADERS];

    /** \brief Diffuse textures of the terrain */
    SCE_STexture *top_diffuse;
    SCE_STexture *side_diffuse;
    SCE_STexture *noise;
};

void SCE_VTerrain_Init (SCE_SVoxelTerrain*);
void SCE_VTerrain_Clear (SCE_SVoxelTerrain*);
SCE_SVoxelTerrain* SCE_VTerrain_Create (void);
void SCE_VTerrain_Delete (SCE_SVoxelTerrain*);

void SCE_VTerrain_SetDimensions (SCE_SVoxelTerrain*, int, int, int);
void SCE_VTerrain_SetWidth (SCE_SVoxelTerrain*, int);
void SCE_VTerrain_SetHeight (SCE_SVoxelTerrain*, int);
void SCE_VTerrain_SetDepth (SCE_SVoxelTerrain*, int);
int SCE_VTerrain_GetWidth (const SCE_SVoxelTerrain*);
int SCE_VTerrain_GetHeight (const SCE_SVoxelTerrain*);
int SCE_VTerrain_GetDepth (const SCE_SVoxelTerrain*);
size_t SCE_VTerrain_GetUsedVRAM (const SCE_SVoxelTerrain*);

void SCE_VTerrain_SetUnit (SCE_SVoxelTerrain*, float);

void SCE_VTerrain_SetNumLevels (SCE_SVoxelTerrain*, SCEuint);
SCEuint SCE_VTerrain_GetNumLevels (const SCE_SVoxelTerrain*);

void SCE_VTerrain_SetSubRegionDimension (SCE_SVoxelTerrain*, SCEuint);
void SCE_VTerrain_SetNumSubRegions (SCE_SVoxelTerrain*, SCEuint);

void SCE_VTerrain_CompressPosition (SCE_SVoxelTerrain*, int);
void SCE_VTerrain_CompressNormal (SCE_SVoxelTerrain*, int);
void SCE_VTerrain_SetPipeline (SCE_SVoxelTerrain*, SCE_EVoxelRenderPipeline);
void SCE_VTerrain_SetAlgorithm (SCE_SVoxelTerrain*, SCE_EVoxelRenderAlgorithm);
void SCE_VTerrain_SetHybrid (SCE_SVoxelTerrain*, SCEuint);
void SCE_VTerrain_SetHybridMCStep (SCE_SVoxelTerrain*, SCEuint);
void SCE_VTerrain_SetVertexBufferPool (SCE_SVoxelTerrain*, SCE_RBufferPool*);
void SCE_VTerrain_SetIndexBufferPool (SCE_SVoxelTerrain*, SCE_RBufferPool*);
void SCE_VTerrain_EnableMaterials (SCE_SVoxelTerrain*);
void SCE_VTerrain_DisableMaterials (SCE_SVoxelTerrain*);

void SCE_VTerrain_SetShader (SCE_SVoxelTerrain*, SCE_SShader*);

void SCE_VTerrain_SetTopDiffuseTexture (SCE_SVoxelTerrain*, SCE_STexture*);
SCE_STexture* SCE_VTerrain_GetTopDiffuseTexture (SCE_SVoxelTerrain*);
void SCE_VTerrain_SetSideDiffuseTexture (SCE_SVoxelTerrain*, SCE_STexture*);
SCE_STexture* SCE_VTerrain_GetSideDiffuseTexture (SCE_SVoxelTerrain*);
void SCE_VTerrain_SetNoiseTexture (SCE_SVoxelTerrain*, SCE_STexture*);
SCE_STexture* SCE_VTerrain_GetNoiseTexture (SCE_SVoxelTerrain*);

int SCE_VTerrain_BuildShader (SCE_SVoxelTerrain*, SCE_SShader*);
int SCE_VTerrain_Build (SCE_SVoxelTerrain*);

void SCE_VTerrain_SetPosition (SCE_SVoxelTerrain*, long, long, long);
void SCE_VTerrain_GetMissingSlices (const SCE_SVoxelTerrain*, SCEuint, long*,
                                    long*, long*);
int SCE_VTerrain_GetMissingRegion (SCE_SVoxelTerrain*, SCE_SLongRect3*);
void SCE_VTerrain_GetRectangle (const SCE_SVoxelTerrain*, SCEuint,
                                SCE_SLongRect3*);
void SCE_VTerrain_SetOrigin (SCE_SVoxelTerrain*, SCEuint, long, long, long);
void SCE_VTerrain_GetOrigin (const SCE_SVoxelTerrain*, SCEuint,
                             long*, long*, long*);

void SCE_VTerrain_SetLevel (SCE_SVoxelTerrain*, SCEuint, const SCE_SGrid*);
SCE_SGrid* SCE_VTerrain_GetLevelGrid (SCE_SVoxelTerrain*, SCEuint);
SCE_SGrid* SCE_VTerrain_GetLevelMaterialGrid (SCE_SVoxelTerrain*, SCEuint);
void SCE_VTerrain_ActivateLevel (SCE_SVoxelTerrain*, SCEuint, int);

void SCE_VTerrain_AppendSlice (SCE_SVoxelTerrain*, SCEuint,
                               SCE_EBoxFace, const unsigned char*, int);
void SCE_VTerrain_SetRegion (SCE_SVoxelTerrain*, const unsigned char*);

void SCE_VTerrain_CullRegions (SCE_SVoxelTerrain*, const SCE_SFrustum*);

int SCE_VTerrain_Update (SCE_SVoxelTerrain*);
void SCE_VTerrain_UpdateGrid (SCE_SVoxelTerrain*, SCEuint, int, int);
void SCE_VTerrain_UpdateSubGrid (SCE_SVoxelTerrain*, SCEuint,
                                 SCE_SIntRect3*, int, int);

void SCE_VTerrain_ActivateShadowMode (SCE_SVoxelTerrain*, int);
void SCE_VTerrain_ActivatePointShadowMode (SCE_SVoxelTerrain*, int);
void SCE_VTerrain_Render (SCE_SVoxelTerrain*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
