lib_LTLIBRARIES = libsceinterface.la

libsceinterface_la_CPPFLAGS = -I$(srcdir)/../include
libsceinterface_la_CFLAGS   = @SCE_UTILS_CFLAGS@ \
                              @SCE_CORE_CFLAGS@ \
                              @SCE_RENDERER_CFLAGS@ \
                              @PTHREAD_CFLAGS@ \
                              @SCE_DEBUG_CFLAGS@ \
                              @SCE_DEBUG_CFLAGS_EXPORT@
libsceinterface_la_LIBADD   = @SCE_UTILS_LIBS@ \
                              @SCE_CORE_LIBS@ \
                              @SCE_RENDERER_LIBS@ \
                              @PTHREAD_LIBS@
libsceinterface_la_LDFLAGS  = -version-info @SCE_INTERFACE_LTVERSION@
libsceinterface_la_SOURCES  = SCELight.c \
                              SCEMesh.c \
                              SCERenderState.c \
                              SCEQuad.c \
                              SCEGeometryInstance.c \
                              SCEBatch.c \
                              SCESceneResource.c \
                              SCEMaterial.c \
                              SCETexture.c \
                              SCEShaders.c \
                              SCESceneEntity.c \
                              SCESkybox.c \
                              SCESprite.c \
                              SCEModel.c \
                              SCEVoxelTerrain.c \
                              SCEDeferred.c \
                              SCEScene.c \
                              SCEInterface.c
