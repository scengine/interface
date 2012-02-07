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
 
/* created: 06/03/2007
   updated: 01/02/2012 */

#include <ctype.h>
#include <SCE/utils/SCEUtils.h>
#include <SCE/renderer/SCERenderer.h>
#include "SCE/interface/SCEShaders.h"

static int is_init = SCE_FALSE;

static int resource_source_type = 0;
static int resource_shader_type = 0;

static int shdlocked = SCE_FALSE;
static int sce_shd_enabled = SCE_FALSE;

static SCE_SShader *used = NULL;

static void* SCE_Shader_LoadResource (const char*, int, void*);

int SCE_Init_Shader (void)
{
    if (is_init)
        return SCE_OK;

    /* register source loader */
    resource_source_type = SCE_Resource_RegisterType (SCE_TRUE, NULL, NULL);
    if (resource_source_type < 0)
        goto fail;
    /* TODO: may fail */
    SCE_Media_Register (resource_source_type, ".glsl",
                        SCE_Shader_LoadSourceFromFile, NULL);
    /* register shader loader */
    resource_shader_type = SCE_Resource_RegisterType (
        SCE_FALSE, SCE_Shader_LoadResource, NULL);
    if (resource_shader_type < 0)
        goto fail;

    is_init = SCE_TRUE;
    sce_shd_enabled = SCE_TRUE;

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    SCEE_LogSrcMsg ("failed to initialize shaders manager");
    return SCE_ERROR;
}

void SCE_Quit_Shader (void)
{
    sce_shd_enabled = SCE_FALSE;
    is_init = SCE_FALSE;
    used = NULL;
}

int SCE_Shader_GetResourceType (void)
{
    return resource_shader_type;
}


static void SCE_Shader_InitParam (SCE_SShaderParam *sp)
{
    sp->param = NULL;
    sp->index = -1;
    sp->size = 0;
    sp->setfv = NULL;
    sp->setm = NULL;
    SCE_List_InitIt (&sp->it);
    SCE_List_SetData (&sp->it, sp);
}
static void SCE_Shader_DeleteParam (void *p)
{
    SCE_free (p);
}

void SCE_Shader_Init (SCE_SShader *shader)
{
    SCE_RShaderType i;

    for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
        shader->shaders[i] = NULL;
        shader->sources[i] = NULL;
        shader->addsrc[i] = NULL;
    }
    shader->p_glsl = NULL;

    shader->ready = SCE_FALSE;
    shader->res = NULL;

    SCE_List_Init (&shader->params_i);
    SCE_List_Init (&shader->params_f);
    SCE_List_Init (&shader->params_m);
    SCE_List_SetFreeFunc (&shader->params_i, SCE_Shader_DeleteParam);
    SCE_List_SetFreeFunc (&shader->params_f, SCE_Shader_DeleteParam);
    SCE_List_SetFreeFunc (&shader->params_m, SCE_Shader_DeleteParam);

    SCE_SceneResource_Init (&shader->s_resource);
    SCE_SceneResource_SetResource (&shader->s_resource, shader);
    shader->path = NULL;
    shader->pipeline.shaders = NULL;
    shader->pipeline.n_shaders = 0;
}

SCE_SShader* SCE_Shader_Create (void)
{
    SCE_SShader *shader = NULL;
    shader = SCE_malloc (sizeof *shader);
    if (!shader)
        SCEE_LogSrc ();
    else {
        SCE_Shader_Init (shader);
        shader->p_glsl = SCE_RCreateProgram ();
        if (!shader->p_glsl) {
            SCE_free (shader);
            SCEE_LogSrc ();
            return NULL;
        }
    }
    return shader;
}


void SCE_Shader_Delete (SCE_SShader *shader)
{
    if (shader) {
        SCE_RShaderType i;
        if (!SCE_Resource_Free (shader))
            return;
        SCE_SceneResource_RemoveResource (&shader->s_resource);
        SCE_RDeleteProgram (shader->p_glsl);
        for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
            SCE_RDeleteShaderGLSL (shader->shaders[i]);
            SCE_free (shader->sources[i]);
            SCE_free (shader->addsrc[i]);
        }

        if (SCE_Resource_Free (shader->res)) {
            for (i = 0; i < SCE_NUM_SHADER_TYPES; i++)
                SCE_free (shader->res[i]);
            SCE_free (shader->res);
        }

        SCE_List_Clear (&shader->params_m);
        SCE_List_Clear (&shader->params_f);
        SCE_List_Clear (&shader->params_i);

        SCE_free (shader->path);
        if (shader->pipeline.shaders) {
            int j;
            /* shaders[0] is \p shader ;) */
            for (j = 1; j < shader->pipeline.n_shaders; j++)
                SCE_Shader_Delete (shader->pipeline.shaders[j]);
            SCE_free (shader->pipeline.shaders);
        }

        SCE_free (shader);
    }
}


SCE_SSceneResource* SCE_Shader_GetSceneResource (SCE_SShader *shader)
{
    return &shader->s_resource;
}

/* NOTE: move this function */
static int SCE_Shader_SetPosFile (FILE *fp, const char *str, int at_end)
{
    size_t i = 0;
    long cur = 0, curo = 0;
    char buf[256] = {0};
    size_t lenstr = 0;

    lenstr = strlen (str);
    curo = ftell (fp);

    while (1) {
        fseek (fp, curo+cur, SEEK_SET);
        cur++;

        for (i = 0; i < lenstr; i++) {
            buf[i] = fgetc (fp);
            if (buf[i] == EOF) {
                fseek (fp, curo, SEEK_SET);
                return 0;
            }
        }

        if (SCE_String_Cmp (str, buf, SCE_FALSE) == 0) {
            if (!at_end)
                fseek (fp, -lenstr, SEEK_CUR);
            return 1;
        }
    }

    fseek (fp, curo, SEEK_SET);

    return 0;
}

static char* SCE_Shader_LoadSource (FILE *fp, long end)
{
    char *src = NULL;
    long curpos;
    long len = ftell (fp);

    if (end != EOF) {
        len = end - len;
    } else {
        curpos = len;
        fseek (fp, 0, SEEK_END);
        len = ftell (fp) - len;
        fseek (fp, curpos, SEEK_SET);
    }

    if (len <= 0)
        return NULL;

    src = SCE_malloc (len + 1);
    if (!src) {
        SCEE_LogSrc ();
        return NULL;
    }
    src[len] = '\0';

    fread (src, 1, len, fp);

    return src;
}

static void* SCE_Shader_LoadSources (FILE *fp, const char *fname)
{
    char **srcs = NULL;
    /* mess. */
    long pos[SCE_NUM_SHADER_TYPES];
    long begin[SCE_NUM_SHADER_TYPES];
    long end[SCE_NUM_SHADER_TYPES];
    int type;
    SCE_RShaderType i;

    if (!(srcs = SCE_malloc (SCE_NUM_SHADER_TYPES * sizeof *srcs))) {
        SCEE_LogSrc ();
        return NULL;
    }

    for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
        srcs[i] = NULL;
        pos[i] = begin[i] = end[i] = EOF;
    }

#define SCE_SETPOS(name, length, id)                                    \
    if (SCE_Shader_SetPosFile (fp, name, SCE_FALSE)) {                  \
        pos[id] = ftell (fp);                                           \
        begin[id] = pos[id] + length; /* assume length = strlen(name); */ \
        rewind (fp);                                                    \
    }

    SCE_SETPOS ("[vertex shader]", 15, SCE_VERTEX_SHADER);
    SCE_SETPOS ("[control shader]", 16, SCE_TESS_CONTROL_SHADER);
    SCE_SETPOS ("[evaluation shader]", 19, SCE_TESS_EVALUATION_SHADER);
    SCE_SETPOS ("[geometry shader]", 17, SCE_GEOMETRY_SHADER);
    SCE_SETPOS ("[pixel shader]", 14, SCE_PIXEL_SHADER);
#undef SCE_SETPOS

    /* finding blocks (mess) */
    for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
        SCE_RShaderType j;
        for (j = 0; j < SCE_NUM_SHADER_TYPES; j++) {
            if (j == i) continue;
            if (pos[j] > pos[i]) {
                if (end[i] == EOF || (pos[j] < end[i] && pos[j] != EOF))
                    end[i] = pos[j];
            }
        }
    }

    for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
        fseek (fp, begin[i], SEEK_SET);
        srcs[i] = SCE_Shader_LoadSource (fp, end[i]);
    }

    return srcs;
}

/* TODO: w00t */
#define SCE_SHADER_INCLUDE "#include"

void* SCE_Shader_LoadSourceFromFile (FILE *fp, const char *fname, void *uusd)
{
    int i;
    SCE_RShaderType j;
    char buf[BUFSIZ] = {0};
    char *ptr = NULL;
    char **srcs = NULL;

    srcs = SCE_Shader_LoadSources (fp, fname);
    if (!srcs) {
        SCEE_LogSrc ();
        return NULL;
    }

    for (j = 0; j < SCE_NUM_SHADER_TYPES; j++)
        if (srcs[j] && (ptr = strstr (srcs[j], SCE_SHADER_INCLUDE))) {
            size_t len, diff = ptr - srcs[j];
            char *tsrc = NULL;
            char **isrcs = NULL;

            /* reading file name (syntax: #include <filename>) */
            memset (buf, '\0', BUFSIZ);
            i = 0;
            while (*ptr++ != '<');
            while (*ptr != '>')
                buf[i++] = *ptr++;
            /* recursive load of inclusions */
            /* TODO: include path of fname into buf */
            isrcs = SCE_Resource_Load (resource_source_type, buf, SCE_FALSE,
                                       NULL);
            if (!isrcs) {
                SCEE_LogSrc ();
                return NULL;
            }
            if (isrcs[j]) {
                len = strlen (srcs[j]) + strlen (isrcs[j]) + 1;
                tsrc = SCE_malloc (len);
                memset (tsrc, '\0', len);
                strncpy (tsrc, srcs[j], diff);
                strcat (tsrc, isrcs[j]);
                strcat (tsrc, ptr);
                SCE_free (srcs[j]);
                srcs[j] = tsrc;
            }
        }

    return srcs;
}

static void* SCE_Shader_LoadResource (const char *name, int force, void *data)
{
    SCE_RShaderType i;
    SCE_SShader *shader = NULL;
    char **srcs = NULL;

    if (force > 0)
        force--;

    if (!(shader = SCE_Shader_Create ()))
        goto fail;
    if (!(shader->path = SCE_String_Dup (name)))
        goto fail;

    srcs = SCE_Resource_Load (resource_source_type, name, force, NULL);
    if (!srcs)
        goto fail;

    shader->res = srcs;

    return shader;
fail:
    SCE_Shader_Delete (shader);
    SCEE_LogSrc ();
    return NULL;
}


SCE_SShader* SCE_Shader_Load (const char *name, int force)
{
    return SCE_Resource_Load (resource_shader_type, name, force, NULL);
}


void SCE_Shader_SetupFeedbackVaryings (SCE_SShader *shader, SCEuint n,
                                       const char **varyings,
                                       SCE_RFeedbackStorageMode mode)
{
    SCE_RSetProgramFeedbackVaryings (shader->p_glsl, n, varyings, mode);
}
void SCE_Shader_SetOutputTarget (SCE_SShader *shader, const char *output,
                                 SCE_RBufferType target)
{
    SCE_RSetProgramOutputTarget (shader->p_glsl, output, target);
}

int SCE_Shader_SetupPipeline (SCE_SShader *shader,
                              const SCE_SRenderState *state)
{
    size_t i, j;
    size_t n_states;

    n_states = SCE_Math_Powi (2, state->n_states);

    if (!(shader->pipeline.shaders =
          SCE_malloc (n_states * sizeof *shader->pipeline.shaders)))
        goto fail;
    shader->pipeline.n_shaders = n_states;

    for (i = 0; i < n_states; i++) {

        if (i == 0)
            shader->pipeline.shaders[0] = shader; /* tkt */
        else {
            /* 1 to force reloading shader but not source code */
            if (!(shader->pipeline.shaders[i] =SCE_Shader_Load(shader->path,1)))
                goto fail;
        }

        for (j = 0; j < state->n_states; j++) {
            const char *foo = "0";
            if (i & (1 << j))
                foo = "1";
            if (SCE_Shader_Global (shader->pipeline.shaders[i],
                                   SCE_RenderState_GetStateName (state, j),
                                   foo) < 0)
                goto fail;
        }
    }

    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

static int SCE_Shader_BuildShader (SCE_RShaderType type, char *source,
                                   SCE_RShaderGLSL **shader)
{
    if (source) {
        *shader = SCE_RCreateShaderGLSL (type);
        if (!*shader) {
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
        SCE_RSetShaderGLSLSource (*shader, source);
        if (SCE_RBuildShaderGLSL (*shader) < 0) {
            SCE_RDeleteShaderGLSL (*shader);
            SCEE_LogSrc ();
            return SCE_ERROR;
        }
    }
    return SCE_OK;
}

static int SCE_Shader_BuildGLSL (SCE_SShader *shader)
{
    SCE_RShaderType i;

    for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
        if (SCE_Shader_BuildShader (i, shader->sources[i],
                                    &shader->shaders[i]) < 0) return SCE_ERROR;
    }
    for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
        if (shader->shaders[i])
            SCE_RSetProgramShader (shader->p_glsl, shader->shaders[i],SCE_TRUE);
    }

    if (SCE_RBuildProgram (shader->p_glsl) < 0) {
        SCE_RDeleteProgram (shader->p_glsl);
        for (i = 0; i < SCE_NUM_SHADER_TYPES; i++)
            SCE_RDeleteShaderGLSL (shader->shaders[i]);
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    return SCE_OK;
}

static void SCE_Shader_BuildUniformSamplers (SCE_SShader *shader)
{
    SCE_Shader_Use (shader);
    SCE_Shader_Param (SCE_SHADER_UNIFORM_SAMPLER_0, 0);
    SCE_Shader_Param (SCE_SHADER_UNIFORM_SAMPLER_1, 1);
    SCE_Shader_Param (SCE_SHADER_UNIFORM_SAMPLER_2, 2);
    SCE_Shader_Param (SCE_SHADER_UNIFORM_SAMPLER_3, 3);
    SCE_Shader_Param (SCE_SHADER_UNIFORM_SAMPLER_4, 4);
    SCE_Shader_Param (SCE_SHADER_UNIFORM_SAMPLER_5, 5);
    SCE_Shader_Param (SCE_SHADER_UNIFORM_SAMPLER_6, 6);
    SCE_Shader_Param (SCE_SHADER_UNIFORM_SAMPLER_7, 7);
    SCE_Shader_Use (NULL);
}

static int SCE_Shader_BuildAux (SCE_SShader *shader)
{
    SCE_RShaderType i;

    if (shader->ready) {
        /* looks like we are rebuilding the shader! */
        shader->ready = SCE_FALSE;
        SCE_RDeleteProgram (shader->p_glsl);
        shader->p_glsl = NULL;
        if (!(shader->p_glsl = SCE_RCreateProgram ())) goto fail;
        for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
            SCE_RDeleteShaderGLSL (shader->shaders[i]);
            shader->shaders[i] = NULL;
        }
    }

    for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
        if ((shader->res && shader->res[i]) || shader->addsrc[i]) {
            SCE_free (shader->sources[i]);
            shader->sources[i] = SCE_String_CatDup (shader->addsrc[i],
                                                    shader->res ?
                                                    shader->res[i] : NULL);
            if (!shader->sources[i]) goto fail;
        }
    }

    if (SCE_Shader_BuildGLSL (shader) < 0) goto fail;
    SCE_Shader_BuildUniformSamplers (shader);

    shader->ready = SCE_TRUE;
    return SCE_OK;
fail:
    SCEE_LogSrc ();
    return SCE_ERROR;
}

int SCE_Shader_Build (SCE_SShader *shader)
{
    if (!shader->pipeline.shaders)
        return SCE_Shader_BuildAux (shader);
    else {
        size_t i;

        for (i = 0; i < shader->pipeline.n_shaders; i++) {
            if (SCE_Shader_BuildAux (shader->pipeline.shaders[i]) < 0)
                return SCE_ERROR;
        }
    }
    return SCE_OK;
}

int SCE_Shader_Validate (SCE_SShader *shader)
{
    return SCE_RValidateProgram (shader->p_glsl);
}

void SCE_Shader_SetupAttributesMapping (SCE_SShader *shader)
{
    SCE_RSetupProgramAttributesMapping (shader->p_glsl);
}
void SCE_Shader_ActivateAttributesMapping (SCE_SShader *shader, int activate)
{
    SCE_RActivateProgramAttributesMapping (shader->p_glsl, activate);
}

void SCE_Shader_SetupMatricesMapping (SCE_SShader *shader)
{
    SCE_RSetupProgramMatricesMapping (shader->p_glsl);
}
void SCE_Shader_ActivateMatricesMapping (SCE_SShader *shader, int activate)
{
    SCE_RActivateProgramMatricesMapping (shader->p_glsl, activate);
}

void SCE_Shader_SetPatchVertices (SCE_SShader *shader, int vertices)
{
    SCE_RSetProgramPatchVertices (shader->p_glsl, vertices);
}


int SCE_Shader_AddSource (SCE_SShader *shader, SCE_RShaderType type,
                          const char *src, int prepend)
{
    char *addsrc = NULL, *newsrc = NULL;

    addsrc = shader->addsrc[type];

    if (!addsrc) {
        newsrc = SCE_String_Dup (src);
    } else {
        if (prepend)
            newsrc = SCE_String_CatDup (src, addsrc);
        else
            newsrc = SCE_String_CatDup (addsrc, src);
    }

    if (!newsrc) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }

    SCE_free (addsrc);
    shader->addsrc[type] = newsrc;

    return SCE_OK;
}

int SCE_Shader_Local (SCE_SShader *shader, SCE_RShaderType type,
                      const char *define, const char *value)
{
#define SCE_LOL 256
    char buf[SCE_LOL] = {0};
    size_t size;

    size = strlen (define) + 11; /* 11 = strlen ("\n#define \n") + 1 */
    if (value)
        size += strlen (value) + 1; /* 1 for space between value and macro */

    if (size > SCE_LOL) {
        SCEE_Log (4242);
        SCEE_LogMsg ("your define string(s) is too long");
        return SCE_ERROR;
    }
#undef SCE_LOL

    strcpy (buf, "\n#define ");
    strcat (buf, define);
    if (value) {
        strcat (buf, " ");
        strcat (buf, value);
    }
    strcat (buf, "\n");

    if (SCE_Shader_AddSource (shader, type, buf, SCE_TRUE) < 0) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }

    return SCE_OK;
}
int SCE_Shader_Locali (SCE_SShader *shader, SCE_RShaderType type,
                       const char *define, int value)
{
    char buf[256] = {0};
    sprintf (buf, "(%d)", value);
    return SCE_Shader_Local (shader, type, define, buf);
}
int SCE_Shader_Localf (SCE_SShader *shader, SCE_RShaderType type,
                       const char *define, float value)
{
    char buf[256] = {0};
    sprintf (buf, "(%f)", value);
    return SCE_Shader_Local (shader, type, define, buf);
}


int SCE_Shader_Global (SCE_SShader *shader, const char *define,
                       const char *value)
{
    int i;
    for (i = 0; i < SCE_NUM_SHADER_TYPES; i++) {
        if ((shader->res && shader->res[i]) || shader->addsrc[i]) {
            if (SCE_Shader_Local (shader, i, define, value) < 0) {
                SCEE_LogSrc ();
                return SCE_ERROR;
            }
        }
    }
    return SCE_OK;
}
int SCE_Shader_Globali (SCE_SShader *shader, const char *define, int value)
{
    char buf[256] = {0};
    sprintf (buf, "(%d)", value);
    return SCE_Shader_Global (shader, define, buf);
}
int SCE_Shader_Globalf (SCE_SShader *shader, const char *define, float value)
{
    char buf[256] = {0};
    sprintf (buf, "(%f)", value);
    return SCE_Shader_Global (shader, define, buf);
}


int SCE_Shader_InputPrimitive (SCE_SShader *shader, SCE_EPrimitiveType prim,
                               int adj)
{
    return SCE_RSetProgramInputPrimitive (shader->p_glsl, prim, adj);
}
int SCE_Shader_OutputPrimitive (SCE_SShader *shader, SCE_EPrimitiveType prim)
{
    return SCE_RSetProgramOutputPrimitive (shader->p_glsl, prim);
}


int SCE_Shader_GetIndex (SCE_SShader *shader, const char *name)
{
    return SCE_RGetProgramIndex (shader->p_glsl, name);
}

int SCE_Shader_GetAttribIndex (SCE_SShader *shader, const char *name)
{
    return SCE_RGetProgramAttribIndex (shader->p_glsl, name);
}

void SCE_Shader_Param (const char *name, int value)
{
    SCE_RSetProgramParam (SCE_RGetProgramIndex (used->p_glsl, name), value);
}

void SCE_Shader_Paramf (const char *name, float value)
{
    SCE_RSetProgramParamf (SCE_RGetProgramIndex (used->p_glsl, name), value);
}


#define SCE_GLSL_PARAM_FUNC(n)\
void SCE_Shader_Param##n##fv (const char *name, size_t size, float *value)\
{\
    SCE_RSetProgramParam##n##fv (SCE_RGetProgramIndex (used->p_glsl, name), size, value);\
}
SCE_GLSL_PARAM_FUNC(1)
SCE_GLSL_PARAM_FUNC(2)
SCE_GLSL_PARAM_FUNC(3)
SCE_GLSL_PARAM_FUNC(4)
#undef SCE_GLSL_PARAM_FUNC

void SCE_Shader_SetParam (int index, int value)
{
    SCE_RSetProgramParam (index, value);
}

void SCE_Shader_SetParamf (int index, float value)
{
    SCE_RSetProgramParamf (index, value);
}

#define SCE_GLSL_SETPARAM_FUNC(n)                                       \
    void SCE_Shader_SetParam##n##fv (int index, size_t size, float *value) \
    {                                                                   \
        SCE_RSetProgramParam##n##fv (index, size, value);               \
    }
SCE_GLSL_SETPARAM_FUNC(1)
SCE_GLSL_SETPARAM_FUNC(2)
SCE_GLSL_SETPARAM_FUNC(3)
SCE_GLSL_SETPARAM_FUNC(4)
#undef SCE_GLSL_SETPARAM_FUNC

void SCE_Shader_SetMatrix3 (int index, SCE_TMatrix3 m)
{
    SCE_RSetProgramMatrix3 (index, 1, m); /* count forced */
}
void SCE_Shader_SetMatrix3v (int index, const float *m, size_t num)
{
    SCE_RSetProgramMatrix3 (index, num, m);
}

void SCE_Shader_SetMatrix4 (int index, SCE_TMatrix4 m)
{
    SCE_RSetProgramMatrix4 (index, 1, m); /* count forced */
}
void SCE_Shader_SetMatrix4v (int index, const float *m, size_t num)
{
    SCE_RSetProgramMatrix4 (index, num, m);
}

/**
 * \brief Adds a constant parameter
 * \param name the name of the parameter in the shader source code
 * \param p pointer to the values that will be send
 *
 * Adds a parameter that will be send on each call of SCE_Shader_Use(\p shader).
 * The shader must have been built.
 *
 * \sa SCE_Shader_AddParamfv()
 */
int SCE_Shader_AddParamv (SCE_SShader *shader, const char *name, void *p)
{
    SCE_SShaderParam *param = NULL;
    if (!(param = SCE_malloc (sizeof *param))) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    SCE_Shader_InitParam (param);
    SCE_List_Appendl (&shader->params_i, &param->it);
    param->param = p;
    param->index = SCE_Shader_GetIndex (shader, name);
    return SCE_OK;
}
/**
 * \brief Adds a constant floating parameter
 * \param name the name of the parameter in the shader source code
 * \param num vector's length, can be 1, 2, 3 or 4
 * \param size number of vectors in \p p (only if is an array, set it as 1
 * otherwise)
 * \param p pointer to the values that will be send
 *
 * Adds a floating parameter that will be send on each call of
 * SCE_Shader_Use(\p shader).
 * The shader must have been built.
 *
 * \sa SCE_Shader_AddParamv()
 */
int SCE_Shader_AddParamfv (SCE_SShader *shader, const char *name,
                           int num, int size, void *p)
{
    SCE_SShaderParam *param = NULL;
    if (!(param = SCE_malloc (sizeof *param))) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    SCE_Shader_InitParam (param);
    SCE_List_Appendl (&shader->params_f, &param->it);
    param->param = p;
    switch (num) {
#define SCE_SHDCASE(n) case n: param->setfv = SCE_Shader_SetParam##n##fv; break;
    SCE_SHDCASE (1)
    SCE_SHDCASE (2)
    SCE_SHDCASE (3)
    SCE_SHDCASE (4)
#undef SCE_SHDCASE
    }
    param->size = size;
    param->index = SCE_Shader_GetIndex (shader, name);
    return SCE_OK;
}
/**
 * \brief Adds a constant matrix
 * \param name the name of the matrix in the shader source code
 * \param p pointer to the matrix
 *
 * Adds a matrix that will be send on each call of SCE_Shader_Use(\p shader).
 * The shader must have been built.
 *
 * \sa SCE_Shader_AddParamfv()
 */
int SCE_Shader_AddMatrix (SCE_SShader *shader, const char *name,
                          int size, void *p)
{
    SCE_SShaderParam *param = NULL;
    if (!(param = SCE_malloc (sizeof *param))) {
        SCEE_LogSrc ();
        return SCE_ERROR;
    }
    SCE_Shader_InitParam (param);
    SCE_List_Appendl (&shader->params_m, &param->it);
    param->param = p;
    param->index = SCE_Shader_GetIndex (shader, name);
    if (size == 3)
        param->setm = SCE_Shader_SetMatrix3;
    else
        param->setm = SCE_Shader_SetMatrix4;
    return SCE_OK;
}

void SCE_Shader_Active (int enabled)
{
    sce_shd_enabled = enabled;
}
void SCE_Shader_Enable (void)
{
    sce_shd_enabled = SCE_TRUE;
}
void SCE_Shader_Disable (void)
{
    sce_shd_enabled = SCE_FALSE;
}

static void SCE_Shader_SetParams (SCE_SShader *shader)
{
    SCE_SListIterator *i = NULL;
    SCE_SShaderParam *p = NULL;
    SCE_List_ForEach (i, &shader->params_i) {
        p = SCE_List_GetData (i);
        SCE_Shader_SetParam (p->index, *((int*)p->param));
    }
    SCE_List_ForEach (i, &shader->params_f) {
        p = SCE_List_GetData (i);
        p->setfv (p->index, p->size, p->param);
    }
    SCE_List_ForEach (i, &shader->params_m) {
        p = SCE_List_GetData (i);
        p->setm (p->index, p->param);
    }
}

static void SCE_Shader_UseGLSL (SCE_SShader *shader)
{
    if (!sce_shd_enabled)
        return;
    else if (shader)
        SCE_RUseProgram (shader->p_glsl);
    else
        SCE_RUseProgram (NULL);
}
void SCE_Shader_Use (SCE_SShader *shader)
{
    if (shdlocked)
        return;
    if (!shader) {
        if (used) {
            SCE_Shader_UseGLSL (NULL);
            used = NULL;
        }
    } else if (shader == used)
        SCE_Shader_SetParams (shader);
    else {
        SCE_Shader_UseGLSL (shader);
        used = shader;        /* before setparams() call */
        SCE_Shader_SetParams (shader);
    }
}
void SCE_Shader_UsePipeline (SCE_SShader *shader, SCEuint state)
{
    SCE_Shader_Use (shader->pipeline.shaders[state]);
}
SCE_SShader* SCE_Shader_GetShader (SCE_SShader *shader, SCEuint state)
{
    return shader->pipeline.shaders[state];
}


/**
 * \brief Any further call to SCE_Shader_Use() is ignored
 * \sa SCE_Shader_Unlock()
 */
void SCE_Shader_Lock (void)
{
    shdlocked = SCE_TRUE;
}
/**
 * \brief Further calls to SCE_Shader_Use() are no longer ignored
 * \sa SCE_Shader_Lock()
 */
void SCE_Shader_Unlock (void)
{
    shdlocked = SCE_FALSE;
}
