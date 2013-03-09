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

/* created: 14/02/2012
   updated: 09/03/2013 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCEVoxelRenderer.h"

static SCE_SGeometry non_empty_geom;
static SCE_SGeometry list_verts_geom;
static SCE_SGeometry final_geom_pos; /* just for shiets. */
static SCE_SGeometry final_geom_pos_nor;
static SCE_SGeometry final_geom_pos_cnor; /* c stands for "compressed" */
static SCE_SGeometry final_geom_cpos_nor;
static SCE_SGeometry final_geom_cpos_cnor;

static unsigned int sce_max_vertices = 0;
static unsigned int sce_max_indices = 0;
static unsigned int sce_vertices_limit = 0;
static unsigned int sce_indices_limit = 0;

static const unsigned char mc_lt_edges_pbourke[254 * 15] = {
    0, 8, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 8, 3, 9, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 8, 3, 1, 2, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    9, 2, 10, 0, 2, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 8, 3, 2, 10, 8, 10, 9, 8, 0, 0, 0, 0, 0, 0,
    3, 11, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 11, 2, 8, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 9, 0, 2, 3, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 11, 2, 1, 9, 11, 9, 8, 11, 0, 0, 0, 0, 0, 0,
    3, 10, 1, 11, 10, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 10, 1, 0, 8, 10, 8, 11, 10, 0, 0, 0, 0, 0, 0,
    3, 9, 0, 3, 11, 9, 11, 10, 9, 0, 0, 0, 0, 0, 0,
    9, 8, 10, 10, 8, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 3, 0, 7, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 9, 8, 4, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 1, 9, 4, 7, 1, 7, 3, 1, 0, 0, 0, 0, 0, 0,
    1, 2, 10, 8, 4, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 4, 7, 3, 0, 4, 1, 2, 10, 0, 0, 0, 0, 0, 0,
    9, 2, 10, 9, 0, 2, 8, 4, 7, 0, 0, 0, 0, 0, 0,
    2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, 0, 0, 0,
    8, 4, 7, 3, 11, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    11, 4, 7, 11, 2, 4, 2, 0, 4, 0, 0, 0, 0, 0, 0,
    9, 0, 1, 8, 4, 7, 2, 3, 11, 0, 0, 0, 0, 0, 0,
    4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, 0, 0, 0,
    3, 10, 1, 3, 11, 10, 7, 8, 4, 0, 0, 0, 0, 0, 0,
    1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, 0, 0, 0,
    4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, 0, 0, 0,
    4, 7, 11, 4, 11, 9, 9, 11, 10, 0, 0, 0, 0, 0, 0,
    9, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    9, 5, 4, 0, 8, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 5, 4, 1, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    8, 5, 4, 8, 3, 5, 3, 1, 5, 0, 0, 0, 0, 0, 0,
    1, 2, 10, 9, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 0, 8, 1, 2, 10, 4, 9, 5, 0, 0, 0, 0, 0, 0,
    5, 2, 10, 5, 4, 2, 4, 0, 2, 0, 0, 0, 0, 0, 0,
    2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, 0, 0, 0,
    9, 5, 4, 2, 3, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 11, 2, 0, 8, 11, 4, 9, 5, 0, 0, 0, 0, 0, 0,
    0, 5, 4, 0, 1, 5, 2, 3, 11, 0, 0, 0, 0, 0, 0,
    2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, 0, 0, 0,
    10, 3, 11, 10, 1, 3, 9, 5, 4, 0, 0, 0, 0, 0, 0,
    4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, 0, 0, 0,
    5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, 0, 0, 0,
    5, 4, 8, 5, 8, 10, 10, 8, 11, 0, 0, 0, 0, 0, 0,
    9, 7, 8, 5, 7, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    9, 3, 0, 9, 5, 3, 5, 7, 3, 0, 0, 0, 0, 0, 0,
    0, 7, 8, 0, 1, 7, 1, 5, 7, 0, 0, 0, 0, 0, 0,
    1, 5, 3, 3, 5, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    9, 7, 8, 9, 5, 7, 10, 1, 2, 0, 0, 0, 0, 0, 0,
    10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, 0, 0, 0,
    8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, 0, 0, 0,
    2, 10, 5, 2, 5, 3, 3, 5, 7, 0, 0, 0, 0, 0, 0,
    7, 9, 5, 7, 8, 9, 3, 11, 2, 0, 0, 0, 0, 0, 0,
    9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, 0, 0, 0,
    2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, 0, 0, 0,
    11, 2, 1, 11, 1, 7, 7, 1, 5, 0, 0, 0, 0, 0, 0,
    9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, 0, 0, 0,
    5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0,
    11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0,
    11, 10, 5, 7, 11, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    10, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 8, 3, 5, 10, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    9, 0, 1, 5, 10, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 8, 3, 1, 9, 8, 5, 10, 6, 0, 0, 0, 0, 0, 0,
    1, 6, 5, 2, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 6, 5, 1, 2, 6, 3, 0, 8, 0, 0, 0, 0, 0, 0,
    9, 6, 5, 9, 0, 6, 0, 2, 6, 0, 0, 0, 0, 0, 0,
    5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, 0, 0, 0,
    2, 3, 11, 10, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    11, 0, 8, 11, 2, 0, 10, 6, 5, 0, 0, 0, 0, 0, 0,
    0, 1, 9, 2, 3, 11, 5, 10, 6, 0, 0, 0, 0, 0, 0,
    5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, 0, 0, 0,
    6, 3, 11, 6, 5, 3, 5, 1, 3, 0, 0, 0, 0, 0, 0,
    0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, 0, 0, 0,
    3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, 0, 0, 0,
    6, 5, 9, 6, 9, 11, 11, 9, 8, 0, 0, 0, 0, 0, 0,
    5, 10, 6, 4, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 3, 0, 4, 7, 3, 6, 5, 10, 0, 0, 0, 0, 0, 0,
    1, 9, 0, 5, 10, 6, 8, 4, 7, 0, 0, 0, 0, 0, 0,
    10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, 0, 0, 0,
    6, 1, 2, 6, 5, 1, 4, 7, 8, 0, 0, 0, 0, 0, 0,
    1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, 0, 0, 0,
    8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, 0, 0, 0,
    7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9,
    3, 11, 2, 7, 8, 4, 10, 6, 5, 0, 0, 0, 0, 0, 0,
    5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, 0, 0, 0,
    0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, 0, 0, 0,
    9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6,
    8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, 0, 0, 0,
    5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11,
    0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7,
    6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, 0, 0, 0,
    10, 4, 9, 6, 4, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 10, 6, 4, 9, 10, 0, 8, 3, 0, 0, 0, 0, 0, 0,
    10, 0, 1, 10, 6, 0, 6, 4, 0, 0, 0, 0, 0, 0, 0,
    8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, 0, 0, 0,
    1, 4, 9, 1, 2, 4, 2, 6, 4, 0, 0, 0, 0, 0, 0,
    3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, 0, 0, 0,
    0, 2, 4, 4, 2, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    8, 3, 2, 8, 2, 4, 4, 2, 6, 0, 0, 0, 0, 0, 0,
    10, 4, 9, 10, 6, 4, 11, 2, 3, 0, 0, 0, 0, 0, 0,
    0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, 0, 0, 0,
    3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, 0, 0, 0,
    6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1,
    9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, 0, 0, 0,
    8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1,
    3, 11, 6, 3, 6, 0, 0, 6, 4, 0, 0, 0, 0, 0, 0,
    6, 4, 8, 11, 6, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    7, 10, 6, 7, 8, 10, 8, 9, 10, 0, 0, 0, 0, 0, 0,
    0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, 0, 0, 0,
    10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, 0, 0, 0,
    10, 6, 7, 10, 7, 1, 1, 7, 3, 0, 0, 0, 0, 0, 0,
    1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, 0, 0, 0,
    2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9,
    7, 8, 0, 7, 0, 6, 6, 0, 2, 0, 0, 0, 0, 0, 0,
    7, 3, 2, 6, 7, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, 0, 0, 0,
    2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7,
    1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11,
    11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, 0, 0, 0,
    8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6,
    0, 9, 1, 11, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, 0, 0, 0,
    7, 11, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    7, 6, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 0, 8, 11, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 9, 11, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    8, 1, 9, 8, 3, 1, 11, 7, 6, 0, 0, 0, 0, 0, 0,
    10, 1, 2, 6, 11, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 10, 3, 0, 8, 6, 11, 7, 0, 0, 0, 0, 0, 0,
    2, 9, 0, 2, 10, 9, 6, 11, 7, 0, 0, 0, 0, 0, 0,
    6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, 0, 0, 0,
    7, 2, 3, 6, 2, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    7, 0, 8, 7, 6, 0, 6, 2, 0, 0, 0, 0, 0, 0, 0,
    2, 7, 6, 2, 3, 7, 0, 1, 9, 0, 0, 0, 0, 0, 0,
    1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, 0, 0, 0,
    10, 7, 6, 10, 1, 7, 1, 3, 7, 0, 0, 0, 0, 0, 0,
    10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, 0, 0, 0,
    0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, 0, 0, 0,
    7, 6, 10, 7, 10, 8, 8, 10, 9, 0, 0, 0, 0, 0, 0,
    6, 8, 4, 11, 8, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 6, 11, 3, 0, 6, 0, 4, 6, 0, 0, 0, 0, 0, 0,
    8, 6, 11, 8, 4, 6, 9, 0, 1, 0, 0, 0, 0, 0, 0,
    9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, 0, 0, 0,
    6, 8, 4, 6, 11, 8, 2, 10, 1, 0, 0, 0, 0, 0, 0,
    1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, 0, 0, 0,
    4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, 0, 0, 0,
    10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3,
    8, 2, 3, 8, 4, 2, 4, 6, 2, 0, 0, 0, 0, 0, 0,
    0, 4, 2, 4, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, 0, 0, 0,
    1, 9, 4, 1, 4, 2, 2, 4, 6, 0, 0, 0, 0, 0, 0,
    8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, 0, 0, 0,
    10, 1, 0, 10, 0, 6, 6, 0, 4, 0, 0, 0, 0, 0, 0,
    4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3,
    10, 9, 4, 6, 10, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 9, 5, 7, 6, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 8, 3, 4, 9, 5, 11, 7, 6, 0, 0, 0, 0, 0, 0,
    5, 0, 1, 5, 4, 0, 7, 6, 11, 0, 0, 0, 0, 0, 0,
    11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, 0, 0, 0,
    9, 5, 4, 10, 1, 2, 7, 6, 11, 0, 0, 0, 0, 0, 0,
    6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, 0, 0, 0,
    7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, 0, 0, 0,
    3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6,
    7, 2, 3, 7, 6, 2, 5, 4, 9, 0, 0, 0, 0, 0, 0,
    9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, 0, 0, 0,
    3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, 0, 0, 0,
    6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8,
    9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, 0, 0, 0,
    1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4,
    4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10,
    7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, 0, 0, 0,
    6, 9, 5, 6, 11, 9, 11, 8, 9, 0, 0, 0, 0, 0, 0,
    3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, 0, 0, 0,
    0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, 0, 0, 0,
    6, 11, 3, 6, 3, 5, 5, 3, 1, 0, 0, 0, 0, 0, 0,
    1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, 0, 0, 0,
    0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10,
    11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5,
    6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, 0, 0, 0,
    5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, 0, 0, 0,
    9, 5, 6, 9, 6, 0, 0, 6, 2, 0, 0, 0, 0, 0, 0,
    1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8,
    1, 5, 6, 2, 1, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6,
    10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, 0, 0, 0,
    0, 3, 8, 5, 6, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    10, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    11, 5, 10, 7, 5, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    11, 5, 10, 11, 7, 5, 8, 3, 0, 0, 0, 0, 0, 0, 0,
    5, 11, 7, 5, 10, 11, 1, 9, 0, 0, 0, 0, 0, 0, 0,
    10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, 0, 0, 0,
    11, 1, 2, 11, 7, 1, 7, 5, 1, 0, 0, 0, 0, 0, 0,
    0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, 0, 0, 0,
    9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, 0, 0, 0,
    7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2,
    2, 5, 10, 2, 3, 5, 3, 7, 5, 0, 0, 0, 0, 0, 0,
    8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, 0, 0, 0,
    9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, 0, 0, 0,
    9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2,
    1, 3, 5, 3, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 8, 7, 0, 7, 1, 1, 7, 5, 0, 0, 0, 0, 0, 0,
    9, 0, 3, 9, 3, 5, 5, 3, 7, 0, 0, 0, 0, 0, 0,
    9, 8, 7, 5, 9, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    5, 8, 4, 5, 10, 8, 10, 11, 8, 0, 0, 0, 0, 0, 0,
    5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, 0, 0, 0,
    0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, 0, 0, 0,
    10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4,
    2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, 0, 0, 0,
    0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11,
    0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5,
    9, 4, 5, 2, 11, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, 0, 0, 0,
    5, 10, 2, 5, 2, 4, 4, 2, 0, 0, 0, 0, 0, 0, 0,
    3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9,
    5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, 0, 0, 0,
    8, 4, 5, 8, 5, 3, 3, 5, 1, 0, 0, 0, 0, 0, 0,
    0, 4, 5, 1, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, 0, 0, 0,
    9, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 11, 7, 4, 9, 11, 9, 10, 11, 0, 0, 0, 0, 0, 0,
    0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, 0, 0, 0,
    1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, 0, 0, 0,
    3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4,
    4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, 0, 0, 0,
    9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3,
    11, 7, 4, 11, 4, 2, 2, 4, 0, 0, 0, 0, 0, 0, 0,
    11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, 0, 0, 0,
    2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, 0, 0, 0,
    9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7,
    3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10,
    1, 10, 2, 8, 7, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 9, 1, 4, 1, 7, 7, 1, 3, 0, 0, 0, 0, 0, 0,
    4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, 0, 0, 0,
    4, 0, 3, 7, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    4, 8, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    9, 10, 8, 10, 11, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 0, 9, 3, 9, 11, 11, 9, 10, 0, 0, 0, 0, 0, 0,
    0, 1, 10, 0, 10, 8, 8, 10, 11, 0, 0, 0, 0, 0, 0,
    3, 1, 10, 11, 3, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 2, 11, 1, 11, 9, 9, 11, 8, 0, 0, 0, 0, 0, 0,
    3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, 0, 0, 0,
    0, 2, 11, 8, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    3, 2, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 3, 8, 2, 8, 10, 10, 8, 9, 0, 0, 0, 0, 0, 0,
    9, 10, 2, 0, 9, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, 0, 0, 0,
    1, 10, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 3, 8, 9, 1, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 3, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};


/* TODO: AddArrayDub() can fail dude */
int SCE_Init_VRender (void)
{
    SCE_SGeometryArray ar1, ar2;
    SCE_SGeometry *final_geom = NULL;

    SCE_Geometry_Init (&non_empty_geom);
    SCE_Geometry_Init (&list_verts_geom);

    /* create non-empty cells geometry */
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayData (&ar1, SCE_IPOSITION, SCE_UNSIGNED_INT, 0, 1,
                               NULL, SCE_FALSE);
    SCE_Geometry_AddArrayDup (&non_empty_geom, &ar1, SCE_FALSE);
    SCE_Geometry_SetPrimitiveType (&non_empty_geom, SCE_POINTS);

    /* create vertices-to-generate list geometry */
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayData (&ar1, SCE_IPOSITION, SCE_UNSIGNED_INT, 0, 1,
                               NULL, SCE_FALSE);
    SCE_Geometry_AddArrayDup (&list_verts_geom, &ar1, SCE_FALSE);
    SCE_Geometry_SetPrimitiveType (&list_verts_geom, SCE_POINTS);

    /* create final vertices geometry */
    /* no compression */
    final_geom = &final_geom_pos_nor;
    SCE_Geometry_Init (final_geom);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_InitArray (&ar2);
    SCE_Geometry_SetArrayData (&ar1, SCE_POSITION, SCE_FLOAT, 0, 4,
                               NULL, SCE_FALSE);
    SCE_Geometry_SetArrayData (&ar2, SCE_NORMAL, SCE_FLOAT, 0, 3,
                               NULL, SCE_FALSE);
    SCE_Geometry_AttachArray (&ar1, &ar2);
    SCE_Geometry_AddArrayRecDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayIndices (&ar1, SCE_UNSIGNED_INT, NULL, SCE_FALSE);
    SCE_Geometry_SetIndexArrayDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_SetPrimitiveType (final_geom, SCE_TRIANGLES);


    /* not interleaved nor compressed */
    final_geom = &final_geom_pos;
    SCE_Geometry_Init (final_geom);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayData (&ar1, SCE_POSITION, SCE_VERTICES_TYPE, 0, 3,
                               NULL, SCE_FALSE);
    SCE_Geometry_AddArrayDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayData (&ar1, SCE_NORMAL, SCE_VERTICES_TYPE, 0, 3,
                               NULL, SCE_FALSE);
    SCE_Geometry_AddArrayDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayIndices (&ar1, SCE_INDICES_TYPE, NULL, SCE_FALSE);
    SCE_Geometry_SetIndexArrayDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_SetPrimitiveType (final_geom, SCE_TRIANGLES);


    /* compressed normal */
    final_geom = &final_geom_pos_cnor;
    SCE_Geometry_Init (final_geom);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_InitArray (&ar2);
    SCE_Geometry_SetArrayData (&ar1, SCE_POSITION, SCE_FLOAT, 0, 4,
                               NULL, SCE_FALSE);
    SCE_Geometry_SetArrayData (&ar2, SCE_INORMAL, SCE_UNSIGNED_BYTE, 0, 4,
                               NULL, SCE_FALSE);
    SCE_Geometry_AttachArray (&ar1, &ar2);
    SCE_Geometry_AddArrayRecDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayIndices (&ar1, SCE_UNSIGNED_INT, NULL, SCE_FALSE);
    SCE_Geometry_SetIndexArrayDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_SetPrimitiveType (final_geom, SCE_TRIANGLES);


    /* compressed position */
    final_geom = &final_geom_cpos_nor;
    SCE_Geometry_Init (final_geom);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_InitArray (&ar2);
    SCE_Geometry_SetArrayData (&ar1, SCE_IPOSITION, SCE_UNSIGNED_BYTE, 0, 4,
                               NULL, SCE_FALSE);
    SCE_Geometry_SetArrayData (&ar2, SCE_NORMAL, SCE_FLOAT, 0, 3,
                               NULL, SCE_FALSE);
    SCE_Geometry_AttachArray (&ar1, &ar2);
    SCE_Geometry_AddArrayRecDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayIndices (&ar1, SCE_UNSIGNED_INT, NULL, SCE_FALSE);
    SCE_Geometry_SetIndexArrayDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_SetPrimitiveType (final_geom, SCE_TRIANGLES);


    /* both compressed */
    final_geom = &final_geom_cpos_cnor;
    SCE_Geometry_Init (final_geom);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_InitArray (&ar2);
    SCE_Geometry_SetArrayData (&ar1, SCE_IPOSITION, SCE_UNSIGNED_BYTE, 0, 4,
                               NULL, SCE_FALSE);
    SCE_Geometry_SetArrayData (&ar2, SCE_INORMAL, SCE_UNSIGNED_BYTE, 0, 4,
                               NULL, SCE_FALSE);
    SCE_Geometry_AttachArray (&ar1, &ar2);
    SCE_Geometry_AddArrayRecDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_InitArray (&ar1);
    SCE_Geometry_SetArrayIndices (&ar1, SCE_UNSIGNED_INT, NULL, SCE_FALSE);
    SCE_Geometry_SetIndexArrayDup (final_geom, &ar1, SCE_FALSE);
    SCE_Geometry_SetPrimitiveType (final_geom, SCE_TRIANGLES);

    return SCE_OK;
}

void SCE_Quit_VRender (void)
{
    SCE_Geometry_Clear (&non_empty_geom);
    SCE_Geometry_Clear (&list_verts_geom);
    SCE_Geometry_Clear (&final_geom_pos_nor);
    SCE_Geometry_Clear (&final_geom_pos_cnor);
    SCE_Geometry_Clear (&final_geom_cpos_nor);
    SCE_Geometry_Clear (&final_geom_cpos_cnor);
}

void SCE_VRender_Init (SCE_SVoxelTemplate *vt)
{
    vt->pipeline = SCE_VRENDER_SOFTWARE;
    vt->algo = SCE_VRENDER_MARCHING_CUBES;

    vt->compressed_pos = SCE_FALSE;
    vt->compressed_nor = SCE_FALSE;
    vt->comp_scale = 1.0;
    vt->final_geom = NULL;

    vt->vwidth = vt->vheight = vt->vdepth = 0;
    vt->width = vt->height = vt->depth = 0;

    SCE_MC_Init (&vt->mc_gen);
    vt->vertices = NULL;
    vt->normals = NULL;
    vt->indices = NULL;

    SCE_Geometry_Init (&vt->grid_geom);
    SCE_Mesh_Init (&vt->grid_mesh);
    SCE_Mesh_Init (&vt->non_empty);
    SCE_Mesh_Init (&vt->list_verts);

    vt->non_empty_shader = NULL;
    vt->non_empty_offset_loc = 0;
    vt->list_verts_shader = NULL;
    vt->final_shader = NULL;
    vt->final_offset_loc = SCE_SHADER_BAD_INDEX;
    vt->comp_scale_loc = SCE_SHADER_BAD_INDEX;

    vt->splat_shader = NULL;
    vt->indices_shader = NULL;
    vt->splat = NULL;
    vt->mc_table = NULL;
}
void SCE_VRender_Clear (SCE_SVoxelTemplate *vt)
{
    SCE_Geometry_Clear (&vt->grid_geom);
    SCE_Mesh_Clear (&vt->grid_mesh);
    SCE_Mesh_Clear (&vt->non_empty);
    SCE_Mesh_Clear (&vt->list_verts);

    SCE_MC_Clear (&vt->mc_gen);
    SCE_free (vt->vertices);
    SCE_free (vt->normals);
    SCE_free (vt->indices);

    SCE_Shader_Delete (vt->non_empty_shader);
    SCE_Shader_Delete (vt->list_verts_shader);
    SCE_Shader_Delete (vt->final_shader);

    SCE_Shader_Delete (vt->splat_shader);
    SCE_Shader_Delete (vt->indices_shader);
    SCE_Texture_Delete (vt->splat);
}
SCE_SVoxelTemplate* SCE_VRender_Create (void)
{
    SCE_SVoxelTemplate *vt = NULL;
    if (!(vt = SCE_malloc (sizeof *vt)))
        SCEE_LogSrc ();
    else
        SCE_VRender_Init (vt);
    return vt;
}
void SCE_VRender_Delete (SCE_SVoxelTemplate *vt)
{
    if (vt) {
        SCE_VRender_Clear (vt);
        SCE_free (vt);
    }
}

void SCE_VRender_InitMesh (SCE_SVoxelMesh *vm)
{
    SCE_Vector3_Set (vm->wrap, 0.0, 0.0, 0.0);
    vm->mesh = NULL;
    vm->render = SCE_FALSE;
    vm->vertex_range[0] = vm->vertex_range[1] = -1;
    vm->index_range[0] = vm->index_range[1] = -1;
    vm->n_vertices = 0;
    vm->n_indices = 0;
}
void SCE_VRender_ClearMesh (SCE_SVoxelMesh *vm)
{
}
SCE_SVoxelMesh* SCE_VRender_CreateMesh (void)
{
    SCE_SVoxelMesh *vm = NULL;
    if (!(vm = SCE_malloc (sizeof *vm)))
        SCEE_LogSrc ();
    else
        SCE_VRender_InitMesh (vm);
    return vm;
}
void SCE_VRender_DeleteMesh (SCE_SVoxelMesh *vm)
{
    if (vm) {
        SCE_VRender_ClearMesh (vm);
        SCE_free (vm);
    }
}

void SCE_VRender_SetPipeline (SCE_SVoxelTemplate *vt,
                              SCE_EVoxelRenderPipeline pipeline)
{
    vt->pipeline = pipeline;
}

void SCE_VRender_SetDimensions (SCE_SVoxelTemplate *vt, int w, int h, int d)
{
    vt->width = w; vt->height = h; vt->depth = d;
}
void SCE_VRender_SetWidth (SCE_SVoxelTemplate *vt, int w)
{
    vt->width = w;
}
void SCE_VRender_SetHeight (SCE_SVoxelTemplate *vt, int h)
{
    vt->height = h;
}
void SCE_VRender_SetDepth (SCE_SVoxelTemplate *vt, int d)
{
    vt->depth = d;
}

void SCE_VRender_SetVolumeDimensions (SCE_SVoxelTemplate *vt, int w, int h, int d)
{
    vt->vwidth = w; vt->vheight = h; vt->vdepth = d;
}
void SCE_VRender_SetVolumeWidth (SCE_SVoxelTemplate *vt, int w)
{
    vt->vwidth = w;
}
void SCE_VRender_SetVolumeHeight (SCE_SVoxelTemplate *vt, int h)
{
    vt->vheight = h;
}
void SCE_VRender_SetVolumeDepth (SCE_SVoxelTemplate *vt, int d)
{
    vt->vdepth = d;
}


void SCE_VRender_CompressPosition (SCE_SVoxelTemplate *vt, int comp)
{
    vt->compressed_pos = comp;
}
void SCE_VRender_CompressNormal (SCE_SVoxelTemplate *vt, int comp)
{
    vt->compressed_nor = comp;
}
void SCE_VRender_SetCompressedScale (SCE_SVoxelTemplate *vt, float scale)
{
    vt->comp_scale = scale;
}
void SCE_VRender_SetAlgorithm (SCE_SVoxelTemplate *vt,
                               SCE_EVoxelRenderAlgorithm a)
{
    vt->algo = a;
}

static const char *non_empty_vs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "in vec3 sce_position;"
    "out vec3 pos;"

    "void main (void)"
    "{"
    "  pos = sce_position + 0.5 * vec3 (OW, OH, OD);"
    "}";

/* TODO: should use macro SCE_SHADER_UNIFORM_SAMPLER_0 */
static const char *mt_non_empty_gs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "layout (points, max_vertices = 1) in;"
    "layout (points, max_vertices = 1) out;"

    "in vec3 pos[1];"
    "out uint xyz8_case8;"

    "uniform sampler3D sce_tex0;"
    "uniform vec3 offset;"

    "void main (void)"
    "{"
    "  uint case8;"
    "  vec3 p = pos[0];"
       /* texture fetch */
    "  vec3 tc = p + offset;"
    "  case8  = uint(0.5 + texture3D (sce_tex0, tc).x);"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (OW, 0., 0.)).x) << 1;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (OW, 0., OD)).x) << 2;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (0., 0., OD)).x) << 3;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (0., OH, OD)).x) << 4;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (OW, OH, OD)).x) << 5;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (OW, OH, 0.)).x) << 6;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (0., OH, 0.)).x) << 7;"

    "  if (case8 > 0 && case8 < 255) {"
    "    p *= vec3 (W, H, D);"
    "    case8 |= uint(p.x) << 24;"
    "    case8 |= uint(p.y) << 16;"
    "    case8 |= uint(p.z) << 8;"
    "    xyz8_case8 = case8;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "}";

static const char *mc_non_empty_gs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "layout (points, max_vertices = 1) in;"
    "layout (points, max_vertices = 1) out;"

    "in vec3 pos[1];"
    "out uint xyz8_case8;"

    "uniform sampler3D sce_tex0;"
    "uniform vec3 offset;"

    "void main (void)"
    "{"
    "  uint case8;"
    "  vec3 p = pos[0];"
       /* texture fetch */
    "  vec3 tc = p + offset;"
    "  case8  = uint(0.5 + texture3D (sce_tex0, tc                    ).x) << 3;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (OW, 0., 0.)).x) << 2;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (OW, 0., OD)).x) << 6;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (0., 0., OD)).x) << 7;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (0., OH, OD)).x) << 4;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (OW, OH, OD)).x) << 5;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (OW, OH, 0.)).x) << 1;"
    "  case8 |= uint(0.5 + texture3D (sce_tex0, tc + vec3 (0., OH, 0.)).x);"

    "  if (case8 > 0 && case8 < 255) {"
    "    p *= vec3 (W, H, D);"
    "    case8 |= uint(p.x) << 24;"
    "    case8 |= uint(p.y) << 16;"
    "    case8 |= uint(p.z) << 8;"
    "    xyz8_case8 = case8;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "}";


static const char *list_verts_vs =
    "in uint sce_position;"
    "out uint xyz8_case8;"

    "void main (void)"
    "{"
    "  xyz8_case8 = sce_position;"
    "}";

/*
 *  4_____________5
 *  |\           |\
 *  | \          | \
 *  |  \         |  \
 *  |   \        |   \
 *  |    \3___________\2
 *  |    |            |         y   z
 *  |7__ | ______|6   |          \ |
 *   \   |        \   |           \|__ x
 *    \  |         \  |
 *     \ |          \ |
 *      \|___________\|
 *       0            1
 */

/* index  edge
 * 0:     0-6
 * 1:     0-7
 * 2:     0-1
 * 3:     0-3
 * 4:     3-7
 * 5:     3-1
 * 6:     3-6
 */
static const char *mt_list_verts_gs =
    "layout (points, max_vertices = 1) in;"
    "layout (points, max_vertices = 7) out;"

    "in uint xyz8_case8[1];"
    "out uint xyz8_edge8;"      /* actually there are some bits left unused */

    "void main (void)"
    "{"
    "  uint xyz = 0xFFFFFF00 & xyz8_case8[0];"
    "  uint case8 = 0x000000FF & xyz8_case8[0];"

    "  if ((case8 & 1) != ((case8 >> 6) & 1)) {"
    "    xyz8_edge8 = xyz;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "  if ((case8 & 1) != ((case8 >> 7) & 1)) {"
    "    xyz8_edge8 = xyz + 1;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "  if ((case8 & 1) != ((case8 >> 1) & 1)) {"
    "    xyz8_edge8 = xyz + 2;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "  if ((case8 & 1) != ((case8 >> 3) & 1)) {"
    "    xyz8_edge8 = xyz + 3;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "  if (((case8 >> 3) & 1) != ((case8 >> 7) & 1)) {"
    "    xyz8_edge8 = xyz + 4;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "  if (((case8 >> 3) & 1) != ((case8 >> 1) & 1)) {"
    "    xyz8_edge8 = xyz + 5;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "  if (((case8 >> 3) & 1) != ((case8 >> 6) & 1)) {"
    "    xyz8_edge8 = xyz + 6;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "}";


/*
 *  4________4________5
 *  |\               |\
 *  | \              | \
 *  |  \             |  \
 *  |   \7           |   \5
 * 8|    \           |9   \
 *  |     \          |     \
 *  |      \7______6________\6
 *  |      |                |         y   z
 *  |0____ | ________|1     |          \ |
 *   \     |   0      \     |           \|__ x
 *    \  11|           \    |
 *    3\   |            \1  |10
 *      \  |             \  |
 *       \ |              \ |
 *        \|_______2_______\|
 *         3                2
 *
 *
 * local numbering:
 *
 *    y   z
 *     \  |
 *     0\ |2
 *       \|____ x
 *           1
 */

static const char *mc_list_verts_gs =
    "layout (points, max_vertices = 1) in;"
    "layout (points, max_vertices = 3) out;"

    "in uint xyz8_case8[1];"
    "out uint xyz8_edge8;"      /* actually there are some bits left unused */

    "void main (void)"
    "{"
    "  uint xyz = 0xFFFFFF00 & xyz8_case8[0];"
    "  uint case8 = 0x000000FF & xyz8_case8[0];"

       /* corners 3 and 0 */
    "  uint corner3 = (case8 >> 3) & 1;"
    "  if (corner3 != (case8 & 1)) {"
    "    xyz8_edge8 = xyz;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
       /* corners 3 and 2 */
    "  if (corner3 != ((case8 >> 2) & 1)) {"
    "    xyz8_edge8 = xyz + 1;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
       /* corners 3 and 7 */
    "  if (corner3 != ((case8 >> 7) & 1)) {"
    "    xyz8_edge8 = xyz + 2;"
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "}";


static const char *mt_final_vs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "in uint sce_position;"
    "\n#if SCE_VRENDER_HIGHP_VERTEX_POS\n"
    "out vec4 pos;"
    "\n#else\n"
    "out uint pos;"
    "\n#endif\n"

    "\n#if SCE_VRENDER_HIGHP_VERTEX_NOR\n"
    "out vec3 nor;"
    "\n#else\n"
    "out uint nor;"
    "\n#endif\n"

    "uniform sampler3D sce_tex0;"
    "uniform vec3 offset;"
    "uniform float comp_scale;"

    "\n#if !SCE_VRENDER_HIGHP_VERTEX_POS\n"
    "uint encode_pos (vec3 pos)"
    "{"
    "  const float factor = 256.0 * comp_scale;"
    "  uvec3 v = uvec3 (pos * factor + vec3 (0.5));"
    "  uint p;"
    "  p = 0xFF000000 & (v.x << 24) |"
    "      0x00FF0000 & (v.y << 16) |"
    "      0x0000FF00 & (v.z << 8);"
    "  return p;"               /* 8 bits left unused :( */
    "}"
    "\n#endif\n"
    "\n#if !SCE_VRENDER_HIGHP_VERTEX_NOR\n"
    "uint encode_nor (vec3 nor)"
    "{"
    "  uvec3 v = uvec3 ((nor + vec3 (1.0)) * 127.0);"
    "  uint p;"
    "  p = (0xFF000000 & (v.x << 24)) |"
    "      (0x00FF0000 & (v.y << 16)) |"
    "      (0x0000FF00 & (v.z << 8));"
    "  return p;"               /* 8 bits left unused :( */
    "}"
    "\n#endif\n"

    "void main (void)"
    "{"
       /* extracting texture coordinates */
    "  uint xyz8 = sce_position;"
    "  uvec3 utc;"
    "  utc = uvec3 ((xyz8 >> 24) & 0xFF,"
    "               (xyz8 >> 16) & 0xFF,"
    "               (xyz8 >> 8)  & 0xFF);"
    "  vec3 p = (vec3 (utc) + vec3 (0.5)) * vec3 (OW, OH, OD);"
    "  vec3 tc = p + offset;"
       /* texture fetch */
    "  float p0, p1, p3, p6, p7;" /* corners */
    "  p0 = texture3D (sce_tex0, tc).x;"
    "  p1 = texture3D (sce_tex0, tc + vec3 (OW, 0., 0.)).x;"
    "  p3 = texture3D (sce_tex0, tc + vec3 (0., 0., OD)).x;"
    "  p6 = texture3D (sce_tex0, tc + vec3 (OW, OH, 0.)).x;"
    "  p7 = texture3D (sce_tex0, tc + vec3 (0., OH, 0.)).x;"
       /* corners */
    /* TODO: - 0.5 ? aren't vertices divided by 2 ? */
    "  vec4 corners[5] = {"
    "    vec4 (p,                     p0 - 0.5),"
    "    vec4 (p + vec3 (OW, 0., 0.), p1 - 0.5),"
    "    vec4 (p + vec3 (0., 0., OD), p3 - 0.5),"
    "    vec4 (p + vec3 (OW, OH, 0.), p6 - 0.5),"
    "    vec4 (p + vec3 (0., OH, 0.), p7 - 0.5)"
    "  };"
       /* edges */
    "  int edges[14] = {"
    "    0, 3,"
    "    0, 4,"
    "    0, 1,"
    "    0, 2,"
    "    2, 4,"
    "    2, 1,"
    "    2, 3"
    "  };"
       /* vertex extraction */
    "  uint edge = 0xFF & sce_position;"
    "  vec4 c1 = corners[edges[edge * 2 + 0]];"
    "  vec4 c2 = corners[edges[edge * 2 + 1]];"
    "  float w = -c1.w / (c2.w - c1.w);"
    "  vec3 position;"
    "  position = c1.xyz * (1.0 - w) + c2.xyz * w;"

    "\n#if SCE_VRENDER_HIGHP_VERTEX_POS\n"
    "  pos = vec4 (position, 1.0);"
    "\n#else\n"
    "  pos = encode_pos (position);"
    "\n#endif\n"

    "  tc = position + offset;"

       /* normal generation */
    "  vec3 grad = vec3 (0.0);"
#if 0
#if 0
    "  const vec3 kernel[28] = {"
    "    vec3 (-1.0, -1.0, -1.0), vec3 (0.0, -1.0, -1.0), vec3 (1.0, -1.0, -1.0),"
    "    vec3 (-1.0,  0.0, -1.0), vec3 (0.0,  0.0, -1.0), vec3 (1.0,  0.0, -1.0),"
    "    vec3 (-1.0,  1.0, -1.0), vec3 (0.0,  1.0, -1.0), vec3 (1.0,  1.0, -1.0),"

    "    vec3 (-1.0, -1.0, 0.0), vec3 (0.0, -1.0, 0.0), vec3 (1.0, -1.0, 0.0),"
    "    vec3 (-1.0,  0.0, 0.0), vec3 (0.0,  0.0, 0.0), vec3 (1.0,  0.0, 0.0),"
    "    vec3 (-1.0,  1.0, 0.0), vec3 (0.0,  1.0, 0.0), vec3 (1.0,  1.0, 0.0),"

    "    vec3 (-1.0, -1.0,  1.0), vec3 (0.0, -1.0,  1.0), vec3 (1.0, -1.0,  1.0),"
    "    vec3 (-1.0,  0.0,  1.0), vec3 (0.0,  0.0,  1.0), vec3 (1.0,  0.0,  1.0),"
    "    vec3 (-1.0,  1.0,  1.0), vec3 (0.0,  1.0,  1.0), vec3 (1.0,  1.0,  1.0)"
    "  };"
#else
    "  const vec3 kernel[28] = {"
    "    vec3 (-0.5, -0.5, -0.5), vec3 (0.0, -0.7, -0.7), vec3 (0.5, -0.5, -0.5),"
    "    vec3 (-0.7,  0.0, -0.7), vec3 (0.0,  0.0, -1.0), vec3 (0.7,  0.0, -0.7),"
    "    vec3 (-0.5,  0.5, -0.5), vec3 (0.0,  0.7, -0.7), vec3 (0.5,  0.5, -0.5),"

    "    vec3 (-0.7, -0.7, 0.0), vec3 (0.0, -1.0, 0.0), vec3 (0.7, -0.7, 0.0),"
    "    vec3 (-1.0,  0.0, 0.0), vec3 (0.0,  0.0, 0.0), vec3 (1.0,  0.0, 0.0),"
    "    vec3 (-0.7,  0.7, 0.0), vec3 (0.0,  1.0, 0.0), vec3 (0.7,  0.7, 0.0),"

    "    vec3 (-0.5, -0.5, 0.5), vec3 (0.0, -0.7, 0.7), vec3 (0.5, -0.5, 0.5),"
    "    vec3 (-0.7,  0.0, 0.7), vec3 (0.0,  0.0, 1.0), vec3 (0.7,  0.0, 0.7),"
    "    vec3 (-0.5,  0.5, 0.5), vec3 (0.0,  0.7, 0.7), vec3 (0.5,  0.5, 0.5)"
    "  };"
#endif
    "  int i;"

    "  for (int i = 0; i < 28; i++) {"
    "    grad += kernel[i] *"
    "            texture3D (sce_tex0, tc + vec3 (OW, OH, OD) * kernel[i]).x;"
    "  }"
#else
    "  grad.x = texture3D (sce_tex0, tc + vec3 (OW, 0., 0.)).x -"
    "           texture3D (sce_tex0, tc + vec3 (-OW, 0., 0.)).x;"
    "  grad.y = texture3D (sce_tex0, tc + vec3 (0., OH, 0.)).x -"
    "           texture3D (sce_tex0, tc + vec3 (0., -OH, 0.)).x;"
    "  grad.z = texture3D (sce_tex0, tc + vec3 (0., 0., OD)).x -"
    "           texture3D (sce_tex0, tc + vec3 (0., 0., -OD)).x;"
#endif

    "\n#if SCE_VRENDER_HIGHP_VERTEX_NOR\n"
    "  nor = -normalize (grad);"
    "\n#else\n"
    "  nor = encode_nor (-normalize (grad));"
    "\n#endif\n"
    "}";

static const char *mc_final_vs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "in uint sce_position;"
    "\n#if SCE_VRENDER_HIGHP_VERTEX_POS\n"
    "out vec4 pos;"
    "\n#else\n"
    "out uint pos;"
    "\n#endif\n"

    "\n#if SCE_VRENDER_HIGHP_VERTEX_NOR\n"
    "out vec3 nor;"
    "\n#else\n"
    "out uint nor;"
    "\n#endif\n"

    "uniform sampler3D sce_tex0;"
    "uniform vec3 offset;"
    "uniform float comp_scale;"

    "\n#if !SCE_VRENDER_HIGHP_VERTEX_POS\n"
    "uint encode_pos (vec3 pos)"
    "{"
    "  const float factor = 256.0 * comp_scale;"
    "  uvec3 v = uvec3 (pos * factor + vec3 (0.5));"
    "  uint p;"
    "  p = 0xFF000000 & (v.x << 24) |"
    "      0x00FF0000 & (v.y << 16) |"
    "      0x0000FF00 & (v.z << 8);"
    "  return p;"               /* 8 bits left unused :( */
    "}"
    "\n#endif\n"
    "\n#if !SCE_VRENDER_HIGHP_VERTEX_NOR\n"
    "uint encode_nor (vec3 nor)"
    "{"
    "  uvec3 v = uvec3 ((nor + vec3 (1.0)) * 127.0);"
    "  uint p;"
    "  p = (0xFF000000 & (v.x << 24)) |"
    "      (0x00FF0000 & (v.y << 16)) |"
    "      (0x0000FF00 & (v.z << 8));"
    "  return p;"               /* 8 bits left unused :( */
    "}"
    "\n#endif\n"

    "void main (void)"
    "{"
       /* extracting texture coordinates */
    "  uint xyz8 = sce_position;"
    "  uvec3 utc;"
    "  utc = uvec3 ((xyz8 >> 24) & 0xFF,"
    "               (xyz8 >> 16) & 0xFF,"
    "               (xyz8 >> 8)  & 0xFF);"
    "  vec3 p = (vec3 (utc) + vec3 (0.5)) * vec3 (OW, OH, OD);"
    "  vec3 tc = p + offset;"
       /* texture fetch */
    "  float p0, p2, p3, p7;" /* corners */
    "  p3 = texture3D (sce_tex0, tc).x;"
    "  p2 = texture3D (sce_tex0, tc + vec3 (OW, 0., 0.)).x;"
    "  p7 = texture3D (sce_tex0, tc + vec3 (0., 0., OD)).x;"
    "  p0 = texture3D (sce_tex0, tc + vec3 (0., OH, 0.)).x;"
       /* corners */
    /* TODO: - 0.5 ? aren't vertices divided by 2 ? */
    "  vec4 corners[4] = {"
    "    vec4 (p,                     p3 - 0.5),"
    "    vec4 (p + vec3 (0., OH, 0.), p0 - 0.5),"
    "    vec4 (p + vec3 (OW, 0., 0.), p2 - 0.5),"
    "    vec4 (p + vec3 (0., 0., OD), p7 - 0.5)"
    "  };"
       /* vertex extraction
        *
        *  0 y   z 7
        *     \  |
        *     0\ |2
        *       \|____ x 2
        *           1
        */
    "  vec4 c1 = corners[0];"
    "  vec4 c2 = corners[1 + (0xFF & sce_position)];"
    "  float w = -c1.w / (c2.w - c1.w);"
    "  vec3 position;"
    "  position = c1.xyz * (1.0 - w) + c2.xyz * w;"

    "\n#if SCE_VRENDER_HIGHP_VERTEX_POS\n"
    "  pos = vec4 (position, 1.0);"
    "\n#else\n"
    "  pos = encode_pos (position);"
    "\n#endif\n"

    "  tc = position + offset;"

       /* normal generation */
    "  vec3 grad = vec3 (0.0);"
    "  grad.x = texture3D (sce_tex0, tc + vec3 (OW, 0., 0.)).x -"
    "           texture3D (sce_tex0, tc + vec3 (-OW, 0., 0.)).x;"
    "  grad.y = texture3D (sce_tex0, tc + vec3 (0., OH, 0.)).x -"
    "           texture3D (sce_tex0, tc + vec3 (0., -OH, 0.)).x;"
    "  grad.z = texture3D (sce_tex0, tc + vec3 (0., 0., OD)).x -"
    "           texture3D (sce_tex0, tc + vec3 (0., 0., -OD)).x;"
    "\n#if SCE_VRENDER_HIGHP_VERTEX_NOR\n"
    "  nor = -normalize (grad);"
    "\n#else\n"
    "  nor = encode_nor (-normalize (grad));"
    "\n#endif\n"
    "}";


static const char *final_gs =
    "layout (points, max_vertices = 1) in;"
    "layout (points, max_vertices = 1) out;"

    "\n#if SCE_VRENDER_HIGHP_VERTEX_POS\n"
    "in vec4 pos[1];"
    "out vec4 pos_;"
    "\n#else\n"
    "in uint pos[1];"
    "out uint pos_;"
    "\n#endif\n"
    "\n#if SCE_VRENDER_HIGHP_VERTEX_NOR\n"
    "in vec3 nor[1];"
    "out vec3 nor_;"
    "\n#else\n"
    "in uint nor[1];"
    "out uint nor_;"
    "\n#endif\n"

    "void main (void)"
    "{"
    "  pos_ = pos[0];"
    "  nor_ = nor[0];"
    "  EmitVertex ();"
    "  EndPrimitive ();"
    "}";



static const char *mt_splat_vs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "in uint sce_position;"
    "out vec2 pos;"
    "out int depth;"
    "out int vertex_id;"

    "void main (void)"
    "{"
       /* extracting position */
    "  uint xyz8 = sce_position;"
    "  vec2 ip;"
    "  ip = vec2 ((xyz8 >> 24) & 0xFF, (xyz8 >> 16) & 0xFF);"
    "  vec2 p = vec2 (ip) * vec2 (OW, OH);"
       /* extracting edge; and thus offset */
    "  int edge = int (xyz8 & 0xFF);"
    "  float edgef = float (edge) * OW / 8.0;"

    "  pos = 2.0 * vec2 (p.x + edgef, p.y) - vec2 (1.0, 1.0);"
    "  depth = int((xyz8 >> 8) & 0xFF);"
    "  vertex_id = gl_VertexID;"
    "}";

/* s/8.0/4.0/ */
static const char *mc_splat_vs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "in uint sce_position;"
    "out vec2 pos;"
    "out int depth;"
    "out int vertex_id;"

    "void main (void)"
    "{"
       /* extracting position */
    "  uint xyz8 = sce_position;"
    "  vec2 ip;"
    "  ip = vec2 ((xyz8 >> 24) & 0xFF, (xyz8 >> 16) & 0xFF);"
    "  vec2 p = vec2 (ip) * vec2 (OW, OH);"
       /* extracting edge; and thus offset */
    "  int edge = int (xyz8 & 0xFF);"
    "  float edgef = float (edge) * OW / 4.0;"

    "  pos = 2.0 * vec2 (p.x + edgef, p.y) - vec2 (1.0, 1.0);"
    "  depth = int((xyz8 >> 8) & 0xFF);"
    "  vertex_id = gl_VertexID;"
    "}";

static const char *splat_gs =
    "\n#extension ARB_draw_buffers : require\n"
    "#extension EXT_gpu_shader4 : require\n"
    "#extension EXT_geometry_shader4 : require\n"

    "layout (points, max_vertices = 1) in;"
    "layout (points, max_vertices = 1) out;"

    "in vec2 pos[1];"
    "in int depth[1];"
    "in int vertex_id[1];"
    "out uint vid;"

    "void main (void)"
    "{"
    "  vid = uint (vertex_id[0]);"
    "  gl_Layer = depth[0];"
    "  gl_Position = vec4 (pos[0], 0.0, 1.0);"
    "  EmitVertex ();"
    "  EndPrimitive ();"
    "}";

static const char *splat_ps =
    "in uint vid;"
    "out uint fragdata;"

    "void main (void)"
    "{"
    "  fragdata = vid;"
    "}";


static const char *indices_vs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "in uint sce_position;"     /* xyz8_case8 */
    "out vec3 pos;"             /* coordinates */
    "out uint case8;"           /* case */

    "void main (void)"
    "{"
    "  uvec3 xyz = uvec3 ((sce_position & 0xFF000000) >> 24,"
    "                     (sce_position & 0x00FF0000) >> 16,"
    "                     (sce_position & 0x0000FF00) >> 8);"
    "  case8 = sce_position & 0x000000FF;"
    "  pos = vec3 (xyz) * vec3 (OW, OH, OD);"
    "}";

static const char *mt_indices_gs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "layout (points, max_vertices = 1) in;"
    "layout (points, max_vertices = 12) out;"

    "in vec3 pos[1];"
    "in uint case8[1];"
    "out uvec3 index;"

    "uniform usampler3D sce_tex0;"

    /*                + 0
                     /|\
                    / | \
                   /  |  \
                  1   2   0
                 /    |    \
                /     |     \
               +---4--|------+ 1
              3 \     |     /
                 \    |    /
                  5   |   3
                   \  |  /
                    \ | /
                     \|/
                      + 2
    */
    "\n#define N_CASES (16)\n"

    /* lookup tables */

    /* number of polygons, false indicates one and true indicates two */
    "const bool lt_triangles_count[N_CASES] = {"
    "    false, false, false, true, false, true, true, false, false, true,"
    "    true, false, true, false, false, false"
    "};"

    /* frontfacing triangles are clockwise */
    "const int lt_triangles[N_CASES * 4] = {"
                                      // 3210 vertex index
    "    0, 0, 0, 0,"                 // 0000
    "    0, 1, 2, 0,"                 // 0001
    "    0, 3, 4, 0,"                 // 0010
    "    1, 2, 3, 4,"                 // 0011
    "    2, 5, 3, 0,"                 // 0100
    "    0, 1, 5, 3,"                 // 0101
    "    0, 2, 5, 4,"                 // 0110
    "    1, 5, 4, 0,"                 // 0111
    "    1, 4, 5, 0,"                 // 1000
    "    0, 4, 5, 2,"                 // 1001
    "    0, 3, 5, 1,"                 // 1010
    "    2, 3, 5, 0,"                 // 1011
    "    1, 4, 3, 2,"                 // 1100
    "    0, 4, 3, 0,"                 // 1101
    "    0, 2, 1, 0,"                 // 1110
    "    0, 0, 0, 0"                  // 1111
    "};"

    "uint get_edge (in vec3 coord, in float edge)"
    "{"
    "  vec3 c = coord;"
    "  c.x += (edge + 0.5) * OW / 8.0;"
    "  uvec4 v = texture (sce_tex0, c);"
    "  return v.x;"
    "}"

    "void get_edges (out uint edges[19])"
    "{"
    "  vec3 t = pos[0];"

    "  edges[0] = get_edge (t, 0.0);"
    "  edges[1] = get_edge (t, 1.0);"
    "  edges[2] = get_edge (t, 2.0);"
    "  edges[3] = get_edge (t, 3.0);"
    "  edges[4] = get_edge (t, 4.0);"
    "  edges[5] = get_edge (t, 5.0);"
    "  edges[6] = get_edge (t, 6.0);"

    "  t = pos[0] + vec3 (OW, 0.0, 0.0);"
    "  edges[7] = get_edge (t, 1.0);"
    "  edges[8] = get_edge (t, 3.0);"
    "  edges[9] = get_edge (t, 4.0);"

    "  t = pos[0] + vec3 (OW, 0.0, OD);"
    "  edges[10] = get_edge (t, 1.0);"

    "  t = pos[0] + vec3 (OW, OH, 0.0);"
    "  edges[11] = get_edge (t, 3.0);"

    "  t = pos[0] + vec3 (0.0, OH, 0.0);"
    "  edges[12] = get_edge (t, 2.0);"
    "  edges[13] = get_edge (t, 3.0);"
    "  edges[14] = get_edge (t, 5.0);"

    "  t = pos[0] + vec3 (0.0, OH, OD);"
    "  edges[15] = get_edge (t, 2.0);"

    "  t = pos[0] + vec3 (0.0, 0.0, OD);"
    "  edges[16] = get_edge (t, 0.0);"
    "  edges[17] = get_edge (t, 1.0);"
    "  edges[18] = get_edge (t, 2.0);"
    "}"

    /* given a couple of vertices, gives the index of the associated edge
       in the cube */
    "const int lt_cube_edges[8 * 8] = {"
    "  -1,  2, -1,  3, -1, -1,  0,  1,"
    "   2, -1,  8,  5, -1, -1,  7, -1,"
    "  -1,  8, -1, 18, -1, 10,  9, -1,"
    "   3,  5, 18, -1, 17, 16,  6,  4,"
    "  -1, -1, -1, 17, -1, 15, 14, 13,"
    "  -1, -1, 10, 16, 15, -1, 11, -1,"
    "   0,  7,  9,  6, 14, 11, -1, 12,"
    "   1, -1, -1,  4, 13, -1, 12, -1"
    "};"

    /* look up table that constructs a tetrahedron given its ID and
       returning the indices of the corners forming the tetrahedron */
    "const int lt_vertices[6 * 4] = {"
    "  3, 6, 7, 0,"
    "  3, 4, 7, 6,"
    "  3, 5, 4, 6,"
    "  3, 2, 5, 6,"
    "  3, 1, 2, 6,"
    "  3, 0, 1, 6"
    "};"

    "void output_tetrahedron (uint c, in uint edges[6])"
    "{"
    "  if (c > 0 && c < 15) {"
    "    index.x = edges[lt_triangles[c * 4 + 0]];"
    "    index.y = edges[lt_triangles[c * 4 + 1]];"
    "    index.z = edges[lt_triangles[c * 4 + 2]];"
    "    EmitVertex ();"
    "    EndPrimitive ();"

    "    if (lt_triangles_count[c]) {"
    "      index.x = edges[lt_triangles[c * 4 + 2]];"
    "      index.y = edges[lt_triangles[c * 4 + 3]];"
    "      index.z = edges[lt_triangles[c * 4 + 0]];"
    "      EmitVertex ();"
    "      EndPrimitive ();"
    "    }"
    "  }"
    "}"

    "void output_indices (in uint edges[19])"
    "{"
    "  int i;"
    "  uint tetra[6];"

       /* for each tetrahedron in the cube */
    "  for (i = 0; i < 6; i++) {"
         /* retrieve the 6 edges of the tetrahedron */
    "    int v1, v2;"
    "    v1 = lt_vertices[i * 4 + 0]; v2 = lt_vertices[i * 4 + 1];"
    "    tetra[0] = edges[lt_cube_edges[v1 * 8 + v2]];"
    "    v1 = lt_vertices[i * 4 + 0]; v2 = lt_vertices[i * 4 + 3];"
    "    tetra[1] = edges[lt_cube_edges[v1 * 8 + v2]];"
    "    v1 = lt_vertices[i * 4 + 0]; v2 = lt_vertices[i * 4 + 2];"
    "    tetra[2] = edges[lt_cube_edges[v1 * 8 + v2]];"
    "    v1 = lt_vertices[i * 4 + 1]; v2 = lt_vertices[i * 4 + 2];"
    "    tetra[3] = edges[lt_cube_edges[v1 * 8 + v2]];"
    "    v1 = lt_vertices[i * 4 + 1]; v2 = lt_vertices[i * 4 + 3];"
    "    tetra[4] = edges[lt_cube_edges[v1 * 8 + v2]];"
    "    v1 = lt_vertices[i * 4 + 2]; v2 = lt_vertices[i * 4 + 3];"
    "    tetra[5] = edges[lt_cube_edges[v1 * 8 + v2]];"
         /* retrieve case */
    "    uint c = 1 & (case8[0] >> lt_vertices[i * 4 + 0]);"
    "    c |= (1 & (case8[0] >> lt_vertices[i * 4 + 1])) << 1;"
    "    c |= (1 & (case8[0] >> lt_vertices[i * 4 + 2])) << 2;"
    "    c |= (1 & (case8[0] >> lt_vertices[i * 4 + 3])) << 3;"
         /* emit tetrahedron */
    "    output_tetrahedron (c, tetra);"
    "  }"
    "}"


    "void main (void)"
    "{"
    "  if (pos[0].x < MAXW - OW &&"
    "      pos[0].y < MAXH - OH &&"
    "      pos[0].z < MAXD - OD)"
    "  {"
    "    uint edges[19];"
    "    get_edges (edges);"
    "    output_indices (edges);"
    "  }"
    "}";



static const char *mc_indices_gs =
    "#define OW (1.0/W)\n"
    "#define OH (1.0/H)\n"
    "#define OD (1.0/D)\n"

    "layout (points, max_vertices = 1) in;"
    "layout (points, max_vertices = 5) out;"

    "in vec3 pos[1];"
    "in uint case8[1];"
    "out uvec3 index;"

    "uniform usampler3D sce_tex0;"
    "uniform usampler1D sce_tex1;"

    /* lookup tables (achtung) */
    "const int lt_num_tri[256] = {"
    "  0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 2, 1, 2, 2, 3, 2, 3, 3, 4,"
    "  2, 3, 3, 4, 3, 4, 4, 3, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,"
    "  2, 3, 3, 2, 3, 4, 4, 3, 3, 4, 4, 3, 4, 5, 5, 2, 1, 2, 2, 3, 2, 3, 3, 4,"
    "  2, 3, 3, 4, 3, 4, 4, 3, 2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 4,"
    "  2, 3, 3, 4, 3, 4, 2, 3, 3, 4, 4, 5, 4, 5, 3, 2, 3, 4, 4, 3, 4, 5, 3, 2,"
    "  4, 5, 5, 4, 5, 2, 4, 1, 1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 3,"
    "  2, 3, 3, 4, 3, 4, 4, 5, 3, 2, 4, 3, 4, 3, 5, 2, 2, 3, 3, 4, 3, 4, 4, 5,"
    "  3, 4, 4, 5, 4, 5, 5, 4, 3, 4, 4, 3, 4, 5, 5, 4, 4, 3, 5, 2, 5, 4, 2, 1,"
    "  2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 2, 3, 3, 2, 3, 4, 4, 5, 4, 5, 5, 2,"
    "  4, 3, 5, 4, 3, 2, 4, 1, 3, 4, 4, 5, 4, 5, 3, 4, 4, 5, 5, 2, 3, 4, 2, 1,"
    "  2, 3, 3, 2, 3, 4, 2, 1, 3, 2, 4, 1, 2, 1, 1, 0"
    "};"

#define SCE_TEXTURE_LT

#ifndef SCE_TEXTURE_LT
    "const int lt_edges[254 * 15] = {"
    "  0, 8, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 1, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 8, 3, 9, 8, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 2, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 8, 3, 1, 2, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  9, 2, 10, 0, 2, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  2, 8, 3, 2, 10, 8, 10, 9, 8, 0, 0, 0, 0, 0, 0,"
    "  3, 11, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 11, 2, 8, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 9, 0, 2, 3, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 11, 2, 1, 9, 11, 9, 8, 11, 0, 0, 0, 0, 0, 0,"
    "  3, 10, 1, 11, 10, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 10, 1, 0, 8, 10, 8, 11, 10, 0, 0, 0, 0, 0, 0,"
    "  3, 9, 0, 3, 11, 9, 11, 10, 9, 0, 0, 0, 0, 0, 0,"
    "  9, 8, 10, 10, 8, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  4, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  4, 3, 0, 7, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 1, 9, 8, 4, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  4, 1, 9, 4, 7, 1, 7, 3, 1, 0, 0, 0, 0, 0, 0,"
    "  1, 2, 10, 8, 4, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  3, 4, 7, 3, 0, 4, 1, 2, 10, 0, 0, 0, 0, 0, 0,"
    "  9, 2, 10, 9, 0, 2, 8, 4, 7, 0, 0, 0, 0, 0, 0,"
    "  2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, 0, 0, 0,"
    "  8, 4, 7, 3, 11, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  11, 4, 7, 11, 2, 4, 2, 0, 4, 0, 0, 0, 0, 0, 0,"
    "  9, 0, 1, 8, 4, 7, 2, 3, 11, 0, 0, 0, 0, 0, 0,"
    "  4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, 0, 0, 0,"
    "  3, 10, 1, 3, 11, 10, 7, 8, 4, 0, 0, 0, 0, 0, 0,"
    "  1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, 0, 0, 0,"
    "  4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, 0, 0, 0,"
    "  4, 7, 11, 4, 11, 9, 9, 11, 10, 0, 0, 0, 0, 0, 0,"
    "  9, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  9, 5, 4, 0, 8, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 5, 4, 1, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  8, 5, 4, 8, 3, 5, 3, 1, 5, 0, 0, 0, 0, 0, 0,"
    "  1, 2, 10, 9, 5, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  3, 0, 8, 1, 2, 10, 4, 9, 5, 0, 0, 0, 0, 0, 0,"
    "  5, 2, 10, 5, 4, 2, 4, 0, 2, 0, 0, 0, 0, 0, 0,"
    "  2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, 0, 0, 0,"
    "  9, 5, 4, 2, 3, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 11, 2, 0, 8, 11, 4, 9, 5, 0, 0, 0, 0, 0, 0,"
    "  0, 5, 4, 0, 1, 5, 2, 3, 11, 0, 0, 0, 0, 0, 0,"
    "  2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, 0, 0, 0,"
    "  10, 3, 11, 10, 1, 3, 9, 5, 4, 0, 0, 0, 0, 0, 0,"
    "  4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, 0, 0, 0,"
    "  5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, 0, 0, 0,"
    "  5, 4, 8, 5, 8, 10, 10, 8, 11, 0, 0, 0, 0, 0, 0,"
    "  9, 7, 8, 5, 7, 9, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  9, 3, 0, 9, 5, 3, 5, 7, 3, 0, 0, 0, 0, 0, 0,"
    "  0, 7, 8, 0, 1, 7, 1, 5, 7, 0, 0, 0, 0, 0, 0,"
    "  1, 5, 3, 3, 5, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  9, 7, 8, 9, 5, 7, 10, 1, 2, 0, 0, 0, 0, 0, 0,"
    "  10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, 0, 0, 0,"
    "  8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, 0, 0, 0,"
    "  2, 10, 5, 2, 5, 3, 3, 5, 7, 0, 0, 0, 0, 0, 0,"
    "  7, 9, 5, 7, 8, 9, 3, 11, 2, 0, 0, 0, 0, 0, 0,"
    "  9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, 0, 0, 0,"
    "  2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, 0, 0, 0,"
    "  11, 2, 1, 11, 1, 7, 7, 1, 5, 0, 0, 0, 0, 0, 0,"
    "  9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, 0, 0, 0,"
    "  5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0,"
    "  11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0,"
    "  11, 10, 5, 7, 11, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  10, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 8, 3, 5, 10, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  9, 0, 1, 5, 10, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 8, 3, 1, 9, 8, 5, 10, 6, 0, 0, 0, 0, 0, 0,"
    "  1, 6, 5, 2, 6, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 6, 5, 1, 2, 6, 3, 0, 8, 0, 0, 0, 0, 0, 0,"
    "  9, 6, 5, 9, 0, 6, 0, 2, 6, 0, 0, 0, 0, 0, 0,"
    "  5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, 0, 0, 0,"
    "  2, 3, 11, 10, 6, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  11, 0, 8, 11, 2, 0, 10, 6, 5, 0, 0, 0, 0, 0, 0,"
    "  0, 1, 9, 2, 3, 11, 5, 10, 6, 0, 0, 0, 0, 0, 0,"
    "  5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, 0, 0, 0,"
    "  6, 3, 11, 6, 5, 3, 5, 1, 3, 0, 0, 0, 0, 0, 0,"
    "  0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, 0, 0, 0,"
    "  3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, 0, 0, 0,"
    "  6, 5, 9, 6, 9, 11, 11, 9, 8, 0, 0, 0, 0, 0, 0,"
    "  5, 10, 6, 4, 7, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  4, 3, 0, 4, 7, 3, 6, 5, 10, 0, 0, 0, 0, 0, 0,"
    "  1, 9, 0, 5, 10, 6, 8, 4, 7, 0, 0, 0, 0, 0, 0,"
    "  10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, 0, 0, 0,"
    "  6, 1, 2, 6, 5, 1, 4, 7, 8, 0, 0, 0, 0, 0, 0,"
    "  1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, 0, 0, 0,"
    "  8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, 0, 0, 0,"
    "  7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9,"
    "  3, 11, 2, 7, 8, 4, 10, 6, 5, 0, 0, 0, 0, 0, 0,"
    "  5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, 0, 0, 0,"
    "  0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, 0, 0, 0,"
    "  9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6,"
    "  8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, 0, 0, 0,"
    "  5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11,"
    "  0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7,"
    "  6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, 0, 0, 0,"
    "  10, 4, 9, 6, 4, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  4, 10, 6, 4, 9, 10, 0, 8, 3, 0, 0, 0, 0, 0, 0,"
    "  10, 0, 1, 10, 6, 0, 6, 4, 0, 0, 0, 0, 0, 0, 0,"
    "  8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, 0, 0, 0,"
    "  1, 4, 9, 1, 2, 4, 2, 6, 4, 0, 0, 0, 0, 0, 0,"
    "  3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, 0, 0, 0,"
    "  0, 2, 4, 4, 2, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  8, 3, 2, 8, 2, 4, 4, 2, 6, 0, 0, 0, 0, 0, 0,"
    "  10, 4, 9, 10, 6, 4, 11, 2, 3, 0, 0, 0, 0, 0, 0,"
    "  0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, 0, 0, 0,"
    "  3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, 0, 0, 0,"
    "  6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1,"
    "  9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, 0, 0, 0,"
    "  8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1,"
    "  3, 11, 6, 3, 6, 0, 0, 6, 4, 0, 0, 0, 0, 0, 0,"
    "  6, 4, 8, 11, 6, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  7, 10, 6, 7, 8, 10, 8, 9, 10, 0, 0, 0, 0, 0, 0,"
    "  0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, 0, 0, 0,"
    "  10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, 0, 0, 0,"
    "  10, 6, 7, 10, 7, 1, 1, 7, 3, 0, 0, 0, 0, 0, 0,"
    "  1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, 0, 0, 0,"
    "  2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9,"
    "  7, 8, 0, 7, 0, 6, 6, 0, 2, 0, 0, 0, 0, 0, 0,"
    "  7, 3, 2, 6, 7, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, 0, 0, 0,"
    "  2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7,"
    "  1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11,"
    "  11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, 0, 0, 0,"
    "  8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6,"
    "  0, 9, 1, 11, 6, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, 0, 0, 0,"
    "  7, 11, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  7, 6, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  3, 0, 8, 11, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 1, 9, 11, 7, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  8, 1, 9, 8, 3, 1, 11, 7, 6, 0, 0, 0, 0, 0, 0,"
    "  10, 1, 2, 6, 11, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 2, 10, 3, 0, 8, 6, 11, 7, 0, 0, 0, 0, 0, 0,"
    "  2, 9, 0, 2, 10, 9, 6, 11, 7, 0, 0, 0, 0, 0, 0,"
    "  6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, 0, 0, 0,"
    "  7, 2, 3, 6, 2, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  7, 0, 8, 7, 6, 0, 6, 2, 0, 0, 0, 0, 0, 0, 0,"
    "  2, 7, 6, 2, 3, 7, 0, 1, 9, 0, 0, 0, 0, 0, 0,"
    "  1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, 0, 0, 0,"
    "  10, 7, 6, 10, 1, 7, 1, 3, 7, 0, 0, 0, 0, 0, 0,"
    "  10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, 0, 0, 0,"
    "  0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, 0, 0, 0,"
    "  7, 6, 10, 7, 10, 8, 8, 10, 9, 0, 0, 0, 0, 0, 0,"
    "  6, 8, 4, 11, 8, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  3, 6, 11, 3, 0, 6, 0, 4, 6, 0, 0, 0, 0, 0, 0,"
    "  8, 6, 11, 8, 4, 6, 9, 0, 1, 0, 0, 0, 0, 0, 0,"
    "  9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, 0, 0, 0,"
    "  6, 8, 4, 6, 11, 8, 2, 10, 1, 0, 0, 0, 0, 0, 0,"
    "  1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, 0, 0, 0,"
    "  4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, 0, 0, 0,"
    "  10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3,"
    "  8, 2, 3, 8, 4, 2, 4, 6, 2, 0, 0, 0, 0, 0, 0,"
    "  0, 4, 2, 4, 6, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, 0, 0, 0,"
    "  1, 9, 4, 1, 4, 2, 2, 4, 6, 0, 0, 0, 0, 0, 0,"
    "  8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, 0, 0, 0,"
    "  10, 1, 0, 10, 0, 6, 6, 0, 4, 0, 0, 0, 0, 0, 0,"
    "  4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3,"
    "  10, 9, 4, 6, 10, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  4, 9, 5, 7, 6, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 8, 3, 4, 9, 5, 11, 7, 6, 0, 0, 0, 0, 0, 0,"
    "  5, 0, 1, 5, 4, 0, 7, 6, 11, 0, 0, 0, 0, 0, 0,"
    "  11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, 0, 0, 0,"
    "  9, 5, 4, 10, 1, 2, 7, 6, 11, 0, 0, 0, 0, 0, 0,"
    "  6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, 0, 0, 0,"
    "  7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, 0, 0, 0,"
    "  3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6,"
    "  7, 2, 3, 7, 6, 2, 5, 4, 9, 0, 0, 0, 0, 0, 0,"
    "  9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, 0, 0, 0,"
    "  3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, 0, 0, 0,"
    "  6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8,"
    "  9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, 0, 0, 0,"
    "  1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4,"
    "  4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10,"
    "  7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, 0, 0, 0,"
    "  6, 9, 5, 6, 11, 9, 11, 8, 9, 0, 0, 0, 0, 0, 0,"
    "  3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, 0, 0, 0,"
    "  0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, 0, 0, 0,"
    "  6, 11, 3, 6, 3, 5, 5, 3, 1, 0, 0, 0, 0, 0, 0,"
    "  1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, 0, 0, 0,"
    "  0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10,"
    "  11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5,"
    "  6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, 0, 0, 0,"
    "  5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, 0, 0, 0,"
    "  9, 5, 6, 9, 6, 0, 0, 6, 2, 0, 0, 0, 0, 0, 0,"
    "  1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8,"
    "  1, 5, 6, 2, 1, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6,"
    "  10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, 0, 0, 0,"
    "  0, 3, 8, 5, 6, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  10, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  11, 5, 10, 7, 5, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  11, 5, 10, 11, 7, 5, 8, 3, 0, 0, 0, 0, 0, 0, 0,"
    "  5, 11, 7, 5, 10, 11, 1, 9, 0, 0, 0, 0, 0, 0, 0,"
    "  10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, 0, 0, 0,"
    "  11, 1, 2, 11, 7, 1, 7, 5, 1, 0, 0, 0, 0, 0, 0,"
    "  0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, 0, 0, 0,"
    "  9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, 0, 0, 0,"
    "  7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2,"
    "  2, 5, 10, 2, 3, 5, 3, 7, 5, 0, 0, 0, 0, 0, 0,"
    "  8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, 0, 0, 0,"
    "  9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, 0, 0, 0,"
    "  9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2,"
    "  1, 3, 5, 3, 7, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 8, 7, 0, 7, 1, 1, 7, 5, 0, 0, 0, 0, 0, 0,"
    "  9, 0, 3, 9, 3, 5, 5, 3, 7, 0, 0, 0, 0, 0, 0,"
    "  9, 8, 7, 5, 9, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  5, 8, 4, 5, 10, 8, 10, 11, 8, 0, 0, 0, 0, 0, 0,"
    "  5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, 0, 0, 0,"
    "  0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, 0, 0, 0,"
    "  10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4,"
    "  2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, 0, 0, 0,"
    "  0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11,"
    "  0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5,"
    "  9, 4, 5, 2, 11, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, 0, 0, 0,"
    "  5, 10, 2, 5, 2, 4, 4, 2, 0, 0, 0, 0, 0, 0, 0,"
    "  3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9,"
    "  5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, 0, 0, 0,"
    "  8, 4, 5, 8, 5, 3, 3, 5, 1, 0, 0, 0, 0, 0, 0,"
    "  0, 4, 5, 1, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, 0, 0, 0,"
    "  9, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  4, 11, 7, 4, 9, 11, 9, 10, 11, 0, 0, 0, 0, 0, 0,"
    "  0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, 0, 0, 0,"
    "  1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, 0, 0, 0,"
    "  3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4,"
    "  4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, 0, 0, 0,"
    "  9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3,"
    "  11, 7, 4, 11, 4, 2, 2, 4, 0, 0, 0, 0, 0, 0, 0,"
    "  11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, 0, 0, 0,"
    "  2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, 0, 0, 0,"
    "  9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7,"
    "  3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10,"
    "  1, 10, 2, 8, 7, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  4, 9, 1, 4, 1, 7, 7, 1, 3, 0, 0, 0, 0, 0, 0,"
    "  4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, 0, 0, 0,"
    "  4, 0, 3, 7, 4, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  4, 8, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  9, 10, 8, 10, 11, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  3, 0, 9, 3, 9, 11, 11, 9, 10, 0, 0, 0, 0, 0, 0,"
    "  0, 1, 10, 0, 10, 8, 8, 10, 11, 0, 0, 0, 0, 0, 0,"
    "  3, 1, 10, 11, 3, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 2, 11, 1, 11, 9, 9, 11, 8, 0, 0, 0, 0, 0, 0,"
    "  3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, 0, 0, 0,"
    "  0, 2, 11, 8, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  3, 2, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  2, 3, 8, 2, 8, 10, 10, 8, 9, 0, 0, 0, 0, 0, 0,"
    "  9, 10, 2, 0, 9, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, 0, 0, 0,"
    "  1, 10, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  1, 3, 8, 9, 1, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 9, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "  0, 3, 8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,"
    "};"
#endif

    "uint get_edge (in vec3 coord, in float edge)"
    "{"
    "  vec3 c = coord;"
    "  c.x += (edge + 0.5) * OW / 4.0;"
    "  uvec4 v = texture (sce_tex0, c);"
    "  return v.x;"
    "}"

       /* indices numbering inside 3D splat texture
        *
        *    y   z
        *     \  |
        *     0\ |2
        *       \|____ x
        *           1
        */

    "void get_edges (out uint edges[12])"
    "{"
    "  vec3 t = pos[0];"

    "  edges[3]  = get_edge (t, 0.0);"
    "  edges[2]  = get_edge (t, 1.0);"
    "  edges[11] = get_edge (t, 2.0);"

    "  t = pos[0] + vec3 (OW, 0.0, 0.0);"
    "  edges[1]  = get_edge (t, 0.0);"
    "  edges[10] = get_edge (t, 2.0);"

    "  t = pos[0] + vec3 (OW, 0.0, OD);"
    "  edges[5] = get_edge (t, 0.0);"

    "  t = pos[0] + vec3 (OW, OH, 0.0);"
    "  edges[9] = get_edge (t, 2.0);"

    "  t = pos[0] + vec3 (0.0, OH, 0.0);"
    "  edges[0] = get_edge (t, 1.0);"
    "  edges[8] = get_edge (t, 2.0);"

    "  t = pos[0] + vec3 (0.0, OH, OD);"
    "  edges[4] = get_edge (t, 1.0);"

    "  t = pos[0] + vec3 (0.0, 0.0, OD);"
    "  edges[6] = get_edge (t, 1.0);"
    "  edges[7] = get_edge (t, 0.0);"
    "}"

    "void output_indices (in uint edges[12])"
    "{"
    "  int i, n;"

    "  n = lt_num_tri[case8[0]] * 3;"
    "  for (i = 0; i < n; i += 3) {"
#ifdef SCE_TEXTURE_LT
    "    const int idx = 15 * (int (case8[0]) - 1);"
    "    uint edge0 = texelFetch (sce_tex1, idx + i + 0, 0).x;"
    "    uint edge1 = texelFetch (sce_tex1, idx + i + 1, 0).x;"
    "    uint edge2 = texelFetch (sce_tex1, idx + i + 2, 0).x;"

    "    index.x = edges[edge0];"
    "    index.y = edges[edge1];"
    "    index.z = edges[edge2];"
#else
    "    const uint idx = 15 * (case8[0] - 1);"
    "    index.x = edges[lt_edges[idx + i + 0]];"
    "    index.y = edges[lt_edges[idx + i + 1]];"
    "    index.z = edges[lt_edges[idx + i + 2]];"
#endif
    "    EmitVertex ();"
    "    EndPrimitive ();"
    "  }"
    "}"

    "void main (void)"
    "{"
    "  if (pos[0].x < MAXW - OW &&"
    "      pos[0].y < MAXH - OH &&"
    "      pos[0].z < MAXD - OD)"
    "  {"
    "    uint edges[12];"
    "    get_edges (edges);"
    "    output_indices (edges);"
    "  }"
    "}";


static int SCE_VRender_BuildHW (SCE_SVoxelTemplate *vt)
{
    SCE_SGrid grid;
    size_t n_points;
    const char *varyings[2] = {NULL, NULL};
    SCE_STexData tc;
    int width, height, depth;
    float maxw, maxh, maxd;
    unsigned int n_vertices = 0;
    unsigned int n_indices = 0;

    /* sources */
    const char *non_empty_gs = NULL;
    const char *list_verts_gs = NULL;
    const char *final_vs = NULL;
    const char *splat_vs = NULL;
    const char *indices_gs = NULL;

    /* create grid geometry and grid mesh */
    SCE_Grid_Init (&grid);
    SCE_Grid_SetDimensions (&grid, vt->width, vt->height, vt->depth);
    SCE_Grid_SetPointSize (&grid, 1);
    /* fortunately, grids do not need to be built to generate geometry */
    if (SCE_Grid_ToGeometryDiv (&grid, &vt->grid_geom,
                                vt->vwidth, vt->vheight, vt->vdepth) < 0)
        goto fail;
    if (SCE_Mesh_SetGeometry (&vt->grid_mesh, &vt->grid_geom, SCE_FALSE) < 0)
        goto fail;
    SCE_Mesh_AutoBuild (&vt->grid_mesh);
    n_points = SCE_Grid_GetNumPoints (&grid);

    if (vt->compressed_pos && vt->compressed_nor)
        vt->final_geom = &final_geom_cpos_cnor;
    else if (vt->compressed_nor)
        vt->final_geom = &final_geom_pos_cnor;
    else if (vt->compressed_pos)
        vt->final_geom = &final_geom_cpos_nor;
    else
        vt->final_geom = &final_geom_pos_nor;

    switch (vt->algo) {
    case SCE_VRENDER_MARCHING_TETRAHEDRA:
        non_empty_gs = mt_non_empty_gs;
        list_verts_gs = mt_list_verts_gs;
        final_vs = mt_final_vs;
        splat_vs = mt_splat_vs;
        indices_gs = mt_indices_gs;
        n_vertices = 7 * n_points;
        n_indices = 36 * n_points;
        break;
    case SCE_VRENDER_MARCHING_CUBES:
        non_empty_gs = mc_non_empty_gs;
        list_verts_gs = mc_list_verts_gs;
        final_vs = mc_final_vs;
        splat_vs = mc_splat_vs;
        indices_gs = mc_indices_gs;
        n_vertices = 3 * n_points;
        n_indices = 15 * n_points;
        break;
    }

    /* setup sizes */
    SCE_Geometry_SetNumVertices (&non_empty_geom, n_points);
    SCE_Geometry_SetNumVertices (&list_verts_geom, n_vertices);
    SCE_Geometry_SetNumVertices (vt->final_geom, n_vertices);
    SCE_Geometry_SetNumIndices (vt->final_geom, n_indices);

    sce_vertices_limit = n_vertices;
    sce_indices_limit = n_indices;

    /* create meshes */
    /* TODO: use Mesh_Build() and setup buffers storage mode manually */
    if (SCE_Mesh_SetGeometry (&vt->non_empty, &non_empty_geom, SCE_FALSE) < 0)
        goto fail;
    SCE_Mesh_AutoBuild (&vt->non_empty);
    if (SCE_Mesh_SetGeometry (&vt->list_verts, &list_verts_geom, SCE_FALSE) < 0)
        goto fail;
    SCE_Mesh_AutoBuild (&vt->list_verts);

    /* build shaders */
    /* non empty cells list shader */
    if (!(vt->non_empty_shader = SCE_Shader_Create ())) goto fail;
    if (SCE_Shader_AddSource (vt->non_empty_shader, SCE_VERTEX_SHADER,
                              non_empty_vs, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_AddSource (vt->non_empty_shader, SCE_GEOMETRY_SHADER,
                              non_empty_gs, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->non_empty_shader, "W", vt->vwidth) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->non_empty_shader, "H", vt->vheight) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->non_empty_shader, "D", vt->vdepth) < 0) goto fail;
    varyings[0] = "xyz8_case8";
    SCE_Shader_SetupFeedbackVaryings (vt->non_empty_shader, 1, varyings,
                                      SCE_FEEDBACK_INTERLEAVED);
    SCE_Shader_SetupAttributesMapping (vt->non_empty_shader);
    SCE_Shader_ActivateAttributesMapping (vt->non_empty_shader, SCE_TRUE);
    if (SCE_Shader_Build (vt->non_empty_shader) < 0) goto fail;
    vt->non_empty_offset_loc = SCE_Shader_GetIndex (vt->non_empty_shader,
                                                    "offset");

    /* vertices list shader */
    if (!(vt->list_verts_shader = SCE_Shader_Create ())) goto fail;
    if (SCE_Shader_AddSource (vt->list_verts_shader, SCE_VERTEX_SHADER,
                              list_verts_vs, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_AddSource (vt->list_verts_shader, SCE_GEOMETRY_SHADER,
                              list_verts_gs, SCE_FALSE) < 0) goto fail;
    varyings[0] = "xyz8_edge8";
    SCE_Shader_SetupFeedbackVaryings (vt->list_verts_shader, 1, varyings,
                                      SCE_FEEDBACK_INTERLEAVED);
    SCE_Shader_SetupAttributesMapping (vt->list_verts_shader);
    SCE_Shader_ActivateAttributesMapping (vt->list_verts_shader, SCE_TRUE);
    if (SCE_Shader_Build (vt->list_verts_shader) < 0) goto fail;

    /* generate final vertices shader */
    if (!(vt->final_shader = SCE_Shader_Create ())) goto fail;
    if (SCE_Shader_AddSource (vt->final_shader, SCE_VERTEX_SHADER,
                              final_vs, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_AddSource (vt->final_shader, SCE_GEOMETRY_SHADER,
                              final_gs, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->final_shader, "W", vt->vwidth) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->final_shader, "H", vt->vheight) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->final_shader, "D", vt->vdepth) < 0) goto fail;
    if (SCE_Shader_Globali (vt->final_shader,
                            "SCE_VRENDER_HIGHP_VERTEX_POS",
                            !vt->compressed_pos) < 0)
        goto fail;
    if (SCE_Shader_Globali (vt->final_shader,
                            "SCE_VRENDER_HIGHP_VERTEX_NOR",
                            !vt->compressed_nor) < 0)
        goto fail;
    varyings[0] = "pos_";
    varyings[1] = "nor_";
    SCE_Shader_SetupFeedbackVaryings (vt->final_shader, 2, varyings,
                                      SCE_FEEDBACK_INTERLEAVED);
    SCE_Shader_SetupAttributesMapping (vt->final_shader);
    SCE_Shader_ActivateAttributesMapping (vt->final_shader, SCE_TRUE);
    if (SCE_Shader_Build (vt->final_shader) < 0) goto fail;
    vt->final_offset_loc = SCE_Shader_GetIndex (vt->final_shader, "offset");
    vt->comp_scale_loc = SCE_Shader_GetIndex (vt->final_shader, "comp_scale");

    width = SCE_Math_NextPowerOfTwo (vt->width);
    height = SCE_Math_NextPowerOfTwo (vt->height);
    depth = SCE_Math_NextPowerOfTwo (vt->depth);
    maxw = (float)vt->width / width;
    maxh = (float)vt->height / height;
    maxd = (float)vt->depth / depth;

    /* splat vertices index shader */
    if (!(vt->splat_shader = SCE_Shader_Create ())) goto fail;
    if (SCE_Shader_AddSource (vt->splat_shader, SCE_VERTEX_SHADER,
                              splat_vs, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_AddSource (vt->splat_shader, SCE_GEOMETRY_SHADER,
                              splat_gs, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_AddSource (vt->splat_shader, SCE_PIXEL_SHADER,
                              splat_ps, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->splat_shader, "W", width) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->splat_shader, "H", height) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->splat_shader, "D", depth) < 0) goto fail;
    SCE_Shader_SetupAttributesMapping (vt->splat_shader);
    SCE_Shader_ActivateAttributesMapping (vt->splat_shader, SCE_TRUE);
    SCE_Shader_SetOutputTarget (vt->splat_shader, "fragdata",
                                SCE_COLOR_BUFFER0);
    if (SCE_Shader_Build (vt->splat_shader) < 0) goto fail;

    /* generate indices shader */
    if (!(vt->indices_shader = SCE_Shader_Create ())) goto fail;
    if (SCE_Shader_AddSource (vt->indices_shader, SCE_VERTEX_SHADER,
                              indices_vs, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_AddSource (vt->indices_shader, SCE_GEOMETRY_SHADER,
                              indices_gs, SCE_FALSE) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->indices_shader, "W", width) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->indices_shader, "H", height) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->indices_shader, "D", depth) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->indices_shader, "MAXW", maxw) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->indices_shader, "MAXH", maxh) < 0) goto fail;
    if (SCE_Shader_Globalf (vt->indices_shader, "MAXD", maxd) < 0) goto fail;
    SCE_Shader_SetVersion (vt->indices_shader, 140);
    varyings[0] = "index";
    SCE_Shader_SetupFeedbackVaryings (vt->indices_shader, 1, varyings,
                                      SCE_FEEDBACK_INTERLEAVED);
    SCE_Shader_SetupAttributesMapping (vt->indices_shader);
    SCE_Shader_ActivateAttributesMapping (vt->indices_shader, SCE_TRUE);
    if (SCE_Shader_Build (vt->indices_shader) < 0) goto fail;

    /* constructing indices 3D map */
    if (!(vt->splat = SCE_Texture_Create (SCE_TEX_3D, 0, 0, 0))) goto fail;
    SCE_TexData_Init (&tc);
    if (vt->algo == SCE_VRENDER_MARCHING_TETRAHEDRA)
        SCE_TexData_SetDimensions (&tc, width * 8, height, depth);
    else
        SCE_TexData_SetDimensions (&tc, width * 4, height, depth);
    if (n_vertices < 256 * 256) {
        SCE_TexData_SetDataType (&tc, SCE_UNSIGNED_SHORT);
        SCE_TexData_SetPixelFormat (&tc, SCE_PXF_R16UI);
    } else {
        SCE_TexData_SetDataType (&tc, SCE_UNSIGNED_INT);
        SCE_TexData_SetPixelFormat (&tc, SCE_PXF_R32UI);
    }
    SCE_TexData_SetType (&tc, SCE_IMAGE_3D);
    SCE_TexData_SetDataFormat (&tc, SCE_IMAGE_RED);
    SCE_Texture_AddTexDataDup (vt->splat, 0, &tc);
    /* index values must not be modified by magnification filter */
    SCE_Texture_Pixelize (vt->splat, SCE_TRUE);
    SCE_Texture_SetFilter (vt->splat, SCE_TEX_NEAREST);
    SCE_Texture_Build (vt->splat, SCE_FALSE);
    if (SCE_Texture_SetupFramebuffer (vt->splat, SCE_RENDER_COLOR,
                                      0, SCE_FALSE, SCE_FALSE) < 0)
        goto fail;

    if (vt->algo == SCE_VRENDER_MARCHING_CUBES) {
        /* marching cubes lookup table texture */
        if (!(vt->mc_table = SCE_Texture_Create (SCE_TEX_1D, 0, 0, 0)))
            goto fail;
        SCE_TexData_Init (&tc);
        SCE_TexData_SetDimensions (&tc, 15 * 254, 0, 0);
        SCE_TexData_SetDataType (&tc, SCE_UNSIGNED_BYTE);
        SCE_TexData_SetType (&tc, SCE_IMAGE_1D);
        SCE_TexData_SetDataFormat (&tc, SCE_IMAGE_RED);
        SCE_TexData_SetPixelFormat (&tc, SCE_PXF_R8UI);
        SCE_TexData_SetData (&tc, mc_lt_edges_pbourke, SCE_FALSE);
        SCE_Texture_AddTexDataDup (vt->mc_table, 0, &tc);
        /* index values must not be modified by magnification filter */
        SCE_Texture_Pixelize (vt->mc_table, SCE_TRUE);
        SCE_Texture_SetFilter (vt->mc_table, SCE_TEX_NEAREST);
        SCE_Texture_SetWrapMode (vt->mc_table, SCE_TEX_CLAMP);
        SCE_Texture_Build (vt->mc_table, SCE_FALSE);
        SCE_Texture_SetUnit (vt->mc_table, 1);
    }

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

static int SCE_VRender_BuildSoftware (SCE_SVoxelTemplate *vt)
{
    size_t n_vertices, n_indices, n_points;

#if 1
    vt->final_geom = &final_geom_pos;
#else
    if (vt->compressed_pos && vt->compressed_nor)
        vt->final_geom = &final_geom_cpos_cnor;
    else if (vt->compressed_nor)
        vt->final_geom = &final_geom_pos_cnor;
    else if (vt->compressed_pos)
        vt->final_geom = &final_geom_cpos_nor;
    else
        vt->final_geom = &final_geom_pos_nor;
#endif

    n_points = vt->width * vt->height * vt->depth;
    n_vertices = 3 * n_points;
    n_indices = 15 * n_points;

    SCE_Geometry_SetNumVertices (vt->final_geom, n_vertices);
    SCE_Geometry_SetNumIndices (vt->final_geom, n_indices);

    if (!(vt->vertices = SCE_malloc (n_vertices * 3 * sizeof *vt->vertices)))
        goto fail;
    if (!(vt->normals = SCE_malloc (n_vertices * 3 * sizeof *vt->normals)))
        goto fail;
    if (!(vt->indices = SCE_malloc (n_indices * sizeof *vt->indices)))
        goto fail;

    SCE_MC_SetNumCells (&vt->mc_gen, n_points);
    if (SCE_MC_Build (&vt->mc_gen) < 0)
        goto fail;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

int SCE_VRender_Build (SCE_SVoxelTemplate *vt)
{
    switch (vt->pipeline) {
    case SCE_VRENDER_HARDWARE: return SCE_VRender_BuildHW (vt);
    case SCE_VRENDER_SOFTWARE: return SCE_VRender_BuildSoftware (vt);
    default:;
    }
    return SCE_OK;              /* duh */
}

SCE_SGeometry* SCE_VRender_GetFinalGeometry (SCE_SVoxelTemplate *vt)
{
    return vt->final_geom;
}


void SCE_VRender_SetWrap (SCE_SVoxelMesh *vm, SCE_TVector3 wrap)
{
    SCE_Vector3_Copy (vm->wrap, wrap);
}
void SCE_VRender_SetMesh (SCE_SVoxelMesh *vm, SCE_SMesh *mesh)
{
    vm->mesh = mesh;
}


void SCE_VRender_SetVBRange (SCE_SVoxelMesh *vm, const int *r)
{
    if (r) {
        vm->vertex_range[0] = r[0];
        vm->vertex_range[1] = r[1];
    } else {
        vm->vertex_range[0] = vm->vertex_range[1] = -1;
    }
}
void SCE_VRender_SetIBRange (SCE_SVoxelMesh *vm, const int *r)
{
    if (r) {
        vm->index_range[0] = r[0];
        vm->index_range[1] = r[1];
    } else {
        vm->index_range[0] = vm->index_range[1] = -1;
    }
}


/**
 * \brief Generates geometry from a density field using the CPU
 * \param vt a voxel template
 * \param vm abstract output mesh
 * \param volume source voxels
 * \param x,y,z coordinates of the origin for volume texture fetches
 */
void SCE_VRender_Software (SCE_SVoxelTemplate *vt, const SCE_SGrid *volume,
                           SCE_SVoxelMesh *vm, int x, int y, int z)
{
    SCE_SIntRect3 rect;
    size_t n_vertices, n_indices;
    size_t vertex_size, index_size;
    SCE_RVertexBuffer *vb;
    SCE_RIndexBuffer *ib;

    SCE_Rectangle3_SetFromOrigin (&rect, x, y, z, vt->width, vt->height,
                                  vt->depth);
    /* TODO: pos and nor shall be interleaved */
    n_vertices = SCE_MC_GenerateVertices (&vt->mc_gen, &rect, volume,
                                          vt->vertices);
    if (n_vertices != 0) {
        vm->render = SCE_TRUE;
    } else {
        vm->render = SCE_FALSE;
        SCE_Mesh_SetNumVertices (vm->mesh, 0);
        SCE_Mesh_SetNumIndices (vm->mesh, 0);
        return;
    }
    /*SCE_MC_GenerateNormals (&vt->mc_gen, &rect, volume, vt->v_pos, vt->v_nor);*/
    n_indices = SCE_MC_GenerateIndices (&vt->mc_gen, vt->indices);
    SCE_MC_GenerateNormals (&vt->mc_gen, volume, vt->normals);

    vertex_size = 3 * sizeof (SCEvertices);
    index_size = sizeof (SCEindices);

    /* we kinda wanna upload these data asap, because we dont really want
       to keep a copy on CPU memory */
    SCE_Mesh_UploadVertices (vm->mesh, SCE_MESH_STREAM_G, vt->vertices,
                             0, n_vertices * vertex_size);
    SCE_Mesh_UploadVertices (vm->mesh, SCE_MESH_STREAM_N, vt->normals,
                             0, n_vertices * vertex_size);
    SCE_Mesh_UploadIndices (vm->mesh, vt->indices, n_indices * index_size);
    /* TODO: we want to use BindBufferRange() since only a small part of the
       buffer will actually be needed. */

    SCE_Mesh_SetNumVertices (vm->mesh, n_vertices);
    SCE_Mesh_SetNumIndices (vm->mesh, n_indices);
}

/**
 * \brief Generates geometry from a density field using the GPU
 * \param vt a voxel template
 * \param vm abstract output mesh
 * \param volume source voxels
 * \param x,y,z coordinates of the origin for volume texture fetches
 */
void SCE_VRender_Hardware (SCE_SVoxelTemplate *vt, SCE_STexture *volume,
                           SCE_SVoxelMesh *vm, int x, int y, int z)
{
    SCE_TVector3 wrap;
    float w, h, d;
    int i;

    w = SCE_Texture_GetWidth (volume);
    h = SCE_Texture_GetHeight (volume);
    d = SCE_Texture_GetDepth (volume);
    SCE_Vector3_Copy (wrap, vm->wrap);
    wrap[0] += (float)x / w;
    wrap[1] += (float)y / h;
    wrap[2] += (float)z / d;

    /* 1st pass: render non empty cells */
    SCE_Texture_Use (volume);
    SCE_Shader_Use (vt->non_empty_shader);
    SCE_Shader_SetParam3fv (vt->non_empty_offset_loc, 1, wrap);
    SCE_Mesh_BeginRenderTo (&vt->non_empty);
    SCE_Mesh_Use (&vt->grid_mesh);
    SCE_Mesh_Render ();
    SCE_Mesh_Unuse ();
    SCE_Mesh_EndRenderTo (&vt->non_empty);

    /* this test doesn't seem to be deterministic... */
    if (SCE_Mesh_GetNumVertices (&vt->non_empty) != 0) {
        vm->render = SCE_TRUE;
    } else {
        vm->render = SCE_FALSE;
        /* ... so I setup those as a precaution :) */
        SCE_Mesh_SetNumVertices (vm->mesh, 0);
        SCE_Mesh_SetNumIndices (vm->mesh, 0);
        SCE_Shader_Use (NULL);
        SCE_Texture_Use (NULL);
        return;
    }

    /* 2nd pass: render a list of vertices */
    SCE_Shader_Use (vt->list_verts_shader);
    SCE_Mesh_BeginRenderTo (&vt->list_verts);
    SCE_Mesh_Use (&vt->non_empty);
    SCE_Mesh_Render ();
    SCE_Mesh_Unuse ();
    SCE_Mesh_EndRenderTo (&vt->list_verts);

    /* 3rd pass: process vertices to generate final coords & normal */
    SCE_Mesh_SetPrimitiveType (vm->mesh, SCE_POINTS);
    SCE_Shader_Use (vt->final_shader);
    SCE_Shader_SetParam3fv (vt->final_offset_loc, 1, wrap);
    SCE_Shader_SetParamf (vt->comp_scale_loc, vt->comp_scale);
    SCE_Mesh_BeginRenderTo (vm->mesh);
    SCE_Mesh_Use (&vt->list_verts);
    SCE_Mesh_Render ();
    SCE_Mesh_Unuse ();
    SCE_Mesh_EndRenderTo (vm->mesh);

    glPointSize (1.0);  /* TODO: take care of point size */
    /* 4th pass: generate the 3D map of indices */
    SCE_Texture_Use (NULL);
    SCE_Texture_RenderTo (vt->splat, 0);
    SCE_Shader_Use (vt->splat_shader);
    SCE_Mesh_Use (&vt->list_verts);
    SCE_Mesh_Render ();
    SCE_Mesh_Unuse ();
    SCE_Texture_RenderTo (NULL, 0);

    /* 5th pass: generate the index buffer */
    SCE_Mesh_SetPrimitiveType (vm->mesh, SCE_POINTS);
    SCE_Texture_BeginLot ();
    SCE_Texture_Use (vt->splat);
    if (vt->algo == SCE_VRENDER_MARCHING_CUBES)
        SCE_Texture_Use (vt->mc_table);
    SCE_Texture_EndLot ();
    SCE_Shader_Use (vt->indices_shader);
    SCE_Mesh_BeginRenderToIndices (vm->mesh);
    SCE_Mesh_Use (&vt->non_empty);
    SCE_Mesh_Render ();
    SCE_Mesh_Unuse ();
    SCE_Mesh_EndRenderToIndices (vm->mesh);
    SCE_Mesh_SetPrimitiveType (vm->mesh, SCE_TRIANGLES);
    i = SCE_Mesh_GetNumIndices (vm->mesh);
    SCE_Mesh_SetNumIndices (vm->mesh, i * 3);

    sce_max_vertices = MAX (sce_max_vertices, SCE_Mesh_GetNumVertices (vm->mesh));
    sce_max_indices = MAX (sce_max_indices, SCE_Mesh_GetNumIndices (vm->mesh));

    SCE_Shader_Use (NULL);
    SCE_Texture_Flush ();
}


unsigned int SCE_VRender_GetMaxV (void)
{
    return sce_max_vertices;
}
unsigned int SCE_VRender_GetMaxI (void)
{
    return sce_max_indices;
}
unsigned int SCE_VRender_GetLimitV (void)
{
    return sce_vertices_limit;
}
unsigned int SCE_VRender_GetLimitI (void)
{
    return sce_indices_limit;
}

int SCE_VRender_IsVoid (const SCE_SVoxelMesh *vm)
{
    return vm->render;
}
