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

/* created: 30/01/2012
   updated: 07/04/2012 */

#ifndef SCEVOXELTERRAIN_H
#define SCEVOXELTERRAIN_H

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>   /* SCE_SGrid */
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
    int x, y, z;                /**< Coordinates of this region */
    SCE_SVoxelMesh vm;          /**< Abstract voxel mesh */
    int draw;                   /**< Whether this region should be rendered */
    SCE_SListIterator it;
    SCE_SVoxelTerrainLevel *level; /**< Owner of this region */
};

/** \copydoc sce_svoxelterraintransition */
typedef struct sce_svoxelterraintransition SCE_SVoxelTerrainTransition;
/**
 * \brief Structure to store stuff for seamless transition between
 * two consecutive LOD. Structure name is of Funes.
 */
struct sce_svoxelterraintransition {
    SCE_SGeometry geom;         /**< Generated geomtry */
    SCE_SMesh mesh;             /**< Generated mesh */
    SCE_SIntRect3 area;         /**< Area in the voxel field */
    const unsigned char *voxels; /**< Derp :) */
    SCEuint w, h, d;             /**< Voxel field dimensions */
    SCE_SListIterator it;        /**< For the "to update" list */
};

/**
 * \brief Single level (in terms of LOD) of a voxel terrain
 */
struct sce_svoxelterrainlevel {
    SCE_SGrid grid;          /**< Uniform grid of this level */
    int wrap[3];             /**< Texture wrapping */
    SCE_STexture *tex;       /**< GPU-side grid */
    SCEuint subregions;      /**< Number of sub-regions per side */
    SCE_SVoxelTerrainRegion *regions;  /**< Level sub-regions */
    SCE_SMesh *mesh;         /**< Final meshes */
    SCE_SVoxelTerrainTransition *trans[6]; /**< LOD transition meshes */
    int wrap_x, wrap_y, wrap_z; /**< Sub-regions wrapping */
    int enabled;             /**< Is this level enabled? */
    int x, y, z;             /**< Position of the origin of the regions */

    long map_x, map_y, map_z;/**< Origin of the grid in the map */

    int need_update;         /**< Does the texture need to be updated? */
    SCE_SIntRect3 update_zone;
};

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

    SCEuint subregion_dim;      /**< Dimensions of one subregion */
    SCEuint n_subregions;       /**< Number of subregions per side */
    SCE_SVoxelTerrainLevel levels[SCE_MAX_VTERRAIN_LEVELS];
    size_t n_levels;
    long x, y, z;               /**< Position of the theoretical viewer */
    int width, height, depth;   /**< Dimensions of one level */
    float unit;                 /**< Distance between two consecutive voxels */
    float scale;                /**< Scale to apply to the terrain */
    int built;                  /**< Is the terrain built? */

    SCE_SList to_update;        /**< List of regions to update */
    SCEuint n_update;           /**< Size of \c to_update */
    SCEuint max_updates;        /**< Maximum number of updated regions
                                     per frame */

    SCE_SShader *lod_shd;       /**< Seamless LOD transition shader */
    int lodregions_loc;
    int lodcurrent_loc;
    int lodwrapping0_loc, lodwrapping1_loc;
    int lodtcorigin_loc;
    int lodhightex_loc;
    int lodlowtex_loc;
    int lodenabled_loc;
    int lodtopdiffuse_loc;
    int lodsidediffuse_loc;
    int lodtrans_enabled;

    SCE_SShader *def_shd;       /**< Simple terrain rendering shader */
    int defwrapping0_loc;
    int deftcorigin_loc;
    int deftopdiffuse_loc;
    int defsidediffuse_loc;

    /** \brief Diffuse textures of the terrain */
    SCE_STexture *top_diffuse;
    SCE_STexture *side_diffuse;
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

void SCE_VTerrain_SetUnit (SCE_SVoxelTerrain*, float);

void SCE_VTerrain_SetNumLevels (SCE_SVoxelTerrain*, SCEuint);
SCEuint SCE_VTerrain_GetNumLevels (const SCE_SVoxelTerrain*);

void SCE_VTerrain_SetSubRegionDimension (SCE_SVoxelTerrain*, SCEuint);
void SCE_VTerrain_SetNumSubRegions (SCE_SVoxelTerrain*, SCEuint);

void SCE_VTerrain_SetTopDiffuseTexture (SCE_SVoxelTerrain*, SCE_STexture*);
SCE_STexture* SCE_VTerrain_GetTopDiffuseTexture (SCE_SVoxelTerrain*);
void SCE_VTerrain_SetSideDiffuseTexture (SCE_SVoxelTerrain*, SCE_STexture*);
SCE_STexture* SCE_VTerrain_GetSideDiffuseTexture (SCE_SVoxelTerrain*);

int SCE_VTerrain_Build (SCE_SVoxelTerrain*);

void SCE_VTerrain_SetPosition (SCE_SVoxelTerrain*, long, long, long);
void SCE_VTerrain_GetMissingSlices (const SCE_SVoxelTerrain*, SCEuint, long*,
                                    long*, long*);
void SCE_VTerrain_SetOrigin (SCE_SVoxelTerrain*, SCEuint, long, long, long);
void SCE_VTerrain_GetOrigin (const SCE_SVoxelTerrain*, SCEuint,
                             long*, long*, long*);

void SCE_VTerrain_SetLevel (SCE_SVoxelTerrain*, SCEuint, const SCE_SGrid*);
SCE_SGrid* SCE_VTerrain_GetLevelGrid (SCE_SVoxelTerrain*, SCEuint);
void SCE_VTerrain_ActivateLevel (SCE_SVoxelTerrain*, SCEuint, int);

void SCE_VTerrain_AppendSlice (SCE_SVoxelTerrain*, SCEuint,
                               SCE_EBoxFace, const unsigned char*);

void SCE_VTerrain_Update (SCE_SVoxelTerrain*);
void SCE_VTerrain_UpdateGrid (SCE_SVoxelTerrain*, SCEuint, int);
void SCE_VTerrain_UpdateSubGrid (SCE_SVoxelTerrain*, SCEuint,
                                 SCE_SIntRect3*, int);

int SCE_VTerrain_GetOffset (const SCE_SVoxelTerrain*, SCEuint,
                            int*, int*, int*);

void SCE_VTerrain_Render (SCE_SVoxelTerrain*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
