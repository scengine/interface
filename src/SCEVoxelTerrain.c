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
   updated: 15/03/2013 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCEVoxelRenderer.h"
#include "SCE/interface/SCEVoxelTerrain.h"

static void SCE_VTerrain_InitRegion (SCE_SVoxelTerrainRegion *tr)
{
    tr->x = tr->y = tr->z = 0;
    tr->wx = tr->wy = tr->wz = 0;
    SCE_VRender_InitMesh (&tr->vm);
    tr->mesh = NULL;
    SCE_Matrix4_Identity (tr->matrix);
    tr->draw = SCE_FALSE;
    SCE_List_InitIt (&tr->it);
    SCE_List_SetData (&tr->it, tr);
    SCE_List_InitIt (&tr->it2);
    SCE_List_SetData (&tr->it2, tr);
    SCE_List_InitIt (&tr->it3);
    SCE_List_SetData (&tr->it3, tr);
    tr->need_update = SCE_FALSE;
    tr->level_list = NULL;
    tr->level = NULL;
}
static void SCE_VTerrain_ClearRegion (SCE_SVoxelTerrainRegion *tr)
{
    SCE_VRender_ClearMesh (&tr->vm);
    SCE_List_Remove (&tr->it);
    SCE_List_Remove (&tr->it2);
    SCE_List_Remove (&tr->it3);
}

static void SCE_VTerrain_InitLevel (SCE_SVoxelTerrainLevel *tl)
{
    tl->level = 0;
    SCE_Grid_Init (&tl->grid);
    SCE_Grid_Init (&tl->grid2);
    tl->wrap[0] = tl->wrap[1] = tl->wrap[2] = 0;
    tl->tex = NULL;
    tl->mat = NULL;
    tl->subregions = 1;
    tl->regions = NULL;
    tl->mesh = NULL;
    tl->wrap_x = tl->wrap_y = tl->wrap_z = 0;
    tl->enabled = SCE_TRUE;
    tl->x = tl->y = tl->z = 0;
    tl->map_x = tl->map_y = tl->map_z = 0;

    tl->need_update = SCE_FALSE;
    SCE_Rectangle3_Init (&tl->update_zone);
    SCE_List_Init (&tl->list1);
    SCE_List_Init (&tl->list2);
    tl->updating = &tl->list1;
    tl->queue = &tl->list2;

    SCE_List_Init (&tl->to_render);

    SCE_List_InitIt (&tl->it);
    SCE_List_SetData (&tl->it, tl);
}
static void SCE_VTerrain_ClearLevel (SCE_SVoxelTerrainLevel *tl)
{
    SCEuint i, n;

    SCE_Grid_Clear (&tl->grid);
    SCE_Grid_Clear (&tl->grid2);
    SCE_Texture_Delete (tl->tex);
    SCE_Texture_Delete (tl->mat);

    n = tl->subregions * tl->subregions;
    n *= tl->subregions;
    if (tl->regions) {
        for (i = 0; i < n; i++)
            SCE_VTerrain_ClearRegion (&tl->regions[i]);
        SCE_free (tl->regions);
    }
    if (tl->mesh) {
        for (i = 0; i < n; i++)
            SCE_Mesh_Clear (&tl->mesh[i]);
        SCE_free (tl->mesh);
    }

    SCE_List_Remove (&tl->it);
}

static void SCE_VTerrain_InitShader (SCE_SVoxelTerrainShader *shader)
{
    shader->shd = NULL;
    shader->regions_loc = shader->current_loc = SCE_SHADER_BAD_INDEX;
    shader->wrapping0_loc = shader->wrapping1_loc = SCE_SHADER_BAD_INDEX;
    shader->enabled_loc = shader->tcorigin_loc = SCE_SHADER_BAD_INDEX;
    shader->hightex_loc = shader->lowtex_loc = SCE_SHADER_BAD_INDEX;
    shader->topdiffuse_loc = shader->sidediffuse_loc = SCE_SHADER_BAD_INDEX;
    shader->noise_loc = SCE_SHADER_BAD_INDEX;
    shader->material_loc = SCE_SHADER_BAD_INDEX;
}
static void SCE_VTerrain_ClearShader (SCE_SVoxelTerrainShader *shader)
{
    SCE_Shader_Delete (shader->shd);
}

static void
SCE_VTerrain_InitHybridGenerator (SCE_SVoxelTerrainHybridGenerator *hybrid)
{
    SCE_List_Init (&hybrid->queue);
    hybrid->dim = 0;
    SCE_Grid_Init (&hybrid->grid);
    SCE_MC_Init (&hybrid->mc_gen);
    hybrid->mc_step = 10000;    /* seems legit. */
    hybrid->query = SCE_FALSE;
    hybrid->grid_ready = SCE_FALSE;
    hybrid->vertices = NULL;
    hybrid->normals = NULL;
    hybrid->indices = NULL;
    hybrid->anchors = NULL;
    hybrid->interleaved = NULL;
    hybrid->n_vertices = 0;
    hybrid->n_indices = 0;
    hybrid->x = hybrid->y = hybrid->z = 0;
    hybrid->level = 0;
    SCE_QEMD_Init (&hybrid->qmesh);
}
static void
SCE_VTerrain_ClearHybridGenerator (SCE_SVoxelTerrainHybridGenerator *hybrid)
{
    SCE_List_Clear (&hybrid->queue);
    SCE_Grid_Clear (&hybrid->grid);
    SCE_free (hybrid->vertices);
    SCE_free (hybrid->normals);
    SCE_free (hybrid->indices);
    SCE_free (hybrid->anchors);
    SCE_free (hybrid->interleaved);
    SCE_QEMD_Clear (&hybrid->qmesh);
}

void SCE_VTerrain_Init (SCE_SVoxelTerrain *vt)
{
    size_t i;

    SCE_VRender_Init (&vt->template);
    vt->comp_pos = vt->comp_nor = SCE_FALSE;
    vt->rpipeline = SCE_VRENDER_SOFTWARE;
    SCE_VRender_SetPipeline (&vt->template, vt->rpipeline);
    vt->cut = 0;
    SCE_VTerrain_InitHybridGenerator (&vt->hybrid);
    vt->vertex_pool = vt->index_pool = NULL;

    vt->subregion_dim = 0;
    vt->n_subregions = 1;
    for (i = 0; i < SCE_MAX_VTERRAIN_LEVELS; i++) {
        SCE_VTerrain_InitLevel (&vt->levels[i]);
        vt->levels[i].level = i;
    }

    vt->x = vt->y = vt->z = 0;
    vt->width = vt->height = vt->depth = 0;
    vt->unit = 1.0;
    vt->scale = 1.0;
    vt->built = SCE_FALSE;

    SCE_List_Init (&vt->to_update);
    vt->update_level = NULL;
    vt->max_updates = 8;

    vt->trans_enabled = SCE_TRUE;
    vt->shadow_mode = SCE_FALSE;
    vt->point_shadow = SCE_FALSE;

    vt->pipeline = NULL;
    for (i = 0; i < SCE_NUM_VTERRAIN_SHADERS; i++)
        SCE_VTerrain_InitShader (&vt->shaders[i]);

    vt->top_diffuse = vt->side_diffuse = vt->noise = NULL;
}
void SCE_VTerrain_Clear (SCE_SVoxelTerrain *vt)
{
    size_t i;

    SCE_VRender_Clear (&vt->template);
    SCE_VTerrain_ClearHybridGenerator (&vt->hybrid);
    for (i = 0; i < SCE_NUM_VTERRAIN_SHADERS; i++)
        SCE_VTerrain_ClearShader (&vt->shaders[i]);
    for (i = 0; i < SCE_MAX_VTERRAIN_LEVELS; i++)
        SCE_VTerrain_ClearLevel (&vt->levels[i]);
}
SCE_SVoxelTerrain* SCE_VTerrain_Create (void)
{
    SCE_SVoxelTerrain *vt = NULL;
    if (!(vt = SCE_malloc (sizeof *vt)))
        SCEE_LogSrc ();
    else
        SCE_VTerrain_Init (vt);
    return vt;
}
void SCE_VTerrain_Delete (SCE_SVoxelTerrain *vt)
{
    if (vt) {
        SCE_VTerrain_Clear (vt);
        SCE_free (vt);
    }
}

static void SCE_VTerrain_AddRegion (SCE_SVoxelTerrain *vt,
                                    SCE_SVoxelTerrainRegion *tr)
{
    SCE_SListIterator *it = &tr->it;

    /* dont need to update with the GPU since it's gonna be done with the CPU
       anyway. plus, if it's already in hybrid->queue then it already has at
       least a GPU generated mesh, so there's really no need to do anything */
    if (SCE_List_IsAttached (&tr->it3))
        return;

    if (!SCE_List_IsAttached (it)) {
        SCE_List_Appendl (tr->level->queue, it);
        tr->need_update = SCE_FALSE;
        tr->level_list = tr->level->queue;
    } else {
        /* double update query: if it is in the list being updated "updating",
           it will need to be inserted back into the "queue" list */
        if (tr->level_list == tr->level->updating)
            tr->need_update = SCE_TRUE;
    }

    /* TODO: double insertions might not be handled perhaps maybe.. ?
       dont really know. */
    if (tr->level->level >= vt->cut && vt->cut)
        SCE_List_Appendl (&vt->hybrid.queue, &tr->it3);

    it = &tr->level->it;
    if (!SCE_List_IsAttached (it))
        SCE_List_Appendl (&vt->to_update, it);
}
static void SCE_VTerrain_RemoveRegion (SCE_SVoxelTerrain *vt,
                                       SCE_SVoxelTerrainRegion *tr)
{
    SCE_SListIterator *it = &tr->it;
    (void)vt;
    if (SCE_List_IsAttached (it)) {
        SCE_List_Removel (it);
        /* insert it back */
        if (tr->need_update)
            SCE_VTerrain_AddRegion (vt, tr);
        else
            tr->level_list = NULL;
    }
}

static unsigned int
SCE_VTerrain_Get (const SCE_SVoxelTerrainLevel *tl, int x, int y, int z)
{
    x = SCE_Math_Ring (x + tl->wrap_x, tl->subregions);
    y = SCE_Math_Ring (y + tl->wrap_y, tl->subregions);
    z = SCE_Math_Ring (z + tl->wrap_z, tl->subregions);
    return x + tl->subregions * (y + tl->subregions * z);
}

static SCE_SVoxelTerrainRegion*
SCE_VTerrain_GetRegion (const SCE_SVoxelTerrainLevel *tl, int x, int y, int z)
{
    unsigned int offset = SCE_VTerrain_Get (tl, x, y, z);
    return &tl->regions[offset];
}

void SCE_VTerrain_SetDimensions (SCE_SVoxelTerrain *vt, int w, int h, int d)
{
    vt->width = w; vt->height = h; vt->depth = d;
    vt->scale = vt->width * vt->unit;
}
void SCE_VTerrain_SetWidth (SCE_SVoxelTerrain *vt, int w)
{
    vt->width = w;
    vt->scale = vt->width * vt->unit;
}
void SCE_VTerrain_SetHeight (SCE_SVoxelTerrain *vt, int h)
{
    vt->height = h;
}
void SCE_VTerrain_SetDepth (SCE_SVoxelTerrain *vt, int d)
{
    vt->depth = d;
}
int SCE_VTerrain_GetWidth (const SCE_SVoxelTerrain *vt)
{
    return vt->width;
}
int SCE_VTerrain_GetHeight (const SCE_SVoxelTerrain *vt)
{
    return vt->height;
}
int SCE_VTerrain_GetDepth (const SCE_SVoxelTerrain *vt)
{
    return vt->depth;
}
size_t SCE_VTerrain_GetUsedVRAM (const SCE_SVoxelTerrain *vt)
{
    size_t size = 0, i, j, n;
    SCE_SVoxelTerrainLevel *l = NULL;

    for (i = 0; i < vt->n_levels; i++) {
        l = &vt->levels[i];
        n = l->subregions;
        n = n * n * n;
        for (j = 0; j < n; j++)
            size += SCE_Mesh_GetUsedVRAM (l->regions[j].mesh);
    }

    return size;
}

void SCE_VTerrain_SetUnit (SCE_SVoxelTerrain *vt, float unit)
{
    vt->unit = unit;
    vt->scale = vt->width * vt->unit;
}


void SCE_VTerrain_SetNumLevels (SCE_SVoxelTerrain *vt, SCEuint n_levels)
{
    vt->n_levels = MIN (n_levels, SCE_MAX_VTERRAIN_LEVELS);
}
SCEuint SCE_VTerrain_GetNumLevels (const SCE_SVoxelTerrain *vt)
{
    return vt->n_levels;
}

void SCE_VTerrain_SetSubRegionDimension (SCE_SVoxelTerrain *vt, SCEuint dim)
{
    vt->subregion_dim = dim;
}

void SCE_VTerrain_SetNumSubRegions (SCE_SVoxelTerrain *vt, SCEuint n)
{
    vt->n_subregions = n;
}


void SCE_VTerrain_CompressPosition (SCE_SVoxelTerrain *vt, int comp)
{
    vt->comp_pos = comp;
    SCE_VRender_CompressPosition (&vt->template, comp);
}
void SCE_VTerrain_CompressNormal (SCE_SVoxelTerrain *vt, int comp)
{
    vt->comp_nor = comp;
    SCE_VRender_CompressNormal (&vt->template, comp);
}
void SCE_VTerrain_SetPipeline (SCE_SVoxelTerrain *vt,
                                SCE_EVoxelRenderPipeline pipeline)
{
    vt->rpipeline = pipeline;
    SCE_VRender_SetPipeline (&vt->template, pipeline);
}
void SCE_VTerrain_SetAlgorithm (SCE_SVoxelTerrain *vt,
                                SCE_EVoxelRenderAlgorithm algo)
{
    SCE_VRender_SetAlgorithm (&vt->template, algo);
}
void SCE_VTerrain_SetHybrid (SCE_SVoxelTerrain *vt, SCEuint cut)
{
    vt->cut = cut;
}
void SCE_VTerrain_SetHybridMCStep (SCE_SVoxelTerrain *vt, SCEuint step)
{
    vt->hybrid.mc_step = step;
}
void SCE_VTerrain_SetVertexBufferPool (SCE_SVoxelTerrain *vt,
                                       SCE_RBufferPool *pool)
{
    vt->vertex_pool = pool;
    SCE_VRender_SetVertexBufferPool (&vt->template, pool);
}
void SCE_VTerrain_SetIndexBufferPool (SCE_SVoxelTerrain *vt,
                                      SCE_RBufferPool *pool)
{
    vt->index_pool = pool;
    SCE_VRender_SetIndexBufferPool (&vt->template, pool);
}
void SCE_VTerrain_EnableMaterials (SCE_SVoxelTerrain *vt)
{
    vt->use_materials = SCE_TRUE;
}
void SCE_VTerrain_DisableMaterials (SCE_SVoxelTerrain *vt)
{
    vt->use_materials = SCE_FALSE;
}

/**
 * \brief Set the shader that will be used for rendering
 * \param vt a voxel terrain
 * \param shader a shader
 *
 * You must call SCE_VTerrain_BuildShader() upon \p shader before
 * building your terrain with SCE_VTerrain_Build(). \p shader should
 * implement every combination of the voxel terrain shader flags
 * SCE_VTERRAIN_USE_LOD and SCE_VTERRAIN_USE_SHADOWS
 */
void SCE_VTerrain_SetShader (SCE_SVoxelTerrain *vt, SCE_SShader *shader)
{
    vt->pipeline = shader;
}

void SCE_VTerrain_SetTopDiffuseTexture (SCE_SVoxelTerrain *vt, SCE_STexture *tex)
{
    vt->top_diffuse = tex;
}
SCE_STexture* SCE_VTerrain_GetTopDiffuseTexture (SCE_SVoxelTerrain *vt)
{
    return vt->top_diffuse;
}

void SCE_VTerrain_SetSideDiffuseTexture (SCE_SVoxelTerrain *vt, SCE_STexture *tex)
{
    vt->side_diffuse = tex;
}
SCE_STexture* SCE_VTerrain_GetSideDiffuseTexture (SCE_SVoxelTerrain *vt)
{
    return vt->side_diffuse;
}

void SCE_VTerrain_SetNoiseTexture (SCE_SVoxelTerrain *vt, SCE_STexture *tex)
{
    vt->noise = tex;
}
SCE_STexture* SCE_VTerrain_GetNoiseTexture (SCE_SVoxelTerrain *vt)
{
    return vt->noise;
}


static int SCE_VTerrain_BuildLevel (SCE_SVoxelTerrain *vt,
                                    SCE_SVoxelTerrainLevel *tl)
{
    SCE_STexData *tc = NULL;
    SCEuint i, num;
    SCEuint x, y, z;
    SCE_SGeometry *geom = NULL;


    /* setup volume */
    SCE_Grid_SetPointSize (&tl->grid, 1);
    SCE_Grid_SetDimensions (&tl->grid, vt->width, vt->height, vt->depth);
    if (SCE_Grid_Build (&tl->grid) < 0)
        goto fail;

    if (!(tc = SCE_TexData_Create ()))
        goto fail;
    /* SCE_PXF_LUMINANCE: 8 bits density precision */
    /* TODO: luminance format is deprecated */
    SCE_Grid_ToTexture (&tl->grid, tc, SCE_PXF_LUMINANCE, SCE_UNSIGNED_BYTE);

    if (!(tl->tex = SCE_Texture_Create (SCE_TEX_3D, 0, 0, 0)))
        goto fail;
    SCE_Texture_AddTexData (tl->tex, SCE_TEX_3D, tc);
    SCE_Texture_Build (tl->tex, SCE_FALSE);

    if (vt->use_materials) {
        SCE_Grid_SetPointSize (&tl->grid2, 1);
        SCE_Grid_SetDimensions (&tl->grid2, vt->width, vt->height, vt->depth);
        if (SCE_Grid_Build (&tl->grid2) < 0)
            goto fail;

        if (!(tc = SCE_TexData_Create ()))
            goto fail;
        /* SCE_PXF_LUMINANCE: 8 bits density precision */
        /* TODO: luminance format is deprecated */
        SCE_Grid_ToTexture (&tl->grid2, tc, SCE_PXF_LUMINANCE,
                            SCE_UNSIGNED_BYTE);

        if (!(tl->mat = SCE_Texture_Create (SCE_TEX_3D, 0, 0, 0)))
            goto fail;
        SCE_Texture_SetUnit (tl->mat, 5);
        SCE_Texture_AddTexData (tl->mat, SCE_TEX_3D, tc);
        SCE_Texture_Build (tl->mat, SCE_FALSE);
    }

    /* setup regions */
    tl->subregions = vt->n_subregions;
    num = tl->subregions * tl->subregions * tl->subregions;
    if (!(tl->regions = SCE_malloc (num * sizeof *tl->regions))) goto fail;
    if (!(tl->mesh = SCE_malloc (num * sizeof *tl->mesh))) goto fail;

    for (i = 0; i < num; i++) {
        SCE_VTerrain_InitRegion (&tl->regions[i]);
        tl->regions[i].level = tl;
        SCE_Mesh_Init (&tl->mesh[i]);

        geom = SCE_VRender_GetFinalGeometry (&vt->template);
        if (SCE_Mesh_SetGeometry (&tl->mesh[i], geom, SCE_FALSE) < 0)
            goto fail;
        SCE_Mesh_AutoBuild (&tl->mesh[i]);

        SCE_VRender_SetMesh (&tl->regions[i].vm, &tl->mesh[i]);
        tl->regions[i].mesh = &tl->mesh[i];
    }

    /* NOTE: must be done only once; any subsequent call to BuildLevel()
             on the same level would screw everything up.
             shortly: wrapping must be null */
    for (x = 0; x < tl->subregions; x++) {
        for (y = 0; y < tl->subregions; y++) {
            for (z = 0; z < tl->subregions; z++) {
                SCE_SVoxelTerrainRegion *r = SCE_VTerrain_GetRegion (tl,x,y,z);
                r->x = x; r->y = y; r->z = z;
            }
        }
    }

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


static const char *vs_fun =
    "uniform sampler3D sce_hightex;" /* high LOD */
    "uniform sampler3D sce_lowtex;" /* low LOD */

    /* in pixels, origin of regions in the low LOD 3D texture*/
    "uniform vec3 sce_regions_origin;"
    /* region_origin = lod0.xyz + lod0.map_xyz - 2 * lod1.map_xyz */

    /* in pixels/dimension, origin of the current region in the high LOD 3D texture
       relative to the beginning of the texture */
    "uniform vec3 sce_tc_origin;"
    /* sce_tc_origin = lod0.xyz / dimension */

    /* in pixels/dimension, origin of the current region relative to regions_origin */
    "uniform vec3 sce_current_origin;"
    /* current_origin = region_pos * (subregion_dim - 1) / dimension */

    /* texture wrapping offsets */
    "uniform vec3 sce_wrapping0;"
    "uniform vec3 sce_wrapping1;"

    "void sce_vterrain_seamlesslod (in out vec3 pos, in out vec3 nor)"
    "{"
       /* assuming ... assuming what? */
    "  vec3 offset = sce_current_origin + pos;"
       /* border position */
    "  const float limit = ((SCE_SUBREGION_DIM-1)*SCE_N_SUBREGIONS + 1)/SCE_W;"
       /* how close we are to the border */
    "  float weight = 0.0;"

       /* this threshold defines vertex selection accuracy;
          bigger values select more vertices, smaller values less vertices */
    "  const float threshold = 0.034;"
    "  weight = max (weight, SCE_W * (offset.x - (limit - threshold)));"
    "  weight = max (weight, SCE_H * (offset.y - (limit - threshold)));"
    "  weight = max (weight, SCE_D * (offset.z - (limit - threshold)));"
    "  weight = max (weight, SCE_W * (threshold - offset.x));"
    "  weight = max (weight, SCE_H * (threshold - offset.y));"
    "  weight = max (weight, SCE_D * (threshold - offset.z));"
    "  weight = clamp (weight * 0.5, 0.0, 1.0);"

       /* one branch to save (most of the time) 8 texture fetches */
    "  if (weight > 0.001) {"
    "    const float ow = 1.0 / SCE_W;"
    "    const float oh = 1.0 / SCE_H;"
    "    const float od = 1.0 / SCE_D;"
    "    int i;"
    "    float diff = 0.0;"

    "    vec3 tc0 = offset + sce_tc_origin + sce_wrapping0 + 0.5 * vec3 (ow, oh, od);"
    "    vec3 texcoord = (sce_regions_origin + vec3 (1.0)) * vec3 (ow, oh, od);"
    "    texcoord = 0.5 * (texcoord + offset);"
    "    vec3 tc1 = texcoord + sce_wrapping1;"
    "    diff = texture (sce_lowtex, tc1).x - texture (sce_hightex, tc0).x;"

         /* move the vertex along normal vector */
    "    vec3 foo = 4.0 * weight * diff * nor * vec3 (ow, oh, od);"
    "    offset += foo;"
    "    pos += foo;"

         /* compute low resolution normal */
    "    vec3 grad;"
    "    grad.x = texture (sce_lowtex, tc1 + vec3 (ow, 0., 0.)).x -"
    "             texture (sce_lowtex, tc1 + vec3 (-ow, 0., 0.)).x;"
    "    grad.y = texture (sce_lowtex, tc1 + vec3 (0., oh, 0.)).x -"
    "             texture (sce_lowtex, tc1 + vec3 (0., -oh, 0.)).x;"
    "    grad.z = texture (sce_lowtex, tc1 + vec3 (0., 0., od)).x -"
    "             texture (sce_lowtex, tc1 + vec3 (0., 0., -od)).x;"
    "    grad = -normalize (grad);"

         /* interpolate low and high resolution normals */
    "    nor = normalize (grad * weight + nor * (1.0 - weight));"
    "  }"
    "}"

    "vec3 sce_vterrain_shadowoffset (vec3 pos, vec3 nor)"
    "{"
    "  const float weight = /* clever maths */ -0.002;"
    "  return pos + weight * nor;"
    "}";

static const char *ps_fun =
    "vec3 sce_triplanar (in vec3 pos, in out vec3 nor,"
                        "out vec2 tc1, out vec2 tc2, out vec2 tc3)"
    "{"
       /* texturing: triplanar projection */
    "  vec3 weight = abs (nor);"

    "  weight = (weight - vec3 (0.2)) * 7;" /* wtf ? */
    "  weight = max (weight, vec3 (0.0));"
    "  weight /= weight.x + weight.y + weight.z;"

    "  tc1 = pos.yz;"
    "  tc2 = pos.zx;"
    "  tc3 = pos.xy;"

//    "  nor = normal_mapping;"
    "  return weight;"
    "}";


static const char *vs_main =
    "\n#define OW (1.0/SCE_W)\n"
    "\n#define OH (1.0/SCE_H)\n"
    "\n#define OD (1.0/SCE_D)\n"

    "in vec3 sce_position;"
    /* TODO: blblbl */
#if 1
    "\n#define SCE_COMPUTE_NORMAL 0\n" /* xD */
#else
    "\n#define SCE_COMPUTE_NORMAL 1\n" /* xD */
#endif
    "\n#if !SCE_COMPUTE_NORMAL\n"
    "in vec3 sce_normal;"
    "\n#endif\n"

    "out vec3 pos;"
    "out vec3 nor;"
    "out vec4 col;"

    "uniform mat4 sce_objectmatrix;"
    "uniform mat4 sce_cameramatrix;"
    "uniform mat4 sce_projectionmatrix;"

    "uniform bool enabled;"

    "vec3 light (vec3 normal, vec3 lightpos, vec3 color)"
    "{"
    "  return color * clamp (dot (normalize (lightpos), normal), 0.0, 1.0);"
    "}"

    "void main (void)"
    "{"
    "  vec3 position = sce_position;"
    "  vec3 normal;"

       /* retrieve normal */
    /* TODO: make SCE_COMPUTE_NORMAL a vterrain shader flag too */
    "\n#if !SCE_COMPUTE_NORMAL\n"
    "  normal = normalize (sce_normal);"
    "\n#else\n"
    "  vec3 grad;"
    "  vec3 offset = sce_current_origin + sce_position;"
    "  vec3 tc = offset + sce_tc_origin + sce_wrapping0;"
    "  grad.x = texture (sce_tex0, tc + vec3 (OW, 0., 0.)).x -"
    "           texture (sce_tex0, tc + vec3 (-OW, 0., 0.)).x;"
    "  grad.y = texture (sce_tex0, tc + vec3 (0., OH, 0.)).x -"
    "           texture (sce_tex0, tc + vec3 (0., -OH, 0.)).x;"
    "  grad.z = texture (sce_tex0, tc + vec3 (0., 0., OD)).x -"
    "           texture (sce_tex0, tc + vec3 (0., 0., -OD)).x;"
    "  normal = -normalize (grad);"
    "\n#endif\n"

    "\n#if "SCE_VTERRAIN_USE_LOD_NAME"\n"
    "  if (enabled)"
    "    sce_vterrain_seamlesslod (position, normal);"
    "\n#endif\n"
    "  nor = normal;"
    "\n#if "SCE_VTERRAIN_USE_SHADOWS_NAME"\n"
    "  position = sce_vterrain_shadowoffset (position, normal);"
    "\n#endif\n"

    "  vec3 amb = vec3 (0.1, 0.1, 0.1);"
    "  vec3 lighting = amb;"
    "  lighting += 0.3 * light (nor, vec3 (0.0, 0.0, 2.0), vec3 (0.6, 0.7, 1.0));"
    "  lighting += 0.7 * light (nor, vec3 (1.0, 1.0, 2.0), vec3 (1.0, 0.9, 0.8));"
    "  col = vec4 (lighting, 1.0);"

    "  vec4 p = sce_objectmatrix * vec4 (position, 1.0);"
    "  pos = p.xyz;"

    "  mat4 final_matrix = sce_projectionmatrix * sce_cameramatrix;"
    "  gl_Position = final_matrix * p;"
    "}";

static const char *ps_main =
    "\n#if "SCE_VTERRAIN_USE_SHADOWS_NAME"\n"
    "void main (void)"
    "{"
    "  gl_FragColor = vec4 (0.0);" /* ?? */
    "}"
    "\n#else\n"
    "in vec3 pos;"
    "in vec3 nor;"
    "in vec4 col;"

    "uniform sampler2D sce_side_diffuse;"
    "uniform sampler2D sce_top_diffuse;"
    "uniform sampler2D sce_noise_tex;"

    "void main (void)"
    "{"
    "  vec2 tc1, tc2, tc3;"
    "  vec3 weights;"
    "  vec3 normal = nor;"

    "  weights = sce_triplanar (pos, normal, tc1, tc2, tc3);"
    "  weights = pow (weights, vec3 (8.0));"
    "  weights /= dot (weights, vec3 (1.0));"
    "  const float scale = 5.0;"
    "  vec4 col1 = texture (sce_side_diffuse, tc1 * scale);"
    "  vec4 col2 = texture (sce_side_diffuse, tc2 * scale);"
    "  vec4 col3 = texture (sce_top_diffuse, tc3 * scale);"

    "  vec4 color = weights.x * col1 + weights.y * col2 + weights.z * col3;"
    "  gl_FragColor = col * color;"
    "}"
    "\n#endif\n";


int SCE_VTerrain_BuildShader (SCE_SVoxelTerrain *vt, SCE_SShader *shd)
{
    (void)vt;
    /* add functions code */
    if (SCE_Shader_AddSource (shd, SCE_VERTEX_SHADER, vs_fun, SCE_TRUE) < 0)
        goto fail;
    if (SCE_Shader_AddSource (shd, SCE_PIXEL_SHADER, ps_fun, SCE_TRUE) < 0)
        goto fail;
    if (SCE_Shader_Globalf (shd, "SCE_W", vt->width) < 0) goto fail;
    if (SCE_Shader_Globalf (shd, "SCE_H", vt->height) < 0) goto fail;
    if (SCE_Shader_Globalf (shd, "SCE_D", vt->depth) < 0) goto fail;
    if (SCE_Shader_Globali (shd, "SCE_SUBREGION_DIM",
                            vt->subregion_dim) < 0) goto fail;
    if (SCE_Shader_Globali (shd, "SCE_N_SUBREGIONS",
                            vt->n_subregions) < 0) goto fail;
    /* TODO: first, the user should know that this functions forces the shader
       version. second, this version should match vrender's (which is currently
       the case but you know it's kinda hardcoded right now) */
    SCE_Shader_SetVersion (shd, 150);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

static int SCE_VTerrain_BuildHybrid (SCE_SVoxelTerrain *vt)
{
    size_t n, dim, size;
    SCE_SVoxelTerrainHybridGenerator *h = &vt->hybrid;

    h->dim = vt->subregion_dim;
    dim = n = 2 * h->dim;
    SCE_Grid_SetDimensions (&h->grid, n, n, n);
    SCE_Grid_SetPointSize (&h->grid, 1);
    if (SCE_Grid_Build (&h->grid) < 0)
        goto fail;

    n = SCE_Grid_GetNumPoints (&h->grid);
    SCE_MC_SetNumCells (&h->mc_gen, n);
    if (SCE_MC_Build (&h->mc_gen) < 0)
        goto fail;

    if (!(h->vertices = SCE_malloc (n * 9 * sizeof *h->vertices)))
        goto fail;
    if (!(h->normals = SCE_malloc (n * 9 * sizeof *h->normals)))
        goto fail;
    if (!(h->indices = SCE_malloc (n * 15 * sizeof *h->indices)))
        goto fail;
    if (!(h->anchors = SCE_malloc (dim * dim * 6 * sizeof *h->anchors)))
        goto fail;
    size  = vt->comp_pos ? 4 : 4 * sizeof (SCEvertices);
    size += vt->comp_nor ? 4 : 3 * sizeof (SCEvertices);
    size *= n * 3;
    if (!(h->interleaved = SCE_malloc (size)))
        goto fail;

    SCE_QEMD_SetMaxVertices (&h->qmesh, n * 3);
    SCE_QEMD_SetMaxIndices (&h->qmesh, n * 15);
    if (SCE_QEMD_Build (&h->qmesh) < 0)
        goto fail;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

int SCE_VTerrain_Build (SCE_SVoxelTerrain *vt)
{
    size_t i;
    SCEuint state;

    if (vt->built)
        return SCE_OK;

    SCE_VRender_SetDimensions (&vt->template, vt->subregion_dim,
                               vt->subregion_dim, vt->subregion_dim);
    SCE_VRender_SetVolumeDimensions (&vt->template, vt->width,
                                     vt->height, vt->depth);
    SCE_VRender_SetCompressedScale (&vt->template, vt->n_subregions);

    if (SCE_VRender_Build (&vt->template) < 0)
        goto fail;

    if (vt->cut) {
        if (SCE_VTerrain_BuildHybrid (vt) < 0)
            goto fail;
    }

    for (i = 0; i < vt->n_levels; i++) {
        if (SCE_VTerrain_BuildLevel (vt, &vt->levels[i]) < 0)
            goto fail;
    }

    if (!vt->pipeline) {
        /* user hasn't specified any: use default main code */
        if (!(vt->pipeline = SCE_Shader_Create ())) goto fail;
        if (SCE_Shader_AddSource (vt->pipeline, SCE_VERTEX_SHADER,
                                  vs_main, SCE_FALSE) < 0) goto fail;
        if (SCE_Shader_AddSource (vt->pipeline, SCE_PIXEL_SHADER,
                                  ps_main, SCE_FALSE) < 0) goto fail;
        /* completely ridiculous: */
        if (SCE_VTerrain_BuildShader (vt, vt->pipeline) < 0) goto fail;
    }

    {
        const char *states[3] = {
            /* order matters */
            SCE_VTERRAIN_USE_LOD_NAME,
            SCE_VTERRAIN_USE_SHADOWS_NAME,
            SCE_VTERRAIN_USE_POINT_SHADOWS_NAME
        };
        SCE_SRenderState rs;

        SCE_RenderState_Init (&rs);
        if (SCE_RenderState_SetStates (&rs, states, 3) < 0)
            goto fail;
        if (SCE_Shader_SetupPipeline (vt->pipeline, &rs) < 0) goto fail;
        SCE_RenderState_Clear (&rs);
    }

    state = SCE_VTERRAIN_USE_SHADOWS;
    SCE_Shader_DisablePipelineShader (vt->pipeline, state, SCE_PIXEL_SHADER);
    state |= SCE_VTERRAIN_USE_LOD;
    SCE_Shader_DisablePipelineShader (vt->pipeline, state, SCE_PIXEL_SHADER);

    /* build shaders */
    SCE_Shader_SetVersion (vt->pipeline, 150);
    if (SCE_Shader_Build (vt->pipeline) < 0) goto fail;

    /* get uniform locations and setup mapping */
    for (i = 0; i < SCE_NUM_VTERRAIN_SHADERS; i++) {
        SCE_SVoxelTerrainShader *shd = &vt->shaders[i];

        shd->shd = SCE_Shader_GetShader (vt->pipeline, i);
        SCE_Shader_SetupAttributesMapping (shd->shd);
        SCE_Shader_ActivateAttributesMapping (shd->shd, SCE_TRUE);
        SCE_Shader_SetupMatricesMapping (shd->shd);
        SCE_Shader_ActivateMatricesMapping (shd->shd, SCE_TRUE);

#define SCE_LOC(el, name)                               \
        shd->el = SCE_Shader_GetIndex (shd->shd, name)
        SCE_LOC (regions_loc, "sce_regions_origin");
        SCE_LOC (current_loc, "sce_current_origin");
        SCE_LOC (wrapping0_loc, "sce_wrapping0");
        SCE_LOC (wrapping1_loc, "sce_wrapping1");
        SCE_LOC (tcorigin_loc, "sce_tc_origin");
        SCE_LOC (hightex_loc, "sce_hightex");
        SCE_LOC (lowtex_loc, "sce_lowtex");
        SCE_LOC (enabled_loc, "enabled");
        SCE_LOC (topdiffuse_loc, "sce_top_diffuse");
        SCE_LOC (sidediffuse_loc, "sce_side_diffuse");
        SCE_LOC (noise_loc, "sce_noise_tex");
        SCE_LOC (material_loc, "sce_material_tex");
#undef SCE_LOC

    }

    vt->built = SCE_TRUE;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


void SCE_VTerrain_SetPosition (SCE_SVoxelTerrain *vt, long x, long y, long z)
{
    vt->x = x;
    vt->y = y;
    vt->z = z;
}

void SCE_VTerrain_GetMissingSlices (const SCE_SVoxelTerrain *vt, SCEuint level,
                                    long *x, long *y, long *z)
{
    long center[3];
    const SCE_SVoxelTerrainLevel *tl = &vt->levels[level];
    long herp = 1 << level;

    /* compute center in terms of the origin of the grid in the map */
    center[0] = (tl->map_x + vt->width / 2) * herp;
    center[1] = (tl->map_y + vt->height / 2) * herp;
    center[2] = (tl->map_z + vt->depth / 2) * herp;

    /* compute difference */
    *x = (vt->x - center[0]) / herp;
    *y = (vt->y - center[1]) / herp;
    *z = (vt->z - center[2]) / herp;
}
int SCE_VTerrain_GetMissingRegion (SCE_SVoxelTerrain *vt, SCE_SLongRect3 *r)
{
    /* dont ask for another region if there's already data waiting to
       be processed */
    if (vt->hybrid.query) {
        long dim;
        dim = 2 * vt->hybrid.dim;
        SCE_Rectangle3_SetFromOriginl (r, vt->hybrid.x, vt->hybrid.y,
                                       vt->hybrid.z, dim, dim, dim);
        vt->hybrid.query = SCE_FALSE;
        return vt->hybrid.level;
    }
    return -1;
}

/**
 * \brief Gets the rectangle around the theoretical position of the viewer
 * \param vt voxel terrain
 * \param level level of detail
 * \param r resulting rectangle
 *
 * ... that is, not based on map_xyz.
 */
void SCE_VTerrain_GetRectangle (const SCE_SVoxelTerrain *vt, SCEuint level,
                                SCE_SLongRect3 *r)
{
    long x, y, z;
    x = vt->x << level;
    y = vt->y << level;
    z = vt->z << level;
    SCE_Rectangle3_SetFromCenterl (r, x,y,z, vt->width, vt->height, vt->depth);
}

/* TODO: what's this function for? */
void SCE_VTerrain_SetOrigin (SCE_SVoxelTerrain *vt, SCEuint level,
                             long x, long y, long z)
{
    SCE_SVoxelTerrainLevel *tl = &vt->levels[level];
    tl->map_x = x; tl->map_y = y; tl->map_z = z;
}
void SCE_VTerrain_GetOrigin (const SCE_SVoxelTerrain *vt, SCEuint level,
                             long *x, long *y, long *z)
{
    const SCE_SVoxelTerrainLevel *tl = &vt->levels[level];
    *x = tl->map_x; *y = tl->map_y; *z = tl->map_z;
}


void SCE_VTerrain_SetLevel (SCE_SVoxelTerrain *vt, SCEuint level,
                            const SCE_SGrid *grid)
{
    int w, h, d;

    w = SCE_Grid_GetWidth (grid);
    h = SCE_Grid_GetHeight (grid);
    d = SCE_Grid_GetDepth (grid);
    if (w != vt->width || h != vt->height || d != vt->depth) {
        /* wrong dimensions */
        SCEE_SendMsg ("you're trying to set a level with a grid with "
                      "wrong dimensions\n");
        return;
    }

    SCE_Grid_CopyData (&vt->levels[level].grid, grid);
    /* TODO: UpdateGrid() */
}

SCE_SGrid* SCE_VTerrain_GetLevelGrid (SCE_SVoxelTerrain *vt, SCEuint level)
{
    return &vt->levels[level].grid;
}
SCE_SGrid* SCE_VTerrain_GetLevelMaterialGrid (SCE_SVoxelTerrain *vt, SCEuint level)
{
    return &vt->levels[level].grid2;
}

void SCE_VTerrain_ActivateLevel (SCE_SVoxelTerrain *vt, SCEuint level,
                                 int activate)
{
    vt->levels[level].enabled = activate;
}


static void
SCE_VTerrain_AppendDensitySlice (SCE_SVoxelTerrain *vt, SCEuint level,
                                 SCE_EBoxFace f, const unsigned char *slice)
{
    SCE_SVoxelTerrainLevel *tl = NULL;
    int w, h, d, dim;
    SCE_SIntRect3 r;

    tl = &vt->levels[level];
    SCE_Grid_UpdateFace (&tl->grid, f, slice);

    /* TODO: direct access to structure attributes */
    tl->wrap[0] = tl->grid.wrap_x;
    tl->wrap[1] = tl->grid.wrap_y;
    tl->wrap[2] = tl->grid.wrap_z;

    w = SCE_Grid_GetWidth (&tl->grid);
    h = SCE_Grid_GetHeight (&tl->grid);
    d = SCE_Grid_GetDepth (&tl->grid);

#if 0
    /* TODO: take grid wrapping into account... it will be crappy */
    texdata = SCE_RGetTextureTexData (SCE_Texture_GetCTexture (tl->tex), 0, 0);
    switch (f) {
    case SCE_BOX_POSX:
        SCE_TexData_Modified3 (texdata, 0, 0, 0, 1, h, d);
        break;
    case SCE_BOX_NEGX:
        SCE_TexData_Modified3 (texdata, w - 1, 0, 0, 1, h, d);
        break;
    case SCE_BOX_POSY:
        SCE_TexData_Modified3 (texdata, 0, 0, 0, w, 1, d);
        break;
    case SCE_BOX_NEGY:
        SCE_TexData_Modified3 (texdata, 0, h - 1, 0, w, 1, d);
        break;
    case SCE_BOX_POSZ:
        SCE_TexData_Modified3 (texdata, 0, 0, 0, w, h, 1);
        break;
    case SCE_BOX_NEGZ:
        SCE_TexData_Modified3 (texdata, 0, 0, d - 1, w, h, 1);
        break;
    }
#endif

    dim = (vt->subregion_dim - 1) * vt->n_subregions + 1;

#define HERP 2
    switch (f) {
    case SCE_BOX_POSX:
        tl->map_x++;
        tl->x--;
        /* TODO: replace with a while() */
        if (tl->x < 1) {
            tl->x += vt->subregion_dim - 1;
            tl->wrap_x++;
            SCE_Rectangle3_Set (&r, tl->x + dim - HERP, 0, 0, w, h, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE, SCE_FALSE);
        }
        break;
    case SCE_BOX_NEGX:
        tl->map_x--;
        tl->x++;
        if (tl->x + dim >= vt->width) {
            tl->x -= vt->subregion_dim - 1;
            tl->wrap_x--;
            SCE_Rectangle3_Set (&r, 0, 0, 0, vt->subregion_dim - 2, h, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE, SCE_FALSE);
        }
        break;
    case SCE_BOX_POSY:
        tl->map_y++;
        tl->y--;
        if (tl->y < 1) {
            tl->y += vt->subregion_dim - 1;
            tl->wrap_y++;
            SCE_Rectangle3_Set (&r, 0, tl->y + dim - HERP, 0, w, h, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE, SCE_FALSE);
        }
        break;
    case SCE_BOX_NEGY:
        tl->map_y--;
        tl->y++;
        if (tl->y + dim >= vt->height) {
            tl->y -= vt->subregion_dim - 1;
            tl->wrap_y--;
            SCE_Rectangle3_Set (&r, 0, 0, 0, w, vt->subregion_dim - 2, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE, SCE_FALSE);
        }
        break;
    case SCE_BOX_POSZ:
        tl->map_z++;
        tl->z--;
        if (tl->z < 1) {
            tl->z += vt->subregion_dim - 1;
            tl->wrap_z++;
            SCE_Rectangle3_Set (&r, 0, 0, tl->z + dim - HERP, w, h, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE, SCE_FALSE);
        }
        break;
    case SCE_BOX_NEGZ:
        tl->map_z--;
        tl->z++;
        if (tl->z + dim >= vt->depth) {
            tl->z -= vt->subregion_dim - 1;
            tl->wrap_z--;
            SCE_Rectangle3_Set (&r, 0, 0, 0, w, h, vt->subregion_dim - 2);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE, SCE_FALSE);
        }
        break;
    }

    tl->need_update = SCE_TRUE;
}

static void
SCE_VTerrain_AppendMaterialSlice (SCE_SVoxelTerrain *vt, SCEuint level,
                                  SCE_EBoxFace f,const unsigned char *slice)
{
    SCE_SVoxelTerrainLevel *tl = NULL;

    tl = &vt->levels[level];
    SCE_Grid_UpdateFace (&tl->grid2, f, slice);
}

void SCE_VTerrain_AppendSlice (SCE_SVoxelTerrain *vt, SCEuint level,
                               SCE_EBoxFace f, const unsigned char *slice,
                               int mat)
{
    if (mat)
        SCE_VTerrain_AppendMaterialSlice (vt, level, f, slice);
    else
        SCE_VTerrain_AppendDensitySlice (vt, level, f, slice);
}

/* TODO: add a parameter to specify whether data is full or empty */
void SCE_VTerrain_SetRegion (SCE_SVoxelTerrain *vt, const unsigned char *data)
{
    memcpy (SCE_Grid_GetRaw (&vt->hybrid.grid), data,
            SCE_Grid_GetSize (&vt->hybrid.grid));
    vt->hybrid.grid_ready = SCE_TRUE;
    vt->hybrid.query = SCE_FALSE;
}


static void
SCE_VTerrain_CullLevelRegions1 (SCE_SVoxelTerrain *vt, SCEuint level,
                                SCE_SVoxelTerrainLevel *tl,
                                const SCE_SFrustum *frustum)
{
    SCE_TVector3 pos, origin;
    float invw, invh, invd;
    float scale;
    int x, y, z;
    SCE_SBoundingBox box;
    SCE_SBox b;

    SCE_List_Flush (&tl->to_render);

    scale = 1 << level;
    scale *= vt->scale;

    invw = 1.0 / vt->width;
    invh = 1.0 / vt->height;
    invd = 1.0 / vt->depth;

    SCE_Vector3_Set (origin,
                     (tl->map_x + tl->x) * invw,
                     (tl->map_y + tl->y) * invh,
                     (tl->map_z + tl->z) * invd);

    /* construct bounding box */
    SCE_Box_Init (&b);
    SCE_Vector3_Set (pos, 0.0, 0.0, 0.0);
    SCE_Box_Set (&b, pos, vt->subregion_dim * invw,
                 vt->subregion_dim * invh,
                 vt->subregion_dim * invd);
    SCE_BoundingBox_Init (&box);
    SCE_BoundingBox_SetFrom (&box, &b);

    for (z = 0; z < tl->subregions; z++) {
        for (y = 0; y < tl->subregions; y++) {
            for (x = 0; x < tl->subregions; x++) {
                unsigned int offset = SCE_VTerrain_Get (tl, x, y, z);
                SCE_SVoxelTerrainRegion *region = &tl->regions[offset];

                region->wx = x;
                region->wy = y;
                region->wz = z;

                if (region->draw) {
                    int draw;
#define DERP 1
                    SCE_Vector3_Set
                        (pos,
                         (float)x * (vt->subregion_dim - DERP) * invw,
                         (float)y * (vt->subregion_dim - DERP) * invh,
                         (float)z * (vt->subregion_dim - DERP) * invd);

                    SCE_Vector3_Operator1v (pos, +=, origin);
                    SCE_Matrix4_Identity (region->matrix);
                    SCE_Matrix4_SetScale (region->matrix, scale, scale, scale);
                    SCE_Matrix4_MulTranslatev (region->matrix, pos);

                    if (!frustum) {
                        SCE_List_Appendl (&tl->to_render, &region->it2);
                    } else {
                        /* frustum culling */
                        SCE_BoundingBox_Push (&box, region->matrix, &b);
                        /* cull test */
                        draw = SCE_Frustum_BoundingBoxInBool (frustum, &box);
                        SCE_BoundingBox_Pop (&box, &b);

                        if (draw)
                            SCE_List_Appendl (&tl->to_render, &region->it2);
                    }
                }
            }
        }
    }

}

static void
SCE_VTerrain_CullLevelRegions2 (SCE_SVoxelTerrain *vt, SCEuint level,
                                SCE_SVoxelTerrainLevel *tl,
                                SCE_SVoxelTerrainLevel *tl2,
                                const SCE_SFrustum *frustum)
{
    int draw = SCE_TRUE, p1[3], p2[3];
    SCE_SIntRect3 inner_lod, zone;
    SCE_TVector3 pos, origin;
    float invw, invh, invd;
    float scale;
    int x, y, z;
    SCE_SBoundingBox box;
    SCE_SBox b;

    SCE_List_Flush (&tl->to_render);

    scale = 1 << level;
    scale *= vt->scale;

    invw = 1.0 / vt->width;
    invh = 1.0 / vt->height;
    invd = 1.0 / vt->depth;

    SCE_Vector3_Set (origin,
                     (tl->map_x + tl->x) * invw,
                     (tl->map_y + tl->y) * invh,
                     (tl->map_z + tl->z) * invd);

    /* construct bounding box */
    SCE_Box_Init (&b);
    SCE_Vector3_Set (pos, 0.0, 0.0, 0.0);
    SCE_Box_Set (&b, pos, vt->subregion_dim * invw,
                 vt->subregion_dim * invh,
                 vt->subregion_dim * invd);
    SCE_BoundingBox_Init (&box);
    SCE_BoundingBox_SetFrom (&box, &b);

    for (z = 0; z < tl->subregions; z++) {
        for (y = 0; y < tl->subregions; y++) {
            for (x = 0; x < tl->subregions; x++) {
                unsigned int offset = SCE_VTerrain_Get (tl, x, y, z);
                SCE_SVoxelTerrainRegion *region = &tl->regions[offset];

                region->wx = x;
                region->wy = y;
                region->wz = z;

                /* compute current region area */
                p1[0] = 2 * (tl->map_x + tl->x + x * (vt->subregion_dim-1));
                p1[1] = 2 * (tl->map_y + tl->y + y * (vt->subregion_dim-1));
                p1[2] = 2 * (tl->map_z + tl->z + z * (vt->subregion_dim-1));
                p2[0] = p1[0] + 2 * vt->subregion_dim;
                p2[1] = p1[1] + 2 * vt->subregion_dim;
                p2[2] = p1[2] + 2 * vt->subregion_dim;
                SCE_Rectangle3_Setv (&zone, p1, p2);

                /* compute inner LOD grid area */
                p1[0] = tl2->map_x + tl2->x + 1;
                p1[1] = tl2->map_y + tl2->y + 1;
                p1[2] = tl2->map_z + tl2->z + 1;
                p2[0] = p1[0] + vt->n_subregions * (vt->subregion_dim-1);
                p2[1] = p1[1] + vt->n_subregions * (vt->subregion_dim-1);
                p2[2] = p1[2] + vt->n_subregions * (vt->subregion_dim-1);
                SCE_Rectangle3_Setv (&inner_lod, p1, p2);

                draw = !SCE_Rectangle3_IsInside (&inner_lod, &zone);

                if (draw && region->draw) {
                    SCE_Vector3_Set
                        (pos,
                         (float)x * (vt->subregion_dim - DERP) * invw,
                         (float)y * (vt->subregion_dim - DERP) * invh,
                         (float)z * (vt->subregion_dim - DERP) * invd);

                    SCE_Vector3_Operator1v (pos, +=, origin);
                    SCE_Matrix4_Identity (region->matrix);
                    SCE_Matrix4_SetScale (region->matrix, scale, scale, scale);
                    SCE_Matrix4_MulTranslatev (region->matrix, pos);

                    if (!frustum) {
                        SCE_List_Appendl (&tl->to_render, &region->it2);
                    } else {
                        /* frustum culling */
                        SCE_BoundingBox_Push (&box, region->matrix, &b);
                        /* cull test */
                        draw = SCE_Frustum_BoundingBoxInBool (frustum, &box);
                        SCE_BoundingBox_Pop (&box, &b);

                        if (draw)
                            SCE_List_Appendl (&tl->to_render, &region->it2);
                    }
                }
            }
        }
    }
}


void SCE_VTerrain_CullRegions (SCE_SVoxelTerrain *vt,
                               const SCE_SFrustum *frustum)
{
    size_t i;

    SCE_VTerrain_CullLevelRegions1 (vt, 0, &vt->levels[0], frustum);
    for (i = 1; i < vt->n_levels; i++)
        SCE_VTerrain_CullLevelRegions2 (vt, i, &vt->levels[i],
                                        &vt->levels[i - 1], frustum);
}

/* get borders and multiplies every vertex */
static SCEuint SCE_VTerrain_Derp (float coef, SCEvertices *vertices,
                                  SCEuint n_vertices, float inf, float sup,
                                  SCEindices *out)
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
        SCE_Vector3_Operator1 (v, *=, coef);
    }

    return j;
}

static void SCE_VTerrain_EncodePos (SCEvertices *v, size_t n, size_t stride,
                                    SCEubyte *out)
{
    size_t i;
    SCEvertices *u;

    for (i = 0; i < n; i++) {
        memcpy (out, &v[i * 3], 3 * sizeof *v);
        u = (SCEvertices*)out;
        u[3] = 1.0;
        out = &out[stride];
    }
}
static void SCE_VTerrain_EncodeNor (SCEvertices *v, size_t n, size_t stride,
                                    SCEubyte *out)
{
    size_t i;

    for (i = 0; i < n; i++) {
        memcpy (out, &v[i * 3], 3 * sizeof *v);
        out = &out[stride];
    }
}
static void SCE_VTerrain_EncodeCompressPos (float scale, SCEvertices *v, size_t n,
                                            size_t stride, SCEubyte *out)
{
    size_t i;

    for (i = 0; i < n; i++) {
        /* TODO: HAHAHA we have to consider endianess of the GPU,
           quite funny. */
        out[3] = (SCEubyte)(scale * v[i * 3]     * 256.0);
        out[2] = (SCEubyte)(scale * v[i * 3 + 1] * 256.0);
        out[1] = (SCEubyte)(scale * v[i * 3 + 2] * 256.0);
        out = &out[stride];
    }
}
static void SCE_VTerrain_EncodeCompressNor (SCEvertices *v, size_t n,
                                            size_t stride, SCEubyte *out)
{
    size_t i;

    for (i = 0; i < n; i++) {
        out[3] = (SCEubyte)((1.0 + v[i * 3])     * 127.0);
        out[2] = (SCEubyte)((1.0 + v[i * 3 + 1]) * 127.0);
        out[1] = (SCEubyte)((1.0 + v[i * 3 + 2]) * 127.0);
        out = &out[stride];
    }
}

static int SCE_VTerrain_ReallocMesh (SCE_SMesh *mesh, SCE_RBufferPool *v,
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

static int SCE_VTerrain_UpdateHybrid (SCE_SVoxelTerrain *vt)
{
    SCE_SVoxelTerrainHybridGenerator *h = &vt->hybrid;
    SCE_SVoxelTerrainLevel *l = NULL;
    SCE_SVoxelTerrainRegion *tr = NULL;
    long x, y, z;

    if (!SCE_List_HasElements (&h->queue) || h->query)
        return SCE_OK;

    tr = SCE_List_GetData (SCE_List_GetFirst (&h->queue));
    l = tr->level;

    x = SCE_Math_Ring (tr->x - l->wrap_x, l->subregions);
    y = SCE_Math_Ring (tr->y - l->wrap_y, l->subregions);
    z = SCE_Math_Ring (tr->z - l->wrap_z, l->subregions);

    x = l->x + x * (vt->subregion_dim - 1);
    y = l->y + y * (vt->subregion_dim - 1);
    z = l->z + z * (vt->subregion_dim - 1);

    if (!h->grid_ready) {
        /* query the corresponding region */
        /* *2 because we want the coordinates in l->level - 1 */
        h->x = 2 * (x + l->map_x);
        h->y = 2 * (y + l->map_y);
        h->z = 2 * (z + l->map_z);
        h->level = l->level - 1;
        h->query = SCE_TRUE;

    } else {
        /* generate the geometry */
        SCE_SIntRect3 r;
        int dim;
        size_t size, stride;
        float factor, sup, inf;
        SCEuint n_collapses;
        SCEubyte *ptr = NULL;

        factor = ((float)vt->subregion_dim) / (float)vt->width;

        /* TODO: we can wrap around grid with those coordinates and generate
                 never-to-be-used vertices */
        /* -1 would solve this problem AND prevent regions from crossing each
           other, but normal generation would be screwed up, maybe use the
           grid for normal generation? faster, slower? */
        dim = 2 * h->dim /* - 1 */;
        SCE_Rectangle3_Set (&r, 0, 0, 0, dim, dim, dim);
        h->n_vertices = SCE_MC_GenerateVerticesRange (&h->mc_gen, &r, &h->grid,
                                                      h->vertices, h->mc_step);
        if (!SCE_MC_IsGenerationFinished (&h->mc_gen))
            return SCE_OK;
        if (!h->n_vertices) {
            SCE_List_RemoveFirst (&h->queue);
            SCE_Mesh_SetNumVertices (tr->mesh, 0);
            SCE_Mesh_SetNumIndices (tr->mesh, 0);
            if (SCE_VTerrain_ReallocMesh (tr->mesh, vt->vertex_pool,
                                          vt->index_pool) < 0)
                goto fail;
            tr->draw = SCE_FALSE;
            h->grid_ready = SCE_FALSE;
            return SCE_OK;
        }
        h->n_indices = SCE_MC_GenerateIndices (&h->mc_gen, h->indices);

        /* simplification */
        sup = (1.0 - 2.0 / (dim - 1.0));
        inf = 1.0001 / (dim - 1.0);
        h->n_anchors = SCE_VTerrain_Derp (factor, h->vertices, h->n_vertices,
                                          inf, sup, h->anchors);
        SCE_QEMD_Set (&h->qmesh, h->vertices, NULL, NULL, NULL, h->indices,
                      h->n_vertices, h->n_indices);
        SCE_QEMD_AnchorVertices (&h->qmesh, h->anchors, h->n_anchors);
        /* aiming at 80% vertex reduction */
        n_collapses = (h->n_vertices - h->n_anchors) * 0.8;
        SCE_QEMD_Process (&h->qmesh, n_collapses);
        SCE_QEMD_Get (&h->qmesh, h->vertices, NULL, NULL, h->indices,
                      &h->n_vertices, &h->n_indices);

        /* generate normals */
        SCE_Geometry_ComputeNormals (h->vertices, h->indices, h->n_vertices,
                                     h->n_indices, h->normals);

        /* encode (and compress if needed) */
        /* compute stride */
        /* TODO: these numbers depend on those setup by the voxel renderer */
        stride = size = vt->comp_pos ? 4 : (4 * sizeof (SCEvertices));
        stride +=       vt->comp_nor ? 4 : (3 * sizeof (SCEvertices));
        ptr = h->interleaved;
        /* n_subregions is the same we give VRender_SetCompressedScale() */
        factor = (float)vt->n_subregions;
        if (vt->comp_pos)
            SCE_VTerrain_EncodeCompressPos (factor, h->vertices,
                                            h->n_vertices, stride, ptr);
        else
            SCE_VTerrain_EncodePos (h->vertices, h->n_vertices, stride, ptr);
        ptr = &ptr[size];
        if (vt->comp_nor)
            SCE_VTerrain_EncodeCompressNor (h->normals, h->n_vertices, stride,
                                            ptr);
        else
            SCE_VTerrain_EncodeNor (h->normals, h->n_vertices, stride, ptr);

        /* TODO: we can only hope that the mesh has been simplified enough to
           fit into the buffers (which size have been allocated for smaller
           chunks, GPU chunks). thankfully this issue will be fixed with
           the incoming variable length vertex buffer feature */

        /* upload geometry */
        SCE_Mesh_SetNumVertices (tr->mesh, h->n_vertices);
        SCE_Mesh_SetNumIndices (tr->mesh, h->n_indices);
        /* indices are now SCE_INDICES_TYPE: pretty ugly, because it forces
           us to put the type back to SCE_UNSIGNED_INT when generating the
           mesh with the GPU */
        /* change it before reallocating the mesh */
        {
            SCE_RIndexBuffer *ib = SCE_Mesh_GetIndexBuffer (tr->mesh);
            /* TODO: direct member access */
            ib->ia.type = SCE_INDICES_TYPE;
        }
        if (SCE_VTerrain_ReallocMesh (tr->mesh, vt->vertex_pool,
                                      vt->index_pool) < 0)
            goto fail;
        size = stride * h->n_vertices;
        SCE_Mesh_UploadVertices (tr->mesh, SCE_MESH_STREAM_G, h->interleaved,
                                 0, size);
        size = h->n_indices * sizeof *h->indices;
        SCE_Mesh_UploadIndices (tr->mesh, h->indices, size);

        /* done */
        tr->draw = (h->n_vertices>0 && h->n_indices>0) ? SCE_TRUE : SCE_FALSE;
        SCE_List_RemoveFirst (&h->queue);
        h->grid_ready = SCE_FALSE;
    }

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

int SCE_VTerrain_Update (SCE_SVoxelTerrain *vt)
{
    size_t i;
    SCE_SListIterator *it = NULL, *pro = NULL;
    unsigned int x, y, z;
    SCE_SVoxelTerrainLevel *fresh = NULL;
    SCE_SList *list = NULL;

    fresh = vt->update_level;
    /* whether there is no level being updated or if the level being updated
       will be completed at the end of this function */
    if (!vt->update_level ||
        SCE_List_GetLength (vt->update_level->updating) <= vt->max_updates) {
        if (SCE_List_HasElements (&vt->to_update)) {
            SCE_SList *derp = NULL;
            it = SCE_List_GetFirst (&vt->to_update);
            fresh = SCE_List_GetData (it);
            SCE_List_Removel (it);
            SCE_Texture_Update (fresh->tex);
            if (fresh->mat)
                SCE_Texture_Update (fresh->mat);
            derp = fresh->updating;
            fresh->updating = fresh->queue;
            fresh->queue = derp;
            fresh->need_update = SCE_FALSE;
        }
    }

    /* dequeue some regions for update */
    i = 0;
    if (vt->update_level)
        list = vt->update_level->updating;
    if (list) SCE_List_ForEachProtected (pro, it, list) {
        SCE_SVoxelTerrainRegion *tr = SCE_List_GetData (it);
        SCE_SVoxelTerrainLevel *l = vt->update_level;

        x = SCE_Math_Ring (tr->x - l->wrap_x, l->subregions);
        y = SCE_Math_Ring (tr->y - l->wrap_y, l->subregions);
        z = SCE_Math_Ring (tr->z - l->wrap_z, l->subregions);

        x = l->x + x * (vt->subregion_dim - 1);
        y = l->y + y * (vt->subregion_dim - 1);
        z = l->z + z * (vt->subregion_dim - 1);

        switch (vt->rpipeline) {
        case SCE_VRENDER_SOFTWARE:
            if (SCE_VRender_Software (&vt->template, &tr->level->grid, &tr->vm,
                                      x, y, z) < 0)
                goto fail;
            break;
        case SCE_VRENDER_HARDWARE:
            x += l->wrap[0];
            y += l->wrap[1];
            z += l->wrap[2];

            /* TODO: see UpdateHybrid() */
            /* update before Hardware(), so that the realloc work
               properly */
            {
                SCE_RIndexBuffer *ib = SCE_Mesh_GetIndexBuffer (tr->mesh);
                /* TODO: direct member access */
                ib->ia.type = SCE_UNSIGNED_INT;
            }
            if (SCE_VRender_Hardware (&vt->template, tr->level->tex, NULL,
                                      &tr->vm, x, y, z) < 0)
                goto fail;

            break;
        default:;
        }

        tr->draw = !SCE_VRender_IsEmpty (&tr->vm);
        SCE_VTerrain_RemoveRegion (vt, tr);

        i++;
        if (i >= vt->max_updates)
            break;
    }

    if (vt->update_level && !SCE_List_HasElements (list)) {
        SCE_List_Remove (&vt->update_level->it);
        if (SCE_List_HasElements (vt->update_level->queue))
            SCE_List_Appendl (&vt->to_update, &vt->update_level->it);
    }
    vt->update_level = fresh;

    if (vt->cut) {
        if (SCE_VTerrain_UpdateHybrid (vt) < 0)
            goto fail;
    }

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


void SCE_VTerrain_UpdateGrid (SCE_SVoxelTerrain *vt, SCEuint level, int mat,
                              int draw)
{
    SCE_SIntRect3 r;

    SCE_Rectangle3_Set (&r, 0, 0, 0,
                        vt->width - 1, vt->height - 1, vt->depth - 1);
    SCE_VTerrain_UpdateSubGrid (vt, level, &r, mat, draw);
}

void SCE_VTerrain_UpdateSubGrid (SCE_SVoxelTerrain *vt, SCEuint level,
                                 SCE_SIntRect3 *rect, int mat, int draw)
{
    SCEuint x, y, z;
    SCEuint sx, sy, sz;
    SCEuint w, h, d;
    int p1[3], p2[3];
    int l;
    SCE_SIntRect3 r, grid_area;
    SCE_SVoxelTerrainLevel *tl = &vt->levels[level];

    if (mat) {
        /* TODO: use rect and stuff blbl */
        SCE_Texture_Update (tl->mat);
        return;
    }

    /* get the intersection between the area to update and the grid area */
    SCE_Rectangle3_Set (&grid_area, 0, 0, 0, vt->width, vt->height, vt->depth);
    if (!SCE_Rectangle3_Intersection (&grid_area, rect, &r))
        return;                 /* does not intersect */

    /* update 3D rectangle of updated area for 3D texture update */
    if (!vt->levels[level].need_update) {
        vt->levels[level].need_update = SCE_TRUE;
        vt->levels[level].update_zone = r;
    } else {
        SCE_Rectangle3_Union (&vt->levels[level].update_zone,
                              &r, &vt->levels[level].update_zone);
    }

    SCE_Rectangle3_Move (&r, -tl->x, -tl->y, -tl->z);
    SCE_Rectangle3_GetPointsv (&r, p1, p2);

    l = vt->subregion_dim - 1;
    sx = MAX (p1[0] - 1, 0) / l;
    sy = MAX (p1[1] - 1, 0) / l;
    sz = MAX (p1[2] - 1, 0) / l;
    w = MAX (p2[0], 0) / l;
    h = MAX (p2[1], 0) / l;
    d = MAX (p2[2], 0) / l;
    if (w >= tl->subregions)
        w = tl->subregions - 1;
    if (h >= tl->subregions)
        h = tl->subregions - 1;
    if (d >= tl->subregions)
        d = tl->subregions - 1;

    for (z = sz; z <= d; z++) {
        for (y = sy; y <= h; y++) {
            for (x = sx; x <= w; x++) {
                SCE_SVoxelTerrainRegion *region = NULL;
                region = SCE_VTerrain_GetRegion (tl, x, y, z);
                SCE_VTerrain_AddRegion (vt, region);
                region->draw = draw;
            }
        }
    }
}


/**
 * \brief Activate/deactivate shadow rendering mode
 * \param vt a voxel terrain
 * \param shadow SCE_TRUE to activate, SCE_FALSE to deactivate
 * \sa SCE_VTerrain_SetLodShadowShader(), SCE_VTerrain_SetDefaultShadowShader()
 */
void SCE_VTerrain_ActivateShadowMode (SCE_SVoxelTerrain *vt, int shadow)
{
    vt->shadow_mode = shadow;
}

void SCE_VTerrain_ActivatePointShadowMode (SCE_SVoxelTerrain *vt, int point)
{
    vt->point_shadow = point;
}

static void
SCE_VTerrain_RenderLevel (const SCE_SVoxelTerrain *vt, SCEuint level,
                          const SCE_SVoxelTerrainLevel *tl,
                          const SCE_SVoxelTerrainLevel *tl3,
                          SCE_SVoxelTerrainShader *shd)
{
    SCE_TMatrix4 m;
    SCE_TVector3 pos, origin;
    SCE_SListIterator *it = NULL;
    float invw, invh, invd;
    float scale;
    SCE_TVector3 v, invv;

    scale = 1 << level;
    scale *= vt->scale;

    invw = 1.0 / vt->width;
    invh = 1.0 / vt->height;
    invd = 1.0 / vt->depth;
    SCE_Vector3_Set (invv, invw, invh, invd);

    if (tl3) {
        SCE_Vector3_Set (v, tl->x + tl->map_x - tl3->map_x * 2.0,
                         tl->y + tl->map_y - tl3->map_y * 2.0,
                         tl->z + tl->map_z - tl3->map_z * 2.0);
        SCE_Shader_SetParam3fv (shd->regions_loc, 1, v);
        SCE_Vector3_Operator2v (v, =, tl->wrap, *, invv);
        SCE_Shader_SetParam3fv (shd->wrapping0_loc, 1, v);
        SCE_Vector3_Operator2v (v, =, tl3->wrap, *, invv);
        SCE_Shader_SetParam3fv (shd->wrapping1_loc, 1, v);
    } else {

        if (0/*generate_normals*/) {
            SCE_Vector3_Operator2v (v, =, tl->wrap, *, invv);
            SCE_Shader_SetParam3fv (shd->wrapping0_loc, 1, v);
        }
        /*SCE_Shader_SetParam (def_shd->hightex_loc, 0);*/
    }

    SCE_Vector3_Set (v, tl->x * invw, tl->y * invh, tl->z * invd);
    SCE_Shader_SetParam3fv (shd->tcorigin_loc, 1, v);

    SCE_List_ForEach (it, &tl->to_render) {
        SCE_SVoxelTerrainRegion *region = SCE_List_GetData (it);

        SCE_Vector3_Set
            (pos,
             (float)region->wx * (vt->subregion_dim - DERP) * invw,
             (float)region->wy * (vt->subregion_dim - DERP) * invh,
             (float)region->wz * (vt->subregion_dim - DERP) * invd);

        SCE_Shader_SetParam3fv (shd->current_loc, 1, pos);

        SCE_RLoadMatrix (SCE_MAT_OBJECT, region->matrix);
        SCE_Mesh_Use (region->mesh);
        SCE_Mesh_Render ();
        SCE_Mesh_Unuse ();
    }
}

void SCE_VTerrain_Render (SCE_SVoxelTerrain *vt)
{
    int i;
    SCE_SVoxelTerrainShader *lodshd = NULL, *defshd = NULL;
    SCE_SVoxelTerrainLevel *tl = NULL, *tl3 = NULL;

    SCE_Texture_SetUnit (vt->top_diffuse, 2);
    SCE_Texture_SetUnit (vt->side_diffuse, 3);
    SCE_Texture_SetUnit (vt->noise, 4);

    if (vt->shadow_mode) {
        unsigned int state = 0;

        SCE_Shader_Unlock ();       /* haxxx */

        state |= SCE_VTERRAIN_USE_SHADOWS;
        if (vt->point_shadow)
            state |= SCE_VTERRAIN_USE_POINT_SHADOWS;

        lodshd = &vt->shaders[state | SCE_VTERRAIN_USE_LOD];
        defshd = &vt->shaders[state];

        SCE_Shader_Use (lodshd->shd);

        SCE_Shader_SetParam (lodshd->enabled_loc, vt->trans_enabled);

        SCE_Texture_BeginLot ();

        for (i = 0; i < vt->n_levels - 1; i++) {

            if (vt->cut && i >= vt->cut - 1)
                SCE_Shader_SetParam (lodshd->enabled_loc, SCE_FALSE);

            tl = &vt->levels[i];
            tl3 = &vt->levels[i + 1];

            /* just switch textures unit */
            SCE_Texture_SetUnit (tl3->tex, (i % 2 ? 0 : 1));
            SCE_Texture_Use (tl3->tex);
            SCE_Texture_Use (tl->tex);
            SCE_Texture_EndLot ();

            if (i % 2) {
                SCE_Shader_SetParam (lodshd->hightex_loc, 1);
                SCE_Shader_SetParam (lodshd->lowtex_loc, 0);
            } else {
                SCE_Shader_SetParam (lodshd->hightex_loc, 0);
                SCE_Shader_SetParam (lodshd->lowtex_loc, 1);
            }

            SCE_VTerrain_RenderLevel (vt, i, tl, tl3, lodshd);

            SCE_Texture_BeginLot ();
        }
        SCE_Texture_EndLot ();

        for (i = 0; i < vt->n_levels; i++)
            SCE_Texture_SetUnit (vt->levels[i].tex, 0);

        tl = &vt->levels[vt->n_levels - 1];

        SCE_Shader_Use (defshd->shd);

        SCE_VTerrain_RenderLevel (vt, vt->n_levels - 1, tl, NULL, defshd);

        SCE_Shader_Use (NULL);
        SCE_Texture_Flush ();
        SCE_Shader_Lock ();
    } else {
        SCE_SVoxelTerrainLevel *tl3 = NULL;

        lodshd = &vt->shaders[SCE_VTERRAIN_USE_LOD];
        defshd = &vt->shaders[0];

        SCE_RSetStencilOp (SCE_KEEP, SCE_KEEP, SCE_REPLACE);
        SCE_RClearStencil (255);
        SCE_RClear (SCE_STENCIL_BUFFER_BIT);
        SCE_REnableStencilTest ();

        SCE_Shader_Use (lodshd->shd);

        SCE_Shader_SetParam (lodshd->enabled_loc, vt->trans_enabled);
        SCE_Shader_SetParam (lodshd->topdiffuse_loc, 2);
        SCE_Shader_SetParam (lodshd->sidediffuse_loc, 3);
        SCE_Shader_SetParam (lodshd->noise_loc, 4);
        SCE_Shader_SetParam (lodshd->material_loc, 5);

        SCE_Texture_BeginLot ();

        for (i = 0; i < vt->n_levels - 1; i++) {
            SCE_RSetStencilFunc (SCE_LEQUAL, i + 1, ~0U);

            if (vt->cut && i >= vt->cut - 1)
                SCE_Shader_SetParam (lodshd->enabled_loc, SCE_FALSE);

            tl = &vt->levels[i];
            tl3 = &vt->levels[i + 1];

            SCE_Texture_SetUnit (tl3->tex, (i % 2 ? 0 : 1));
            SCE_Texture_Use (tl3->tex);
            SCE_Texture_Use (tl->tex);
            if (vt->use_materials)
                SCE_Texture_Use (tl->mat);
            SCE_Texture_Use (vt->top_diffuse);
            SCE_Texture_Use (vt->side_diffuse);
            SCE_Texture_Use (vt->noise);
            SCE_Texture_EndLot ();

            if (i % 2) {
                SCE_Shader_SetParam (lodshd->hightex_loc, 1);
                SCE_Shader_SetParam (lodshd->lowtex_loc, 0);
            } else {
                SCE_Shader_SetParam (lodshd->hightex_loc, 0);
                SCE_Shader_SetParam (lodshd->lowtex_loc, 1);
            }

            SCE_VTerrain_RenderLevel (vt, i, tl, tl3, lodshd);

            SCE_Texture_BeginLot ();
        }
        SCE_Texture_EndLot ();

        for (i = 0; i < vt->n_levels; i++)
            SCE_Texture_SetUnit (vt->levels[i].tex, 0);

        tl = &vt->levels[vt->n_levels - 1];

        /* non lod */
        SCE_Texture_BeginLot ();
        if (0/*generate_normals*/)
            SCE_Texture_Use (tl->tex);
        if (vt->use_materials)
            SCE_Texture_Use (tl->mat);
        SCE_Texture_Use (vt->top_diffuse);
        SCE_Texture_Use (vt->side_diffuse);
        SCE_Texture_Use (vt->noise);
        SCE_Texture_EndLot ();

        SCE_Shader_Use (defshd->shd);

        SCE_Shader_SetParam (defshd->topdiffuse_loc, 2);
        SCE_Shader_SetParam (defshd->sidediffuse_loc, 3);
        SCE_Shader_SetParam (defshd->noise_loc, 4);
        SCE_Shader_SetParam (defshd->material_loc, 5);

        SCE_RSetStencilFunc (SCE_LEQUAL, vt->n_levels, ~0U);
        SCE_VTerrain_RenderLevel (vt, vt->n_levels - 1, tl, NULL, defshd);

        SCE_Shader_Use (NULL);
        SCE_Texture_Flush ();
        SCE_RDisableStencilTest ();
    }
}
