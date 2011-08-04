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
   updated: 04/08/2011 */

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCEScene.h"
#include "SCE/interface/SCEDeferred.h"

/* shader uniform's names */
static const char *sce_deferred_target_names[SCE_NUM_DEFERRED_TARGETS] = {
    "sce_deferred_color_map",
    "sce_deferred_depth_map",
    "sce_deferred_normal_map",
    "sce_deferred_specular_map",
    "sce_deferred_emissive_map"
};


static void SCE_Deferred_Init (SCE_SDeferred *def)
{
    int i;
    def->gbuf = NULL;
    for (i = 0; i < SCE_NUM_DEFERRED_TARGETS; i++)
        def->targets[i] = NULL;
    def->n_targets = 0;
    def->w = def->h = 64;       /* xd */
}
static void SCE_Deferred_Clear (SCE_SDeferred *def)
{
    int i;
    for (i = 0; i < SCE_NUM_DEFERRED_TARGETS; i++)
        SCE_Texture_Delete (def->targets[i]);
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


int SCE_Deferred_Build (SCE_SDeferred *def)
{
    int i;
    int colorid = 1;

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

    SCE_Shader_Use (shader);
    for (i = 0; i < def->n_targets; i++)
        SCE_Shader_Param (sce_deferred_target_names[i], i);
    SCE_Shader_Use (shader);

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


void SCE_Deferred_Render (SCE_SDeferred *def, void *scene_,
                          SCE_SCamera *cam, SCE_STexture *target, int cubeface)
{
    SCE_SScene *scene = scene_;
}
