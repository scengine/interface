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

/* created: 31/07/2009
   updated: 09/02/2012 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/core/SCECore.h>
#include "SCE/interface/SCEMesh.h"

typedef void (*SCE_SMeshRenderFunc)(SCEenum);
typedef void (*SCE_SMeshRenderInstancedFunc)(SCEenum, SCEuint);

static int is_init = SCE_FALSE;

static int resource_type = 0;

static SCE_SMesh *mesh_bound = NULL;
static int activated_streams[SCE_MESH_NUM_STREAMS];
static SCE_SMeshRenderFunc render_func = NULL;
static SCE_SMeshRenderInstancedFunc render_func_instanced = NULL;

static SCE_SMesh *feedback_target = NULL;
static int feedback_enabled = SCE_FALSE;

static void* SCE_Mesh_LoadResource (const char*, int, void*);

int SCE_Init_Mesh (void)
{
    size_t i;
    if (is_init)
        return SCE_OK;
    resource_type = SCE_Resource_RegisterType (SCE_FALSE,
                                               SCE_Mesh_LoadResource, NULL);
    if (resource_type < 0)
        goto fail;
    for (i = 0; i < SCE_MESH_NUM_STREAMS; i++)
        activated_streams[i] = SCE_TRUE;
    is_init = SCE_TRUE;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    SCEE_LogSrcMsg ("failed to initialize mesh module");
    return SCE_ERROR;
}
void SCE_Quit_Mesh (void)
{
    is_init = SCE_FALSE;
}

void SCE_Mesh_InitArray (SCE_SMeshArray *marray)
{
    SCE_RInitVertexBufferData (&marray->data);
    SCE_Geometry_InitArrayUser (&marray->auser);
    SCE_List_InitIt (&marray->it);
    SCE_List_SetData (&marray->it, marray);
}
SCE_SMeshArray* SCE_Mesh_CreateArray (void)
{
    SCE_SMeshArray *marray = NULL;
    if (!(marray = SCE_malloc (sizeof *marray)))
        SCEE_LogSrc ();
    else
        SCE_Mesh_InitArray (marray);
    return marray;
}
static void SCE_Mesh_RemoveArray (SCE_SMeshArray*);
void SCE_Mesh_ClearArray (SCE_SMeshArray *marray)
{
    /* free memory of vertex arrays (see AddArrayArrays()) */
    SCE_RDeleteVertexBufferDataArrays (&marray->data);
    SCE_RClearVertexBufferData (&marray->data);
    SCE_Geometry_ClearArrayUser (&marray->auser);
    SCE_Mesh_RemoveArray (marray);
}
void SCE_Mesh_DeleteArray (SCE_SMeshArray *marray)
{
    if (marray) {
        SCE_Mesh_ClearArray (marray);
        SCE_free (marray);
    }
}


static void SCE_Mesh_FreeArray (void *marray)
{
    SCE_Mesh_RemoveArray (marray);
}
void SCE_Mesh_Init (SCE_SMesh *mesh)
{
    size_t i;
    mesh->geom = NULL;
    mesh->canfree_geom = SCE_FALSE;
    mesh->prim = SCE_POINTS;    /* do not take any risk. */
    SCE_List_Init (&mesh->arrays);
    SCE_List_SetFreeFunc (&mesh->arrays, SCE_Mesh_FreeArray);
    for (i = 0; i < SCE_MESH_NUM_STREAMS; i++) {
        SCE_RInitVertexBuffer (&mesh->streams[i]);
        mesh->used_streams[i] = SCE_FALSE;
    }
    SCE_RInitIndexBuffer (&mesh->ib);
    mesh->use_ib = SCE_FALSE;
    mesh->n_vertices = 0;
    mesh->n_indices = 0;
    SCE_RInitFeedback (&mesh->feedback);
    mesh->counting_buffer = NULL;
    SCE_Geometry_InitArrayUser (&mesh->index_auser);
    mesh->rmode = SCE_VA_RENDER_MODE;
    mesh->bmode = SCE_INDEPENDANT_VERTEX_BUFFER;
    mesh->built = SCE_FALSE;
}
SCE_SMesh* SCE_Mesh_Create (void)
{
    SCE_SMesh *mesh = NULL;
    if (!(mesh = SCE_malloc (sizeof *mesh)))
        SCEE_LogSrc ();
    else
        SCE_Mesh_Init (mesh);
    return mesh;
}
SCE_SMesh* SCE_Mesh_CreateFrom (SCE_SGeometry *geom, int canfree)
{
    SCE_SMesh *mesh = NULL;
    if (!(mesh = SCE_Mesh_Create ()))
        SCEE_LogSrc ();
    else if (SCE_Mesh_SetGeometry (mesh, geom, canfree) < 0) {
        SCE_Mesh_Delete (mesh), mesh = NULL;
        SCEE_LogSrc ();
    }
    return mesh;
}
void SCE_Mesh_Clear (SCE_SMesh *mesh)
{
    size_t i;
    SCE_RClearIndexBuffer (&mesh->ib);
    SCE_List_Clear (&mesh->arrays);
    for (i = 0; i < SCE_MESH_NUM_STREAMS; i++)
        SCE_RClearVertexBuffer (&mesh->streams[i]);
}
void SCE_Mesh_Delete (SCE_SMesh *mesh)
{
    if (mesh) {
        SCE_Mesh_Clear (mesh);
        SCE_free (mesh);
    }
}


/**
 * \brief Sets the state of a stream (activated or deactivated)
 * \sa SCE_Mesh_EnableStream(), SCE_Mesh_DisableStream()
 */
void SCE_Mesh_ActivateStream (SCE_EMeshStream s, int a)
{
    activated_streams[s] = a;
}
/**
 * \brief Enables a stream
 * \sa SCE_Mesh_ActivateStream(), SCE_Mesh_DisableStream()
 */
void SCE_Mesh_EnableStream (SCE_EMeshStream s)
{
    activated_streams[s] = SCE_TRUE;
}
/**
 * \brief Disables a stream
 * \sa SCE_Mesh_ActivateStream(), SCE_Mesh_EnableStream()
 */
void SCE_Mesh_DisableStream (SCE_EMeshStream s)
{
    activated_streams[s] = SCE_FALSE;
}

void SCE_Mesh_ActivateIndices (SCE_SMesh *mesh, int activate)
{
    mesh->use_ib = activate;
}
/**
 * \brief Sets the primitive type of a mesh
 * \param mesh a mesh
 * \param t primitive type
 * \sa SCE_Geometry_GetPrimitiveType(), SCE_Geometry_SetPrimitiveType()
 */
void SCE_Mesh_SetPrimitiveType (SCE_SMesh *mesh, SCE_EPrimitiveType t)
{
    mesh->prim = t;
}

/**
 * \brief Gets the geometry of a mesh
 * \sa SCE_Mesh_SetGeometry()
 */
SCE_SGeometry* SCE_Mesh_GetGeometry (SCE_SMesh *mesh)
{
    return mesh->geom;
}


static void SCE_Mesh_UpdateArrayCallback (void *data, size_t *range)
{
    SCE_RModifiedVertexBufferData (data, range);
}
/**
 * \internal
 * \brief Adds a root geometry array to a mesh array (ie vertex buffer data)
 * 
 * The root geometry array is the first geometry array of a set of interleaved
 * geometry arrays.
 * \sa SCE_Mesh_AddArray(), SCE_Mesh_AddArrayFrom()
 */
static int SCE_Mesh_AddArrayArrays (SCE_SMeshArray *marray,
                                    SCE_SGeometryArray *array,
                                    size_t n_vertices)
{
    /* set user at root */
    SCE_Geometry_AddUser (array, &marray->auser, SCE_Mesh_UpdateArrayCallback,
                          &marray->data);
    /* add children */
    while (array) {
        /* create vertex arrays using vertex array data of the geometry */
        SCE_RVertexArray *va = NULL;
        if (!(va = SCE_RCreateVertexArray ()))
            goto fail;
        SCE_RSetVertexArrayData (va, SCE_Geometry_GetArrayData (array));
        if (SCE_RAddVertexBufferDataArray (&marray->data, va, n_vertices) < 0)
            goto fail;
        array = SCE_Geometry_GetChild (array);
    }
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


/**
 * \internal
 */
static void SCE_Mesh_AddArray (SCE_SMesh *mesh, SCE_SMeshArray *marray)
{
/*     if (marray->mesh) */
    SCE_List_Appendl (&mesh->arrays, &marray->it);
}
/**
 * \internal
 */
static SCE_SMeshArray* SCE_Mesh_AddNewArray (SCE_SMesh *mesh)
{
    SCE_SMeshArray *marray = NULL;
    if (!(marray = SCE_Mesh_CreateArray ()))
        SCEE_LogSrc ();
    else
        SCE_Mesh_AddArray (mesh, marray);
    return marray;
}
/**
 * \internal
 * \warning \p array must be a root array, all its children will be added
 */
static SCE_SMeshArray* SCE_Mesh_AddNewArrayFrom (SCE_SMesh *mesh,
                                                 SCE_SGeometryArray *array,
                                                 size_t n_vertices)
{
    SCE_SMeshArray *marray = NULL;
    if (!(marray = SCE_Mesh_AddNewArray (mesh)))
        SCEE_LogSrc ();
    else {
        if (SCE_Mesh_AddArrayArrays (marray, array, n_vertices) < 0) {
            SCE_Mesh_DeleteArray (marray);
            SCEE_LogSrc ();
            return NULL;
        }
    }
    return marray;
}
/**
 * \internal
 */
static void SCE_Mesh_RemoveArray (SCE_SMeshArray *marray)
{
    SCE_List_Remove (&marray->it);
}

static void SCE_Mesh_UpdateIndexArrayCallback (void *ib, size_t *range)
{
    SCE_RModifiedIndexBuffer (ib, range);
}
/**
 * \brief Sets the geometry of a mesh
 * \sa SCE_Mesh_CreateFrom(), SCE_Mesh_Build(), SCE_SGeometry
 */
int SCE_Mesh_SetGeometry (SCE_SMesh *mesh, SCE_SGeometry *geom, int canfree)
{
    size_t n_vertices;
    SCE_SGeometryArray *index_array = NULL;
    SCE_SListIterator *it = NULL;
    SCE_SList *arrays = NULL;

    /* TODO: adding the 'user' structures before build the vertex buffers
       may update some invalid buffers if the geometry is updated and
       SCE_RUpdateModifiedBuffers() called before SCE_Mesh_Build() */

    n_vertices = SCE_Geometry_GetNumVertices (geom);
    arrays = SCE_Geometry_GetArrays (geom);
    SCE_List_ForEach (it, arrays) {
        SCE_SGeometryArray *array = SCE_List_GetData (it);
        if (!SCE_Geometry_GetRoot (array)) {
            if (!SCE_Mesh_AddNewArrayFrom (mesh, array, n_vertices))
                goto fail;
        }
    }
    arrays = SCE_Geometry_GetModifiedArrays (geom);
    SCE_List_ForEach (it, arrays) {
        SCE_SGeometryArray *array = SCE_List_GetData (it);
        if (!SCE_Geometry_GetRoot (array)) {
            if (!SCE_Mesh_AddNewArrayFrom (mesh, array, n_vertices))
                goto fail;
        }
    }
    index_array = SCE_Geometry_GetIndexArray (geom);
    if (index_array) {
        /* NOTE: not very beautiful... */
        SCE_RIndexArray ia;
        SCE_SGeometryArrayData *vdata = SCE_Geometry_GetArrayData (index_array);
        ia.type = vdata->type;
        ia.data = vdata->data;
        mesh->n_indices = SCE_Geometry_GetNumIndices (geom);
        SCE_RSetIndexBufferIndexArray (&mesh->ib, &ia);
        SCE_RSetIndexBufferNumIndices (&mesh->ib, mesh->n_indices);
        mesh->use_ib = SCE_TRUE;
        SCE_Geometry_AddUser (index_array, &mesh->index_auser,
                              SCE_Mesh_UpdateIndexArrayCallback, &mesh->ib);

    }
    mesh->prim = SCE_Geometry_GetPrimitiveType (geom);
    mesh->geom = geom;
    mesh->canfree_geom = canfree;
    mesh->n_vertices = n_vertices;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}


static void SCE_Mesh_AddStreamData (SCE_RVertexBuffer *vb,
                                    SCE_SMeshArray *marray)
{
    SCE_RAddVertexBufferData (vb, &marray->data);
}
static void SCE_Mesh_MakeIndependantVB (SCE_SMesh *mesh)
{
    SCE_SListIterator *it = NULL;
    SCE_List_ForEach (it, &mesh->arrays) {
        SCE_EMeshStream stream = SCE_MESH_NUM_STREAMS;
        SCE_SGeometryArray *array;
        SCE_SMeshArray *marray = SCE_List_GetData (it);
        /* determine stream for these arrays */
        array = SCE_Geometry_GetUserArray (&marray->auser);
        while (array) {
            SCE_EVertexAttribute attrib;
            attrib = SCE_Geometry_GetArrayVertexAttribute (array);
            /* TODO: just shit. */
            if (attrib == SCE_POSITION) {
                stream = SCE_MESH_STREAM_G;
                break;
            } else if (attrib == SCE_NORMAL || attrib == SCE_TANGENT ||
                       attrib == SCE_BINORMAL) {
                stream = SCE_MESH_STREAM_N;
            } else if (attrib >= SCE_TEXCOORD0 && attrib < SCE_ATTRIB0 &&
                       stream > SCE_MESH_STREAM_T) {
                stream = SCE_MESH_STREAM_T;
            } else              /* A is considered as a trash, SCE_ATTRIBn
                                   and SCE_COLOR will be stored here */
                stream = SCE_MESH_STREAM_A;
            array = SCE_Geometry_GetChild (array);
        }
#ifdef SCE_DEBUG
        if (stream == SCE_MESH_NUM_STREAMS) {
            SCEE_SendMsg ("obviously, there is no any array set to this mesh "
                          "array because its 'array user' structure is empty of"
                          " any geometry array, what the fuck?");
            continue;
        }
#endif
        SCE_Mesh_AddStreamData (&mesh->streams[stream], marray);
        mesh->used_streams[stream] = SCE_TRUE;
    }
}
static void SCE_Mesh_MakeGlobalVB (SCE_SMesh *mesh)
{
    SCE_SListIterator *it = NULL;
    SCE_List_ForEach (it, &mesh->arrays) {
        SCE_Mesh_AddStreamData (&mesh->streams[SCE_MESH_STREAM_G],
                                SCE_List_GetData (it));
    }
    mesh->used_streams[SCE_MESH_STREAM_G] = SCE_TRUE;
}

/**
 * \internal
 */
static void SCE_Mesh_BuildBuffers (SCE_SMesh *mesh, SCE_RBufferUsage
                                   usage[SCE_MESH_NUM_STREAMS + 1])
{
    size_t i;
    SCE_RBufferRenderMode rmode;
    SCE_RBufferUsage default_usage[SCE_MESH_NUM_STREAMS + 1] = {
        SCE_BUFFER_STATIC_DRAW,
        SCE_BUFFER_STATIC_DRAW,
        SCE_BUFFER_STATIC_DRAW,
        SCE_BUFFER_DYNAMIC_DRAW,
        SCE_BUFFER_STATIC_DRAW  /* index buffer */
    };

    if (usage) {
        /* replace defaults */
        for (i = 0; i < SCE_MESH_NUM_STREAMS + 1; i++) {
            if (usage != SCE_BUFFER_DEFAULT_USAGE)
                default_usage[i] = usage[i];
        }
    }
    usage = default_usage;

    /* try to setup the better render mode */
    /* TODO: for unknown reasons, the VAO mode produces strange results */
    if (SCE_RHasCap (SCE_VAO) && mesh->bmode && 0)
        rmode = SCE_VAO_RENDER_MODE;
    else if (SCE_RHasCap (SCE_VBO))
        rmode = SCE_VBO_RENDER_MODE;
    else
        rmode = SCE_VA_RENDER_MODE;
    mesh->rmode = rmode;

    if (mesh->use_ib)
        SCE_RBuildIndexBuffer (&mesh->ib, usage[SCE_MESH_NUM_STREAMS]);
    for (i = 0; i < SCE_MESH_NUM_STREAMS; i++) {
        if (mesh->used_streams[i]) {
            SCE_RSetVertexBufferNumVertices (
                &mesh->streams[i], SCE_Geometry_GetNumVertices (mesh->geom));
            SCE_RBuildVertexBuffer (&mesh->streams[i], usage[i], rmode);
        }
    }
}
/**
 * \internal
 * \todo Maximum number of streams may conflict.
 */
static void SCE_Mesh_BuildFeedback (SCE_SMesh *mesh)
{
    size_t i;
    size_t n_streams;
    int once = SCE_FALSE;

    /* TODO: : d */
    n_streams = MIN (SCE_MESH_NUM_STREAMS, SCE_MAX_FEEDBACK_STREAMS);

    /* TODO: if global buffer storage mode has been used for mesh construction,
       add the same buffer many times with different ranges */
    for (i = 0; i < n_streams; i++) {
        if (mesh->used_streams[i]) {
            SCE_RBuffer *buf = SCE_RGetVertexBufferBuffer (&mesh->streams[i]);
            SCE_RAddFeedbackStream (&mesh->feedback, buf, NULL);
            if (!once) {
                SCE_REnableFeedbackCounting (&mesh->feedback, buf);
                mesh->counting_buffer = buf;
            }
        }
    }
    /* index buffer usually won't be generated in the same time vertices are */
}
/**
 * \brief Builds a mesh by creating vertex buffers with requested usages
 * \sa SCE_Mesh_AutoBuild()
 */
void SCE_Mesh_Build (SCE_SMesh *mesh, SCE_EMeshBuildMode bmode,
                     SCE_RBufferUsage usage[SCE_MESH_NUM_STREAMS + 1])
{
    if (mesh->built)
        return;
    mesh->built = SCE_TRUE;     /* do not do a return after that, it could
                                   consider an unbuilt mesh as built */
    /* SCE_Mesh_Unbuild (mesh); */

    switch (bmode) {
    case SCE_INDEPENDANT_VERTEX_BUFFER:
        SCE_Mesh_MakeIndependantVB (mesh);
        break;
    case SCE_GLOBAL_VERTEX_BUFFER:
        SCE_Mesh_MakeGlobalVB (mesh);
        break;
    }
    mesh->bmode = bmode;
    SCE_Mesh_BuildBuffers (mesh, usage);
    SCE_Mesh_BuildFeedback (mesh);
}

/**
 * \brief Uses default value for mesh construction
 *
 * It is recommanded to use this function to build meshes so the SCEngine can
 * do some optimization under data structure to prevent for rendering
 * situations that needs specific mesh data structure.
 * \sa SCE_Mesh_Build()
 */
void SCE_Mesh_AutoBuild (SCE_SMesh *mesh)
{
    SCE_Mesh_Build (mesh, SCE_INDEPENDANT_VERTEX_BUFFER, NULL);
}

/**
 * \brief Sets the render mode of a mesh
 * \sa SCE_RSetVertexBufferRenderMode()
 */
void SCE_Mesh_SetRenderMode (SCE_SMesh *mesh, SCE_RBufferRenderMode rmode)
{
    size_t i;
    for (i = 0; i < SCE_MESH_NUM_STREAMS; i++)
        SCE_RSetVertexBufferRenderMode (&mesh->streams[i], rmode);
}

/* why se breaker les balls, les fonctions inutiles c'est chiant
 * TODO: ah non spas inutile, apres generation TBN, besoin de mettre a jour */
#if 0
/**
 * \brief Takes new (if any) arrays from the geometry of a mesh, and rebuild it
 * \sa SCE_Mesh_SetGeometry(), SCE_Mesh_Build()
 */
int SCE_Mesh_Update (SCE_SMesh *mesh)
{
    /* not yet implemented, trop difficulte. (et trop chiant) */
}
#endif

/**
 * \internal
 */
static void* SCE_Mesh_LoadResource (const char *fname, int force, void *data)
{
    SCE_SMesh *mesh = NULL;
    SCE_SGeometry *geom = NULL;
    (void)data;
    if (!(mesh = SCE_Mesh_Create ()))
        goto fail;
    if (force > 0)
        force--;
    if (!(geom = SCE_Geometry_Load (fname, force)))
        goto fail;
    if (SCE_Mesh_SetGeometry (mesh, geom, SCE_TRUE) < 0)
        goto fail;
    return mesh;
fail:
    SCE_Mesh_Delete (mesh);
    SCEE_LogSrc ();
    return NULL;
}
/**
 * \brief Loads a mesh from a file containing geometry
 * \param fname name of the geometry file
 * \param force see SCE_Resource_Load()
 * \returns a new or existing mesh, depends on resource availability
 * and on \p force
 *
 * This function loads a mesh resource by loading a geometry resource using
 * SCE_Geometry_Load() and set it to the mesh. This function doesn't build
 * the returned mesh itself, so you have to do it before using the mesh.
 * \sa SCE_Mesh_SetGeometry(), SCE_Geometry_Load(), SCE_Resource_Load()
 */
SCE_SMesh* SCE_Mesh_Load (const char *fname, int force)
{
    return SCE_Resource_Load (resource_type, fname, force, NULL);
}


/**
 * \brief Declares a mesh as activated for rendering
 * \sa SCE_Mesh_Render(), SCE_Mesh_RenderInstanced(), SCE_Mesh_Unuse()
 * \todo Geometry cannot updates its number of vertices nor indices.
 */
void SCE_Mesh_Use (SCE_SMesh *mesh)
{
    size_t i;
    for (i = 0; i < SCE_MESH_NUM_STREAMS; i++) {
        if (activated_streams[i] && mesh->used_streams[i]) {
            /* TODO: if the geometry updates its number of vertices,
               we're screwed */
            SCE_RSetVertexBufferNumVertices(&mesh->streams[i],mesh->n_vertices);
            SCE_RUseVertexBuffer (&mesh->streams[i]);
        }
    }
    if (mesh->use_ib) {
        /* TODO: if the geometry updates its number of indices, we're screwed */
        SCE_RSetIndexBufferNumIndices (&mesh->ib, mesh->n_indices);
        SCE_RUseIndexBuffer (&mesh->ib);
        render_func = SCE_RRenderVertexBufferIndexed;
        render_func_instanced = SCE_RRenderVertexBufferIndexedInstanced;
    } else {
        render_func = SCE_RRenderVertexBuffer;
        render_func_instanced = SCE_RRenderVertexBufferInstanced;
    }
    mesh_bound = mesh;
}
/**
 * \brief Render an instance of the mesh bound with SCE_Mesh_Use()
 * \sa SCE_Mesh_Use(), SCE_Mesh_RenderInstanced(), SCE_Mesh_Unuse()
 */
void SCE_Mesh_Render (void)
{
    if (feedback_target && !feedback_enabled) {
        SCE_RBeginFeedback (&feedback_target->feedback, feedback_target->prim);
        feedback_enabled = SCE_TRUE;
    }
    render_func (mesh_bound->prim);
}
/**
 * \brief Render \p n_instances instances of the mesh bound with
 * SCE_Mesh_Use() using hardware geometry instancing
 * \sa SCE_Mesh_Use(), SCE_Mesh_Render(), SCE_Mesh_Unuse()
 */
void SCE_Mesh_RenderInstanced (SCEuint n_instances)
{
    render_func_instanced (mesh_bound->prim, n_instances);
}
/**
 * \brief Kicks the bound mesh
 * \note Useless in a pure GL 3 context
 * \sa SCE_Mesh_Use(), SCE_Mesh_Render(), SCE_Mesh_RenderInstanced(),
 * SCE_RFinishVertexBufferRender()
 */
void SCE_Mesh_Unuse (void)
{
    if (SCE_TRUE/*non_full_gl3*/)
        SCE_RFinishVertexBufferRender ();
    mesh_bound = NULL;
}


void SCE_Mesh_BeginRenderTo (SCE_SMesh *mesh)
{
    feedback_target = mesh;
}
void SCE_Mesh_EndRenderTo (SCE_SMesh *mesh)
{
    if (feedback_enabled) {
        SCEuint n_prim;
        SCE_REndFeedback (&mesh->feedback);
        n_prim = SCE_RGetFeedbackNumPrimitives (&mesh->feedback,
                                                mesh->counting_buffer);
        mesh->n_vertices =
            n_prim * SCE_Geometry_GetPrimitiveVertices (feedback_target->prim);
        feedback_enabled = SCE_FALSE;
        feedback_target = NULL;
    }
}
