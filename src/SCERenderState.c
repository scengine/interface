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

/* created: 05/11/2011
   updated: 06/11/2011 */

#include <SCE/core/SCECore.h>
#include <SCE/renderer/SCERenderer.h>

#include "SCE/interface/SCERenderState.h"

void SCE_RenderState_Init (SCE_SRenderState *state)
{
    state->names = NULL;
    state->n_states = 0;
    state->state = 0;
}
void SCE_RenderState_Clear (SCE_SRenderState *state)
{
    size_t i;
    for (i = 0; i < state->n_states; i++)
        SCE_free (state->names[i]);
    SCE_free (state->names);
}

int SCE_RenderState_SetStates (SCE_SRenderState *state, const char **names,
                               size_t n_names)
{
    size_t i;

    if (!(state->names = SCE_malloc (n_names * sizeof *state->names)))
        goto fail;
    for (i = 0; i < n_names; i++) {
        if (!(state->names[i] = SCE_String_Dup (names[i])))
            goto fail;
    }

    state->n_states = n_names;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

const char* SCE_RenderState_GetStateName (const SCE_SRenderState *state,
                                          size_t n)
{
    return state->names[n];
}
