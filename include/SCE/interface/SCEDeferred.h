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

#ifndef SCEDEFERRED_H
#define SCEDEFERRED_H

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCEShaders.h"
#include "SCE/interface/SCELight.h"

#ifdef __cplusplus
extern "C" {
#endif

/* TODO: ugly */
#define SCE_DEFERRED_AMBIENT_COLOR_NAME "sce_ambient_color"
#define SCE_DEFERRED_SKYBOX_MAP_NAME "sce_skybox_map"
#define SCE_DEFERRED_SHADOW_MAP_NAME "sce_shadow_map"
#define SCE_DEFERRED_SHADOW_CUBE_MAP_NAME "sce_shadow_cube_map"

#define SCE_DEFERRED_INVPROJ_NAME "sce_deferred_invproj_matrix"

/* projection * view matrix of the light camera, used for shadowing */
#define SCE_DEFERRED_LIGHT_VIEWPROJ_NAME "sce_light_viewproj"
#define SCE_DEFERRED_LIGHT_POSITION_NAME "sce_light_position"
#define SCE_DEFERRED_LIGHT_DIRECTION_NAME "sce_light_direction"
#define SCE_DEFERRED_LIGHT_COLOR_NAME "sce_light_color"
#define SCE_DEFERRED_LIGHT_RADIUS_NAME "sce_light_radius"
#define SCE_DEFERRED_LIGHT_ANGLE_NAME "sce_light_angle"
#define SCE_DEFERRED_LIGHT_ATTENUATION_NAME "sce_light_attenuation"
#define SCE_DEFERRED_CSM_NUM_SPLITS_NAME "sce_csm_num_splits"

/* shader light flags */
#define SCE_DEFERRED_USE_SHADOWS (0x00000001)
#define SCE_DEFERRED_USE_SOFT_SHADOWS (0x00000002)
#define SCE_DEFERRED_USE_SPECULAR (0x00000004)
#define SCE_DEFERRED_USE_IMAGE (0x00000008)
/* number of flags combinations actually. */
#define SCE_NUM_DEFERRED_LIGHT_FLAGS (SCE_DEFERRED_USE_IMAGE << 1)
#define SCE_DEFERRED_USE_ALL (SCE_NUM_DEFERRED_LIGHT_FLAGS - 1)

/* shader light flags names */
#define SCE_DEFERRED_POINT_LIGHT "SCE_DEFERRED_POINT_LIGHT"
#define SCE_DEFERRED_SPOT_LIGHT "SCE_DEFERRED_SPOT_LIGHT"
#define SCE_DEFERRED_SUN_LIGHT "SCE_DEFERRED_SUN_LIGHT"

#define SCE_DEFERRED_USE_SHADOWS_NAME "SCE_DEFERRED_USE_SHADOWS"
#define SCE_DEFERRED_USE_SOFT_SHADOWS_NAME "SCE_DEFERRED_USE_SOFT_SHADOWS"
#define SCE_DEFERRED_USE_SPECULAR_NAME "SCE_DEFERRED_USE_SPECULAR"
#define SCE_DEFERRED_USE_IMAGE_NAME "SCE_DEFERRED_USE_IMAGE"

/* huhu. */
#define SCE_DEFERRED_CAMERA_SPACE_POS_NAME "sce_camera_space_pos"
#define SCE_DEFERRED_PROJECTION_SPACE_Z_NAME "sce_projection_space_z"

#define SCE_MAX_DEFERRED_CASCADED_SPLITS 16

typedef enum {
    SCE_DEFERRED_DEPTH_TARGET = 0,
    SCE_DEFERRED_LIGHT_TARGET,
    SCE_DEFERRED_COLOR_TARGET,
    SCE_DEFERRED_NORMAL_TARGET,
    SCE_DEFERRED_SPECULAR_TARGET,
    SCE_NUM_DEFERRED_TARGETS
} SCE_EDeferredTarget;

/* NOTE: see SCELight.h */
#ifndef SCE_DECLARED_SDEFERREDLIGHTINGSHADER
#define SCE_DECLARED_SDEFERREDLIGHTINGSHADER
typedef struct sce_sdeferredlightingshader SCE_SDeferredLightingShader;
#endif
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
    int csmnumsplits_loc;     /**< Used for CSM */
    int camviewproj_loc;  /**< Light's camera viewproj matrix location */
};

#define SCE_MAX_DEFERRED_POINT_LIGHT_RADIUS (1000.0)
#define SCE_DEFERRED_POINT_LIGHT_DEPTH_FACTOR   \
    (1.0/SCE_MAX_DEFERRED_POINT_LIGHT_RADIUS)

typedef struct sce_sdeferred SCE_SDeferred;
struct sce_sdeferred {
    SCE_STexture *gbuf;
    SCE_STexture *targets[SCE_NUM_DEFERRED_TARGETS]; /**< Render buffers */
    int n_targets;                                   /**< Used targets */
    SCEuint w, h;               /**< targets' dimensions */

    float amb_color[3];

    SCE_SShader *skybox_shader;
    SCE_SShader *final_shader;

    /** Shader for rendering point light shadow maps */
    SCE_SShader *shadowcube_shader;
    int shadowcube_factor_loc;  /* TODO: unused.. yet. */

    int lightflags_mask;
    SCE_SDeferredLightingShader
        shaders[SCE_NUM_LIGHT_TYPES][SCE_NUM_DEFERRED_LIGHT_FLAGS];
    SCE_STexture *shadowmaps[SCE_NUM_LIGHT_TYPES];
    SCEuint sm_w, sm_h;
    SCEuint cascaded_splits;
    float csm_far;              /* customized far plane for CSM */

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

void SCE_Deferred_InitLightingShader (SCE_SDeferredLightingShader*);
void SCE_Deferred_ClearLightingShader (SCE_SDeferredLightingShader*);

SCE_SDeferred* SCE_Deferred_Create (void);
void SCE_Deferred_Delete (SCE_SDeferred*);

void SCE_Deferred_SetDimensions (SCE_SDeferred*, SCEuint, SCEuint);
void SCE_Deferred_SetShadowMapsDimensions (SCE_SDeferred*, SCEuint, SCEuint);
void SCE_Deferred_SetCascadedSplits (SCE_SDeferred*, SCEuint);
void SCE_Deferred_SetCascadedFar (SCE_SDeferred*, float);

void SCE_Deferred_AddLightFlag (SCE_SDeferred*, int);
void SCE_Deferred_RemoveLightFlag (SCE_SDeferred*, int);
void SCE_Deferred_SetLightFlagsMask (SCE_SDeferred*, int);
int SCE_Deferred_GetLightFlagsMask (SCE_SDeferred*);

int SCE_Deferred_Build (SCE_SDeferred*, const char*[SCE_NUM_LIGHT_TYPES]);
int SCE_Deferred_BuildShader (SCE_SDeferred*, SCE_SShader*);
int SCE_Deferred_BuildPointShadowShader (SCE_SDeferred*, SCE_SShader*);
int SCE_Deferred_BuildLightShader (SCE_SDeferred*, SCE_ELightType,
                                   SCE_SDeferredLightingShader*, const char*);

void SCE_Deferred_PushStates (SCE_SDeferred*);
void SCE_Deferred_PopStates (SCE_SDeferred*);

void SCE_Deferred_Render (SCE_SDeferred*, void*, SCE_SCamera*,
                          SCE_STexture*, SCE_EBoxFace);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
