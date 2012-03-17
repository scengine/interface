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
   updated: 14/03/2012 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCEVoxelRenderer.h"
#include "SCE/interface/SCEVoxelTerrain.h"

static void SCE_VTerrain_InitRegion (SCE_SVoxelTerrainRegion *tr)
{
    tr->x = tr->y = tr->z = 0;
    SCE_VRender_InitMesh (&tr->vm);
    tr->draw = SCE_FALSE;
    SCE_List_InitIt (&tr->it);
    SCE_List_SetData (&tr->it, tr);
}
static void SCE_VTerrain_ClearRegion (SCE_SVoxelTerrainRegion *tr)
{
    SCE_VRender_ClearMesh (&tr->vm);
    SCE_List_Remove (&tr->it);
}


static void SCE_VTerrain_InitLevel (SCE_SVoxelTerrainLevel *tl)
{
    SCE_Grid_Init (&tl->grid);
    SCE_Vector3_Set (tl->wrap, 0.0, 0.0, 0.0);
    tl->tex = NULL;
    tl->subregions = 1;
    tl->regions = NULL;
    tl->mesh = NULL;
    tl->wrap_x = tl->wrap_y = tl->wrap_z = 0;
    tl->enabled = SCE_TRUE;
    tl->x = tl->y = tl->z = 0;

    tl->need_update = SCE_FALSE;
    SCE_Rectangle3_Init (&tl->update_zone);
}
static void SCE_VTerrain_ClearLevel (SCE_SVoxelTerrainLevel *tl)
{
    SCEuint i, n;

    SCE_Grid_Clear (&tl->grid);
    SCE_Texture_Delete (tl->tex);

    n = tl->subregions * tl->subregions * tl->subregions;
    for (i = 0; i < n; i++) {
        SCE_VTerrain_ClearRegion (&tl->regions[i]);
        SCE_Mesh_Clear (&tl->mesh[i]);
    }
    SCE_free (tl->regions);
    SCE_free (tl->mesh);
}

void SCE_VTerrain_Init (SCE_SVoxelTerrain *vt)
{
    size_t i;

    SCE_VRender_Init (&vt->template);
    vt->subregion_dim = 0;
    vt->n_subregions = 1;
    for (i = 0; i < SCE_MAX_VTERRAIN_LEVELS; i++)
        SCE_VTerrain_InitLevel (&vt->levels[i]);

    vt->x = vt->y = vt->z = 0;
    vt->width = vt->height = vt->depth = 0;
    vt->built = SCE_FALSE;

    SCE_List_Init (&vt->to_update);
    vt->n_update = 0;
    vt->max_updates = 4;
}
void SCE_VTerrain_Clear (SCE_SVoxelTerrain *vt)
{
    size_t i;

    SCE_VRender_Clear (&vt->template);
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
    if (!SCE_List_IsAttached (it)) {
        SCE_List_Appendl (&vt->to_update, it);
        vt->n_update++;
    }
}
static void SCE_VTerrain_RemoveRegion (SCE_SVoxelTerrain *vt,
                                       SCE_SVoxelTerrainRegion *tr)
{
    SCE_SListIterator *it = &tr->it;
    if (SCE_List_IsAttached (it)) {
        SCE_List_Removel (it);
        vt->n_update--;
    }
}

static unsigned int
SCE_VTerrain_Get (SCE_SVoxelTerrainLevel *tl, int x, int y, int z)
{
    x = SCE_Math_Ring (x + tl->wrap_x, tl->subregions);
    y = SCE_Math_Ring (y + tl->wrap_y, tl->subregions);
    z = SCE_Math_Ring (z + tl->wrap_z, tl->subregions);
    return x + tl->subregions * (y + tl->subregions * z);
}

static SCE_SVoxelTerrainRegion*
SCE_VTerrain_GetRegion (SCE_SVoxelTerrainLevel *tl, int x, int y, int z)
{
    unsigned int offset = SCE_VTerrain_Get (tl, x, y, z);
    return &tl->regions[offset];
}

void SCE_VTerrain_SetDimensions (SCE_SVoxelTerrain *vt, int w, int h, int d)
{
    vt->width = w; vt->height = h; vt->depth = d;
}
void SCE_VTerrain_SetWidth (SCE_SVoxelTerrain *vt, int w)
{
    vt->width = w;
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

static int SCE_VTerrain_BuildLevel (SCE_SVoxelTerrain *vt,
                                    SCE_SVoxelTerrainLevel *tl)
{
    SCE_STexData *tc = NULL;
    SCEuint i, num;

    SCE_Grid_SetType (&tl->grid, SCE_UNSIGNED_BYTE);
    SCE_Grid_SetDimensions (&tl->grid, vt->width, vt->height, vt->depth);
    if (SCE_Grid_Build (&tl->grid) < 0)
        goto fail;

    if (!(tc = SCE_TexData_Create ()))
        goto fail;
    /* SCE_PXF_LUMINANCE: 8 bits density precision */
    /* TODO: luminance format is deprecated */
    SCE_Grid_ToTexture (&tl->grid, tc, SCE_PXF_LUMINANCE);

    if (!(tl->tex = SCE_Texture_Create (SCE_TEX_3D, 0, 0)))
        goto fail;
    SCE_Texture_AddTexData (tl->tex, SCE_TEX_3D, tc);
    SCE_Texture_Build (tl->tex, SCE_FALSE);

    tl->subregions = vt->n_subregions;
    num = tl->subregions * tl->subregions * tl->subregions;
    if (!(tl->regions = SCE_malloc (num * sizeof *tl->regions))) goto fail;
    if (!(tl->mesh = SCE_malloc (num * sizeof *tl->mesh))) goto fail;

    for (i = 0; i < num; i++) {
        SCE_VTerrain_InitRegion (&tl->regions[i]);
        SCE_Mesh_Init (&tl->mesh[i]);

        /* TODO: make sure the geometry has the good number
           of vertices/indices */
        if (SCE_Mesh_SetGeometry (&tl->mesh[i], SCE_VRender_GetFinalGeometry (),
                                  SCE_FALSE) < 0)
            goto fail;
        SCE_Mesh_AutoBuild (&tl->mesh[i]);

        SCE_VRender_SetVolume (&tl->regions[i].vm, tl->tex);
        SCE_VRender_SetMesh (&tl->regions[i].vm, &tl->mesh[i]);
    }

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

int SCE_VTerrain_Build (SCE_SVoxelTerrain *vt)
{
    size_t i;
    SCE_STexData tc;

    if (vt->built)
        return SCE_OK;

    SCE_VRender_SetDimensions (&vt->template, vt->subregion_dim,
                               vt->subregion_dim, vt->subregion_dim);
    SCE_VRender_SetVolumeDimensions (&vt->template, vt->width,
                                     vt->height, vt->depth);
    if (SCE_VRender_Build (&vt->template) < 0)
        goto fail;

    for (i = 0; i < vt->n_levels; i++) {
        if (SCE_VTerrain_BuildLevel (vt, &vt->levels[i]) < 0)
            goto fail;
    }

    vt->built = SCE_TRUE;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


void SCE_VTerrain_SetPosition (SCE_SVoxelTerrain *vt, int x, int y, int z)
{
    vt->x = x;
    vt->y = y;
    vt->z = z;
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
    vt->levels[level].need_update = SCE_TRUE;
}

SCE_SGrid* SCE_VTerrain_GetLevelGrid (SCE_SVoxelTerrain *vt, SCEuint level)
{
    return &vt->levels[level].grid;
}

void SCE_VTerrain_ActivateLevel (SCE_SVoxelTerrain *vt, SCEuint level,
                                 int activate)
{
    vt->levels[level].enabled = activate;
}


/* TODO: update */
void SCE_VTerrain_AppendSlice (SCE_SVoxelTerrain *vt, SCEuint level,
                               SCE_EBoxFace f, const unsigned char *slice)
{
    SCE_STexData *texdata = NULL;
    SCE_SVoxelTerrainLevel *tl = NULL;
    int w, h, d;

    tl = &vt->levels[level];
    SCE_Grid_UpdateFace (&tl->grid, f, slice);

    /* TODO: update it with grid functions */
    tl->wrap[0] = (float)tl->grid.wrap_x / vt->width;
    tl->wrap[1] = (float)tl->grid.wrap_y / vt->height;
    tl->wrap[2] = (float)tl->grid.wrap_z / vt->depth;

    /* TODO: restore proper wrapping for blelfhlhlbblehblhblbl herp */
    /*SCE_VRender_SetWrap (&tl->vmesh, tl->wrap);*/

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

    tl->need_update = SCE_TRUE;
}


void SCE_VTerrain_Update (SCE_SVoxelTerrain *vt)
{
    size_t i;
    SCE_SListIterator *it = NULL, *pro = NULL;

    for (i = 0; i < vt->n_levels; i++) {
        if (vt->levels[i].need_update) {
            /* TODO: use update_zone of the level */
            SCE_Texture_Update (vt->levels[i].tex);
            vt->levels[i].need_update = SCE_FALSE;
        }
    }

    /* dequeue some regions for update */
    i = 0;
    SCE_List_ForEachProtected (pro, it, &vt->to_update) {
        SCE_SVoxelTerrainRegion *region = SCE_List_GetData (it);
        SCE_VRender_Hardware (&vt->template, &region->vm,
                              region->x * (vt->subregion_dim - 1),
                              region->y * (vt->subregion_dim - 1),
                              region->z * (vt->subregion_dim - 1));
        region->draw = SCE_TRUE;
        SCE_VTerrain_RemoveRegion (vt, region);

        i++;
        if (i >= vt->max_updates)
            break;
    }
}

void SCE_VTerrain_UpdateGrid (SCE_SVoxelTerrain *vt, SCEuint level)
{
    SCE_SIntRect3 r;

    SCE_Rectangle3_Set (&r, 0, 0, 0,
                        vt->width - 1, vt->height - 1, vt->depth - 1);
    SCE_VTerrain_UpdateSubGrid (vt, level, &r);
}

void SCE_VTerrain_UpdateSubGrid (SCE_SVoxelTerrain *vt, SCEuint level,
                                 SCE_SIntRect3 *r)
{
    SCEuint x, y, z;
    SCEuint sx, sy, sz;
    SCEuint w, h, d;
    int p1[3], p2[3];
    int l;
    SCE_SVoxelTerrainLevel *tl = &vt->levels[level];

    /* update 3D rectangle of updated area for 3D texture update */
    if (!vt->levels[level].need_update) {
        vt->levels[level].need_update = SCE_TRUE;
        vt->levels[level].update_zone = *r;
    } else {
        SCE_Rectangle3_Union (&vt->levels[level].update_zone,
                              r, &vt->levels[level].update_zone);
    }

    SCE_Rectangle3_GetPointsv (r, p1, p2);

    l = vt->subregion_dim - 1;
    sx = MAX (p1[0] - 1, 0) / l;
    sy = MAX (p1[1] - 1, 0) / l;
    sz = MAX (p1[2] - 1, 0) / l;
    w = p2[0] / l;
    h = p2[1] / l;
    d = p2[2] / l;
    if (w >= tl->subregions)
        w = tl->subregions - 1;
    if (h >= tl->subregions)
        h = tl->subregions - 1;
    if (d >= tl->subregions)
        d = tl->subregions - 1;

    SCE_Texture_Update (tl->tex);

    for (z = sz; z <= d; z++) {
        for (y = sy; y <= h; y++) {
            for (x = sx; x <= w; x++) {
                SCE_SVoxelTerrainRegion *region = NULL;
                region = SCE_VTerrain_GetRegion (tl, x, y, z);
                region->x = x; region->y = y; region->z = z;
                SCE_VTerrain_AddRegion (vt, region);
            }
        }
    }
}

/**
 * \brief Gets the missing data of a grid's level
 * \param vt a voxel terrain
 * \param level level number
 * \param dx,dy,dz number of missing slices in each direction
 * \return SCE_TRUE if there is any missing slice, SCE_FALSE otherwise
 */
int SCE_VTerrain_GetOffset (const SCE_SVoxelTerrain *vt, SCEuint level,
                            int *dx, int *dy, int *dz)
{
    SCE_SVoxelTerrainLevel *l;
    SCEuint coef = 1 << level;

    l = &vt->levels[level];
    *dx = (vt->x - l->x) / coef;
    *dy = (vt->y - l->y) / coef;
    *dz = (vt->z - l->z) / coef;

    if (*dx != 0 || *dy != 0 || *dz != 0)
        return SCE_TRUE;
    return SCE_FALSE;
}


static void SCE_VTerrain_RenderLevel (const SCE_SVoxelTerrainLevel *tl,
                                      const SCE_SVoxelTerrain *vt)
{
    SCE_TMatrix4 m;
    SCE_TVector3 pos;
    SCEuint x, y, z;

    SCE_Matrix4_Identity (m);

    for (z = 0; z < tl->subregions; z++) {
        for (y = 0; y < tl->subregions; y++) {
            for (x = 0; x < tl->subregions; x++) {
                unsigned int offset = SCE_VTerrain_Get (tl, x, y, z);

                if (tl->regions[offset].draw) {
#define DERP 1
                    SCE_Vector3_Set
                        (pos,
                         (float)x * (vt->subregion_dim - DERP) / vt->width,
                         (float)y * (vt->subregion_dim - DERP) / vt->height,
                         (float)z * (vt->subregion_dim - DERP) / vt->depth);
                    SCE_Matrix4_SetTranslation (m, pos);
                    SCE_RLoadMatrix (SCE_MAT_OBJECT, m);
                    SCE_Mesh_Use (&tl->mesh[offset]);
                    SCE_Mesh_Render ();
                    SCE_Mesh_Unuse ();
                }
            }
        }
    }
}

void SCE_VTerrain_Render (const SCE_SVoxelTerrain *vt)
{
    size_t i;

    for (i = 0; i < vt->n_levels; i++) {
        SCE_VTerrain_RenderLevel (&vt->levels[i], vt);
        /* TODO: clear depth buffer or add an offset or whatev. */
    }
}
