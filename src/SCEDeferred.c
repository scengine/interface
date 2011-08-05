/*------------------------------------------------------------------------------
    SCEngine - A 3D real time rendering engine written in the C language
    Copyright (C) 2006-2011  Antony Martin <martin(dot)antony(at)yahoo(dot)fr>

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
   updated: 05/08/2011 */

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCEScene.h"
#include "SCE/interface/SCEDeferred.h"

#define SCE_DEFERRED_COLOR_TARGET_NAME "sce_deferred_color_map"
#define SCE_DEFERRED_DEPTH_TARGET_NAME "sce_deferred_depth_map"
#define SCE_DEFERRED_NORMAL_TARGET_NAME "sce_deferred_normal_map"
#define SCE_DEFERRED_SPECULAR_TARGET_NAME "sce_deferred_specular_map"
#define SCE_DEFERRED_EMISSIVE_TARGET_NAME "sce_deferred_emissive_map"

/* shader uniform's names */
static const char *sce_deferred_target_names[SCE_NUM_DEFERRED_TARGETS] = {
    SCE_DEFERRED_COLOR_TARGET_NAME,
    SCE_DEFERRED_DEPTH_TARGET_NAME,
    SCE_DEFERRED_NORMAL_TARGET_NAME,
    SCE_DEFERRED_SPECULAR_TARGET_NAME,
    SCE_DEFERRED_EMISSIVE_TARGET_NAME
};


static void SCE_Deferred_Init (SCE_SDeferred *def)
{
    int i;
    def->gbuf = NULL;
    for (i = 0; i < SCE_NUM_DEFERRED_TARGETS; i++)
        def->targets[i] = NULL;
    def->n_targets = 0;
    def->w = def->h = 64;       /* xd */

    def->amb_color[0] = def->amb_color[1] = def->amb_color[2] = 0.1;

    def->amb_shader = NULL;
    def->skybox_shader = NULL;
    def->point_shader = NULL;
    def->spot_shader = NULL;
    def->sun_shader = NULL;
}
static void SCE_Deferred_Clear (SCE_SDeferred *def)
{
    int i;
    for (i = 0; i < SCE_NUM_DEFERRED_TARGETS; i++)
        SCE_Texture_Delete (def->targets[i]);

    SCE_Shader_Delete (def->amb_shader);
    SCE_Shader_Delete (def->skybox_shader);
    SCE_Shader_Delete (def->point_shader);
    SCE_Shader_Delete (def->spot_shader);
    SCE_Shader_Delete (def->sun_shader);
}

SCE_SDeferred* SCE_Deferred_Create (void)
{
    SCE_SDeferred *def = NULL;
    if (!(def = SCE_malloc (sizeof *def)))
        SCEE_LogSrc ();
    else
        SCE_Deferred_Init (def);
    return def;
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

static const char *sce_amb_vs =
    "uniform mat4 sce_modelviewmatrix;"
    "uniform mat4 sce_projectionmatrix;"
    "varying vec2 tc;"
    "void main (void)"
    "{"
    "tc = gl_MultiTexCoord0.xy;"
    "gl_Position = sce_projectionmatrix * sce_modelviewmatrix * gl_Vertex;"
    "}";
static const char *sce_amb_ps =
    "uniform sampler2D "SCE_DEFERRED_COLOR_TARGET_NAME";"
    "uniform vec3 "SCE_DEFERRED_AMBIENT_COLOR_NAME";"
    "varying vec2 tc;"
    "void main (void)"
    "{"
    "gl_FragColor = vec4 ("SCE_DEFERRED_AMBIENT_COLOR_NAME", 1.0)"
    "               * texture2D ("SCE_DEFERRED_COLOR_TARGET_NAME", tc);"
    "}";


static const char *sce_skybox_vs =
    "uniform mat4 sce_modelviewmatrix;"
    "uniform mat4 sce_projectionmatrix;"
    "varying vec3 tc;"
    "varying vec4 pos;"
    "void main (void)"
    "{"
    "tc = gl_MultiTexCoord0.xyz;"
    "pos = sce_projectionmatrix * sce_modelviewmatrix * gl_Vertex;"
    "gl_Position = pos;"
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
    "if (depth > 0.9999)"
    "  gl_FragColor = textureCube ("SCE_DEFERRED_SKYBOX_MAP_NAME", tc);"
    "else"
    "  discard;"
    "}";


int SCE_Deferred_Build (SCE_SDeferred *def, const char *fname)
{
    int i;

#define SCECREATE(a, b)                                         \
    def->targets[a] = SCE_Texture_Create (b, def->w, def->h)
    SCECREATE (SCE_DEFERRED_COLOR_TARGET, SCE_RENDER_COLOR);
    SCECREATE (SCE_DEFERRED_DEPTH_TARGET, SCE_RENDER_DEPTH);
    SCECREATE (SCE_DEFERRED_NORMAL_TARGET, SCE_TEX_2D);
    SCECREATE (SCE_DEFERRED_SPECULAR_TARGET, SCE_TEX_2D);
#undef SCECREATE

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
    SCE_Texture_AddRenderTexture (def->gbuf, SCE_DEPTH_BUFFER, def->targets[1]);
    for (i = 2; i < def->n_targets; i++) {
        SCE_Texture_AddRenderTexture (def->gbuf, SCE_COLOR_BUFFER1 + i - 2,
                                      def->targets[i]);
    }

    /* create shaders */
#if 0
    if (!(def->point_shader = SCE_Shader_Create ()))
        goto fail;
    if (SCE_Shader_AddSource (def->point_shader, SCE_VERTEX_SHADER,
                              sce_point_vs) < 0)
        goto fail;
    if (SCE_Shader_AddSource (def->point_shader, SCE_PIXEL_SHADER,
                              sce_point_ps) < 0)
        goto fail;
#else
    if (!(def->point_shader = SCE_Shader_Load (fname, 0)))
        goto fail;
#endif
    if (SCE_Shader_Build (def->point_shader) < 0)
        goto fail;
    SCE_Shader_Use (def->point_shader);
    for (i = 0; i < def->n_targets; i++)
        SCE_Shader_Param (sce_deferred_target_names[i], i);
    SCE_Shader_Use (NULL);

    /* setup ambient lighting shader */
    if (!(def->amb_shader = SCE_Shader_Create ()))
        goto fail;
    if (SCE_Shader_AddSource (def->amb_shader, SCE_VERTEX_SHADER,
                              sce_amb_vs) < 0)
        goto fail;
    if (SCE_Shader_AddSource (def->amb_shader, SCE_PIXEL_SHADER,
                              sce_amb_ps) < 0)
        goto fail;
    if (SCE_Shader_Build (def->amb_shader) < 0)
        goto fail;
    SCE_Shader_Use (def->amb_shader);
    for (i = 0; i < def->n_targets; i++)
        SCE_Shader_Param (sce_deferred_target_names[i], i);
    SCE_Shader_Use (NULL);
    SCE_Shader_SetupMatricesMapping (def->amb_shader);
    SCE_Shader_ActivateMatricesMapping (def->amb_shader, SCE_TRUE);

    /* setup skybox shader */
    if (!(def->skybox_shader = SCE_Shader_Create ()))
        goto fail;
    if (SCE_Shader_AddSource (def->skybox_shader, SCE_VERTEX_SHADER,
                              sce_skybox_vs) < 0)
        goto fail;
    if (SCE_Shader_AddSource (def->skybox_shader, SCE_PIXEL_SHADER,
                              sce_skybox_ps) < 0)
        goto fail;
    if (SCE_Shader_Build (def->skybox_shader) < 0)
        goto fail;
    SCE_Shader_Use (def->skybox_shader);
    for (i = 0; i < def->n_targets; i++)
        SCE_Shader_Param (sce_deferred_target_names[i], i);
    SCE_Shader_Use (NULL);
    SCE_Shader_SetupMatricesMapping (def->skybox_shader);
    SCE_Shader_ActivateMatricesMapping (def->skybox_shader, SCE_TRUE);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


/**
 * \brief Shaders factory
 * \param def a deferred renderer
 * \param shader the shader to build
 *
 * This function comes in replacement of SCE_Shader_Build(). It adds
 * some code to the shader to make it work well with the given renderer.
 * 
 * \returns SCE_ERROR on error, SCE_OK otherwise
 */
int SCE_Deferred_BuildShader (SCE_SDeferred *def, SCE_SShader *shader)
{
    int i;

    if (SCE_Shader_Build (shader) < 0) goto fail;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}
