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

/* created: 31/07/2009
   updated: 15/03/2013 */

#ifndef SCEMESH_H
#define SCEMESH_H

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#ifdef __cplusplus
extern "C" {
#endif

enum sce_emeshbuildmode {
    SCE_INDEPENDANT_VERTEX_BUFFER, /**< Store each data in its assignated
                                    * stream (default) */
    SCE_GLOBAL_VERTEX_BUFFER    /**< Use G stream for the whole data (G stream
                                 * is always activated) */
};
typedef enum sce_emeshbuildmode SCE_EMeshBuildMode;

enum sce_emeshstream {
    SCE_MESH_STREAM_G = 0,      /**< Geometry */
    SCE_MESH_STREAM_N,          /**< Normals (including tangents & binormals) */
    SCE_MESH_STREAM_T,          /**< Texture coordinates */
    SCE_MESH_STREAM_A,          /**< Animation informations (weights, etc.) */
    SCE_MESH_NUM_STREAMS
};
typedef enum sce_emeshstream SCE_EMeshStream;

typedef struct sce_smesharray SCE_SMeshArray;
struct sce_smesharray {
    SCE_RVertexBufferData data;
    SCE_SGeometryArrayUser auser;
    SCE_SListIterator it;
};

typedef struct sce_smesh SCE_SMesh;
/**
 * \brief Structure of a renderable mesh
 * \sa SCE_SGeometry, SCE_RVertexBuffer
 */
struct sce_smesh {
    SCE_SGeometry *geom;
    int canfree_geom;
    SCE_EPrimitiveType prim;    /**< Primitive type (SCE_TRIANGLES
                                 * recommanded) */
    SCE_SList arrays;           /**< SCE_SMeshArray */
    SCE_RVertexBuffer streams[SCE_MESH_NUM_STREAMS];
    int used_streams[SCE_MESH_NUM_STREAMS];
    SCE_RIndexBuffer ib;
    int use_ib;
    SCEuint n_vertices;
    SCEuint n_indices;
    SCE_RFeedback feedback;     /**< Feedback object */
    SCE_RFeedback feedback_id;  /**< Feedback object for indices */
    SCE_RBuffer *counting_buffer;
    SCE_RBuffer *counting_buffer_id;
    SCE_SGeometryArrayUser index_auser;
    SCE_RBufferRenderMode rmode;/**< Render mode */
    SCE_EMeshBuildMode bmode;   /**< Build mode */
    int built;                  /**< Is the mesh built? */
};

int SCE_Init_Mesh (void);
void SCE_Quit_Mesh (void);

void SCE_Mesh_InitArray (SCE_SMeshArray*);
SCE_SMeshArray* SCE_Mesh_CreateArray (void);
void SCE_Mesh_ClearArray (SCE_SMeshArray*);
void SCE_Mesh_DeleteArray (SCE_SMeshArray*);

void SCE_Mesh_Init (SCE_SMesh*);
SCE_SMesh* SCE_Mesh_Create (void);
SCE_SMesh* SCE_Mesh_CreateFrom (SCE_SGeometry*, int);
void SCE_Mesh_Clear (SCE_SMesh*);
void SCE_Mesh_Delete (SCE_SMesh*);

void SCE_Mesh_ActivateStream (SCE_EMeshStream, int);
void SCE_Mesh_EnableStream (SCE_EMeshStream);
void SCE_Mesh_DisableStream (SCE_EMeshStream);

void SCE_Mesh_ActivateIndices (SCE_SMesh*, int);
void SCE_Mesh_SetPrimitiveType (SCE_SMesh*, SCE_EPrimitiveType);

SCEuint SCE_Mesh_GetNumVertices (const SCE_SMesh*);
SCEuint SCE_Mesh_GetNumIndices (const SCE_SMesh*);
void SCE_Mesh_SetNumVertices (SCE_SMesh*, SCEuint);
void SCE_Mesh_SetNumIndices (SCE_SMesh*, SCEuint);
void SCE_Mesh_UpdateNumVertices (SCE_SMesh*);
void SCE_Mesh_UpdateNumIndices (SCE_SMesh*);

SCE_SGeometry* SCE_Mesh_GetGeometry (SCE_SMesh*);
int SCE_Mesh_SetGeometry (SCE_SMesh*, SCE_SGeometry*, int);
void SCE_Mesh_Build (SCE_SMesh*, SCE_EMeshBuildMode,
                     SCE_RBufferUsage[SCE_MESH_NUM_STREAMS + 1]);
void SCE_Mesh_AutoBuild (SCE_SMesh*);

void SCE_Mesh_SetRenderMode (SCE_SMesh*, SCE_RBufferRenderMode);

int SCE_Mesh_Update (SCE_SMesh*);

SCE_SMesh* SCE_Mesh_Load (const char*, int);

SCE_RVertexBuffer* SCE_Mesh_GetStream (SCE_SMesh*, SCE_EMeshStream);
SCE_RIndexBuffer* SCE_Mesh_GetIndexBuffer (SCE_SMesh*);
size_t SCE_Mesh_GetUsedVRAM (const SCE_SMesh*);

int SCE_Mesh_ReallocStream (SCE_SMesh*, SCE_EMeshStream, SCE_RBufferPool*);
int SCE_Mesh_ReallocIndexBuffer (SCE_SMesh*, SCE_RBufferPool*);

void SCE_Mesh_UploadVertices (SCE_SMesh*, SCE_EMeshStream, const SCEvertices*,
                              size_t, size_t);
void SCE_Mesh_UploadIndices (SCE_SMesh*, const SCEindices*, size_t);
void SCE_Mesh_DownloadVertices (SCE_SMesh*, SCE_EMeshStream, SCEvertices*,
                                size_t, size_t);
void SCE_Mesh_DownloadIndices (SCE_SMesh*, SCEindices*, size_t);
void SCE_Mesh_DownloadAllVertices (SCE_SMesh*, SCE_EMeshStream, SCEvertices*);
void SCE_Mesh_DownloadAllIndices (SCE_SMesh*, SCEindices*);

void SCE_Mesh_Use (SCE_SMesh*);
void SCE_Mesh_Render (void);
void SCE_Mesh_RenderInstanced (SCEuint);
void SCE_Mesh_Unuse (void);

void SCE_Mesh_BeginRenderTo (SCE_SMesh*);
void SCE_Mesh_EndRenderTo (SCE_SMesh*);
void SCE_Mesh_BeginRenderToIndices (SCE_SMesh*);
void SCE_Mesh_EndRenderToIndices (SCE_SMesh*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
