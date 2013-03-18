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
   updated: 17/03/2013 */

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEVoxelRenderer.h"

#include "SCE/interface/SCEVoxelOctreeTerrain.h"


static void SCE_VOTerrain_InitRegion (SCE_SVOTerrainRegion *region)
{
    SCE_VRender_InitMesh (&region->vm);
    SCE_Mesh_Init (&region->mesh);
    SCE_Matrix4_Identity (region->matrix);
    region->draw = SCE_FALSE;
    region->node = NULL;
    region->level = NULL;
    SCE_List_InitIt (&region->it);
    SCE_List_SetData (&region->it, region);
    SCE_List_InitIt (&region->it2);
    SCE_List_SetData (&region->it2, region);
    SCE_List_InitIt (&region->it3);
    SCE_List_SetData (&region->it3, region);
    SCE_List_InitIt (&region->it4);
    SCE_List_SetData (&region->it4, region);
}
static void SCE_VOTerrain_ClearRegion (SCE_SVOTerrainRegion *region)
{
    SCE_VRender_ClearMesh (&region->vm);
    SCE_Mesh_Clear (&region->mesh);
    /* TODO: avoid node callback from being called...? */
    if (region->node)
        SCE_VOctree_SetNodeData (region->node, NULL);
    SCE_List_Remove (&region->it);
    SCE_List_Remove (&region->it2);
    SCE_List_Remove (&region->it3);
    SCE_List_Remove (&region->it4);
}
static SCE_SVOTerrainRegion* SCE_VOTerrain_CreateRegion (void)
{
    SCE_SVOTerrainRegion *region = NULL;
    if (!(region = SCE_malloc (sizeof *region)))
        SCEE_LogSrc ();
    else {
        SCE_VOTerrain_InitRegion (region);
    }
    return region;
}
static void SCE_VOTerrain_DeleteRegion (SCE_SVOTerrainRegion *region)
{
    if (region) {
        SCE_VOTerrain_ClearRegion (region);
        SCE_free (region);
    }
}

static void SCE_VOTerrain_FreeRegion (void *r)
{
    SCE_VOTerrain_DeleteRegion (r);
}
static void SCE_VOTerrain_InitLevel (SCE_SVOTerrainLevel *tl)
{
    tl->level = 0;
    tl->x = tl->y = tl->z = 0;
    tl->origin_x = tl->origin_y = tl->origin_z = 0;
    SCE_List_Init (&tl->regions);
    SCE_List_SetFreeFunc (&tl->regions, SCE_VOTerrain_FreeRegion);
    SCE_List_Init (&tl->to_render);
}
static void SCE_VOTerrain_ClearLevel (SCE_SVOTerrainLevel *tl)
{
    SCE_List_Clear (&tl->regions);
}


static void SCE_VOTerrain_InitPipeline (SCE_SVOTerrainPipeline *pipe)
{
    int i;

    SCE_VRender_Init (&pipe->template);
    SCE_VRender_SetPipeline (&pipe->template, SCE_VRENDER_HARDWARE);
    SCE_Grid_Init (&pipe->grid);
    pipe->tc = pipe->tc2 = NULL;
    pipe->tex = pipe->tex2 = NULL;
    pipe->pool = &pipe->default_pool;
    SCE_RInitBufferPool (&pipe->default_pool);
    /* TODO: set pool usage? */
    SCE_VRender_SetBufferPool (&pipe->template, pipe->pool);

    for (i = 0; i < SCE_VOTERRAIN_NUM_PIPELINE_STAGES; i++)
        SCE_List_Init (&pipe->stages[i]);

    SCE_QEMD_Init (&pipe->qmesh);
    pipe->vertices = NULL;
    pipe->normals = NULL;
    pipe->indices = NULL;
    pipe->anchors = NULL;
    pipe->interleaved = NULL;
    pipe->n_vertices = pipe->n_indices = pipe->n_anchors = 0;
}
static void SCE_VOTerrain_ClearPipeline (SCE_SVOTerrainPipeline *pipe)
{
    int i;

    SCE_VRender_Clear (&pipe->template);
    SCE_Texture_Delete (pipe->tex);
    SCE_Grid_Clear (&pipe->grid);
    SCE_RClearBufferPool (&pipe->default_pool);

    for (i = 0; i < SCE_VOTERRAIN_NUM_PIPELINE_STAGES; i++)
        SCE_List_Clear (&pipe->stages[i]);

    SCE_QEMD_Clear (&pipe->qmesh);
    SCE_free (pipe->vertices);
    SCE_free (pipe->normals);
    SCE_free (pipe->indices);
    SCE_free (pipe->anchors);
    SCE_free (pipe->interleaved);
}


void SCE_VOTerrain_Init (SCE_SVoxelOctreeTerrain *vt)
{
    int i;

    vt->vw = NULL;
    vt->n_levels = 0;
    vt->w = vt->h = vt->d = 0;
    vt->n_regions = 0;

    vt->x = vt->y = vt->z = 0;
    vt->origin_x = vt->origin_y = vt->origin_z = 0;
    vt->scale = 1.0;
    vt->comp_pos = vt->comp_nor = SCE_FALSE;
    SCE_VOTerrain_InitPipeline (&vt->pipe);

    SCE_List_Init (&vt->pool);
    SCE_List_SetFreeFunc (&vt->pool, SCE_VOTerrain_FreeRegion);
    SCE_List_Init (&vt->to_render);

    for (i = 0; i < SCE_VOTERRAIN_MAX_LEVELS; i++) {
        SCE_VOTerrain_InitLevel (&vt->levels[i]);
        vt->levels[i].level = i;
    }
    vt->shader = NULL;
}
void SCE_VOTerrain_Clear (SCE_SVoxelOctreeTerrain *vt)
{
    int i;
    SCE_VOTerrain_ClearPipeline (&vt->pipe);
    SCE_List_Clear (&vt->pool);
    SCE_List_Clear (&vt->to_render);
    for (i = 0; i < SCE_VOTERRAIN_MAX_LEVELS; i++)
        SCE_VOTerrain_ClearLevel (&vt->levels[i]);
}

SCE_SVoxelOctreeTerrain* SCE_VOTerrain_Create (void)
{
    SCE_SVoxelOctreeTerrain *vt = NULL;
    if (!(vt = SCE_malloc (sizeof *vt)))
        SCEE_LogSrc ();
    else {
        SCE_VOTerrain_Init (vt);
    }
    return vt;
}
void SCE_VOTerrain_Delete (SCE_SVoxelOctreeTerrain *vt)
{
    if (vt) {
        SCE_VOTerrain_Clear (vt);
        SCE_free (vt);
    }
}


/* NOTE: dont call this function twice for now. */
void SCE_VOTerrain_SetVoxelWorld (SCE_SVoxelOctreeTerrain *vt,
                                  SCE_SVoxelWorld *vw)
{
    if (vt->vw) {
        /* TODO: derpyderp */
        return;
    }
    vt->vw = vw;
    vt->n_levels = SCE_VWorld_GetNumLevels (vw);
    vt->w = SCE_VWorld_GetWidth (vw);
    vt->h = SCE_VWorld_GetHeight (vw);
    vt->d = SCE_VWorld_GetDepth (vw);
    /* reset vt->origin_* values and allocate regions */
    SCE_VOTerrain_SetPosition (vt, vt->x, vt->y, vt->z);
}
SCE_SVoxelWorld* SCE_VOTerrain_GetVoxelWorld (SCE_SVoxelOctreeTerrain *vt)
{
    return vt->vw;
}
void SCE_VOTerrain_SetNumRegions (SCE_SVoxelOctreeTerrain *vt, SCEuint n)
{
    vt->n_regions = n;
}
SCEuint SCE_VOTerrain_GetNumRegions (const SCE_SVoxelOctreeTerrain *vt)
{
    return vt->n_regions;
}
void SCE_VOTerrain_SetUnit (SCE_SVoxelOctreeTerrain *vt, float unit)
{
    vt->scale = unit;
}
void SCE_VOTerrain_SetShader (SCE_SVoxelOctreeTerrain *vt, SCE_SShader *shader)
{
    vt->shader = shader;
}

static int SCE_VOTerrain_BuildPipeline (SCE_SVoxelOctreeTerrain *vt,
                                        SCE_SVOTerrainPipeline *pipe)
{
    long w, h, d;
    size_t n, dim, size;

    w = SCE_VWorld_GetWidth (vt->vw);
    h = SCE_VWorld_GetHeight (vt->vw);
    d = SCE_VWorld_GetDepth (vt->vw);

    SCE_VRender_SetDimensions (&pipe->template, w, h, d);
    SCE_VRender_SetVolumeDimensions (&pipe->template, w, h, d);
    SCE_VRender_SetCompressedScale (&pipe->template, 1.0);
    if (SCE_VRender_Build (&pipe->template) < 0)
        goto fail;

    /* +1 for borders */
    SCE_Grid_SetDimensions (&pipe->grid, w + 1, h + 1, d + 1);
    SCE_Grid_SetPointSize (&pipe->grid, 1);
    if (SCE_Grid_Build (&pipe->grid) < 0)
        goto fail;

    if (!(pipe->tc = SCE_TexData_Create ()))
        goto fail;
    if (!(pipe->tc2 = SCE_TexData_Create ()))
        goto fail;
    if (!(pipe->tex = SCE_Texture_Create (SCE_TEX_3D, 0, 0, 0)))
        goto fail;
    if (!(pipe->tex2 = SCE_Texture_Create (SCE_TEX_3D, 0, 0, 0)))
        goto fail;
    /* TODO: we dont want PXF_LUMINANCE, and ToTexture() actually sucks because
       it chooses a shitty format */
    SCE_Grid_ToTexture (&pipe->grid, pipe->tc, SCE_PXF_LUMINANCE,
                        SCE_UNSIGNED_BYTE);
    SCE_Texture_AddTexData (pipe->tex, SCE_TEX_3D, pipe->tc);
    SCE_Grid_ToTexture (&pipe->grid, pipe->tc2, SCE_PXF_LUMINANCE,
                        SCE_UNSIGNED_BYTE);
    SCE_Texture_AddTexData (pipe->tex2, SCE_TEX_3D, pipe->tc2);

    SCE_Texture_Build (pipe->tex, SCE_FALSE);
    SCE_Texture_Build (pipe->tex2, SCE_FALSE);

    n = SCE_Grid_GetNumPoints (&pipe->grid);
    dim = w;

    if (!(pipe->vertices = SCE_malloc (n * 9 * sizeof *pipe->vertices)))
        goto fail;
    if (!(pipe->normals = SCE_malloc (n * 9 * sizeof *pipe->normals)))
        goto fail;
    if (!(pipe->indices = SCE_malloc (n * 15 * sizeof *pipe->indices)))
        goto fail;
    if (!(pipe->anchors = SCE_malloc (dim * dim * 6 * sizeof *pipe->anchors)))
        goto fail;
    size  = vt->comp_pos ? 4 : 4 * sizeof (SCEvertices);
    size += vt->comp_nor ? 4 : 3 * sizeof (SCEvertices);
    size *= n * 3;
    if (!(pipe->interleaved = SCE_malloc (size)))
        goto fail;

    SCE_QEMD_SetMaxVertices (&pipe->qmesh, n * 3);
    SCE_QEMD_SetMaxIndices (&pipe->qmesh, n * 15);
    if (SCE_QEMD_Build (&pipe->qmesh) < 0)
        goto fail;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

int SCE_VOTerrain_Build (SCE_SVoxelOctreeTerrain *vt)
{
    if (SCE_VOTerrain_BuildPipeline (vt, &vt->pipe) < 0)
        goto fail;

    /* derp? */

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


static void SCE_VOTerrain_SetToPool (SCE_SVoxelOctreeTerrain *vt,
                                     SCE_SVOTerrainRegion *region)
{
    SCE_Mesh_SetNumVertices (&region->mesh, 0);
    SCE_Mesh_SetNumIndices (&region->mesh, 0);
    SCE_Mesh_ReallocStream (&region->mesh, SCE_MESH_STREAM_G, vt->pipe.pool);
    SCE_Mesh_ReallocIndexBuffer (&region->mesh, vt->pipe.pool);
    SCE_List_Remove (&region->it3);
    SCE_List_Appendl (&vt->pool, &region->it3);
}
static SCE_SVOTerrainRegion*
SCE_VOTerrain_GetFromPool (SCE_SVoxelOctreeTerrain *vt)
{
    if (SCE_List_HasElements (&vt->pool)) {
        void *data;
        SCE_SListIterator *it = SCE_List_GetFirst (&vt->pool);
        data = SCE_List_GetData (it);
        SCE_List_Removel (it);
        return data;
    } else {
        /* allocate a new region... or a bunch of regions? whatev. */
        SCE_SVOTerrainRegion *region = NULL;
        SCE_SGeometry *geom = NULL;

        if (!(region = SCE_VOTerrain_CreateRegion ()))
            goto fail;

        geom = SCE_VRender_GetFinalGeometry (&vt->pipe.template);
        if (SCE_Mesh_SetGeometry (&region->mesh, geom, SCE_FALSE) < 0)
            goto fail;
        /* TODO: maybe not autobuild, maybe.. specify some buffer options
           (like "dont allocate them since the pool will do it") */
        SCE_Mesh_AutoBuild (&region->mesh);
        SCE_VRender_SetMesh (&region->vm, &region->mesh);

        return region;
    }
fail:
    SCEE_LogSrc ();
    return NULL;
}

static int SCE_VOTerrain_UpdateNodes(SCE_SVoxelOctreeTerrain *vt, SCEuint level,
                                     SCE_SLongRect3 *rect)
{
    SCE_SListIterator *it = NULL, *pro = NULL;
    SCE_SList list, tmp;
    SCE_SVoxelOctreeNode *node = NULL;
    SCE_SVOTerrainRegion *region = NULL;
    SCE_SVOTerrainLevel *tl = &vt->levels[level];

    /* flush, but dont append them to vt->pool yet */
    SCE_List_Init (&tmp);
    SCE_List_AppendAll (&tmp, &tl->regions);

    /* select matching nodes */
    SCE_List_Init (&list);
    SCE_VWorld_FetchNodes (vt->vw, tl->level, rect, &list);

    /* keep existing regions */
    SCE_List_ForEachProtected (pro, it, &list) {
        node = SCE_List_GetData (it);
        region = SCE_VOctree_GetNodeData (node);
        if (region) {
            SCE_List_Removel (it); /* keep only null region nodes */
            SCE_List_Removel (&region->it3);
            SCE_List_Appendl (&tl->regions, &region->it3);
        }
    }
    /* those remaining can be marked as expired and added to the global pool */
    SCE_List_ForEachProtected (pro, it, &tmp) {
        region = SCE_List_GetData (it);
        SCE_VOctree_SetNodeData (region->node, NULL);
        /* why do we need node to be NULL? probably for safety purposes */
        region->node = NULL;
        if (SCE_List_IsAttached (&region->it)) {
            SCEE_SendMsg ("removing a pipelined region dErP\n");
            /* TODO: super ugly, remove from the pipeline */
            SCE_List_Remove (&region->it);
        }
        SCE_List_Remove (&region->it2); /* TODO: remove that call see what happen */
        SCE_List_Remove (&region->it4);
        SCE_VOTerrain_SetToPool (vt, region);
    }
    /* create missing regions and queue them for updating */
    SCE_List_ForEach (it, &list) {
        node = SCE_List_GetData (it);
        region = SCE_VOctree_GetNodeData (node);

        /* get a region from the global pool */
        if (!(region = SCE_VOTerrain_GetFromPool (vt)))
            goto fail;
        region->level = tl;
        region->node = node;
        SCE_VOctree_SetNodeData (node, region);
        /* TODO: freefunc */
        /* actually we dont really need any, since we keep every
           region in tl->regions */
        SCE_VOctree_SetNodeFreeFunc (node, NULL);
        SCE_List_Appendl (&tl->regions, &region->it3);

        /* queue for update */
        /* TODO: we do want to remove it from the pipeline and put it back
           actually, because the pipeline could be working on outdated data */
        if (!SCE_List_IsAttached (&region->it))
            SCE_List_Appendl (&vt->pipe.stages[0], &region->it);
    }

    SCE_List_Flush (&list);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

static int SCE_VOTerrain_SetLevelPosition (SCE_SVoxelOctreeTerrain *vt,
                                           SCEuint level, long x, long y,
                                           long z)
{
    int moved = SCE_FALSE;
    long threshold;
    long w, h, d, p1[3], p2[3];
    SCE_SLongRect3 rect;
    SCE_SVOTerrainLevel *tl = &vt->levels[level];

    tl->x = x;
    tl->y = y;
    tl->z = z;

    /* get theoretical origin */
    w = vt->n_regions * vt->w;
    h = vt->n_regions * vt->h;
    d = vt->n_regions * vt->d;
    x = x - w / 2;
    y = y - h / 2;
    z = z - d / 2;

    /* we do hope w = h = d actually */
    threshold = vt->w + vt->w / 2;

    while (x - tl->origin_x > threshold) {
        moved = SCE_TRUE;
        tl->origin_x += 2 * vt->w;
    }
    while (x - tl->origin_x < -threshold) {
        moved = SCE_TRUE;
        tl->origin_x -= 2 * vt->w;
    }
    while (y - tl->origin_y > threshold) {
        moved = SCE_TRUE;
        tl->origin_y += 2 * vt->h;
    }
    while (y - tl->origin_y < -threshold) {
        moved = SCE_TRUE;
        tl->origin_y -= 2 * vt->h;
    }
    while (z - tl->origin_z > threshold) {
        moved = SCE_TRUE;
        tl->origin_z += 2 * vt->d;
    }
    while (z - tl->origin_z < -threshold) {
        moved = SCE_TRUE;
        tl->origin_z -= 2 * vt->d;
    }

    SCE_VOTerrain_GetCurrentRectangle (vt, level, &rect);
    if (moved && SCE_VOTerrain_UpdateNodes (vt, level, &rect) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }

    return SCE_OK;
}

int SCE_VOTerrain_SetPosition (SCE_SVoxelOctreeTerrain *vt, long x, long y,
                               long z)
{
    int i;

    for (i = 0; i < vt->n_levels; i++) {
        if (SCE_VOTerrain_SetLevelPosition (vt, i, x, y, z) < 0) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
        x /= 2;
        y /= 2;
        z /= 2;
    }

    return SCE_OK;
}

void SCE_VOTerrain_GetRectangle (const SCE_SVoxelOctreeTerrain *vt,
                                 SCEuint level, SCE_SLongRect3 *r)
{
    long x, y, z;
    x = vt->x << level;
    y = vt->y << level;
    z = vt->z << level;
    SCE_Rectangle3_SetFromCenterl (r, x, y, z, vt->w, vt->h, vt->d);
}
void SCE_VOTerrain_GetCurrentRectangle (const SCE_SVoxelOctreeTerrain *vt,
                                        SCEuint level, SCE_SLongRect3 *rect)
{
    long w, h, d;
    const SCE_SVOTerrainLevel *tl = &vt->levels[level];
    w = vt->n_regions * vt->w;
    h = vt->n_regions * vt->h;
    d = vt->n_regions * vt->d;
    SCE_Rectangle3_SetFromOriginl (rect, tl->origin_x, tl->origin_y,
                                   tl->origin_z, w, h, d);
}

static void SCE_VOTerrain_MakeRegionMatrix (SCE_SVoxelOctreeTerrain *vt,
                                            SCEuint level,
                                            SCE_SVOTerrainRegion *region)
{
    long x, y, z;
    float x_, y_, z_;
    float scale;

    /* voxel scale */
    SCE_Matrix4_Scale (region->matrix, vt->scale, vt->scale, vt->scale);
    /* level space */
    scale = 1 << level;
    SCE_Matrix4_MulScale (region->matrix, scale, scale, scale);
    /* translate */
    SCE_VOctree_GetNodeOriginv (region->node, &x, &y, &z);
    x_ = x * ((float)(vt->w - 1) / (float)vt->w);
    y_ = y * ((float)(vt->h - 1) / (float)vt->h);
    z_ = z * ((float)(vt->d - 1) / (float)vt->d);

    SCE_Matrix4_MulTranslate (region->matrix, x_, y_, z_);
    /* voxel unit */
    SCE_Matrix4_MulScale (region->matrix, vt->w, vt->h, vt->d);
}


void SCE_VOTerrain_CullRegions (SCE_SVoxelOctreeTerrain *vt,
                                const SCE_SFrustum *f)
{
    int i;
    SCE_SListIterator *it = NULL;
    SCE_SVOTerrainRegion *region = NULL;
    SCE_SBoundingBox box;
    SCE_SBox b;
    SCE_TVector3 pos;

    if (!f)
        return;

    /* construct bounding box */
    SCE_Box_Init (&b);
    SCE_Vector3_Set (pos, 0.0, 0.0, 0.0);
    SCE_Box_Set (&b, pos, 1.0, 1.0, 1.0);
    SCE_BoundingBox_Init (&box);
    SCE_BoundingBox_SetFrom (&box, &b);

    SCE_List_Flush (&vt->to_render);

    for (i = 0; i < vt->n_levels; i++) {
        SCE_List_ForEach (it, &vt->levels[i].to_render) {
            region = SCE_List_GetData (it);
            SCE_VOTerrain_MakeRegionMatrix (vt, i, region);
            SCE_BoundingBox_Push (&box, region->matrix, &b);
            if (SCE_Frustum_BoundingBoxInBool (f, &box))
                SCE_List_Appendl (&vt->to_render, &region->it4);
            SCE_BoundingBox_Pop (&box, &b);
        }
    }
}


static int SCE_VOTerrain_UpdateGeometry (SCE_SVoxelOctreeTerrain *vt)
{
    int level;
    SCE_SLongRect3 rect;

    while ((level = SCE_VWorld_GetNextUpdatedRegion (vt->vw, &rect)) >= 0)
        SCE_VOTerrain_UpdateNodes (vt, level, &rect);

    return SCE_OK;
}


/* upload texture */
static int SCE_VOTerrain_Stage1 (SCE_SVoxelOctreeTerrain *vt,
                                 SCE_SVOTerrainPipeline *pipe)
{
    SCE_SVOTerrainRegion *region;
    SCE_EVoxelOctreeStatus status;
    SCE_SVoxelOctree *vo = NULL;
    SCE_STexData *tc = NULL;
    SCE_SLongRect3 rect;
    long x, y, z;

    do {
        if (!SCE_List_HasElements (&pipe->stages[0]))
            return SCE_OK;

        region = SCE_List_GetData (SCE_List_GetFirst (&pipe->stages[0]));
        SCE_List_Removel (&region->it);
        status = SCE_VOctree_GetNodeStatus (region->node);
    } while (status == SCE_VOCTREE_NODE_EMPTY ||
             status == SCE_VOCTREE_NODE_FULL);

    /* retrieve data */
    SCE_VOctree_GetNodeOriginv (region->node, &x, &y, &z);
    SCE_Rectangle3_SetFromOriginl (&rect, x, y, z, vt->w+1, vt->h+1, vt->d+1);
    SCE_VWorld_GetRegion (vt->vw, region->level->level, &rect,
                          SCE_Grid_GetRaw (&pipe->grid));

    /* upload */
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    SCE_Texture_Update (pipe->tex);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 4);
    SCE_List_Appendl (&pipe->stages[1], &region->it); /* onto the next stage */

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

/* generate geometry using the GPU (preferably) */
static int SCE_VOTerrain_Stage2 (SCE_SVoxelOctreeTerrain *vt,
                                 SCE_SVOTerrainPipeline *pipe)
{
    SCE_SVOTerrainRegion *region;

    if (!SCE_List_HasElements (&pipe->stages[1]))
        return SCE_OK;

    region = SCE_List_GetData (SCE_List_GetFirst (&pipe->stages[1]));
    SCE_List_Removel (&region->it);

    if (SCE_VRender_Hardware (&pipe->template, pipe->tex, &region->vm,
                              0, 0, 0) < 0)
        goto fail;

    if (!SCE_VRender_IsEmpty (&region->vm))
        SCE_List_Appendl (&pipe->stages[2], &region->it);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

/* TODO: download the geometry (and then simplify and then blblbl) */
static int SCE_VOTerrain_Stage3 (SCE_SVoxelOctreeTerrain *vt,
                                 SCE_SVOTerrainPipeline *pipe)
{
    SCE_SVOTerrainRegion *region;

    if (!SCE_List_HasElements (&pipe->stages[2]))
        return SCE_OK;

    region = SCE_List_GetData (SCE_List_GetFirst (&pipe->stages[2]));
    SCE_List_Removel (&region->it);

    SCE_List_Remove (&region->it2);
    SCE_List_Appendl (&region->level->to_render, &region->it2);
    SCE_List_Appendl (&vt->to_render, &region->it4);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

static void SCE_VOTerrain_SwitchPipelineTextures (SCE_SVOTerrainPipeline *pipe)
{
    SCE_STexData *tc;
    SCE_STexture *tex;
    tc = pipe->tc;
    tex = pipe->tex;
    pipe->tc = pipe->tc2;
    pipe->tex = pipe->tex2;
    pipe->tc2 = tc;
    pipe->tex2 = tex;
}
static int SCE_VOTerrain_UpdatePipeline (SCE_SVoxelOctreeTerrain *vt,
                                         SCE_SVOTerrainPipeline *pipe)
{
    if (SCE_VOTerrain_Stage3 (vt, pipe) < 0) goto fail;
    if (SCE_VOTerrain_Stage2 (vt, pipe) < 0) goto fail;
    if (SCE_VOTerrain_Stage1 (vt, pipe) < 0) goto fail;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

int SCE_VOTerrain_Update (SCE_SVoxelOctreeTerrain *vt)
{
    /* queue regions that need to be updated */
    if (SCE_VOTerrain_UpdateGeometry (vt) < 0) goto fail;
    /* render queued regions */
    if (SCE_VOTerrain_UpdatePipeline (vt, &vt->pipe) < 0) goto fail;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

void SCE_VOTerrain_Render (SCE_SVoxelOctreeTerrain *vt)
{
    SCE_SListIterator *it = NULL;
    SCE_SVOTerrainRegion *region = NULL;

    /* TODO: apparently in "shadow mode" shaders are locked, but maybe
       with this kind of terrain we dont need special shadow shaders */

    SCE_Shader_Use (vt->shader);

    SCE_List_ForEach (it, &vt->to_render) {
        region = SCE_List_GetData (it);

        SCE_VOTerrain_MakeRegionMatrix (vt, region->level->level, region);

        SCE_RLoadMatrix (SCE_MAT_OBJECT, region->matrix);
        SCE_Mesh_Use (&region->mesh);
        SCE_Mesh_Render ();
        SCE_Mesh_Unuse ();
    }

    SCE_Shader_Use (NULL);
}
