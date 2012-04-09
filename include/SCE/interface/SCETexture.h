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
 
/* created: 11/03/2007
   updated: 04/02/2012 */

#ifndef SCETEXTURE_H
#define SCETEXTURE_H

#include <stdarg.h>

#include <SCE/utils/SCEUtils.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCESceneResource.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \ingroup texture
 * @{
 */

/* filtres de redimensionnement */
/* NOTE: noms constantes a revoir (conflit avec SCE_TEX_NEAREST & cie) */
/* TODO: qu'est-ce que ces constantes foutent ici ?? */
#define SCE_FILTER_NEAREST      ILU_NEAREST
#define SCE_FILTER_LINEAR       ILU_LINEAR
#define SCE_FILTER_BILINEAR     ILU_BILINEAR
#define SCE_FILTER_BOX          ILU_SCALE_BOX
#define SCE_FILTER_TRIANGLE     ILU_SCALE_TRIANGLE
#define SCE_FILTER_BELL         ILU_SCALE_BELL
#define SCE_FILTER_BSPLINE      ILU_SCALE_BSPLINE
#define SCE_FILTER_LANCZOS3     ILU_SCALE_LANCZOS3
#define SCE_FILTER_MITCHELL     ILU_SCALE_MITCHELL

typedef enum {
    SCE_RENDER_COLOR,
    SCE_RENDER_DEPTH,
    SCE_RENDER_STENCIL,
    SCE_RENDER_DEPTH_STENCIL
} SCE_ETexRenderType;

/* some aliases for compatibility */
#define SCE_RENDER_POSX SCE_BOX_POSX
#define SCE_RENDER_NEGX SCE_BOX_NEGX
#define SCE_RENDER_POSY SCE_BOX_POSY
#define SCE_RENDER_NEGY SCE_BOX_NEGY
#define SCE_RENDER_POSZ SCE_BOX_POSZ
#define SCE_RENDER_NEGZ SCE_BOX_NEGZ

#define SCE_MAX_TEXTURE_UNITS 32


/** \copydoc sce_stexture */
typedef struct sce_stexture SCE_STexture;
/**
 * \brief A SCE texture that stores many informations about one or more textures
 */
struct sce_stexture {
    SCE_RFramebuffer *fb[6]; /**< Integrated frame buffers (6 for cubemaps) */
    SCE_RTexture *tex;       /**< Core texture */
    unsigned int unit;       /**< Texture unit */
    SCE_TMatrix4 matrix;     /**< Texture matrix */
    SCE_RTexType type;       /**< Type of the texture */
    int w, h, d;             /**< Used to save Create() input */
    int used;                /**< Is texture used for rendering? */
    int toremove;            /**< Internal use (like everything lol) */
    SCE_SListIterator it;    /**< Own iterator */
    SCE_SSceneResource s_resource; /**< Scene resource */
};

/** @} */

int SCE_Init_Texture (void);
void SCE_Quit_Texture (void);

SCE_STexture* SCE_Texture_Create (SCE_RTexType, int, int, int);
void SCE_Texture_Delete (SCE_STexture*);

SCE_SSceneResource* SCE_Texture_GetSceneResource (SCE_STexture*);

void SCE_Texture_SetFilter (SCE_STexture*, SCE_RTexFilter);
void SCE_Texture_Pixelize (SCE_STexture*, int);
void SCE_Texture_SetAnisotropic (SCE_STexture*, float);
void SCE_Texture_SetWrapMode (SCE_STexture*, SCE_RTexWrapMode);

void SCE_Texture_SetParam (SCE_STexture*, SCEenum, SCEint) SCE_GNUC_DEPRECATED;
void SCE_Texture_SetParamf (SCE_STexture*, SCEenum, SCEfloat) SCE_GNUC_DEPRECATED;

void SCE_Texture_SetUnit (SCE_STexture*, unsigned int);
unsigned int SCE_Texture_GetUnit (SCE_STexture*);

float* SCE_Texture_GetMatrix (SCE_STexture*);

void SCE_Texture_ForcePixelFormat (int, SCE_EPixelFormat);
void SCE_Texture_ForceType (int, SCE_EType);
void SCE_Texture_ForceFormat (int, SCE_EImageFormat);

SCE_RTexType SCE_Texture_GetType (SCE_STexture*);

SCE_RTexture* SCE_Texture_GetRTexture (SCE_STexture*);
void SCE_Texture_AddTexData (SCE_STexture*, SCE_RTexCubeFace, SCE_STexData*);
SCE_STexData* SCE_Texture_AddTexDataDup (SCE_STexture*, SCE_RTexCubeFace,
                                         const SCE_STexData*);

int SCE_Texture_GetWidth (SCE_STexture*);
int SCE_Texture_GetHeight (SCE_STexture*);
int SCE_Texture_GetDepth (SCE_STexture*);

int SCE_Texture_Build (SCE_STexture*, int);
void SCE_Texture_Update (SCE_STexture*);

void SCE_Texture_MakeRenderTexData (const SCE_STexture*, SCE_ETexRenderType,
                                    SCE_STexData*);
int SCE_Texture_SetupFramebuffer (SCE_STexture*, SCE_ETexRenderType,
                                  SCEuint, int, int);

SCE_STexture* SCE_Texture_Loadv (int, int, int, int, int, const char**);
SCE_STexture* SCE_Texture_Load (int, int, int, int, int, ...);

int SCE_Texture_AddRenderRTexture (SCE_STexture*, SCE_RBufferType,
                                   SCE_RTexture*);
int SCE_Texture_AddRenderTexture (SCE_STexture*, int, SCE_STexture*);

void SCE_Texture_GenericBlit (SCE_SFloatRect*, SCE_STexture*,
                              SCE_SFloatRect*, SCE_STexture*, int);
void SCE_Texture_Blit (SCE_SIntRect*, SCE_STexture*,
                       SCE_SIntRect*, SCE_STexture*);
void SCE_Texture_Blitf (SCE_SFloatRect*, SCE_STexture*,
                        SCE_SFloatRect*, SCE_STexture*);

void SCE_Texture_Use (SCE_STexture*);

void SCE_Texture_Lock (void);
void SCE_Texture_Unlock (void);

void SCE_Texture_Flush (void);

void SCE_Texture_BeginLot (void);
void SCE_Texture_EndLot (void);

#if 0
void SCE_Texture_Mark (void);
void SCE_Texture_Restore (void);
#endif

void SCE_Texture_RenderTo (SCE_STexture*, SCE_EBoxFace);
void SCE_Texture_RenderToLayer (SCE_STexture*, int);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
