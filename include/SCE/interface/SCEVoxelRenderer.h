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

/* created: 14/02/2012
   updated: 14/02/2012 */

#ifndef SCEVOXELRENDERER_H
#define SCEVOXELRENDERER_H

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>   /* SCE_SGeometry */
#include "SCE/interface/SCEMesh.h"
#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEShaders.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sce_svoxeltemplate SCE_SVoxelTemplate;
struct sce_svoxeltemplate {
    SCE_SGeometry grid_geom; /**< Grid of points */
    SCE_SMesh grid_mesh; /**< Mesh of the grid of points for the first stage
                          * of the generation process */
    SCE_SMesh non_empty;     /**< Generated list of non-empty cells */
    SCE_SMesh list_verts;    /**< Generated list of vertices to generate */

    SCE_SShader *non_empty_shader;
    int non_empty_offset_loc;
    SCE_SShader *list_verts_shader;
    SCE_SShader *final_shader;
    int final_offset_loc;

    SCE_SShader *splat_shader;
    SCE_SShader *indices_shader;
    SCE_STexture *splat;     /**< 3D map of indices */

    int vwidth, vheight, vdepth; /**< Dimensions of the volume texture */
    int width, height, depth; /**< Dimensions of the grid to render */
};

typedef struct sce_svoxelmesh SCE_SVoxelMesh;
struct sce_svoxelmesh {
    SCE_STexture *volume;    /**< Volume texture: raw voxel data */
    SCE_TVector3 wrap;       /**< Wrapping into \c volume */
    SCE_SMesh *mesh;         /**< Final mesh */
    int render;              /**< Whether the mesh should be rendered */

    int vertex_range[2];     /**< Range of the vertex buffer to bind */
    int index_range[2];      /**< Range of the index buffer to bind */
    SCEuint n_vertices;      /**< Amount of produced vertices */
    SCEuint n_indices;       /**< Amount of produced indices */
};

int SCE_Init_VRender (void);
void SCE_Quit_VRender (void);

void SCE_VRender_Init (SCE_SVoxelTemplate*);
void SCE_VRender_Clear (SCE_SVoxelTemplate*);
SCE_SVoxelTemplate* SCE_VRender_Create (void);
void SCE_VRender_Delete (SCE_SVoxelTemplate*);

void SCE_VRender_InitMesh (SCE_SVoxelMesh*);
void SCE_VRender_ClearMesh (SCE_SVoxelMesh*);
SCE_SVoxelMesh* SCE_VRender_CreateMesh (void);
void SCE_VRender_DeleteMesh (SCE_SVoxelMesh*);

void SCE_VRender_SetDimensions (SCE_SVoxelTemplate*, int, int, int);
void SCE_VRender_SetWidth (SCE_SVoxelTemplate*, int);
void SCE_VRender_SetHeight (SCE_SVoxelTemplate*, int);
void SCE_VRender_SetDepth (SCE_SVoxelTemplate*, int);

void SCE_VRender_SetVolumeDimensions (SCE_SVoxelTemplate*, int, int, int);
void SCE_VRender_SetVolumeWidth (SCE_SVoxelTemplate*, int);
void SCE_VRender_SetVolumeHeight (SCE_SVoxelTemplate*, int);
void SCE_VRender_SetVolumeDepth (SCE_SVoxelTemplate*, int);

int SCE_VRender_Build (SCE_SVoxelTemplate*);

SCE_SGeometry* SCE_VRender_GetFinalGeometry (void);

void SCE_VRender_SetVolume (SCE_SVoxelMesh*, SCE_STexture*);
void SCE_VRender_SetWrap (SCE_SVoxelMesh*, SCE_TVector3);
void SCE_VRender_SetMesh (SCE_SVoxelMesh*, SCE_SMesh*);

void SCE_VRender_SetVBRange (SCE_SVoxelMesh*, const int*);
void SCE_VRender_SetIBRange (SCE_SVoxelMesh*, const int*);

void SCE_VRender_Hardware (SCE_SVoxelTemplate*, SCE_SVoxelMesh*, int, int, int);

int SCE_VRender_IsVoid (const SCE_SVoxelMesh*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
