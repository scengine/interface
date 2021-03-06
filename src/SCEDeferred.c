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

/* created: 04/08/2011
   updated: 11/04/2012 */

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCEScene.h"
#include "SCE/interface/SCEDeferred.h"

#define SCE_DEFERRED_DEPTH_TARGET_NAME "sce_deferred_depth_map"
#define SCE_DEFERRED_LIGHT_TARGET_NAME "sce_deferred_light_map"
#define SCE_DEFERRED_COLOR_TARGET_NAME "sce_deferred_color_map"
#define SCE_DEFERRED_NORMAL_TARGET_NAME "sce_deferred_normal_map"
#define SCE_DEFERRED_SPECULAR_TARGET_NAME "sce_deferred_specular_map"

/* shader uniform's names */
static const char *sce_deferred_target_names[SCE_NUM_DEFERRED_TARGETS] = {
    SCE_DEFERRED_DEPTH_TARGET_NAME,
    SCE_DEFERRED_LIGHT_TARGET_NAME,
    SCE_DEFERRED_COLOR_TARGET_NAME,
    SCE_DEFERRED_NORMAL_TARGET_NAME,
    SCE_DEFERRED_SPECULAR_TARGET_NAME
};


void SCE_Deferred_InitLightingShader (SCE_SDeferredLightingShader *shader)
{
    shader->shader = NULL;
    shader->invproj_loc = SCE_SHADER_BAD_INDEX;
    shader->lightviewproj_loc = SCE_SHADER_BAD_INDEX;
    shader->lightpos_loc = SCE_SHADER_BAD_INDEX;
    shader->lightdir_loc = SCE_SHADER_BAD_INDEX;
    shader->lightcolor_loc = SCE_SHADER_BAD_INDEX;
    shader->lightradius_loc = SCE_SHADER_BAD_INDEX;
    shader->lightangle_loc = SCE_SHADER_BAD_INDEX;
    shader->lightattenuation_loc = SCE_SHADER_BAD_INDEX;
}
void SCE_Deferred_ClearLightingShader (SCE_SDeferredLightingShader *shader)
{
    SCE_Shader_Delete (shader->shader);
}

static void SCE_Deferred_Init (SCE_SDeferred *def)
{
    int i;
    def->gbuf = NULL;
    for (i = 0; i < SCE_NUM_DEFERRED_TARGETS; i++)
        def->targets[i] = NULL;
    def->n_targets = 0;
    def->w = def->h = 64;       /* xd */

    def->amb_color[0] = def->amb_color[1] = def->amb_color[2] = 0.1f;

    def->skybox_shader = def->final_shader = NULL;
    def->lightflags_mask = SCE_DEFERRED_USE_ALL;

    def->shadowcube_shader = NULL;
    def->shadowcube_factor_loc = SCE_SHADER_BAD_INDEX;

    for (i = 0; i < SCE_NUM_LIGHT_TYPES; i++) {
        int j;
        def->shadowmaps[i] = NULL;
        for (j = 0; j < SCE_NUM_DEFERRED_LIGHT_FLAGS; j++)
            SCE_Deferred_InitLightingShader (&def->shaders[i][j]);
    }
    def->sm_w = def->sm_h = 128; /* xd */
    def->cascaded_splits = 1;
    def->csm_far = -1.0;
    def->cam = NULL;
}
static void SCE_Deferred_Clear (SCE_SDeferred *def)
{
    int i;
    for (i = 0; i < SCE_NUM_DEFERRED_TARGETS; i++)
        SCE_Texture_Delete (def->targets[i]);

    SCE_Shader_Delete (def->final_shader);
    SCE_Shader_Delete (def->skybox_shader);
    SCE_Shader_Delete (def->shadowcube_shader);
    for (i = 0; i < SCE_NUM_LIGHT_TYPES; i++) {
        SCE_Texture_Delete (def->shadowmaps[i]);
        /* only delete the first shader, let's just hope the lighting shader
           structure doesn't have other pointers to free than the shader */
        SCE_Deferred_ClearLightingShader (def->shaders[i]);
    }
    SCE_Camera_Delete (def->cam);
}

SCE_SDeferred* SCE_Deferred_Create (void)
{
    SCE_SDeferred *def = NULL;
    if (!(def = SCE_malloc (sizeof *def)))
        SCEE_LogSrc ();
    else {
        SCE_Deferred_Init (def);
        if (!(def->cam = SCE_Camera_Create ()))
            goto fail;
    }
    return def;
fail:
    SCE_Deferred_Delete (def);
    SCEE_LogSrc ();
    return NULL;
}
void SCE_Deferred_Delete (SCE_SDeferred *def)
{
    if (def) {
        SCE_Deferred_Clear (def);
        SCE_free (def);
    }
}


/**
 * \brief Sets G-buffer dimensions
 * \param def a deferred renderer
 * \param w,h dimensions
 */
void SCE_Deferred_SetDimensions (SCE_SDeferred *def, SCEuint w, SCEuint h)
{
    def->w = w;
    def->h = h;
}

/**
 * \brief Sets shadow maps dimensions
 * \param def a deferred renderer
 * \param w,h dimensions
 * \todo make it update the shadow maps if they are already built
 */
void SCE_Deferred_SetShadowMapsDimensions (SCE_SDeferred *def, SCEuint w,
                                           SCEuint h)
{
    def->sm_w = w;
    def->sm_h = h;
}

/**
 * \brief Sets the number of splits for the cascaded shadow maps
 * \param def a deferred renderer
 * \param n_splits number of splits (a value less than 5 is highly recommanded),
 * maximum is defined by SCE_MAX_DEFERRED_CASCADED_SPLITS
 */
void SCE_Deferred_SetCascadedSplits (SCE_SDeferred *def, SCEuint n_splits)
{
    if (n_splits <= SCE_MAX_DEFERRED_CASCADED_SPLITS)
        def->cascaded_splits = n_splits;
}

/**
 * \brief Sets far plane for CSM
 * \param def a deferred renderer
 * \param far customized far plane that will replace camera's, or a negative
 * number to use camera's (default)
 */
void SCE_Deferred_SetCascadedFar (SCE_SDeferred *def, float far)
{
    def->csm_far = far;
}


void SCE_Deferred_AddLightFlag (SCE_SDeferred *def, int flag)
{
    SCE_FLAG_ADD (def->lightflags_mask, flag);
}
void SCE_Deferred_RemoveLightFlag (SCE_SDeferred *def, int flag)
{
    SCE_FLAG_REMOVE (def->lightflags_mask, flag);
}
void SCE_Deferred_SetLightFlagsMask (SCE_SDeferred *def, int mask)
{
    def->lightflags_mask = mask;
}
int SCE_Deferred_GetLightFlagsMask (SCE_SDeferred *def)
{
    return def->lightflags_mask;
}

static const char *sce_skybox_vs =
    "uniform mat4 sce_modelviewmatrix;"
    "uniform mat4 sce_projectionmatrix;"
    "varying vec3 tc;"
    "varying vec4 pos;"
    "void main (void)"
    "{"
    "tc = gl_MultiTexCoord0.xyz;"
    "vec4 p = sce_projectionmatrix * sce_modelviewmatrix * gl_Vertex;"
    "pos = p;"
    "gl_Position = p;"
    "}";
static const char *sce_skybox_ps =
    "uniform samplerCube "SCE_DEFERRED_SKYBOX_MAP_NAME";"
    "uniform sampler2D "SCE_DEFERRED_DEPTH_TARGET_NAME";"
    "varying vec3 tc;"
    "varying vec4 pos;"         /* for screen position */
    "void main (void)"
    "{"
    "vec2 coord = pos.xy / (pos.w * 2.0);"
    "coord += vec2 (0.5);"
    "float depth = texture2D ("SCE_DEFERRED_DEPTH_TARGET_NAME", coord).x;"
    "if (depth < 0.9999)"
    "  discard;"
    "gl_FragColor = textureCube ("SCE_DEFERRED_SKYBOX_MAP_NAME", tc);"
    "}";


#define SCE_MY_STR_(a) #a
#define SCE_MY_STR(a) SCE_MY_STR_(a)

static const char *shadowcube_vs_fun =
    "varying out vec4 "SCE_DEFERRED_CAMERA_SPACE_POS_NAME";";
static const char *shadowcube_ps_fun =
    "varying in vec4 "SCE_DEFERRED_CAMERA_SPACE_POS_NAME";"
    "float sce_deferred_getdepth (void)"
    "{"
    "  return "SCE_MY_STR (SCE_DEFERRED_POINT_LIGHT_DEPTH_FACTOR)""
    "        * length ("SCE_DEFERRED_CAMERA_SPACE_POS_NAME");"
    "}";

static const char *shadowcube_main_vs =
    "uniform mat4 sce_modelviewmatrix;"
    "uniform mat4 sce_projectionmatrix;"
    "void main (void)"
    "{"
    "  vec4 p = sce_modelviewmatrix * gl_Vertex;"
    "  "SCE_DEFERRED_CAMERA_SPACE_POS_NAME" = p;"
    "  gl_Position = sce_projectionmatrix * p;"
    "}";

static const char *shadowcube_main_ps =
    "void main (void)"
    "{"
    "  gl_FragDepth = sce_deferred_getdepth ();"
    "}";



static const char *sce_final_vs =
    "uniform mat4 sce_modelviewmatrix;"
    "uniform mat4 sce_projectionmatrix;"
    "varying vec2 tc;"
    "void main (void)"
    "{"
    "  tc = gl_MultiTexCoord0.xy;"
    "  gl_Position = sce_projectionmatrix * (sce_modelviewmatrix * gl_Vertex);"
    "}";
static const char *sce_final_ps =
    "uniform sampler2D "SCE_DEFERRED_DEPTH_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_LIGHT_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_COLOR_TARGET_NAME";"
    "varying vec2 tc;"
    "void main (void)"
    "{"
    "  vec4 lum = texture2D ("SCE_DEFERRED_LIGHT_TARGET_NAME", tc);"
    "  vec4 color = texture2D ("SCE_DEFERRED_COLOR_TARGET_NAME", tc);"
    "  color.xyz *= lum.xyz;"
    "  color.xyz += lum.xyz * color.a * lum.a;"
    "  gl_FragColor = color;"
    "  gl_FragDepth = texture2D ("SCE_DEFERRED_DEPTH_TARGET_NAME", tc).x;"
    "}";

int SCE_Deferred_Build (SCE_SDeferred *def,
                        const char *fnames[SCE_NUM_LIGHT_TYPES])
{
    int i;

    for (i = 0; i < SCE_NUM_DEFERRED_TARGETS; i++) {
        def->targets[i] = SCE_Texture_Create (SCE_TEX_2D, def->w, def->h, 0);
        if (!def->targets[i])
            goto fail;
    }

    /* TODO: use an option to choose between depth and packed depth/stencil */
    if (SCE_Texture_SetupFramebuffer (def->targets[SCE_DEFERRED_DEPTH_TARGET],
                                      SCE_RENDER_DEPTH_STENCIL, 0, 0, 0) < 0)
        goto fail;
    if (SCE_Texture_SetupFramebuffer (def->targets[SCE_DEFERRED_LIGHT_TARGET],
                                      SCE_RENDER_COLOR, 0, 0, 0) < 0)
        goto fail;

    def->gbuf = def->targets[0];
    def->n_targets = 4;

    for (i = 0; i < def->n_targets; i++) {
        if (!def->targets[i])
            goto fail;
        if (SCE_Texture_Build (def->targets[i], SCE_FALSE) < 0)
            goto fail;

        SCE_Texture_SetUnit (def->targets[i], i);
        /* disable filters */
        SCE_Texture_Pixelize (def->targets[i], SCE_TRUE);
        SCE_Texture_SetFilter (def->targets[i], SCE_TEX_NEAREST);
    }

    /* add targets to the G-buffer */
    for (i = 0; i < def->n_targets - 1; i++) {
        SCE_Texture_AddRenderTexture (def->gbuf, SCE_COLOR_BUFFER0 + i,
                                      def->targets[i + 1]);
    }

    /* create shadow maps */
    /* TODO: we may want to change the shadow maps resolution per
       light type, also lights may request for a particular resolution
       (for tiny lights that dont require much) */
    if (!(def->shadowmaps[SCE_POINT_LIGHT] =
          SCE_Texture_Create (SCE_TEX_CUBE, def->sm_w, def->sm_h, 0)))
        goto fail;
    if (!(def->shadowmaps[SCE_SPOT_LIGHT] =
          SCE_Texture_Create (SCE_TEX_2D, def->sm_w, def->sm_h, 0)))
        goto fail;
    if (!(def->shadowmaps[SCE_SUN_LIGHT] =
          SCE_Texture_Create (SCE_TEX_2D, def->sm_w * def->cascaded_splits,
                              def->sm_h, 0)))
        goto fail;

    /* TODO: we might want a stencil buffer (think about the terrain) */
    if (SCE_Texture_SetupFramebuffer (def->shadowmaps[SCE_POINT_LIGHT],
                                      SCE_RENDER_DEPTH, 0, 0, 0) < 0)
        goto fail;
    if (SCE_Texture_SetupFramebuffer (def->shadowmaps[SCE_SPOT_LIGHT],
                                      SCE_RENDER_DEPTH, 0, 0, 0) < 0)
        goto fail;
    if (SCE_Texture_SetupFramebuffer (def->shadowmaps[SCE_SUN_LIGHT],
                                      SCE_RENDER_DEPTH, 0, 0, 0) < 0)
        goto fail;

    for (i = 0; i < SCE_NUM_LIGHT_TYPES; i++)
        SCE_Texture_SetUnit (def->shadowmaps[i], def->n_targets);

    /* create shaders */
    for (i = 0; i < SCE_NUM_LIGHT_TYPES; i++) {
        if (fnames[i]) {
            if (SCE_Deferred_BuildLightShader (def, i, def->shaders[i],
                                               fnames[i]) < 0)
                goto fail;
        }
    }

    /* create final render shader */
    if (!(def->final_shader = SCE_Shader_Create ())) goto fail;
    if (SCE_Shader_AddSource (def->final_shader, SCE_VERTEX_SHADER,
                              sce_final_vs, SCE_FALSE) < 0)
        goto fail;
    if (SCE_Shader_AddSource (def->final_shader, SCE_PIXEL_SHADER,
                              sce_final_ps, SCE_FALSE) < 0)
        goto fail;
    if (SCE_Shader_Build (def->final_shader) < 0)
        goto fail;
    SCE_Shader_Use (def->final_shader);
    for (i = 0; i < def->n_targets; i++)
        SCE_Shader_Param (sce_deferred_target_names[i], i);
    SCE_Shader_Use (NULL);
    SCE_Shader_SetupMatricesMapping (def->final_shader);
    SCE_Shader_ActivateMatricesMapping (def->final_shader, SCE_TRUE);

    /* setup skybox shader */
    if (!(def->skybox_shader = SCE_Shader_Create ()))
        goto fail;
    if (SCE_Shader_AddSource (def->skybox_shader, SCE_VERTEX_SHADER,
                              sce_skybox_vs, SCE_FALSE) < 0)
        goto fail;
    if (SCE_Shader_AddSource (def->skybox_shader, SCE_PIXEL_SHADER,
                              sce_skybox_ps, SCE_FALSE) < 0)
        goto fail;
    if (SCE_Shader_Build (def->skybox_shader) < 0)
        goto fail;
    SCE_Shader_Use (def->skybox_shader);
    for (i = 0; i < def->n_targets; i++)
        SCE_Shader_Param (sce_deferred_target_names[i], i);
    SCE_Shader_Use (NULL);
    SCE_Shader_SetupMatricesMapping (def->skybox_shader);
    SCE_Shader_ActivateMatricesMapping (def->skybox_shader, SCE_TRUE);

    /* setup default point light shadow map rendering shader */
    if (!(def->shadowcube_shader = SCE_Shader_Create ())) goto fail;
    if (SCE_Deferred_BuildPointShadowShader (def, def->shadowcube_shader) < 0)
        goto fail;
    if (SCE_Shader_AddSource (def->shadowcube_shader, SCE_VERTEX_SHADER,
                              shadowcube_main_vs, SCE_FALSE) < 0)
        goto fail;
    if (SCE_Shader_AddSource (def->shadowcube_shader, SCE_PIXEL_SHADER,
                              shadowcube_main_ps, SCE_FALSE) < 0)
        goto fail;
    if (SCE_Shader_Build (def->shadowcube_shader) < 0) goto fail;
    SCE_Shader_SetupMatricesMapping (def->shadowcube_shader);
    SCE_Shader_ActivateMatricesMapping (def->shadowcube_shader, SCE_TRUE);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


static const char *sce_pack_color_fun =
    "void sce_pack_color (in vec3 col)"
    "{"
    "  gl_FragData[1].xyz = col;"
    "}";
/* TODO: inefficient unpacking: another texture fetch will be required to
   unpack the alpha channel */
static const char *sce_unpack_color_fun =
    "vec3 sce_unpack_color (in vec2 coord)"
    "{"
    "  return texture2D ("SCE_DEFERRED_COLOR_TARGET_NAME", coord).xyz;"
    "}";

#if 0
/* see http://aras-p.info/texts/CompactNormalStorage.html */
static const char *sce_pack_normal_fun =
    "void sce_pack_normal (in vec3 norm)"
    "{"
    /* stereographic projection */
    "  const float scale = 1.8;"

    "  vec2 nor = norm.xy / (norm.z + 1.0);"
    "  nor /= scale;"
    "  nor = nor * 0.5 + vec2 (0.5);"
    /* precision mess^W stuff */
    "  float x = nor.x * 256.0;"
    "  float e = floor (x);"
    "  gl_FragData[2].x = e / 256.0;"
    "  gl_FragData[2].y = x - e, 1.0;"
    "  x = nor.y * 256.0;"
    "  e = floor (x);"
    "  gl_FragData[2].z = e / 256.0;"
    "  gl_FragData[2].w = x - e;"
    "}";
static const char *sce_unpack_normal_fun =
    "vec3 sce_unpack_normal (in vec2 coord)"
    "{"
    /* precision stuff */
    "  vec4 n = texture2D ("SCE_DEFERRED_NORMAL_TARGET_NAME", coord);"
    "  vec3 nor;"
    "  nor.x = n.x + n.y / 256.0;"
    "  nor.y = n.z + n.w / 256.0;"
    /* stereographic projection */
    "  const float scale = 1.8;"
    "  nor = vec3 (nor.xy * 2.0 * scale, 0.0) + vec3 (-scale, -scale, 1.0);"
    "  const float g = 2.0 / dot (nor, nor);"
    "  vec3 nn = vec3 (g * nor.xy, g - 1.0);"
    "  return nn;"
    "}";
#else
static const char *sce_pack_normal_fun =
    "void sce_pack_normal (in vec3 nor)"
    "{"
    "  gl_FragData[2].xyz = (nor + vec3 (1.0)) / 2.0;"
    "}";
static const char *sce_unpack_normal_fun =
    "vec3 sce_unpack_normal (in vec2 coord)"
    "{"
    "  vec3 n = texture2D ("SCE_DEFERRED_NORMAL_TARGET_NAME", coord).xyz;"
    "  return normalize (n * 2.0 - vec3 (1.0));"
    "}";
#endif

static const char *sce_pack_position_fun =
    "void sce_pack_position (in vec4 pos) {}";
/* TODO: same here, possible multiple texture fetches */
static const char *sce_unpack_position_fun =
    "vec3 sce_unpack_position (in vec2 coord)"
    "{"
    "  float depth = texture2D ("SCE_DEFERRED_DEPTH_TARGET_NAME", coord).x;"
    "  vec4 pos = vec4 (coord, depth, 1.0);"
    "  pos *= 2.0;"
    "  pos -= vec4 (1.0);"
    "  pos = "SCE_DEFERRED_INVPROJ_NAME" * pos;"
    "  pos.xyz /= pos.w;"
    "  return pos.xyz;"
    "}";


/**
 * \brief Shaders factory
 * \param def a deferred renderer
 * \param shader the shader to build
 *
 * Adds code to make a shader work for use with the given deferred renderer
 *
 * \returns SCE_ERROR on error, SCE_OK otherwise
 * \deprecated
 * \todo deprecated... no it's not deprecated.
 */
int SCE_Deferred_BuildShader (SCE_SDeferred *def, SCE_SShader *shader)
{
    (void)def;
    if (SCE_Shader_AddSource (shader, SCE_PIXEL_SHADER,
                              sce_pack_color_fun, SCE_TRUE) < 0) goto fail;
    if (SCE_Shader_AddSource (shader, SCE_PIXEL_SHADER,
                              sce_pack_normal_fun, SCE_TRUE) < 0) goto fail;
    if (SCE_Shader_AddSource (shader, SCE_PIXEL_SHADER,
                              sce_pack_position_fun, SCE_TRUE) < 0) goto fail;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

/**
 * \brief Shaders factory for point light shadow map rendering
 * \param def a deferred renderer
 * \param shader the shader to build
 *
 * Adds code to make a shader work for use with the given deferred renderer
 *
 * \returns SCE_ERROR on error, SCE_OK otherwise
 */
int SCE_Deferred_BuildPointShadowShader (SCE_SDeferred *def,
                                         SCE_SShader *shader)
{
    (void)def;
    if (SCE_Shader_AddSource (shader, SCE_VERTEX_SHADER,
                              shadowcube_vs_fun, SCE_TRUE) < 0)
        goto fail;
    if (SCE_Shader_AddSource (shader, SCE_PIXEL_SHADER,
                              shadowcube_ps_fun, SCE_TRUE) < 0)
        goto fail;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}



static const char *sce_final_uniforms_code[SCE_NUM_LIGHT_TYPES] = {
    /* point */
    "uniform sampler2D "SCE_DEFERRED_DEPTH_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_LIGHT_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_COLOR_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_NORMAL_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_SHADOW_MAP_NAME";"
    "uniform samplerCube "SCE_DEFERRED_SHADOW_CUBE_MAP_NAME";"
    "uniform mat4 "SCE_DEFERRED_INVPROJ_NAME";"
    "uniform mat4 "SCE_DEFERRED_LIGHT_VIEWPROJ_NAME";"
    "uniform vec3 "SCE_DEFERRED_LIGHT_POSITION_NAME";"
    "uniform vec3 "SCE_DEFERRED_LIGHT_DIRECTION_NAME";"
    "uniform vec3 "SCE_DEFERRED_LIGHT_COLOR_NAME";"
    "uniform float "SCE_DEFERRED_LIGHT_RADIUS_NAME";"
    "uniform float "SCE_DEFERRED_LIGHT_ANGLE_NAME";"
    "uniform float "SCE_DEFERRED_LIGHT_ATTENUATION_NAME";",
    /* spot */
    "uniform sampler2D "SCE_DEFERRED_DEPTH_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_LIGHT_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_COLOR_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_NORMAL_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_SHADOW_MAP_NAME";"
    "uniform mat4 "SCE_DEFERRED_INVPROJ_NAME";"
    "uniform mat4 "SCE_DEFERRED_LIGHT_VIEWPROJ_NAME";"
    "uniform vec3 "SCE_DEFERRED_LIGHT_POSITION_NAME";"
    "uniform vec3 "SCE_DEFERRED_LIGHT_DIRECTION_NAME";"
    "uniform vec3 "SCE_DEFERRED_LIGHT_COLOR_NAME";"
    "uniform float "SCE_DEFERRED_LIGHT_RADIUS_NAME";"
    "uniform float "SCE_DEFERRED_LIGHT_ANGLE_NAME";"
    "uniform float "SCE_DEFERRED_LIGHT_ATTENUATION_NAME";",
    /* sun */
    "uniform sampler2D "SCE_DEFERRED_DEPTH_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_LIGHT_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_COLOR_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_NORMAL_TARGET_NAME";"
    "uniform sampler2D "SCE_DEFERRED_SHADOW_MAP_NAME";"
    "uniform mat4 "SCE_DEFERRED_INVPROJ_NAME";"
    /* TODO: we should use SCE_MAX_DEFERRED_CASCADED_SPLITS */
    "uniform mat4 "SCE_DEFERRED_LIGHT_VIEWPROJ_NAME"[8];"
    "uniform int "SCE_DEFERRED_CSM_NUM_SPLITS_NAME";"
    "uniform vec3 "SCE_DEFERRED_LIGHT_POSITION_NAME";"
    "uniform vec3 "SCE_DEFERRED_LIGHT_DIRECTION_NAME";"
    "uniform vec3 "SCE_DEFERRED_LIGHT_COLOR_NAME";"
};


int SCE_Deferred_BuildLightShader (SCE_SDeferred *def,
                                   SCE_ELightType type,
                                   SCE_SDeferredLightingShader *shd,
                                   const char *fname)
{
    int i, j;
    const char *type_name[SCE_NUM_LIGHT_TYPES] = {
        SCE_DEFERRED_POINT_LIGHT,
        SCE_DEFERRED_SPOT_LIGHT,
        SCE_DEFERRED_SUN_LIGHT
    };
    const char *states[4] = {
        /* order matters */
        SCE_DEFERRED_USE_SHADOWS_NAME,
        SCE_DEFERRED_USE_SOFT_SHADOWS_NAME,
        SCE_DEFERRED_USE_SPECULAR_NAME,
        SCE_DEFERRED_USE_IMAGE_NAME
    };
    SCE_SRenderState rs;

    SCE_RenderState_Init (&rs);
    if (SCE_RenderState_SetStates (&rs, states, 4) < 0) goto fail;
    if (!(shd[0].shader = SCE_Shader_Load (fname, 1))) goto fail;
    if (SCE_Shader_SetupPipeline (shd[0].shader, &rs) < 0) goto fail;
    SCE_RenderState_Clear (&rs);

    for (i = 0; i < SCE_NUM_DEFERRED_LIGHT_FLAGS; i++) {
        shd[i].shader = SCE_Shader_GetShader (shd[0].shader, i);

        /* SCEngine built-in code */
#define SCE_DEF_ADDSRC(src, type)                                   \
    do {                                                            \
        if (SCE_Shader_AddSource (shd[i].shader, type, src, SCE_FALSE) < 0) \
            goto fail;                                                    \
    } while (0)

        /* uniforms */
        SCE_DEF_ADDSRC (sce_final_uniforms_code[type], SCE_PIXEL_SHADER);
        /* add unpacking functions' sources */
        SCE_DEF_ADDSRC (sce_unpack_color_fun, SCE_PIXEL_SHADER);
        SCE_DEF_ADDSRC (sce_unpack_normal_fun, SCE_PIXEL_SHADER);
        SCE_DEF_ADDSRC (sce_unpack_position_fun, SCE_PIXEL_SHADER);
#undef SCE_DEF_ADDSRC


        /* type flag */
        if (SCE_Shader_Global (shd[i].shader, type_name[type], "1") < 0)
            goto fail;
    }

    if (SCE_Shader_Build (shd[0].shader) < 0) goto fail;

    for (i = 0; i < SCE_NUM_DEFERRED_LIGHT_FLAGS; i++) {
        /* setup uniforms */
#define SCE_DEF_UNI(a, b)                       \
    do {                                        \
        shd[i].a##_loc = SCE_Shader_GetIndex (shd[i].shader, b);\
    } while (0)

        SCE_DEF_UNI (invproj, SCE_DEFERRED_INVPROJ_NAME);
        SCE_DEF_UNI (lightviewproj, SCE_DEFERRED_LIGHT_VIEWPROJ_NAME);
        SCE_DEF_UNI (lightpos, SCE_DEFERRED_LIGHT_POSITION_NAME);
        SCE_DEF_UNI (lightdir, SCE_DEFERRED_LIGHT_DIRECTION_NAME);
        SCE_DEF_UNI (lightcolor, SCE_DEFERRED_LIGHT_COLOR_NAME);
        SCE_DEF_UNI (lightradius, SCE_DEFERRED_LIGHT_RADIUS_NAME);
        SCE_DEF_UNI (lightangle, SCE_DEFERRED_LIGHT_ANGLE_NAME);
        SCE_DEF_UNI (lightattenuation, SCE_DEFERRED_LIGHT_ATTENUATION_NAME);
        SCE_DEF_UNI (csmnumsplits, SCE_DEFERRED_CSM_NUM_SPLITS_NAME);
#undef SCE_DEF_UNI

        /* constant uniforms */
        SCE_Shader_Use (shd[i].shader);
        for (j = 0; j < def->n_targets; j++)
            SCE_Shader_Param (sce_deferred_target_names[j], j);
        SCE_Shader_Param (SCE_DEFERRED_SHADOW_MAP_NAME, def->n_targets);
        SCE_Shader_Param (SCE_DEFERRED_SHADOW_CUBE_MAP_NAME, def->n_targets);
    }
    SCE_Shader_Use (NULL);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


void SCE_Deferred_PushStates (SCE_SDeferred *def)
{
    int i;

    /* TODO: gl keywords */
    SCE_RSetState2 (GL_DEPTH_TEST, GL_CULL_FACE, SCE_FALSE);
    SCE_RActivateDepthBuffer (SCE_FALSE);

    /* setup textures */
    SCE_Texture_BeginLot ();
    SCE_Texture_Use (def->targets[0]);
    for (i = 2; i < def->n_targets; i++)
        SCE_Texture_Use (def->targets[i]);
    SCE_Texture_EndLot ();

    SCE_RLoadMatrix (SCE_MAT_CAMERA, sce_matrix4_id);
    SCE_RLoadMatrix (SCE_MAT_PROJECTION, sce_matrix4_id);
}
void SCE_Deferred_PopStates (SCE_SDeferred *def)
{
    (void)def;
    SCE_Texture_Flush ();
    SCE_RActivateDepthBuffer (SCE_TRUE);
    SCE_RSetState2 (GL_DEPTH_TEST, GL_CULL_FACE, SCE_TRUE);
}
