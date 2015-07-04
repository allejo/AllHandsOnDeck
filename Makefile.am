lib_LTLIBRARIES = AllHandsOnDeck.la

AllHandsOnDeck_la_SOURCES = AllHandsOnDeck.cpp
AllHandsOnDeck_la_CPPFLAGS= -I$(top_srcdir)/include -I$(top_srcdir)/plugins/plugin_utils
AllHandsOnDeck_la_LDFLAGS = -module -avoid-version -shared
AllHandsOnDeck_la_LIBADD = $(top_builddir)/plugins/plugin_utils/libplugin_utils.la

AM_CPPFLAGS = $(CONF_CPPFLAGS)
AM_CFLAGS = $(CONF_CFLAGS)
AM_CXXFLAGS = $(CONF_CXXFLAGS)

EXTRA_DIST = \
	README.txt \
	AllHandsOnDeck.def \
	AllHandsOnDeck.sln \
	AllHandsOnDeck.vcxproj

MAINTAINERCLEANFILES =	\
	Makefile.in
