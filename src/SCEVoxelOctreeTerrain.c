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
   updated: 20/03/2013 */

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEVoxelRenderer.h"

#include "SCE/interface/SCEVoxelOctreeTerrain.h"


static void SCE_VOTerrain_InitRegion (SCE_SVOTerrainRegion *region)
{
    region->status = SCE_VOTERRAIN_REGION_POOL; /* even though it's not true. */
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
    SCE_List_Init (&tl->ready);
    SCE_List_Init (&tl->to_render);
    SCE_List_Init (&tl->hidden);
}
static void SCE_VOTerrain_ClearLevel (SCE_SVOTerrainLevel *tl)
{
    SCE_List_Clear (&tl->regions);
    SCE_List_Clear (&tl->ready);
    SCE_List_Clear (&tl->to_render);
    SCE_List_Clear (&tl->hidden);
}


static void SCE_VOTerrain_InitPipeline (SCE_SVOTerrainPipeline *pipe)
{
    int i;

    SCE_VRender_Init (&pipe->template);
    SCE_VRender_SetPipeline (&pipe->template, SCE_VRENDER_HARDWARE);
    SCE_Grid_Init (&pipe->grid);
    pipe->tc = pipe->tc2 = NULL;
    pipe->tex = pipe->tex2 = NULL;
    SCE_Mesh_Init (&pipe->mesh);
    SCE_Mesh_Init (&pipe->mesh2);
    SCE_VRender_InitMesh (&pipe->vmesh);
    SCE_VRender_InitMesh (&pipe->vmesh2);
    SCE_VRender_SetMesh (&pipe->vmesh, &pipe->mesh);
    SCE_VRender_SetMesh (&pipe->vmesh2, &pipe->mesh2);
    pipe->vertex_pool = &pipe->default_vertex_pool;
    pipe->index_pool = &pipe->default_index_pool;
    SCE_RInitBufferPool (&pipe->default_vertex_pool);
    SCE_RSetBufferPoolTarget (&pipe->default_vertex_pool, GL_ARRAY_BUFFER);
    SCE_RInitBufferPool (&pipe->default_index_pool);
    SCE_RSetBufferPoolTarget (&pipe->default_index_pool,
                              GL_ELEMENT_ARRAY_BUFFER);
    /* TODO: set pool usage? */
    SCE_VRender_SetVertexBufferPool (&pipe->template, pipe->vertex_pool);
    SCE_VRender_SetIndexBufferPool (&pipe->template, pipe->index_pool);

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
    SCE_Texture_Delete (pipe->tex2);
    SCE_Mesh_Clear (&pipe->mesh);
    SCE_Mesh_Clear (&pipe->mesh2);
    SCE_Grid_Clear (&pipe->grid);
    SCE_RClearBufferPool (&pipe->default_vertex_pool);
    SCE_RClearBufferPool (&pipe->default_index_pool);

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
    SCE_Geometry_Init (&vt->region_geom);
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
    SCE_Geometry_Clear (&vt->region_geom);
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
    SCE_SGeometry *geom = NULL;

    w = SCE_VWorld_GetWidth (vt->vw);
    h = SCE_VWorld_GetHeight (vt->vw);
    d = SCE_VWorld_GetDepth (vt->vw);

    SCE_VRender_SetDimensions (&pipe->template, w+2, h+2, d+2);
    SCE_VRender_SetVolumeDimensions (&pipe->template, w + 4, h + 4, d + 4);
    SCE_VRender_SetCompressedScale (&pipe->template, 1.0);
    if (SCE_VRender_Build (&pipe->template) < 0)
        goto fail;

    /* one row on each side for normals, one row at the end for complete
       triangle set, one row at the beginning for sharing triangles with
       neighboring regions */
    SCE_Grid_SetDimensions (&pipe->grid, w + 4, h + 4, d + 4);
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

    geom = SCE_VRender_GetFinalGeometry (&pipe->template);
    if (SCE_Mesh_SetGeometry (&pipe->mesh, geom, SCE_FALSE) < 0)
        goto fail;
    if (SCE_Mesh_SetGeometry (&pipe->mesh2, geom, SCE_FALSE) < 0)
        goto fail;
    SCE_Mesh_AutoBuild (&pipe->mesh);
    SCE_Mesh_AutoBuild (&pipe->mesh2);

    n = SCE_Grid_GetNumPoints (&pipe->grid);
    dim = w;

    if (!(pipe->vertices = SCE_malloc (n * 9 * sizeof *pipe->vertices)))
        goto fail;
    if (!(pipe->normals = SCE_malloc (n * 9 * sizeof *pipe->normals)))
        goto fail;
    if (!(pipe->indices = SCE_malloc (n * 15 * sizeof (SCEuint))))
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

static int SCE_VOTerrain_MakeRegionGeometry (SCE_SGeometry *geom)
{
    SCE_SGeometryArray ar1, ar2;

    SCE_Geometry_Init (geom);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_InitArray (&ar2);
    SCE_Geometry_SetArrayData (&ar1, SCE_POSITION, SCE_VERTICES_TYPE, 0, 3,
                               NULL, SCE_FALSE);
    SCE_Geometry_SetArrayData (&ar2, SCE_NORMAL, SCE_VERTICES_TYPE, 0, 3,
                               NULL, SCE_FALSE);
    SCE_Geometry_AttachArray (&ar1, &ar2);
    if (SCE_Geometry_AddArrayRecDup (geom, &ar1, SCE_FALSE) < 0)
        goto fail;
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayIndices (&ar1, SCE_INDICES_TYPE, NULL, SCE_FALSE);
    if (SCE_Geometry_SetIndexArrayDup (geom, &ar1, SCE_FALSE) < 0)
        goto fail;
    SCE_Geometry_SetPrimitiveType (geom, SCE_TRIANGLES);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

int SCE_VOTerrain_Build (SCE_SVoxelOctreeTerrain *vt)
{
    if (SCE_VOTerrain_BuildPipeline (vt, &vt->pipe) < 0)
        goto fail;

    if (SCE_VOTerrain_MakeRegionGeometry (&vt->region_geom) < 0)
        goto fail;

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
    SCE_Mesh_ReallocStream (&region->mesh, SCE_MESH_STREAM_G,
                            vt->pipe.vertex_pool);
    SCE_Mesh_ReallocIndexBuffer (&region->mesh, vt->pipe.index_pool);
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
        SCE_SGeometry geom;

        if (!(region = SCE_VOTerrain_CreateRegion ()))
            goto fail;

        if (SCE_Mesh_SetGeometry (&region->mesh,&vt->region_geom,SCE_FALSE) < 0)
            goto fail;
        /* TODO: maybe not autobuild, maybe.. specify some buffer options
           (like "dont allocate them since the pool will do it") */
        SCE_Mesh_AutoBuild (&region->mesh);

        return region;
    }
fail:
    SCEE_LogSrc ();
    return NULL;
}

static void SCE_VOTerrain_Region (SCE_SVoxelOctreeTerrain *vt,
                                  SCE_SVOTerrainRegion *region,
                                  SCE_EVOTerrainRegionStatus status)
{
    switch (status) {
    case SCE_VOTERRAIN_REGION_POOL:
        if (SCE_List_IsAttached (&region->it)) {
            SCEE_SendMsg ("ERROR: removing a pipelined region dErP\n");
            /* TODO: super ugly, remove from the pipeline */
            SCE_List_Remove (&region->it);
        }
        SCE_VOctree_SetNodeData (region->node, NULL);
        /* why do we need node to be NULL? probably for safety purposes */
        region->node = NULL;
        SCE_List_Remove (&region->it2);
        SCE_List_Remove (&region->it3);
        SCE_List_Remove (&region->it4);
        SCE_VOTerrain_SetToPool (vt, region);
        break;

    case SCE_VOTERRAIN_REGION_PIPELINE:
        /* TODO: we do want to remove it from the pipeline and put it back
           actually, because the pipeline could be working on outdated data */
        if (!SCE_List_IsAttached (&region->it))
            SCE_List_Appendl (&vt->pipe.stages[0], &region->it);
        break;

    case SCE_VOTERRAIN_REGION_READY:
        SCE_List_Remove (&region->it2); /* ? */
        SCE_List_Appendl (&region->level->ready, &region->it2);
        break;

    case SCE_VOTERRAIN_REGION_RENDER:
        SCE_List_Remove (&region->it);
        SCE_List_Remove (&region->it2);
        SCE_List_Remove (&region->it4);
        SCE_List_Appendl (&region->level->to_render, &region->it2);
        SCE_List_Appendl (&vt->to_render, &region->it4);
        break;

    case SCE_VOTERRAIN_REGION_HIDDEN:
        SCE_List_Remove (&region->it2);
        SCE_List_Remove (&region->it4);
        SCE_List_Appendl (&region->level->hidden, &region->it2);
        break;

#if SCE_DEBUG
    default:
        SCEE_SendMsg ("SCE_VOTerrain_Region(): wrong status value\n");
        return;
#endif
    }
    /* TODO: is it always useful? particularly for hidden: we dont really
       need this information in the status, but we do need to know whether
       a region is being updated in the pipeline or not */
    region->status = status;
}


static int
SCE_VOTerrain_AddNewRegion (SCE_SVoxelOctreeTerrain *vt, SCEuint level,
                            SCE_SVoxelOctreeNode *node)
{
    SCE_SVOTerrainRegion *region = NULL;
    /* get a region from the global pool */
    if (!(region = SCE_VOTerrain_GetFromPool (vt))) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    region->level = &vt->levels[level];
    region->node = node;
    SCE_VOctree_SetNodeData (node, region);
    /* TODO: freefunc */
    /* actually we dont really need any, since we keep every
       region in tl->regions */
    SCE_VOctree_SetNodeFreeFunc (node, NULL);
    SCE_List_Appendl (&region->level->regions, &region->it3);
    return SCE_OK;
}

/* same as UpdateNodes() but fetches from a rectangle first */
static int
SCE_VOTerrain_UpdateMatchingNodes (SCE_SVoxelOctreeTerrain *vt, SCEuint level,
                                   SCE_SLongRect3 *rect)
{
    SCE_SList list;
    SCE_SLongRect3 r1, r2;
    SCE_SListIterator *it = NULL;
    SCE_SVoxelOctreeNode *node = NULL;
    SCE_SVOTerrainRegion *region = NULL;
    SCE_EVoxelOctreeStatus status;

    /* crop rect inside level's */
    SCE_VOTerrain_GetCurrentRectangle (vt, level, &r1);
    SCE_Rectangle3_Intersectionl (&r1, rect, &r2);

    /* select matching nodes */
    SCE_List_Init (&list);
    SCE_VWorld_FetchNodes (vt->vw, level, &r2, &list);

    SCE_List_ForEach (it, &list) {
        node = SCE_List_GetData (it);
        status = SCE_VOctree_GetNodeStatus (node);
        region = SCE_VOctree_GetNodeData (node);

        if (status == SCE_VOCTREE_NODE_EMPTY || status == SCE_VOCTREE_NODE_FULL) {
            if (region)
                SCE_VOTerrain_Region (vt, region, SCE_VOTERRAIN_REGION_POOL);
            continue;
        } else if (!region) {
            if (SCE_VOTerrain_AddNewRegion (vt, level, node) < 0)
                goto fail;
            region = SCE_VOctree_GetNodeData (node);
        }
        /* queue for update */
        SCE_VOTerrain_Region (vt, region, SCE_VOTERRAIN_REGION_PIPELINE);
    }

    SCE_List_Flush (&list);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

static int SCE_VOTerrain_UpdateLevel(SCE_SVoxelOctreeTerrain *vt, SCEuint level,
                                     SCE_SLongRect3 *rect)
{
    SCE_SListIterator *it = NULL, *pro = NULL;
    SCE_SList list, tmp;
    SCE_SVoxelOctreeNode *node = NULL;
    SCE_SVOTerrainRegion *region = NULL;
    SCE_EVoxelOctreeStatus status;
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
        SCE_VOTerrain_Region (vt, region, SCE_VOTERRAIN_REGION_POOL);
    }
    /* create missing regions and queue them for updating */
    SCE_List_ForEach (it, &list) {
        node = SCE_List_GetData (it);

        status = SCE_VOctree_GetNodeStatus (node);
        if (status == SCE_VOCTREE_NODE_EMPTY || status == SCE_VOCTREE_NODE_FULL)
            continue;

        /* add new region */
        if (SCE_VOTerrain_AddNewRegion (vt, level, node) < 0)
            goto fail;
        region = SCE_VOctree_GetNodeData (node);

        /* queue for update */
        SCE_VOTerrain_Region (vt, region, SCE_VOTERRAIN_REGION_PIPELINE);
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
    long w, h, d;
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
    if (moved && SCE_VOTerrain_UpdateLevel (vt, level, &rect) < 0) {
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

size_t SCE_VOTerrain_GetUsedVRAM (const SCE_SVoxelOctreeTerrain *vt)
{
    size_t size = 0;
    int i;
    SCE_SListIterator *it = NULL;
    SCE_SVOTerrainRegion *region = NULL;

    for (i = 0; i < vt->n_levels; i++) {
        SCE_List_ForEach (it, &vt->levels[i].regions) {
            region = SCE_List_GetData (it);
            size += SCE_Mesh_GetUsedVRAM (&region->mesh);
        }
    }

    return size;
}


static void SCE_VOTerrain_MakeRegionMatrix (SCE_SVoxelOctreeTerrain *vt,
                                            SCEuint level,
                                            SCE_SVOTerrainRegion *region)
{
    long x, y, z;
    float w, w2;
    float x_, y_, z_;
    float scale;

    w = vt->w + 4;
    w2 = w - 2;

    /* voxel scale */
    SCE_Matrix4_Scale (region->matrix, vt->scale, vt->scale, vt->scale);
    /* level space */
    scale = 1 << level;
    SCE_Matrix4_MulScale (region->matrix, scale, scale, scale);
    /* translate */
    SCE_VOctree_GetNodeOriginv (region->node, &x, &y, &z);
    /* TODO: why -1 ??? probably because we now generate one extra row */
    x_ = x * (w2 - 1.0) / (w - 1.0) - 1;
    y_ = y * (w2 - 1.0) / (w - 1.0) - 1;
    z_ = z * (w2 - 1.0) / (w - 1.0) - 1;

    SCE_Matrix4_MulTranslate (region->matrix, x_, y_, z_);
    /* voxel unit */
    scale = (w - 1.0) / (w2 - 1.0);
    SCE_Matrix4_MulScale (region->matrix, scale, scale, scale);
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
            /* TODO: fix SCE_Frustum_Bounblblbl() 1st param const */
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

    while ((level = SCE_VWorld_GetNextUpdatedRegion (vt->vw, &rect)) >= 0) {
        if (SCE_VOTerrain_UpdateMatchingNodes (vt, level, &rect) < 0) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
    }

    return SCE_OK;
}


static void SCE_VOTerrain_UpdateRegions (SCE_SVoxelOctreeTerrain *vt)
{
    int i, j;
    SCE_SLongRect3 rect;
    SCE_SList list;
    SCE_SListIterator *it = NULL, *pro = NULL;
    SCE_SVoxelOctreeNode *node = NULL, **children = NULL;
    SCE_SVOTerrainRegion *region = NULL, *head = NULL;
    SCE_EVoxelOctreeStatus status;

    SCE_List_Init (&list);

    for (i = 1; i < vt->n_levels; i++) {
        /* fetch nodes underneath the higher LOD */
        SCE_VOTerrain_GetCurrentRectangle (vt, i - 1, &rect);
        SCE_Rectangle3_Divl (&rect, 2, 2, 2);

        SCE_List_ForEachProtected (pro, it, &vt->levels[i].hidden) {
            region = SCE_List_GetData (it);
            /* TODO: maybe there's a better scheme for that: how can we be
               sure that we want to render this region? */
            if (!SCE_List_IsAttached (&region->it))
                SCE_VOTerrain_Region (vt, region, SCE_VOTERRAIN_REGION_RENDER);
        }

        SCE_VWorld_FetchNodes (vt->vw, i, &rect, &list);
        SCE_List_ForEach (it, &list) {
            int k = 0;
            node = SCE_List_GetData (it);
            head = SCE_VOctree_GetNodeData (node);
            status = SCE_VOctree_GetNodeStatus (node);

            if (!head || status != SCE_VOCTREE_NODE_NODE)
                continue;

            children = SCE_VOctree_GetNodeChildren (node);
            for (j = 0; j < 8; j++) {
                status = SCE_VOctree_GetNodeStatus (children[j]);

                switch (status) {
                case SCE_VOCTREE_NODE_EMPTY:
                case SCE_VOCTREE_NODE_FULL:
                    k++;
                    break;

                case SCE_VOCTREE_NODE_NODE:
                case SCE_VOCTREE_NODE_LEAF:
                    region = SCE_VOctree_GetNodeData (children[j]);
                    if (region &&
                        region->status != SCE_VOTERRAIN_REGION_PIPELINE)
                        k++;
                    break;
                }
            }

            if (k == 8) {
                for (j = 0; j < 8; j++) {
                    region = SCE_VOctree_GetNodeData (children[j]);
                    if (region && region->status == SCE_VOTERRAIN_REGION_READY)
                        SCE_VOTerrain_Region (vt, region,
                                              SCE_VOTERRAIN_REGION_RENDER);
                }
                /* NOTE: we dont wanna pull head off the pipeline; make sure
                   Region() doesnt do it */
                SCE_VOTerrain_Region (vt, head, SCE_VOTERRAIN_REGION_HIDDEN);
            }
        }
        SCE_List_Flush (&list);
    }
}


/* upload texture */
static int SCE_VOTerrain_Stage1 (SCE_SVoxelOctreeTerrain *vt,
                                 SCE_SVOTerrainPipeline *pipe)
{
    SCE_SVOTerrainRegion *region;
    SCE_SVoxelOctree *vo = NULL;
    SCE_STexData *tc = NULL;
    SCE_SLongRect3 rect;
    long x, y, z;

    if (!SCE_List_HasElements (&pipe->stages[0]))
        return SCE_OK;

    region = SCE_List_GetData (SCE_List_GetFirst (&pipe->stages[0]));
    SCE_List_Removel (&region->it);

    /* retrieve data */
    SCE_VOctree_GetNodeOriginv (region->node, &x, &y, &z);
    SCE_Rectangle3_SetFromOriginl (&rect, x - 2, y - 2, z - 2,
                                   vt->w + 4, vt->h + 4, vt->d + 4);
    SCE_Grid_FillupZeros (&pipe->grid);
    SCE_VWorld_GetRegion (vt->vw, region->level->level, &rect,
                          SCE_Grid_GetRaw (&pipe->grid));

    /* upload */
    glPixelStorei (GL_UNPACK_ALIGNMENT, 2);
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

    if (SCE_VRender_Hardware (&pipe->template, pipe->tex, &pipe->vmesh,
                              1, 1, 1) < 0)
        goto fail;

    if (!SCE_VRender_IsEmpty (&pipe->vmesh))
        SCE_List_Appendl (&pipe->stages[2], &region->it);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

static int SCE_VOTerrain_ReallocMesh (SCE_SMesh *mesh, SCE_RBufferPool *v,
                                      SCE_RBufferPool *i)
{
    if (v) {
        if (SCE_Mesh_ReallocStream (mesh, SCE_MESH_STREAM_G, v) < 0)
            goto fail;
    }
    if (i) {
        if (SCE_Mesh_ReallocIndexBuffer (mesh, i) < 0)
            goto fail;
    }
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

static void SCE_VOTerrain_Decode (SCE_SVOTerrainPipeline *pipe)
{
    size_t i;
    SCEindices *ind1 = NULL;
    SCEuint *ind2 = NULL;

    /* vertices */
    /* TODO: this stride is defined upon the geometry as defined by the
       vrender module. our mesh actually doesnt need to fit this geometry
       if we dont plan on rendering data directly emitted by the GPU. */
    /* TODO: for some reason vrender position data is 4 components vector */
    for (i = 0; i < pipe->n_vertices; i++) {
        memcpy (&pipe->vertices[i * 3],
                &pipe->interleaved[i * 7],
                3 * sizeof *pipe->vertices);
        memcpy (&pipe->normals[i * 3],
                &pipe->interleaved[i * 7 + 4],
                3 * sizeof *pipe->vertices);
    }

    /* indices */
    ind1 = pipe->indices;
    ind2 = (SCEuint*)pipe->indices;
    for (i = 0; i < pipe->n_indices; i++)
        ind1[i] = ind2[i];
}

static void SCE_VOTerrain_Encode (SCE_SVOTerrainPipeline *pipe)
{
    long i;

    for (i = 0; i < pipe->n_vertices; i++) {
        memcpy (&pipe->interleaved[i * 6],
                &pipe->vertices[i * 3],
                3 * sizeof *pipe->vertices);
        memcpy (&pipe->interleaved[i * 6 + 3],
                &pipe->normals[i * 3],
                3 * sizeof *pipe->vertices);
    }
}

size_t SCE_VOTerrain_Anchors (SCEvertices *vertices, size_t n_vertices,
                              float inf, float sup, SCEindices *out)
{
    SCEuint i, j;
    SCEvertices *v;

    j = 0;
    for (i = 0; i < n_vertices; i++) {
        v = &vertices[i * 3];
        if (v[0] < inf || v[0] > sup ||
            v[1] < inf || v[1] > sup ||
            v[2] < inf || v[2] > sup) {
            out[j++] = i;
        }
    }

    return j;
}

/* decimate the geometry */
static int SCE_VOTerrain_Stage3 (SCE_SVoxelOctreeTerrain *vt,
                                 SCE_SVOTerrainPipeline *pipe)
{
    SCE_SVOTerrainRegion *region;
    SCEuint n_collapses;
    size_t size, stride;
    float inf, sup;

    if (!SCE_List_HasElements (&pipe->stages[2]))
        return SCE_OK;

    region = SCE_List_GetData (SCE_List_GetFirst (&pipe->stages[2]));
    SCE_List_Removel (&region->it);

    /* download geometry */
    pipe->n_vertices = SCE_Mesh_GetNumVertices (&pipe->mesh);
    pipe->n_indices = SCE_Mesh_GetNumIndices (&pipe->mesh);
    SCE_Mesh_DownloadAllVertices (&pipe->mesh, SCE_MESH_STREAM_G,
                                  pipe->interleaved);
    SCE_Mesh_DownloadAllIndices (&pipe->mesh, pipe->indices);

    /* decode (might include decompression) */
    /* TODO: handle materials brah */
    SCE_VOTerrain_Decode (pipe);

    /* decimate */
    inf = 0.00001 + 1.0 / vt->w;
    sup = (vt->w - 4.0) / (vt->w - 1.0) - 0.0001;
    pipe->n_anchors = SCE_VOTerrain_Anchors (pipe->vertices, pipe->n_vertices,
                                             inf, sup, pipe->anchors);
    SCE_QEMD_Set (&pipe->qmesh, pipe->vertices, pipe->normals, pipe->indices,
                  pipe->n_vertices, pipe->n_indices);
    SCE_QEMD_AnchorVertices (&pipe->qmesh, pipe->anchors, pipe->n_anchors);
    /* aiming at 60% vertex reduction */
    n_collapses = (pipe->n_vertices - pipe->n_anchors) * 0.6;
    SCE_QEMD_Process (&pipe->qmesh, n_collapses);
    SCE_QEMD_Get (&pipe->qmesh, pipe->vertices, pipe->normals, pipe->indices,
                  &pipe->n_vertices, &pipe->n_indices);

    /* encode (might include compression) */
    SCE_VOTerrain_Encode (pipe);

    /* upload geometry */
    SCE_Mesh_SetNumVertices (&region->mesh, pipe->n_vertices);
    SCE_Mesh_SetNumIndices (&region->mesh, pipe->n_indices);
    if (SCE_VOTerrain_ReallocMesh (&region->mesh, pipe->vertex_pool,
                                   pipe->index_pool) < 0)
        goto fail;

    stride = 6 * sizeof (SCEvertices);
    size = stride * pipe->n_vertices;
    SCE_Mesh_UploadVertices (&region->mesh, SCE_MESH_STREAM_G,
                             pipe->interleaved, 0, size);
    size = pipe->n_indices * sizeof *pipe->indices;
    SCE_Mesh_UploadIndices (&region->mesh, pipe->indices, size);

#if 0
    /* TODO: separate GPU generated meshes from final meshes */
    {
        SCE_RIndexBuffer *ib = SCE_Mesh_GetIndexBuffer (&region->mesh);
        /* TODO: direct member access */
        ib->ia.type = SCE_INDICES_TYPE;
    }
#endif

    SCE_VOTerrain_Region (vt, region, SCE_VOTERRAIN_REGION_READY);

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
    if (SCE_VOTerrain_Stage2 (vt, pipe) < 0) goto fail;
    SCE_VOTerrain_SwitchPipelineTextures (pipe);
    if (SCE_VOTerrain_Stage1 (vt, pipe) < 0) goto fail;
    if (SCE_VOTerrain_Stage3 (vt, pipe) < 0) goto fail;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

int SCE_VOTerrain_Update (SCE_SVoxelOctreeTerrain *vt)
{
    /* queue regions that need to be updated */
    if (SCE_VOTerrain_UpdateGeometry (vt) < 0) goto fail;
    /* generate queued regions */
    if (SCE_VOTerrain_UpdatePipeline (vt, &vt->pipe) < 0) goto fail;
    /* see which regions can be rendered (compute hidden regions under higher,
       LOD, etc.) */
    SCE_VOTerrain_UpdateRegions (vt);
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

static void
SCE_VOTerrain_MakeRegionRectangle (SCE_SVoxelOctreeTerrain *vt,
                                   SCE_SVOTerrainRegion *region,
                                   SCE_SLongRect3 *rect)
{
    long x, y, z;
    SCE_VOctree_GetNodeOriginv (region->node, &x, &y, &z);
    SCE_Rectangle3_SetFromOriginl (rect, x, y, z, vt->w, vt->h, vt->d);
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
