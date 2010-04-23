/*------------------------------------------------------------------------------
    SCEngine - A 3D real time rendering engine written in the C language
    Copyright (C) 2006-2010  Antony Martin <martin(dot)antony(at)yahoo(dot)fr>

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

/* created: 17/02/2009
   updated: 18/02/2009 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include "SCE/interface/SCETexture.h"
#include "SCE/interface/SCESkybox.h"

/**
 * \defgroup skybox Skybox
 * \ingroup interface
 * \internal
 * \brief 
 * @{
 */

static void SCE_Skybox_Init (SCE_SSkybox *skybox)
{
    skybox->mesh = NULL;
    skybox->tex = NULL;
    skybox->mode = SCE_FALSE;
    skybox->textype = 0;        /* NOTE: what if SCE_TEX_* is 0 ? */
    skybox->shader = NULL;
    skybox->entity = NULL;
    skybox->instance = NULL;
}

/**
 * \brief Creates a skybox
 */
SCE_SSkybox* SCE_Skybox_Create (void)
{
    SCE_SSceneEntityProperties *props = NULL;
    SCE_SSkybox *skybox = NULL;
    SCE_btstart ();
    if (!(skybox = SCE_malloc (sizeof *skybox)))
        goto fail;
    SCE_Skybox_Init (skybox);
    if (!(skybox->mesh = SCE_Mesh_Create ()))
        goto fail;
    if (!(skybox->entity = SCE_SceneEntity_Create ()))
        goto fail;
    if (!(skybox->instance = SCE_SceneEntity_CreateInstance ()))
        goto fail;
    SCE_SceneEntity_AddInstanceToEntity (skybox->entity, skybox->instance);
    props = SCE_SceneEntity_GetProperties (skybox->entity);
    props->cullface = SCE_FALSE;
    props->depthtest = SCE_FALSE;
    props->alphatest = SCE_FALSE;
    goto success;
fail:
    SCE_Skybox_Delete (skybox), skybox = NULL;
    SCEE_LogSrc ();
success:
    SCE_btend ();
    return skybox;
}
/**
 * \brief Deletes a skybox
 */
void SCE_Skybox_Delete (SCE_SSkybox *skybox)
{
    if (skybox) {
        SCE_SceneEntity_DeleteInstance (skybox->instance);
        SCE_SceneEntity_Delete (skybox->entity);
        SCE_Mesh_Delete (skybox->mesh);
        SCE_free (skybox);
    }
}


/**
 * \brief Defines the size of a bounding box
 * \param skybox a skybox
 * \param size the new size of the bounding box
 */
void SCE_Skybox_SetSize (SCE_SSkybox *skybox, float size)
{
    SCE_SNode *node = SCE_SceneEntity_GetInstanceNode (skybox->instance);
    SCE_Matrix4_Scale (SCE_Node_GetMatrix (node, SCE_NODE_WRITE_MATRIX),
                       size, size, size);
    SCE_Node_HasMoved (node);   /* o ser gamer ! !1 */
}
/**
 * \brief Defines the texture of a skybox
 * \param skybox a skybox
 * \param tex the texture to set
 * \param mode see SCE_BoxGeom_Generate()
 * \returns SCE_ERROR on error, SCE_OK otherwise
 * \sa SCE_Skybox_SetShader(), SCE_SceneEntity_AddTexture()
 */
int SCE_Skybox_SetTexture (SCE_SSkybox *skybox, SCE_STexture *tex,
                           SCE_EBoxGeomTexCoordMode mode)
{
    SCE_TVector3 center = {0.0f, 0.0f, 0.0f};
    SCE_SBox box;
    int type;
    SCE_SGeometry *geom = NULL;

    /* remove the previous one */
    if (skybox->tex)
        SCE_SceneEntity_RemoveTexture (skybox->entity, skybox->tex);
    SCE_SceneEntity_AddTexture (skybox->entity, tex);
    skybox->tex = tex;

    SCE_Box_Init (&box);
    SCE_Box_Set (&box, center, 1.0f, 1.0f, 1.0f);

    type = SCE_Texture_GetType (tex);
    if (type == SCE_TEX_CUBE || type == SCE_RENDER_COLOR_CUBE ||
        type == SCE_RENDER_DEPTH_CUBE) {
        geom = SCE_BoxGeom_Create (&box, SCE_TRIANGLES,
                                   SCE_BOX_CUBEMAP_TEXCOORD);
    } else {
        if (!mode)              /* he lies. */
            mode = SCE_BOX_INTERIOR_TEXCOORD;
        geom = SCE_BoxGeom_Create (&box, SCE_TRIANGLES, mode);
    }
    if (!geom)
        goto fail;
    SCE_Mesh_SetGeometry (skybox->mesh, geom, SCE_TRUE);
    geom = NULL;
    SCE_Mesh_Build (skybox->mesh, SCE_GLOBAL_VERTEX_BUFFER, NULL);
    SCE_SceneEntity_SetMesh (skybox->entity, skybox->mesh);
    skybox->mode = mode;
    skybox->textype = type;

    return SCE_OK;
fail:
    SCE_Geometry_Delete (geom);
    SCEE_LogSrc ();
    return SCE_ERROR;
}
/**
 * \brief Defines the shader of a skybox
 * \param skybox a skybox
 * \param shader the shader to set
 */
void SCE_Skybox_SetShader (SCE_SSkybox *skybox, SCE_SShader *shader)
{
    SCE_SceneEntity_SetShader (skybox->entity, shader);
    skybox->shader = shader;
}

/**
 * \brief Gets the scene entity of a skybox
 * \sa SCE_Skybox_GetEntityGroup(), SCE_SSceneEntity
 */
SCE_SSceneEntity* SCE_Skybox_GetEntity (SCE_SSkybox *skybox)
{
    return skybox->entity;
}
/**
 * \brief Gets the entity instance of a skybox
 * \sa SCE_SSkybox, SCE_SSceneEntityInstance
 */
SCE_SSceneEntityInstance* SCE_Skybox_GetInstance (SCE_SSkybox *skybox)
{
    return skybox->instance;
}

/** @} */
