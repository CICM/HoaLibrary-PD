dnl Process this file with autoconf to produce a configure script.
## replace the email below with authors' emails
AC_INIT([Hoa], 2.2-git, musique@univ-paris8.fr,[Hoa],[https://github.com/CICM/HoaLibrary-PD])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_AUX_DIR(config)
AC_CONFIG_FILES([Sources/Makefile])
AC_CONFIG_FILES([ThirdParty/CicmWrapper/Makefile  ThirdParty/CicmWrapper/Sources/Makefile  ThirdParty/CicmWrapper/Sources/ebox/Makefile ThirdParty/CicmWrapper/Sources/eclass/Makefile ThirdParty/CicmWrapper/Sources/ecommon/Makefile ThirdParty/CicmWrapper/Sources/egraphics/Makefile ThirdParty/CicmWrapper/Sources/eobj/Makefile ThirdParty/CicmWrapper/Sources/epopup/Makefile])
AC_CONFIG_FILES([Package/Hoa/Makefile Package/Hoa/helps/Makefile Package/Hoa/media/Makefile Package/Hoa/misc/Makefile Package/Hoa/patchers/Makefile Package/Hoa/tutorials/Makefile])

AC_ARG_WITH([pdextended], [AS_HELP_STRING([--with-pdextended], [use PD-Extended instead of PD-Vanilla])], [], [with_pdextended=no])
AS_IF([test "x$with_pdextended" != xno],  AC_DEFINE(PD_EXTENDED,[],[Use PD-Extended]), [])

AC_ARG_WITH([cblas], [AS_HELP_STRING([--with-cblas], [use cBlas optimizations])], [], [with_cblas=no])
AS_IF([test "x$with_cblas" != xno],  AC_DEFINE(HOA_USE_BLAS,[],[Use cBlas]), [])
AM_CONDITIONAL([HOA_USE_BLAS], [test $HOA_USE_BLAS = yes])

AX_FAT_BINARY()

AC_SUBST(PD_CPPFLAGS)
AC_SUBST(PD_LDFLAGS)

AC_SUBST(EXTENSION)

AM_INIT_AUTOMAKE([1.10 foreign])

AC_ARG_WITH([pd],       [  --with-pd=</path/to/pd> where to look for pd-headers and and -libs])
AC_ARG_WITH([extension],[  --with-extension=<ext>  enforce a certain extension for the dynamic library (e.g. dll)])

ARCH=$(uname -m)
KERN=$(uname -s)

AC_PROG_CXX
AC_PROG_LIBTOOL

if test "x${libdir}" = "x\${exec_prefix}/lib"; then
 libdir='${exec_prefix}/lib/pd/extra'
fi

if test "x$with_pd" != "x"; then
 if test -d "${with_pd}"; then
   PDPATH=${with_pd}
 fi
 if test -d "${PDPATH}/src"; then
  AC_MSG_RESULT([adding ${PDPATH}/src to INCLUDES])
  PD_CPPFLAGS="-I${PDPATH}/src ${CPPFLAGS}"

  AC_MSG_RESULT([adding ${PDPATH}/src to LDFLAGS])
  PD_LDFLAGS="-L${PDPATH}/src ${LDFLAGS}"
 else
  if test -d "${PDPATH}"; then
   AC_MSG_RESULT([adding ${PDPATH} to INCLUDES])
   PD_CPPFLAGS="-I${PDPATH} ${CPPFLAGS}"
  fi
 fi
 if test -d "${PDPATH}/bin"; then
  AC_MSG_RESULT([adding ${PDPATH}/bin to LDFLAGS])
  PD_LDFLAGS="-L${PDPATH}/bin ${LDFLAGS}"
 else
  if test -d "${PDPATH}"; then
   AC_MSG_RESULT([adding ${PDPATH} to LDFLAGS])
   PD_LDFLAGS="-L${PDPATH} ${LDFLAGS}"
  fi
 fi
else 
  AC_MSG_RESULT([no pd path given, trying with pkg-config data])
  PD_CPPFLAGS=`pkg-config --cflags pd`
  PD_LDFLAGS=`pkg-config --libs pd`
fi

AC_MSG_CHECKING([extension])
if test "x$with_extension" != "x"
then
 EXTENSION=$with_extension
else
 case "$KERN" in
 Darwin)
   EXTENSION=pd_darwin
   ;;
 Linux)
   if test "$ARCH" == x86_64
   then
     EXTENSION=l_ia64
   else
     EXTENSION=l_i386
   fi
   ;;
 *CYGWIN*)
   EXTENSION=dll
   ;;
 *MINGW*)
   EXTENSION=dll
   ;;
 *)
   EXTENSION=pd_linux
   ;;
 esac
fi
AC_MSG_RESULT([$EXTENSION])

AC_OUTPUT(Makefile)
