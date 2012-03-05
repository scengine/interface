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
   updated: 05/03/2012 */

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
/**
 * \brief Single level (in terms of LOD) of a voxel terrain
 */
struct sce_svoxelterrainlevel {
    SCE_SGrid grid;          /**< Uniform grid of this level */
    SCE_TVector3 wrap;
    SCE_STexture *tex;       /**< GPU-side grid */
    SCEuint subregions;      /**< Number of sub-regions per side */
    SCE_SVoxelMesh *vmesh;   /**< Abstract voxel meshes */
    SCE_SMesh *mesh;         /**< Final meshes */
    int wrap_x, wrap_y, wrap_z; /**< Subregions wrapping */
    int enabled;             /**< Is this level enabled? */
    int x, y, z;             /**< Position of the center of this level */

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
    int x, y, z;                /**< Position of the theoretical viewer */
    int width, height, depth;   /**< Dimensions of one level */
    int built;                  /**< Is the terrain built? */
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

void SCE_VTerrain_SetNumLevels (SCE_SVoxelTerrain*, SCEuint);
SCEuint SCE_VTerrain_GetNumLevels (const SCE_SVoxelTerrain*);

void SCE_VTerrain_SetSubRegionDimension (SCE_SVoxelTerrain*, SCEuint);
void SCE_VTerrain_SetNumSubRegions (SCE_SVoxelTerrain*, SCEuint);

int SCE_VTerrain_Build (SCE_SVoxelTerrain*);

void SCE_VTerrain_SetPosition (SCE_SVoxelTerrain*, int, int, int);
void SCE_VTerrain_SetLevel (SCE_SVoxelTerrain*, SCEuint, const SCE_SGrid*);
SCE_SGrid* SCE_VTerrain_GetLevelGrid (SCE_SVoxelTerrain*, SCEuint);
void SCE_VTerrain_ActivateLevel (SCE_SVoxelTerrain*, SCEuint, int);

void SCE_VTerrain_AppendSlice (SCE_SVoxelTerrain*, SCEuint,
                               SCE_EBoxFace, const unsigned char*);

void SCE_VTerrain_Update (SCE_SVoxelTerrain*);
void SCE_VTerrain_UpdateGrid (SCE_SVoxelTerrain*, SCEuint);
void SCE_VTerrain_UpdateSubGrid (SCE_SVoxelTerrain*, SCEuint, SCE_SIntRect3*);

int SCE_VTerrain_GetOffset (const SCE_SVoxelTerrain*, SCEuint,
                            int*, int*, int*);

void SCE_VTerrain_Render (const SCE_SVoxelTerrain*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
