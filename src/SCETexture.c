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
 
/* created: 11/03/2007
   updated: 01/09/2011 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCEQuad.h"
#include "SCE/interface/SCETexture.h"

/**
 * \file SCETexture.c
 * \copydoc texture
 * 
 * \file SCETexture.h
 * \copydoc texture
 */

/**
 * \defgroup texture Textures and render textures
 * \ingroup interface
 * \internal
 * \brief Unify all texture's types and render textures in one module to provide
 * an user-friendly textures managment, this module offers a simplified
 * interface to use jointly framebuffer and coretexture managers
 */

/** @{ */

static int resource_type = 0;

static SCE_STexture *textmp = NULL;

static int texlocked = SCE_FALSE;
static SCE_SList texused;
#if 0
static SCE_SListIterator *mark = NULL;
#endif

static SCE_STexture *unitused[SCE_MAX_TEXTURE_UNITS];

static void SCE_Texture_PopTexture (void *tex)
{
    unitused[((SCE_STexture*)tex)->unit] = NULL;
    SCE_RUseTexture (NULL, ((SCE_STexture*)tex)->unit);
    ((SCE_STexture*)tex)->used = SCE_FALSE;
}

static void* SCE_Texture_LoadResource (const char*, int, void*);

int SCE_Init_Texture (void)
{
    unsigned int i;
    SCE_List_Init (&texused);
    SCE_List_SetFreeFunc (&texused, SCE_Texture_PopTexture);
    for (i = 0; i < SCE_MAX_TEXTURE_UNITS; i++)
        unitused[i] = NULL;
    resource_type = SCE_Resource_RegisterType (
        SCE_FALSE, SCE_Texture_LoadResource, NULL);
    if (resource_type < 0)
        goto fail;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

void SCE_Quit_Texture (void)
{
    SCE_List_Clear (&texused);
}

static void SCE_Texture_Init (SCE_STexture *tex)
{
    unsigned int i;
    for (i = 0; i < 6; i++)
        tex->fb[i] = NULL;
    tex->tex = NULL;
    tex->unit = 0;
    SCE_Matrix4_Identity (tex->matrix);
    tex->type = 0;
    tex->used = SCE_FALSE;
    tex->toremove = SCE_FALSE;
    SCE_List_InitIt (&tex->it);
    SCE_List_SetData (&tex->it, tex);
    SCE_SceneResource_Init (&tex->s_resource);
    SCE_SceneResource_SetResource (&tex->s_resource, tex);
}

static void SCE_Texture_SetupParameters (SCE_STexture *tex)
{
    int type = tex->type;
    /* assignation de quelques parametres pour les textures cubemap */
    if (type == SCE_RENDER_COLOR_CUBE || type == SCE_TEX_CUBE ||
        type == SCE_RENDER_DEPTH_CUBE || type == SCE_RENDER_DEPTH ||
        type == SCE_RENDER_COLOR) {
        SCE_RSetTextureParam (tex->tex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        SCE_RSetTextureParam (tex->tex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        SCE_RSetTextureParam (tex->tex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    }
}

static int SCE_Texture_MakeRender (SCE_STexture *tex, int type)
{
    int code = SCE_OK;
    int w = tex->w, h = tex->h/*, d*/;

    tex->fb[0] = SCE_RCreateFramebuffer ();
    if (!tex->fb[0])
        goto failure;

    if (SCE_RCreateRenderTexture (tex->fb[0], type, 0, 0, 0, w, h) < 0)
        goto failure;
    /* s'il s'agit d'une color, on ajoute un depth buffer */
    if (type == SCE_COLOR_BUFFER) {
        if (SCE_RAddRenderBuffer (tex->fb[0], SCE_DEPTH_BUFFER, 0, w, h) < 0)
            goto failure;
    }
    /* le proprietaire de la texture reste le frame buffer
       car tex->canfree[n] est egal a SCE_FALSE par defaut */
    tex->tex = SCE_RGetRenderTexture (tex->fb[0], type);
    goto success;

failure:
    SCEE_LogSrc ();
    code = SCE_ERROR;
success:
    return code;
}

static int SCE_Texture_MakeRenderCube (SCE_STexture *tex, int type)
{
    unsigned int i = 0;
    int w = tex->w, h = tex->h/*, d*/;
    SCE_RTexData data;

    tex->tex = SCE_RCreateTexture (SCE_TEX_CUBE);
    if (!tex->tex)
        goto fail;

    for (i = 0; i < 6; i++) {
        tex->fb[i] = SCE_RCreateFramebuffer ();
        if (!tex->fb[i])
            goto fail;
        SCE_RInitTexData (&data);

        /* le target est defini dans AddTextureTexData */
        data.w = w;
        data.h = h;
        data.type = SCE_UNSIGNED_BYTE;

        if (type == SCE_DEPTH_BUFFER) {
            data.fmt = GL_DEPTH_COMPONENT;
            data.pxf = GL_DEPTH_COMPONENT24;
        }
        else
            data.fmt = data.pxf = GL_RGBA;

        if (SCE_RAddTextureTexDataDup (tex->tex, SCE_TEX_POSX + i, &data) < 0)
            goto fail;
    }

    if (SCE_RBuildTexture (tex->tex, 0, 0) < 0)
        goto fail;

    for (i = 0; i < 6; i++) {
        /* ajout de la texture */
        if (SCE_RAddRenderTexture (tex->fb[i], type, SCE_TEX_POSX + i,
                                   tex->tex, 0, SCE_FALSE) < 0)
            goto fail;

        /* s'il s'agit d'une color, on ajoute un depth buffer */
        if (type == SCE_COLOR_BUFFER) {
            if (SCE_RAddRenderBuffer(tex->fb[i], SCE_DEPTH_BUFFER, 0, w, h) < 0)
                goto fail;
        }
    }

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

/**
 * \brief Creates a new texture or a new render texture
 * \param type defines the type of the texture to create, can be SCE_TEX_1D,
 * SCE_TEX_2D, SCE_TEX_3D or SCE_TEX_CUBE to make a simple texture, to make
 * a render texture, you may use the following types : SCE_RENDER_COLOR,
 * SCE_RENDER_COLOR_CUBE, SCE_RENDER_DEPTH or SCE_RENDER_DEPTH_CUBE
 * \param w the width of the new texture
 * \param h the height of the new texture
 * \returns the new texture
 */
SCE_STexture* SCE_Texture_Create (SCEenum type, int w, int h/*, int d*/)
{
    SCE_STexture *tex = NULL;

    tex = SCE_malloc (sizeof *tex);
    if (!tex)
        goto failure;
    tex->w = w;
    tex->h = h;
    /*tex->d = d;*/

    SCE_Texture_Init (tex);
    tex->type = type;
    switch (type) {
    case SCE_RENDER_DEPTH:
        type = SCE_DEPTH_BUFFER;
    case SCE_RENDER_COLOR:
        if (type == SCE_RENDER_COLOR)
            type = SCE_COLOR_BUFFER;
        if (SCE_Texture_MakeRender (tex, type) < 0)
            goto failure;
        break;

    case SCE_RENDER_DEPTH_CUBE:
        type = SCE_DEPTH_BUFFER;
    case SCE_RENDER_COLOR_CUBE:
        if (type == SCE_RENDER_COLOR_CUBE)
            type = SCE_COLOR_BUFFER;
        if (SCE_Texture_MakeRenderCube (tex, type) < 0)
            goto failure;
        break;

    default:
        /* not a render type, just a normal texture */
        tex->tex = SCE_RCreateTexture (type);
        if (!tex->tex)
            goto failure;
    }
    SCE_Texture_SetupParameters (tex);

    return tex;
failure:
    SCE_Texture_Delete (tex);
    SCEE_LogSrc ();
    return NULL;
}


/**
 * \brief Deletes an existing texture
 * \param tex the texture to delete
 */
void SCE_Texture_Delete (SCE_STexture *tex)
{
    if (tex) {
        unsigned int i;
        SCE_SceneResource_RemoveResource (&tex->s_resource);
        for (i = 0; i < 6; i++)
            SCE_RDeleteFramebuffer (tex->fb[i]);
        SCE_RDeleteTexture (tex->tex);
        SCE_free (tex);
    }
}


/**
 * \brief Gets the scene resource of the given texture
 */
SCE_SSceneResource* SCE_Texture_GetSceneResource (SCE_STexture *tex)
{
    return &tex->s_resource;
}

/**
 * \brief Sets the filter of a texture when is far
 * \param tex the texture of which set the filter
 * \param filter the texture's filtering type, can set SCE_TEX_NEAREST,
 * SCE_TEX_LINEAR, SCE_TEX_BILINEAR, SCE_TEX_TRILINEAR.
 * \sa SCE_RSetTextureFilter()
 */
void SCE_Texture_SetFilter (SCE_STexture *tex, SCEint filter)
{
    SCE_RSetTextureFilter (tex->tex, filter);
}
/**
 * \brief Defines if a texture is pixelized when it is near
 * \param tex a texture
 * \param p whether the texture should be pixelized when near; can be SCE_TRUE
 *          or SCE_FALSE
 * \sa SCE_RPixelizeTexture()
 */
void SCE_Texture_Pixelize (SCE_STexture *tex, int p)
{
    SCE_RPixelizeTexture (tex->tex, p);
}

/**
 * \brief Sets an OpenGL texture's parameter
 * \param tex the texture to set the parameter
 * \param pname second glTexParameteri parameter
 * \param param third glTexParameteri parameter
 */
void SCE_Texture_SetParam (SCE_STexture *tex, SCEenum pname, SCEint param)
{
    SCE_RSetTextureParam (tex->tex, pname, param);
}
/**
 * \brief Float version of SCE_Texture_SetParam()
 */
void SCE_Texture_SetParamf (SCE_STexture *tex, SCEenum pname, SCEfloat param)
{
    SCE_RSetTextureParamf (tex->tex, pname, param);
}

/**
 * \brief Sets the texture unit of \p tex
 * \sa SCE_Texture_GetUnit()
 */
void SCE_Texture_SetUnit (SCE_STexture *tex, unsigned int unit)
{
    tex->unit = unit;
}
/**
 * \brief Gets the texture unit of \p tex
 * \sa SCE_Texture_SetUnit()
 */
unsigned int SCE_Texture_GetUnit (SCE_STexture *tex)
{
    return tex->unit;
}

/**
 * \brief Gets the texture matrix of a texture
 *
 * The matrix of a texture is sent to OpenGL when the texture is specified
 * for rendering by calling SCE_Texture_Use().
 */
float* SCE_Texture_GetMatrix (SCE_STexture *tex)
{
    return tex->matrix;
}


/**
 * \brief Forces the pixel format when calling SCE_RAddTextureTexData()
 * \sa SCE_RForceTexturePixelFormat()
 */
void SCE_Texture_ForcePixelFormat (int force, int pxf)
{
    SCE_RForceTexturePixelFormat (force, pxf);
}
/**
 * \brief Forces the type when calling SCE_RAddTextureTexData()
 * \sa SCE_RForceTextureType()
 */
void SCE_Texture_ForceType (int force, int type)
{
    SCE_RForceTextureType (force, type);
}
/**
 * \brief Forces the format when calling SCE_RAddTextureTexData()
 * \sa SCE_RForceTextureFormat()
 */
void SCE_Texture_ForceFormat (int force, int fmt)
{
    SCE_RForceTextureFormat (force, fmt);
}


/**
 * \brief Gets the \p tex 's type that has been given to SCE_Texture_Create()
 */
int SCE_Texture_GetType (SCE_STexture *tex)
{
    return tex->type;
}
/**
 * \brief Gets the type of the core texture used by \p tex
 * this function calls SCE_RGetTextureTarget()
 */
int SCE_Texture_GetCType (SCE_STexture *tex)
{
    return SCE_RGetTextureTarget (tex->tex);
}

/**
 * \brief Gets the core texture used by \p tex
 */
SCE_RTexture* SCE_Texture_GetCTexture (SCE_STexture *tex)
{
    return tex->tex;
}

/**
 * \brief Gets the width of a texture
 *
 * This function calls SCE_RGetTextureWidth (SCE_Texture_GetCTexture (\p tex),
 * \p target, \p level)
 * \sa SCE_RGetTextureWidth()
 */
int SCE_Texture_GetWidth (SCE_STexture *tex, int target, int level)
{
    return SCE_RGetTextureWidth (tex->tex, target, level);
}
/**
 * \brief Gets the height of a texture
 *
 * This function calls SCE_RGetTextureHeight (SCE_Texture_GetCTexture (\p tex),
 * \p target, \p level)
 * \sa SCE_RGetTextureHeight()
 */
int SCE_Texture_GetHeight (SCE_STexture *tex, int target, int level)
{
    return SCE_RGetTextureHeight (tex->tex, target, level);
}


/**
 * \brief Builds a texture created by SCE_Texture_Create(), you must build
 * your textures before any use of them.
 * \param tex the texture to build
 * \param use_mipmap set at SCE_TRUE, the texture will use mipmapping
 * \returns SCE_ERROR on error, SCE_OK otherwise
 */
int SCE_Texture_Build (SCE_STexture *tex, int use_mipmap)
{
    int hw_mipmap;

    /* trying to generate mipmaps on the hardware */
    hw_mipmap = (use_mipmap && SCE_RHasCap (SCE_TEX_HW_GEN_MIPMAP));
    if (SCE_RGetTextureTarget (tex->tex) != SCE_TEX_CUBE)
        hw_mipmap = (SCE_RGetTextureNumMipmaps (tex->tex, 0) <= 1 && hw_mipmap);
    /* adding some data if needed */
    if (!SCE_RHasTextureData (tex->tex)) {
        SCE_RTexData d;
        SCE_RInitTexData (&d);
        d.w = tex->w; d.h = tex->h;/* d.d = tex->d;*/
        if (SCE_RAddTextureTexDataDup (tex->tex, tex->type /* hope */, &d) < 0) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
    }

    if (SCE_RBuildTexture (tex->tex, use_mipmap, hw_mipmap) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }

    return SCE_OK;
}

/**
 * \brief After any change on your texture, this function will apply yours
 * modifications manpower on your texture
 * \returns SCE_ERROR on error, SCE_OK otherwise
 */
int SCE_Texture_Update (SCE_STexture *tex)
{
    if (SCE_RUpdateTexture (tex->tex, -1, -1) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}


typedef struct
{
    int type, w, h, d;
    const char **names;
} SCE_STexResInfo;

static void* SCE_Texture_LoadResource (const char *name, int force, void *data)
{
    int type, w, h, d;
    SCE_STexture *tex = NULL;
    SCE_STexResInfo *rinfo = data;

    (void)name;
    type = rinfo->type;
    w = rinfo->w; h = rinfo->h; d = rinfo->d;

    tex = SCE_Texture_Create (type, w, h);
    if (!tex) {
        SCEE_LogSrc ();
        return NULL;
    }

    /*< TODO: ! temporaire ! */
    SCE_RDeleteTexture (tex->tex);

    if (force > 0)
        force--;                /* downgrading depth */
    tex->tex = SCE_RLoadTexturev (type, w, h, d, force, rinfo->names);
    if (!tex->tex) {
        SCE_Texture_Delete (tex), tex = NULL;
        SCEE_LogSrc ();
    } else
        SCE_Texture_SetupParameters (tex);

    return tex;
}

/**
 * \brief Loads and creates a new texture from one or more files
 * \param type force the texture's type to load, if SCE_TEX_CUBE is specified,
 * six parameters are expected in \p args defining respectively posx, negx,
 * posy, negy, posz and negz cube face's image
 * \param w,h,d force the new texture's dimentions (0 to keep unchanged)
 * \param names the array of the file names
 * \returns the new texture
 * \sa SCE_Texture_Load()
 */
SCE_STexture* SCE_Texture_Loadv (int type, int w, int h, int d, int force,
                                 const char **names)
{
    unsigned int i;
    char buf[2048] = {0};
    SCE_STexture *tex = NULL;
    SCE_STexResInfo info;

    info.type = type;
    info.w = w; info.h = h; info.d = d;
    info.names = names;
    for (i = 0; names[i]; i++)
        strcat (buf, names[i]);

    tex = SCE_Resource_Load (resource_type, buf, force, &info);
    if (!tex)
        goto fail;
    return tex;
fail:
    SCEE_LogSrc ();
    return NULL;
}
/**
 * \brief Loads and creates a new texture by calling SCE_Texture_LoadArg()
 * with the given variables parameters
 * \sa SCE_Texture_LoadArg()
 */
SCE_STexture* SCE_Texture_Load (int type, int w, int h, int d, int force, ...)
{
    va_list args;
    unsigned int i = 0;
    const char *name = NULL;
    const char *names[42];
    SCE_STexture *tex = NULL;

    va_start (args, force);
    name = va_arg (args, const char*);
    while (name && i < 42 - 1) {
        names[i] = name;
        name = va_arg (args, const char*);
        i++;
    }
    va_end (args);
    names[i] = NULL;
#ifdef SCE_DEBUG
    if (i == 0) {
        SCEE_Log (SCE_INVALID_ARG);
        SCEE_LogMsg ("excpected at least 1 file name, but 0 given");
        return NULL;
    }
#endif
    tex = SCE_Texture_Loadv (type, w, h, d, force, names);
    return tex;
}

/**
 * \brief Adds a texture as a new render target's
 * \param tex the texture to add a new render texture
 * \param id render target's identifier (SCE_COLOR_BUFFER, SCE_DEPTH_BUFFER
 * or SCE_STENCIL_BUFFER)
 * \param ctex the texture to add as a render target (is a core texture)
 * \returns SCE_ERROR on error, SCE_OK otherwise
 * \note If \p ctex has different dimensions or pixel format of \p tex,
 * then this function maybe generate an error that is implementation and/or
 * hardware dependent
 *
 * This function requires \p tex is already a render buffer texture
 * (its type must be SCE_RENDER_*). After adding a new render texture target,
 * SCEngine will use the Multiple Render Targets OpenGL's extension, if you
 * don't support it, SCE_ERROR is returned and \p ctex is not added. This
 * function calls SCE_RAddRenderTexture() under the frame buffer of \p tex.
 * \sa SCE_RAddRenderTexture()
 */
int SCE_Texture_AddRenderCTexture (SCE_STexture *tex, int id,
                                   SCE_RTexture *ctex)
{
    SCEenum target;
    unsigned int i = 0;

    target = SCE_RGetTextureTarget (tex->tex);
    if (target != SCE_RGetTextureTarget (ctex)) {
#ifdef SCE_DEBUG
        SCEE_Log (SCE_INVALID_ARG);
        SCEE_LogMsg ("you can't add a render texture "
                       "with a different target");
#endif
        return SCE_ERROR;
    }

    /* adding texture to framebuffer */
    if (target == SCE_TEX_CUBE)
        for (i=0; i<6; i++) {
            if (SCE_RAddRenderTexture (tex->fb[i], id, SCE_TEX_POSX + i, ctex,
                                       SCE_TRUE, SCE_FALSE) < 0) {
                SCEE_LogSrc ();
                return SCE_ERROR;
            }
        }
    else if (SCE_RAddRenderTexture (tex->fb[0], id, target,
                                    ctex, SCE_TRUE, SCE_FALSE) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}
/**
 * \brief Adds a texture as a new render target's
 *
 * This function calls SCE_Texture_AddRenderCTexture() like this:
 * SCE_Texture_AddRenderCTexture (\p tex, \p id, SCE_Texture_GetCTexture
 * (\p new), SCE_FALSE).
 */
int SCE_Texture_AddRenderTexture (SCE_STexture *tex, int id,
                                  SCE_STexture *new)
{
    return SCE_Texture_AddRenderCTexture (tex, id, new->tex);
}

/**
 * \brief Blit a texture over another texture 
 *
 * This function is equivalent to SCE_Texture_Blitf() except that the
 * rectangles specifies pixels instead of scale
 * \sa SCE_Texture_RenderTo(), SCE_Texture_Blitf()
 */
void SCE_Texture_Blit (SCE_SIntRect *rdst, SCE_STexture *dst,
                       SCE_SIntRect *rsrc, SCE_STexture *src)
{
    SCE_SFloatRect r1 = {{0.0f, 0.0f}, {1.0f, 1.0f}};
    SCE_SFloatRect r2 = {{0.0f, 0.0f}, {1.0f, 1.0f}};
    int w = 1;
    int h = 1;
    if (rdst) {
        if (dst) {
            w = SCE_Texture_GetWidth (dst, 0, 0);
            h = SCE_Texture_GetHeight (dst, 0, 0);
        }
        r1.p1[0] = (float)rdst->p1[0] / (float)w;
        r1.p1[1] = (float)rdst->p1[1] / (float)h;
        r1.p2[0] = (float)rdst->p2[0] / (float)w;
        r1.p2[1] = (float)rdst->p2[1] / (float)h;
    }
    if (rsrc) {
        w = SCE_Texture_GetWidth (src, 0, 0);
        h = SCE_Texture_GetHeight (src, 0, 0);
        r2.p1[0] = (float)rsrc->p1[0] / (float)w;
        r2.p1[1] = (float)rsrc->p1[1] / (float)h;
        r2.p2[0] = (float)rsrc->p2[0] / (float)w;
        r2.p2[1] = (float)rsrc->p2[1] / (float)h;
    }
    SCE_Texture_Blitf (&r1, dst, &r2, src);
}

/* fonction de rendu d'un quad pour la fonction Blitf() */
static void SCE_Texture_RenderQuad (SCE_SFloatRect *r)
{
    SCE_TMatrix4 mat;
    /* mise en place des matrices */
    if (r) {
        SCE_Quad_MakeMatrixFromRectanglef (mat, r);
        SCE_RLoadMatrix (SCE_MAT_TEXTURE, mat);
    }
    SCE_RLoadMatrix (SCE_MAT_CAMERA, sce_matrix4_id);
    SCE_RLoadMatrix (SCE_MAT_OBJECT, sce_matrix4_id);
    SCE_RLoadMatrix (SCE_MAT_PROJECTION, sce_matrix4_id);

    SCE_Quad_Draw (-1., -1., 2., 2.);
}
static void SCE_Texture_Set (SCE_STexture*);
/**
 * \brief Blit a texture over another texture
 * \param rdst the rectangle which defines the area where blit
 * \param dst the texture where blit
 * \param rsrc the rectangle which defines the area to blit
 * \param src the texture to blit (can be NULL)
 *
 * If \p dst doesn't have a frame buffer, a frame buffer is created and added to
 * \p dst, the frame buffer created has only one render target that is
 * SCE_Texture_GetCTexture (\p dst), when calling SCE_Texture_RenderTo() on the
 * frame buffer. The \p rdst and \p rsrc parameters specifies the texture's
 * coordinates between 0 and 1. \p dst can be NULL then the render is done on
 * the default OpenGL render buffer.
 * If \p rsrc is not NULL the texture matrix of \p src will be ignored.
 * \sa SCE_Texture_RenderTo(), SCE_Texture_Blit(), SCE_SFloatRect
 * \todo add blit to cube maps (use the 2nd parameter of RenderTo())
 * \todo this function maybe change some states
 */
void SCE_Texture_Blitf (SCE_SFloatRect *rdst, SCE_STexture *dst,
                        SCE_SFloatRect *rsrc, SCE_STexture *src)
{
    int w = 1, h = 1;

    if (dst && !dst->fb[0]) {
        dst->fb[0] = SCE_RCreateFramebuffer ();
        if (!dst->fb[0]) {
            SCEE_LogSrc ();
            return; /* \o/ */
        }
        SCE_RAddRenderTexture (dst->fb[0], SCE_COLOR_BUFFER, 0,
                               dst->tex, 0, SCE_FALSE);
    }
    if (dst) {
        w = SCE_Texture_GetWidth (dst, 0, 0);
        h = SCE_Texture_GetHeight (dst, 0, 0);
    }
    SCE_Texture_RenderTo (dst, 0);
    if (rdst)
        SCE_RViewport (rdst->p1[0] * w, rdst->p1[1] * h,
                       SCE_Rectangle_GetWidthf (rdst) * w,
                       SCE_Rectangle_GetHeightf (rdst) * h);

    SCE_RSetState2 (GL_DEPTH_TEST, GL_CULL_FACE, SCE_FALSE);
    SCE_RActivateDepthBuffer (SCE_FALSE);
    if (src)
        SCE_Texture_Set (src);

    SCE_Texture_RenderQuad (rsrc);

    if (src)
        SCE_RUseTexture (NULL, src->unit);
    SCE_RActivateDepthBuffer (SCE_TRUE);
    SCE_RSetState2 (GL_DEPTH_TEST, GL_CULL_FACE, SCE_TRUE);

    /* restores viewport too */
    SCE_Texture_RenderTo (NULL, 0);
}


static void SCE_Texture_Set (SCE_STexture *tex)
{
    SCE_RUseTexture (tex->tex, tex->unit);
    /* force matrix initialisation */
    SCE_RLoadMatrix (SCE_MAT_TEXTURE, tex->matrix);
}

/**
 * \brief Sets the used texture for the further renders
 * \param tex the texture to use
 * \sa SCE_RUseTexture(), SCE_Texture_Flush()
 */
void SCE_Texture_Use (SCE_STexture *tex)
{
    if (texlocked)
        return;
    if (!tex) {
        if (SCE_List_HasElements (&texused))
            SCE_List_Erase (&texused, SCE_List_GetFirst (&texused));
    } else {
        tex->toremove = SCE_FALSE;
        if (!tex->used) {
            textmp = unitused[tex->unit];
            if (textmp) {
                SCE_List_Removel (&textmp->it);
                textmp->used = SCE_FALSE;
            }
            SCE_Texture_Set (tex);
            /* add the texture to the used list */
            SCE_List_Prependl (&texused, &tex->it);
            tex->used = SCE_TRUE;
            unitused[tex->unit] = tex;
        }
    }
}

/**
 * \brief Any further call to SCE_Texture_Use(), SCE_Texture_BeginLot()
 * or SCE_Texture_EndLot() is ignored
 * \sa SCE_Texture_Unlock()
 */
void SCE_Texture_Lock (void)
{
    texlocked = SCE_TRUE;
}
/**
 * \brief Further calls to SCE_Texture_Use(), SCE_Texture_BeginLot()
 * or SCE_Texture_EndLot() are no longer ignored
 * \sa SCE_Texture_Lock()
 */
void SCE_Texture_Unlock (void)
{
    texlocked = SCE_FALSE;
}

/**
 * \brief Clears the list of the used textures
 */
void SCE_Texture_Flush (void)
{
    SCE_List_Clear (&texused);
}

void SCE_Texture_BeginLot (void)
{
    if (!texlocked) {
        SCE_SListIterator *it;
        SCE_List_ForEach (it, &texused)
            ((SCE_STexture*)SCE_List_GetData (it))->toremove = SCE_TRUE;
    }
}

void SCE_Texture_EndLot (void)
{
    if (!texlocked) {
        SCE_SListIterator *it, *pro;
        SCE_List_ForEachProtected (pro, it, &texused) {
            if (((SCE_STexture*)SCE_List_GetData (it))->toremove)
                SCE_List_Erase (&texused, it);
        }
    }
}


#if 0
/**
 * \internal
 * \brief Saves the current position in list of the used textures
 * \sa SCE_Texture_Restore()
 */
void SCE_Texture_Mark (void)
{
    mark = SCE_List_GetFirst (&texused);
}
/**
 * \internal
 * \brief Goes back to the previous mark set (if any)
 * \note The comportement of this function is undefined if is called two times
 * without SCE_Texture_Mark() or if the list of the used textures has been
 * modified by calling SCE_Texture_Use(NULL).
 * \sa SCE_Texture_Mark()
 */
void SCE_Texture_Restore (void)
{
    SCE_SListIterator *it = SCE_List_GetFirst (&texused), *toerase;
    while (it != mark) {
        toerase = it;
        it = SCE_List_GetNext (it);
        SCE_List_Erase (&texused, toerase);
    }
}
#endif

/**
 * \brief Uses \p tex instead of the default OpenGL's render buffer
 * \param tex the texture on make the further renders
 * \param cubeface used only for cubemaps, determines on which face of the
 * cubemap the render will be make
 *
 * This function uses the default frame buffer of \p tex. If \p tex isn't a
 * render buffer, calling of this function is equivalent of
 * SCE_RUseFramebuffer (NULL, NULL).
 * \sa SCE_RUseFramebuffer()
 */
void SCE_Texture_RenderTo (SCE_STexture *tex, SCE_EBoxFace cubeface)
{
    if (tex) {
        if (SCE_RGetTextureTarget (tex->tex) == SCE_TEX_CUBE)
            SCE_RUseFramebuffer (tex->fb[cubeface], NULL);
        else
            SCE_RUseFramebuffer (tex->fb[0], NULL);
    } else
        SCE_RUseFramebuffer (NULL, NULL);
}

/** @} */
