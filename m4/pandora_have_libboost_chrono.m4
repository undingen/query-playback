dnl Copyright (C) 2010 Andrew Hutchings
dnl This file is free software; Andrew Hutchings
dnl gives unlimited permission to copy and/or distribute it,
dnl with or without modifications, as long as this notice is preserved.

AC_DEFUN([_PANDORA_SEARCH_BOOST_CHRONO],[
  AC_REQUIRE([AC_LIB_PREFIX])
  AC_REQUIRE([ACX_PTHREAD])

  dnl --------------------------------------------------------------------
  dnl  Check for boost::chrono
  dnl --------------------------------------------------------------------

  save_CFLAGS="${CFLAGS}"
  save_CXXFLAGS="${CXXFLAGS}"
  CFLAGS="${PTHREAD_CFLAGS} ${CFLAGS}"
  CXXFLAGS="${PTHREAD_CFLAGS} ${CXXFLAGS}"

  AC_LANG_PUSH(C++)
  AC_LIB_HAVE_LINKFLAGS(boost_chrono-mt,boost_system-mt,[
    #include <boost/chrono.hpp>
  ],[
    boost::chrono::system_clock::now();
  ])
  AS_IF([test "x${ac_cv_libboost_chrono_mt}" = "xno"],[
    AC_LIB_HAVE_LINKFLAGS(boost_chrono,boost_system,[
      #include <boost/chrono.hpp>
    ],[
      boost::chrono::system_clock::now();
    ])
  ])
  AC_LANG_POP()
  CFLAGS="${save_CFLAGS}"
  CXXFLAGS="${save_CXXFLAGS}"

  
  AM_CONDITIONAL(HAVE_BOOST_CHRONO,
    [test "x${ac_cv_libboost_chrono}" = "xyes" -o "x${ac_cv_libboost_chrono_mt}" = "xyes"])
  BOOST_LIBS="${BOOST_LIBS} ${LTLIBBOOST_CHRONO_MT} ${LTLIBBOOST_CHRONO}"
  AC_SUBST(BOOST_LIBS)
])

AC_DEFUN([PANDORA_HAVE_BOOST_CHRONO],[
  PANDORA_HAVE_BOOST($1)
  _PANDORA_SEARCH_BOOST_CHRONO($1)
])

AC_DEFUN([PANDORA_REQUIRE_BOOST_CHRONO],[
  PANDORA_REQUIRE_BOOST($1)
  _PANDORA_SEARCH_BOOST_CHRONO($1)
  AS_IF([test "x${ac_cv_libboost_chrono}" = "xno" -a "x${ac_cv_libboost_chrono_mt}" = "xno"],
      AC_MSG_ERROR([boost::chrono is required for ${PACKAGE}]))
])

