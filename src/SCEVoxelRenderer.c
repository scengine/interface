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
   updated: 15/04/2012 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCEVoxelRenderer.h"

static SCE_SGeometry non_empty_geom;
static SCE_SGeometry list_verts_geom;
static SCE_SGeometry final_geom_pos_nor;
static SCE_SGeometry final_geom_pos_cnor; /* c stands for "compressed" */
static SCE_SGeometry final_geom_cpos_nor;
static SCE_SGeometry final_geom_cpos_cnor;
static SCE_SGeometry *default_final_geom = &final_geom_pos_nor;


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
    SCE_Geometry_Init (&vt->grid_geom);
    SCE_Mesh_Init (&vt->grid_mesh);
    SCE_Mesh_Init (&vt->non_empty);
    SCE_Mesh_Init (&vt->list_verts);

    vt->compressed_pos = SCE_FALSE;
    vt->compressed_nor = SCE_FALSE;
    vt->comp_scale = 1.0;
    vt->final_geom = NULL;

    vt->non_empty_shader = NULL;
    vt->non_empty_offset_loc = 0;
    vt->list_verts_shader = NULL;
    vt->final_shader = NULL;
    vt->final_offset_loc = SCE_SHADER_BAD_INDEX;
    vt->comp_scale_loc = SCE_SHADER_BAD_INDEX;

    vt->splat_shader = NULL;
    vt->indices_shader = NULL;
    vt->splat = NULL;

    vt->vwidth = vt->vheight = vt->vdepth = 0;
    vt->width = vt->height = vt->depth = 0;
}
void SCE_VRender_Clear (SCE_SVoxelTemplate *vt)
{
    SCE_Geometry_Clear (&vt->grid_geom);
    SCE_Mesh_Clear (&vt->grid_mesh);
    SCE_Mesh_Clear (&vt->non_empty);
    SCE_Mesh_Clear (&vt->list_verts);

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
    vm->volume = NULL;
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
static const char *non_empty_gs =
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
static const char *list_verts_gs =
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


static const char *final_vs =
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
    "  uvec3 v = uvec3 (pos * factor);"
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


static const char *splat_vs =
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

static const char *indices_gs =
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


int SCE_VRender_Build (SCE_SVoxelTemplate *vt)
{
    SCE_SGrid grid;
    size_t n_points;
    const char *varyings[2] = {NULL, NULL};
    SCE_STexData tc;
    int width, height, depth;
    float maxw, maxh, maxd;

    /* create grid geometry and grid mesh */
    SCE_Grid_Init (&grid);
    SCE_Grid_SetDimensions (&grid, vt->width, vt->height, vt->depth);
    SCE_Grid_SetType (&grid, SCE_UNSIGNED_BYTE);
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

    /* setup sizes */
    SCE_Geometry_SetNumVertices (&non_empty_geom, n_points);
    SCE_Geometry_SetNumVertices (&list_verts_geom, 7 * n_points);
    SCE_Geometry_SetNumVertices (vt->final_geom, 6 * n_points);
    SCE_Geometry_SetNumIndices (vt->final_geom, 36 * n_points);

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
    SCE_TexData_SetDimensions (&tc, width * 8, height, depth);
    SCE_TexData_SetDataType (&tc, SCE_UNSIGNED_INT);
    SCE_TexData_SetType (&tc, SCE_IMAGE_3D);
    SCE_TexData_SetDataFormat (&tc, SCE_IMAGE_RED);
    /* TODO: choose pixel format carefully, could be ushort */
    SCE_TexData_SetPixelFormat (&tc, SCE_PXF_R32UI);
    SCE_Texture_AddTexDataDup (vt->splat, 0, &tc);
    /* index values must not be modified by magnification filter */
    SCE_Texture_Pixelize (vt->splat, SCE_TRUE);
    SCE_Texture_SetFilter (vt->splat, SCE_TEX_NEAREST);
    SCE_Texture_Build (vt->splat, SCE_FALSE);
    if (SCE_Texture_SetupFramebuffer (vt->splat, SCE_RENDER_COLOR,
                                      0, SCE_FALSE, SCE_FALSE) < 0)
        goto fail;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

SCE_SGeometry* SCE_VRender_GetFinalGeometry (SCE_SVoxelTemplate *vt)
{
    return vt->final_geom;
}


void SCE_VRender_SetVolume (SCE_SVoxelMesh *vm, SCE_STexture *volume)
{
    vm->volume = volume;
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
 * \brief Generates geometry from a density field using the GPU
 * \param vt a voxel template
 * \param vm abstract output mesh
 * \param x,y,z coordinates of the origin for volume texture fetches
 */
void SCE_VRender_Hardware (SCE_SVoxelTemplate *vt, SCE_SVoxelMesh *vm,
                           int x, int y, int z)
{
    SCE_TVector3 wrap;
    float w, h, d;
    int i;

    w = SCE_Texture_GetWidth (vm->volume);
    h = SCE_Texture_GetHeight (vm->volume);
    d = SCE_Texture_GetDepth (vm->volume);
    SCE_Vector3_Copy (wrap, vm->wrap);
    wrap[0] += (float)x / w;
    wrap[1] += (float)y / h;
    wrap[2] += (float)z / d;

    /* 1st pass: render non empty cells */
    SCE_Texture_Use (vm->volume);
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
    SCE_Texture_Use (vt->splat);
    SCE_Shader_Use (vt->indices_shader);
    SCE_Mesh_BeginRenderToIndices (vm->mesh);
    SCE_Mesh_Use (&vt->non_empty);
    SCE_Mesh_Render ();
    SCE_Mesh_Unuse ();
    SCE_Mesh_EndRenderToIndices (vm->mesh);
    SCE_Mesh_SetPrimitiveType (vm->mesh, SCE_TRIANGLES);
    i = SCE_Mesh_GetNumIndices (vm->mesh);
    SCE_Mesh_SetNumIndices (vm->mesh, i * 3);

    SCE_Shader_Use (NULL);
    SCE_Texture_Use (NULL);
}


int SCE_VRender_IsVoid (const SCE_SVoxelMesh *vm)
{
    return vm->render;
}
