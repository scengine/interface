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
   updated: 15/08/2011 */

#ifndef SCEDEFERRED_H
#define SCEDEFERRED_H

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEShaders.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: ugly */
#define SCE_DEFERRED_AMBIENT_COLOR_NAME "sce_ambient_color"
#define SCE_DEFERRED_SKYBOX_MAP_NAME "sce_skybox_map"
#define SCE_DEFERRED_SHADOW_MAP_NAME "sce_shadow_map"

#define SCE_DEFERRED_DEPTH_FACTOR_NAME "sce_depth_factor"

#define SCE_DEFERRED_INVPROJ_NAME "sce_deferred_invproj_matrix"

/* projection * view matrix of the light camera, used for shadowing */
#define SCE_DEFERRED_LIGHT_VIEWPROJ_NAME "sce_light_viewproj"
#define SCE_DEFERRED_LIGHT_POSITION_NAME "sce_light_position"
#define SCE_DEFERRED_LIGHT_DIRECTION_NAME "sce_light_direction"
#define SCE_DEFERRED_LIGHT_COLOR_NAME "sce_light_color"
#define SCE_DEFERRED_LIGHT_RADIUS_NAME "sce_light_radius"
#define SCE_DEFERRED_LIGHT_ANGLE_NAME "sce_light_angle"
#define SCE_DEFERRED_LIGHT_ATTENUATION_NAME "sce_light_attenuation"

/* shader light flags */
#define SCE_DEFERRED_USE_SHADOWS (0x00000001)
#define SCE_DEFERRED_USE_SOFT_SHADOWS (0x00000002)
#define SCE_DEFERRED_USE_SPECULAR (0x00000004)
#define SCE_DEFERRED_USE_IMAGE (0x00000008)
/* number of flags combinations actually. */
#define SCE_NUM_DEFERRED_LIGHT_FLAGS (SCE_DEFERRED_USE_IMAGE << 1)

/* shader light flags names */
#define SCE_DEFERRED_POINT_LIGHT "SCE_DEFERRED_POINT_LIGHT"
#define SCE_DEFERRED_SPOT_LIGHT "SCE_DEFERRED_SPOT_LIGHT"
#define SCE_DEFERRED_SUN_LIGHT "SCE_DEFERRED_SUN_LIGHT"

#define SCE_DEFERRED_USE_SHADOWS_NAME "SCE_DEFERRED_USE_SHADOWS"
#define SCE_DEFERRED_USE_SOFT_SHADOWS_NAME "SCE_DEFERRED_USE_SOFT_SHADOWS"
#define SCE_DEFERRED_USE_SPECULAR_NAME "SCE_DEFERRED_USE_SPECULAR"
#define SCE_DEFERRED_USE_IMAGE_NAME "SCE_DEFERRED_USE_IMAGE"

typedef enum {
    SCE_DEFERRED_COLOR_TARGET = 0,
    SCE_DEFERRED_DEPTH_TARGET,
    SCE_DEFERRED_NORMAL_TARGET,
    SCE_DEFERRED_SPECULAR_TARGET,
    SCE_DEFERRED_EMISSIVE_TARGET,
    SCE_NUM_DEFERRED_TARGETS
} SCE_EDeferredTarget;

typedef struct sce_sdeferredlightingshader SCE_SDeferredLightingShader;
struct sce_sdeferredlightingshader {
    SCE_SShader *shader;  /**< Lighting shader */
    int invproj_loc;      /**< Location of the inverse projection matrix */
    int lightviewproj_loc;/**< Location of the light projection * view matrix */
    int lightpos_loc;     /**< Location of the light position uniform */
    int lightdir_loc;     /**< Location of the light direction uniform */
    int lightcolor_loc;   /**< Location of the light color uniform */
    int lightradius_loc;  /**< Location of the light radius uniform */
    int lightangle_loc;   /**< Location of the light angle uniform */
    int lightattenuation_loc; /**< Location of the light attenuation uniform */
    int depthfactor_loc;  /**< Location of the depth factor for shadows */
    int camviewproj_loc;  /**< Light's camera viewproj matrix location */
};

typedef struct sce_sdeferred SCE_SDeferred;
struct sce_sdeferred {
    SCE_STexture *gbuf;
    SCE_STexture *targets[SCE_NUM_DEFERRED_TARGETS]; /**< Render buffers */
    int n_targets;                                   /**< Used targets */
    SCEuint w, h;               /**< targets' dimensions */

    float amb_color[3];

    SCE_SShader *amb_shader;
    SCE_SShader *skybox_shader;

    SCE_SShader *shadow_shaders[SCE_NUM_LIGHT_TYPES];

    SCE_SDeferredLightingShader
        shaders[SCE_NUM_LIGHT_TYPES][SCE_NUM_DEFERRED_LIGHT_FLAGS];
    SCE_STexture *shadowmaps[SCE_NUM_LIGHT_TYPES];
    SCEuint sm_w, sm_h;
    /** Location of the factor uniform of shadow shaders */
    int factor_loc[SCE_NUM_LIGHT_TYPES];

    SCE_SCamera *cam;
};

/*
 * Texture units convention
 *
 * field                        | texture unit
 * -------------------------------------------
 * targets[i]                     i
 * shadowmaps[*]                  n_targets
 * Per-light images               n_targets + 1
 *
 * Since targets[SCE_DEFERRED_COLOR_TARGET] may not be used during
 * lighting, I suggest targets[i] has texunit i - 1
 *
 */

SCE_SDeferred* SCE_Deferred_Create (void);
void SCE_Deferred_Delete (SCE_SDeferred*);

void SCE_Deferred_SetDimensions (SCE_SDeferred*, SCEuint, SCEuint);
void SCE_Deferred_SetShadowMapsDimensions (SCE_SDeferred*, SCEuint, SCEuint);

int SCE_Deferred_Build (SCE_SDeferred*, const char*[SCE_NUM_LIGHT_TYPES]);
int SCE_Deferred_BuildShader (SCE_SDeferred*, SCE_SShader*);

void SCE_Deferred_PushStates (SCE_SDeferred*);
void SCE_Deferred_PopStates (SCE_SDeferred*);

void SCE_Deferred_Render (SCE_SDeferred*, void*, SCE_SCamera*,
                          SCE_STexture*, int);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
