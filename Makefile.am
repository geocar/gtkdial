moduledir = $(libdir)/gtk-2.0/modules

AM_CFLAGS = $(gtk_CFLAGS) $(gdk_CFLAGS) $(glib_CFLAGS) $(pcre_CFLAGS)
LIBS += $(gtk_LIBS) $(gdk_LIBS) $(glib_LIBS) $(pcre_LIBS)

libgtkdial_la_SOURCES = gtkdial.c
libgtkdial_la_LDFLAGS = -rpath $(moduledir) -module -avoid-version $(LIBS) @LDFLAGS@
libgtkdial_la_LIBADD =

module_LTLIBRARIES = libgtkdial.la
