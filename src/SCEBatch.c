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
 
/* created: 29/11/2008
   updated: 01/12/2008 */

#include <SCE/utils/SCEUtils.h>
#include "SCE/interface/SCESceneResource.h"
#include "SCE/interface/SCESceneEntity.h"


/* intersection */
static SCE_SList* SCE_Batch_Inter (SCE_SList *a, SCE_SList *b)
{
    SCE_SList *l = NULL;
    SCE_SListIterator *it = NULL, *it2 = NULL;

    if (!(l = SCE_List_Create (NULL)))
        goto failure;

    SCE_List_ForEach (it, a) {
        SCE_List_ForEach (it2, b) {
            if (SCE_List_GetData (it) == SCE_List_GetData (it2)) {
                if (SCE_List_PrependNewl (l, SCE_List_GetData (it)) < 0)
                    goto failure;
                break;
            }
        }
    }

    goto success;
failure:
    SCE_List_Delete (l), l = NULL;
    SCEE_LogSrc ();
success:
    return l;
}

static int SCE_Batch_Sort (SCE_SList *finall, SCE_SList *entities,
                           SCE_SSceneResourceGroup **rgroups, int rec, int foo)
{
    SCE_SList *rlist = NULL, *l = NULL;
    SCE_SListIterator *it = NULL;
    int i;
    int type;
    int code = SCE_OK;

    if (rec < 0) {
        if (SCE_List_PrependNewl (finall, entities) < 0)
            goto failure;
        goto success;
    }

    type = SCE_SceneResource_GetGroupType (rgroups[rec]);

    if (foo) {
        SCE_SSceneEntity *e;
        if (!(l = SCE_List_Create (NULL)))
            goto failure;
        /* check the entities that aren't using a resource with
           the current checked type */
        SCE_List_ForEach (it, entities) {
            e = SCE_List_GetData (it);
            if (!SCE_SceneEntity_HasResourceOfType (e, type)) {
                if (SCE_List_PrependNewl (l, e) < 0)
                    goto failure;
            }
        }
        if (SCE_Batch_Sort (finall, l, rgroups, rec - 1, SCE_TRUE) < 0)
            goto failure;
        if (rec > 0)
            SCE_List_Delete (l);
    }

    rlist = SCE_SceneResource_GetResourcesList (rgroups[rec]);
    i = SCE_TRUE;
    SCE_List_ForEach (it, rlist) {
        if (!(l = SCE_Batch_Inter (entities, SCE_SceneResource_GetOwnersList
                                   (SCE_List_GetData (it)))))
            goto failure;
        if (SCE_Batch_Sort (finall, l, rgroups, rec - 1, i) < 0)
            goto failure;
        if (rec > 0)
            SCE_List_Delete (l);
        /*i = SCE_FALSE;*/
    }

    goto success;
failure:
    SCE_List_Delete (l);
    SCEE_LogSrc ();
    code = SCE_ERROR;
success:
    return code;
}

static int SCE_Batch_IsAlreadyPresent (void *data, SCE_SListIterator *it)
{
    SCE_List_ForEachPrev (it) {
        if (data == SCE_List_GetData (it))
            return SCE_TRUE;
    }
    return SCE_FALSE;
}

int SCE_Batch_SortEntities (SCE_SList *entities, unsigned int size,
                            SCE_SSceneResourceGroup **rgroups, unsigned int n,
                            int *order)
{
    int code = SCE_OK;
    SCE_SSceneResourceGroup **ordered_groups = NULL, *tmpgroup = NULL;
    unsigned int i, j, nok = 0;
    SCE_SList *list = NULL, *list2 = NULL;
    SCE_SListIterator *it = NULL, *it2 = NULL, *it3 = NULL;

    if (!(ordered_groups = SCE_malloc (n * sizeof *ordered_groups)))
        goto failure;
    if (!(list = SCE_List_Create (NULL)))
        goto failure;

    /* ordering the array of groups */
    for (i = 0; i < n; i++) {
        for (j = 0; j < size; j++) {
            if (order[i] == SCE_SceneResource_GetGroupType (rgroups[j])) {
                ordered_groups[i] = rgroups[j];
                nok++;
                break;
            }
        }
    }
    /* reverse order for Batch_Sort() function */
    for (i = 0; i < nok / 2; i++) {
        tmpgroup = ordered_groups[i];
        ordered_groups[i] = ordered_groups[nok - i - 1];
        ordered_groups[nok - i - 1] = tmpgroup;
    }

    /* calling the sort algorithm */
    if (SCE_Batch_Sort (list, entities, ordered_groups, nok - 1, SCE_TRUE) < 0)
        goto failure;

    /* set the entities ordered in 'entities' */
    /* normally, the two lists has the same size */
    it3 = SCE_List_GetFirst (entities);
    SCE_List_ForEach (it, list) {
        list2 = SCE_List_GetData (it);
        SCE_List_ForEach (it2, list2) {
            /* skip doubles */
            if (SCE_List_GetData (it2) == SCE_List_GetData (it3))
                it3 = SCE_List_GetNext (it3);
            else {
                if (!SCE_Batch_IsAlreadyPresent (SCE_List_GetData (it2), it3)) {
                    SCE_List_SetData (it3, SCE_List_GetData (it2));
                    it3 = SCE_List_GetNext (it3);
                }
            }
        }
    }

    goto success;
failure:
    SCEE_LogSrc ();
    code = SCE_ERROR;
success:
    SCE_List_Delete (list);
    SCE_free (ordered_groups);
    return code;
}
