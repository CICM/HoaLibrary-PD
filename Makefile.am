AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

pkglib_LTLIBRARIES = hoa.la

hoa_la_CXXFLAGS = @PD_CPPFLAGS@ -s -O2 -fPIC -fpermissive 
hoa_la_LDFLAGS = -module -avoid-version -shared -shrext .@EXTENSION@  @PD_LDFLAGS@ -fPIC
hoa_la_LIBADD = Sources/libHoaPd.la
hoa_la_LIBADD += ThirdParty/CircmWrapper/Sources/libCicmWrapper.la

hoa_la_SOURCES = c.library.h \
 c.library.cpp

ALL_CFLAGS = -I"$(PD_INCLUDE)"

hoadir=$(pkglibdir)

dist_hoa_DATA = \
	hoa-meta.pd \
	LICENSE.txt \
	README.txt
	
EXTRA_DIST = autogen.sh

DISTDIR=$(PACKAGE_NAME)-$(PACKAGE_VERSION)
ORIGDIR=pd-$(PACKAGE_NAME)_$(PACKAGE_VERSION)
OS=$(shell uname -s)
DISTBINDIR=$(DISTDIR)-$(OS)

SUBDIRS=Sources

SUBDIRS+= ThirdParty/CircmWrapper
SUBDIRS+= Package/Hoa

$(abs_builddir)/.libs/hoa.@EXTENSION@: hoa.la

$(top_builddir)/hoa.@EXTENSION@: $(abs_builddir)/.libs/hoa.@EXTENSION@ 
		rm -f $@ && cd $(top_builddir) && test -e $< && $(LN_S) $< hoa.@EXTENSION@ || true

.PHONY: clean-conveniencelink help

clean-conveniencelink:
	test -L $(top_builddir)/hoa.@EXTENSION@ && rm -f $(top_builddir)/hoa.@EXTENSION@ || true


all-local:: $(top_builddir)/hoa.@EXTENSION@ help

clean-local:: clean-conveniencelink
	
README.txt:
	cp README.md README.txt
