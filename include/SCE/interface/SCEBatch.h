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
   updated: 05/06/2009 */

#ifndef SCEBATCH_H
#define SCEBATCH_H

#include <SCE/utils/SCEUtils.h>
#include "SCE/interface/SCESceneResource.h"

#ifdef __cplusplus
extern "C" {
#endif

int SCE_Batch_SortEntities (SCE_SList*, unsigned int, SCE_SSceneResourceGroup**,
                            unsigned int, int*);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* guard */
