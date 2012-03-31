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
   updated: 30/03/2012 */

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
    tr->level = NULL;
}
static void SCE_VTerrain_ClearRegion (SCE_SVoxelTerrainRegion *tr)
{
    SCE_VRender_ClearMesh (&tr->vm);
    SCE_List_Remove (&tr->it);
}

static void SCE_VTerrain_InitTransition (SCE_SVoxelTerrainTransition *trans)
{
    SCE_Geometry_Init (&trans->geom);
    SCE_Mesh_Init (&trans->mesh);
    SCE_Rectangle3_Set (&trans->area, 0, 0, 0, 0, 0, 0);
    trans->voxels = NULL;
    trans->w = trans->h = trans->d = 0;
    SCE_List_InitIt (&trans->it);
    SCE_List_SetData (&trans->it, trans);
}
static void SCE_VTerrain_ClearTransition (SCE_SVoxelTerrainTransition *trans)
{
    SCE_Geometry_Clear (&trans->geom);
    SCE_Mesh_Clear (&trans->mesh);
    SCE_List_Remove (&trans->it);
}

static void SCE_VTerrain_InitLevel (SCE_SVoxelTerrainLevel *tl)
{
    int i;
    SCE_Grid_Init (&tl->grid);
    tl->wrap[0] = tl->wrap[1] = tl->wrap[2] = 0;
    tl->tex = NULL;
    tl->subregions = 1;
    tl->regions = NULL;
    tl->mesh = NULL;
    for (i = 0; i < 6; i++)
        tl->trans[i] = NULL;
    tl->wrap_x = tl->wrap_y = tl->wrap_z = 0;
    tl->enabled = SCE_TRUE;
    tl->x = tl->y = tl->z = 0;
    tl->map_x = tl->map_y = tl->map_z = 0;

    tl->need_update = SCE_FALSE;
    SCE_Rectangle3_Init (&tl->update_zone);
}
static void SCE_VTerrain_ClearLevel (SCE_SVoxelTerrainLevel *tl)
{
    SCEuint i, j, n;

    SCE_Grid_Clear (&tl->grid);
    SCE_Texture_Delete (tl->tex);

    n = tl->subregions * tl->subregions;
    for (i = 0; i < 6; i++) {
        if (tl->trans[i]) {
            for (j = 0; j < n; j++)
                SCE_VTerrain_ClearTransition (&tl->trans[i][j]);
            SCE_free (tl->trans[i]);
        }
    }
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
    vt->max_updates = 8;
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
    SCEuint i, j, num;
    SCEuint x, y, z;


    /* setup volume */
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


    /* setup regions */
    tl->subregions = vt->n_subregions;
    num = tl->subregions * tl->subregions * tl->subregions;
    if (!(tl->regions = SCE_malloc (num * sizeof *tl->regions))) goto fail;
    if (!(tl->mesh = SCE_malloc (num * sizeof *tl->mesh))) goto fail;

    for (i = 0; i < num; i++) {
        SCE_VTerrain_InitRegion (&tl->regions[i]);
        tl->regions[i].level = tl;
        SCE_Mesh_Init (&tl->mesh[i]);

        /* NOTE: this makes this function non-reentrant */
        if (SCE_Mesh_SetGeometry (&tl->mesh[i], SCE_VRender_GetFinalGeometry (),
                                  SCE_FALSE) < 0)
            goto fail;
        SCE_Mesh_AutoBuild (&tl->mesh[i]);

        SCE_VRender_SetVolume (&tl->regions[i].vm, tl->tex);
        SCE_VRender_SetMesh (&tl->regions[i].vm, &tl->mesh[i]);
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


    /* setup LOD transition structures */
    num = tl->subregions * tl->subregions;
    for (i = 0; i < 6; i++) {
        if (!(tl->trans[i] = SCE_malloc (num * sizeof *tl->trans[i])))
            goto fail;
        for (j = 0; j < num; j++)
            SCE_VTerrain_InitTransition (&tl->trans[i][j]);
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
    SCE_SVoxelTerrainLevel *tl = &vt->levels[level];
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


void SCE_VTerrain_AppendSlice (SCE_SVoxelTerrain *vt, SCEuint level,
                               SCE_EBoxFace f, const unsigned char *slice)
{
    SCE_STexData *texdata = NULL;
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
        if (tl->x < 1) {
            tl->x += vt->subregion_dim - 1;
            tl->wrap_x++;
            SCE_Rectangle3_Set (&r, tl->x + dim - HERP, 0, 0, w, h, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE);
        }
        break;
    case SCE_BOX_NEGX:
        tl->map_x--;
        tl->x++;
        if (tl->x + dim >= vt->width) {
            tl->x -= vt->subregion_dim - 1;
            tl->wrap_x--;
            SCE_Rectangle3_Set (&r, 0, 0, 0, vt->subregion_dim - 2, h, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE);
        }
        break;
    case SCE_BOX_POSY:
        tl->map_y++;
        tl->y--;
        if (tl->y < 1) {
            tl->y += vt->subregion_dim - 1;
            tl->wrap_y++;
            SCE_Rectangle3_Set (&r, 0, tl->y + dim - HERP, 0, w, h, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE);
        }
        break;
    case SCE_BOX_NEGY:
        tl->map_y--;
        tl->y++;
        if (tl->y + dim >= vt->height) {
            tl->y -= vt->subregion_dim - 1;
            tl->wrap_y--;
            SCE_Rectangle3_Set (&r, 0, 0, 0, w, vt->subregion_dim - 2, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE);
        }
        break;
    case SCE_BOX_POSZ:
        tl->map_z++;
        tl->z--;
        if (tl->z < 1) {
            tl->z += vt->subregion_dim - 1;
            tl->wrap_z++;
            SCE_Rectangle3_Set (&r, 0, 0, tl->z + dim - HERP, w, h, d);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE);
        }
        break;
    case SCE_BOX_NEGZ:
        tl->map_z--;
        tl->z++;
        if (tl->z + dim >= vt->depth) {
            tl->z -= vt->subregion_dim - 1;
            tl->wrap_z--;
            SCE_Rectangle3_Set (&r, 0, 0, 0, w, h, vt->subregion_dim - 2);
            SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_FALSE);
        }
        break;
    }

    tl->need_update = SCE_TRUE;
}


void SCE_VTerrain_Update (SCE_SVoxelTerrain *vt)
{
    size_t i;
    SCE_SListIterator *it = NULL, *pro = NULL;
    unsigned int x, y, z;

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
        SCE_SVoxelTerrainRegion *tr = SCE_List_GetData (it);
        SCE_SVoxelTerrainLevel *l = tr->level;

        x = SCE_Math_Ring (tr->x - l->wrap_x, l->subregions);
        y = SCE_Math_Ring (tr->y - l->wrap_y, l->subregions);
        z = SCE_Math_Ring (tr->z - l->wrap_z, l->subregions);

        SCE_VRender_Hardware (&vt->template, &tr->vm,
                              l->wrap[0] + l->x + x * (vt->subregion_dim - 1),
                              l->wrap[1] + l->y + y * (vt->subregion_dim - 1),
                              l->wrap[2] + l->z + z * (vt->subregion_dim - 1));
        tr->draw = SCE_VRender_IsVoid (&tr->vm);
        SCE_VTerrain_RemoveRegion (vt, tr);

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
    SCE_VTerrain_UpdateSubGrid (vt, level, &r, SCE_TRUE);
}

void SCE_VTerrain_UpdateSubGrid (SCE_SVoxelTerrain *vt, SCEuint level,
                                 SCE_SIntRect3 *rect, int draw)
{
    SCEuint x, y, z;
    SCEuint sx, sy, sz;
    SCEuint w, h, d;
    int p1[3], p2[3];
    int l;
    SCE_SIntRect3 r, grid_area;
    SCE_SVoxelTerrainLevel *tl = &vt->levels[level];

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


static void SCE_VTerrain_RenderLevel (const SCE_SVoxelTerrain *vt,
                                      SCEuint level)
{
    SCE_TMatrix4 m;
    SCE_TVector3 pos, origin;
    SCEuint x, y, z;
    float invw, invh, invd;
    long scale;
    const SCE_SVoxelTerrainLevel *tl = &vt->levels[level], *tl2 = NULL;

    if (level > 0)
        tl2 = &vt->levels[level - 1];

    scale = 1 << level;

    invw = 1.0 / vt->width;
    invh = 1.0 / vt->height;
    invd = 1.0 / vt->depth;

    /* wtf? why -0.5 ? */
    SCE_Vector3_Set (origin,
                     (tl->map_x + tl->x - 0.5) * invw,
                     (tl->map_y + tl->y - 0.5) * invh,
                     (tl->map_z + tl->z - 0.5) * invd);

    for (z = 0; z < tl->subregions; z++) {
        for (y = 0; y < tl->subregions; y++) {
            for (x = 0; x < tl->subregions; x++) {
                unsigned int offset = SCE_VTerrain_Get (tl, x, y, z);
                int draw = SCE_TRUE, p1[3], p2[3];
                SCE_SIntRect3 inner_lod, region;

                if (tl2) {
                    /* compute current region area */
                    p1[0] = 2 * (tl->map_x + tl->x + x * (vt->subregion_dim-1));
                    p1[1] = 2 * (tl->map_y + tl->y + y * (vt->subregion_dim-1));
                    p1[2] = 2 * (tl->map_z + tl->z + z * (vt->subregion_dim-1));
                    p2[0] = p1[0] + 2 * vt->subregion_dim;
                    p2[1] = p1[1] + 2 * vt->subregion_dim;
                    p2[2] = p1[2] + 2 * vt->subregion_dim;
                    SCE_Rectangle3_Setv (&region, p1, p2);

                    /* compute inner LOD grid area */
                    p1[0] = tl2->map_x + tl2->x;
                    p1[1] = tl2->map_y + tl2->y;
                    p1[2] = tl2->map_z + tl2->z;
                    p2[0] = 1+p1[0] + vt->n_subregions * (vt->subregion_dim-1);
                    p2[1] = 1+p1[1] + vt->n_subregions * (vt->subregion_dim-1);
                    p2[2] = 1+p1[2] + vt->n_subregions * (vt->subregion_dim-1);
                    SCE_Rectangle3_Setv (&inner_lod, p1, p2);

                    draw = !SCE_Rectangle3_IsInside (&inner_lod, &region);
                }

                draw = draw && tl->regions[offset].draw;

                if (draw) {
#define DERP 1
                    SCE_Vector3_Set
                        (pos,
                         (float)x * (vt->subregion_dim - DERP) * invw,
                         (float)y * (vt->subregion_dim - DERP) * invh,
                         (float)z * (vt->subregion_dim - DERP) * invd);
                    SCE_Vector3_Operator1v (pos, +=, origin);
                    SCE_Matrix4_Identity (m);
                    SCE_Matrix4_SetScale (m, scale, scale, scale);
                    SCE_Matrix4_MulTranslatev (m, pos);
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
        SCE_VTerrain_RenderLevel (vt, i);
        /* TODO: clear depth buffer or add an offset or whatev. */
    }
}
