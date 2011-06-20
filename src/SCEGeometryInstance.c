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
 
/* created: 25/10/2008
   updated: 20/06/2011 */

#include <SCE/utils/SCEUtils.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCEGeometryInstance.h"

static void SCE_Instance_RenderSimple (SCE_SGeometryInstanceGroup*);
static void SCE_Instance_RenderPseudo (SCE_SGeometryInstanceGroup*);
static void SCE_Instance_RenderHardware (SCE_SGeometryInstanceGroup*);

static SCE_FInstanceGroupRenderFunc renderfuncs[3] = {
    SCE_Instance_RenderSimple,
    SCE_Instance_RenderPseudo,
    SCE_Instance_RenderHardware
};

void SCE_Instance_Init (SCE_SGeometryInstance *inst)
{
    inst->node = NULL;
    SCE_List_InitIt (&inst->it);
    SCE_List_SetData (&inst->it, inst);
    inst->data = NULL;
    inst->group = NULL;
}
SCE_SGeometryInstance* SCE_Instance_Create (void)
{
    SCE_SGeometryInstance *inst = NULL;
    if (!(inst = SCE_malloc (sizeof *inst)))
        SCEE_LogSrc ();
    else
        SCE_Instance_Init (inst);
    return inst;
}
void SCE_Instance_Delete (SCE_SGeometryInstance *inst)
{
    if (inst) {
        if (inst->group)
            inst->group->n_instances--;
        SCE_List_Remove (&inst->it);
        SCE_free (inst);
    }
}

static void SCE_Instance_FreeInstance (void *inst)
{
    SCE_SGeometryInstance *i = inst;
    /* if a group is shared, like if you hard instanciate a SCE_SModel,
       it avoids the deletion of the geometry instances of the model
       instances to segfault */
    i->group = NULL;
}
void SCE_Instance_InitGroup (SCE_SGeometryInstanceGroup *group)
{
    group->mesh = NULL;
    SCE_List_Init (&group->instances);
    SCE_List_SetFreeFunc (&group->instances, SCE_Instance_FreeInstance);
    group->n_instances = 0;
    group->renderfunc = renderfuncs[SCE_SIMPLE_INSTANCING];
    group->attrib1 = 3; /* lulz */
    group->attrib1 = 4;
    group->attrib1 = 5;
    group->hwfunc = NULL;
    group->batch_count = 64;
}
SCE_SGeometryInstanceGroup* SCE_Instance_CreateGroup (void)
{
    SCE_SGeometryInstanceGroup *group = NULL;
    if (!(group = SCE_malloc (sizeof *group)))
        SCEE_LogSrc ();
    else
        SCE_Instance_InitGroup (group);
    return group;
}
void SCE_Instance_DeleteGroup (SCE_SGeometryInstanceGroup *group)
{
    if (group) {
        SCE_List_Clear (&group->instances);
        SCE_free (group);
    }
}

/**
 * \brief Defines the instancing method for a group
 * \note If SCE_HARDWARE_INSTANCING is specified, you must specify
 * a function callback with SCE_Instance_SetHwCallback().
 * \sa SCE_EInstancingType
 */
void SCE_Instance_SetInstancingType (SCE_SGeometryInstanceGroup *group,
                                     SCE_EInstancingType type)
{
    if (type != SCE_HARDWARE_INSTANCING || SCE_RHasCap (SCE_HW_INSTANCING))
        group->renderfunc = renderfuncs[type];
    else {
        group->renderfunc = renderfuncs[SCE_SIMPLE_INSTANCING];
#ifdef SCE_DEBUG
        SCEE_SendMsg ("hardware instancing is not supported, "
                      "using simple instancing.\n");
#endif
    }
}
/**
 * \brief Defines the vertex attributes for giving the modelview matrix
 * \param a1 attribute index of the vector for the matrix row 1
 * \param a2 attribute index of the vector for the matrix row 2
 * \param a3 attribute index of the vector for the matrix row 3
 */
void SCE_Instance_SetAttribIndices (SCE_SGeometryInstanceGroup *group,
                                    int a1, int a2, int a3)
{
    group->attrib1 = a1; group->attrib2 = a2; group->attrib3 = a3;
}
/**
 * \brief Sets the number of instances to render per draw call
 * \sa SCE_RRenderVertexBufferInstanced(), SCE_RRenderInstanced(),
 * SCE_HARDWARE_INSTANCING
 */
void SCE_Instance_SetBatchCount (SCE_SGeometryInstanceGroup *group,
                                 unsigned int count)
{
    group->batch_count = count;
}
/**
 * \brief Sets the function to call before each SCE_Mesh_RenderInstanced()
 * in the rendering routine SCE_Instance_RenderGroup() in case of
 * hardware instancing
 * \sa SCE_Instance_SetBatchCount()
 */
void SCE_Instance_SetHwCallback (SCE_SGeometryInstanceGroup *group,
                                 SCE_FInstanceGroupHwFunc func)
{
    group->hwfunc = func;
}

/**
 * \brief Adds an instance into a group of instances
 * \param group the group where add the instance
 * \param inst the instance to add
 */
void SCE_Instance_AddInstance (SCE_SGeometryInstanceGroup *group,
                               SCE_SGeometryInstance *inst)
{
    SCE_List_Appendl (&group->instances, &inst->it);
    inst->group = group;
    group->n_instances++;
}
/**
 * \brief Removes an instance from its group
 * \param inst the instance to remove
 */
void SCE_Instance_RemoveInstance (SCE_SGeometryInstance *inst)
{
    SCE_List_Removel (&inst->it);
    inst->group->n_instances--;
    inst->group = NULL;
}

/**
 * \brief Flushes the instances list of a group
 */
void SCE_Instance_FlushInstancesList (SCE_SGeometryInstanceGroup *group)
{
    SCE_List_Flush (&group->instances);
    group->n_instances = 0;
}
/**
 * \brief Indicates if an instances group have any instance
 */
int SCE_Instance_HasInstances (SCE_SGeometryInstanceGroup *group)
{
    return (group->n_instances > 0);
}

/**
 * \brief Defines the mesh of an instances group
 */
void SCE_Instance_SetGroupMesh (SCE_SGeometryInstanceGroup *group,
                                SCE_SMesh *mesh)
{
    group->mesh = mesh;
}
/**
 * \brief Gets the mesh of an instances group
 */
SCE_SMesh* SCE_Instance_GetGroupMesh (SCE_SGeometryInstanceGroup *group)
{
    return group->mesh;
}


void SCE_Instance_SetNode (SCE_SGeometryInstance *inst, SCE_SNode *node)
{
    inst->node = node;
}
float* SCE_Instance_GetMatrix (SCE_SGeometryInstance *inst)
{
    return SCE_Node_GetFinalMatrix (inst->node);
}


void SCE_Instance_SetData (SCE_SGeometryInstance *inst, void *data)
{
    inst->data = data;
}
void* SCE_Instance_GetData (SCE_SGeometryInstance *inst)
{
    return inst->data;
}


static void SCE_Instance_RenderSimple (SCE_SGeometryInstanceGroup *group)
{
    SCE_SListIterator *it = NULL;
    SCE_SGeometryInstance *inst = NULL;

    SCE_Mesh_Use (group->mesh);
    SCE_List_ForEach (it, &group->instances) {
        inst = SCE_List_GetData (it);
        SCE_RLoadMatrix (SCE_MAT_OBJECT, SCE_Node_GetFinalMatrix (inst->node));
        SCE_Mesh_Render ();
    }
}

static void SCE_Instance_RenderPseudo (SCE_SGeometryInstanceGroup *group)
{
    SCE_TMatrix4 camera, final;
    SCE_SListIterator *it = NULL;
    SCE_SGeometryInstance *inst = NULL;

    SCE_RGetMatrix (SCE_MAT_CAMERA, camera);

    SCE_Mesh_Use (group->mesh);
    SCE_List_ForEach (it, &group->instances) {
        inst = SCE_List_GetData (it);
        /* combine camera and object matrices */
        SCE_Matrix4_Mul (camera, SCE_Node_GetFinalMatrix (inst->node), final);
        /* set persistent vertex attributes */ /* TODO: pouha */
        glVertexAttrib4fv (group->attrib1, &final[0]);
        glVertexAttrib4fv (group->attrib2, &final[4]);
        glVertexAttrib4fv (group->attrib3, &final[8]);
        SCE_Mesh_Render ();
    }
}

static void SCE_Instance_RenderHardware (SCE_SGeometryInstanceGroup *group)
{
    SCE_SListIterator *it = NULL;
    unsigned int rem, num, i;

    num = group->n_instances / group->batch_count;
    rem = group->n_instances % group->batch_count;
    it = SCE_List_GetFirst (&group->instances);
    SCE_Mesh_Use (group->mesh);
    for (i = 0; i < num; i++) {
        group->hwfunc (&it, group->batch_count);
        SCE_Mesh_RenderInstanced (group->batch_count);
    }
    if (rem > 0) {
        group->hwfunc (&it, rem);
        SCE_Mesh_RenderInstanced (rem);
    }
}

/**
 * \brief Render the given instances group
 * \sa SCE_Instance_SetInstancingType()
 */
void SCE_Instance_RenderGroup (SCE_SGeometryInstanceGroup *group)
{
    group->renderfunc (group);
}
