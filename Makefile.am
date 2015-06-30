AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4

pkglib_LTLIBRARIES = Hoa.la

Hoa_la_CXXFLAGS = @PD_CPPFLAGS@ -s -O3 -fPIC -fpermissive -std=c++11
Hoa_la_LDFLAGS = -module -avoid-version -shared -shrext .@EXTENSION@  @PD_LDFLAGS@ -fPIC
Hoa_la_LIBADD = Sources/libHoapd.la
Hoa_la_LIBADD += ThirdParty/CicmWrapper/Sources/libCicmWrapper.la

Hoa_la_SOURCES = hoa.library.hpp \
hoa.library.cpp

ALL_CFLAGS = -I"$(PD_INCLUDE)"

Hoadir=$(pkglibdir)

dist_Hoa_DATA = \
	LICENSE.txt \
	README.txt
	
EXTRA_DIST = autogen.sh

DISTDIR=$(PACKAGE_NAME)-$(PACKAGE_VERSION)
ORIGDIR=pd-$(PACKAGE_NAME)_$(PACKAGE_VERSION)
OS=$(shell uname -s)
DISTBINDIR=$(DISTDIR)-$(OS)

SUBDIRS= Sources
SUBDIRS+= ThirdParty/CicmWrapper
SUBDIRS+= Package/Hoa

$(abs_builddir)/.libs/Hoa.@EXTENSION@: Hoa.la

$(top_builddir)/Hoa.@EXTENSION@: $(abs_builddir)/.libs/Hoa.@EXTENSION@ 
		rm -f $@ && cd $(top_builddir) && test -e $< && $(LN_S) $< Hoa.@EXTENSION@ || true

.PHONY: clean-conveniencelink help

clean-conveniencelink:
	test -L $(top_builddir)/Hoa.@EXTENSION@ && rm -f $(top_builddir)/Hoa.@EXTENSION@ || true


all-local:: $(top_builddir)/Hoa.@EXTENSION@ help

clean-local:: clean-conveniencelink
	
README.txt:
	cp README.md README.txt
