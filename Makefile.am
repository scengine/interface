SUBDIRS = src include doc
pkgconfig_DATA = sceinterface.pc
EXTRA_DIST = $(pkgconfig_DATA)

.PHONY: doc

doc:
	@SCE_DOC_TARGET@

ACLOCAL_AMFLAGS = -I build/m4 -I build
