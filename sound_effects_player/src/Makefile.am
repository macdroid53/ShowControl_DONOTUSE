## Process this file with automake to produce Makefile.in
## Originally created by Anjuta

uidir = $(pkgdatadir)/ui
ui_DATA = sound_effects_player.ui app-menu.ui preferences.ui

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(SOUND_EFFECTS_PLAYER_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	-pthread\
	-g

bin_PROGRAMS = sound_effects_player

sound_effects_player_SOURCES = \
	button_subroutines.c \
	button_subroutines.h \
	display_subroutines.c \
	display_subroutines.h \
	gstreamer_subroutines.c \
	gstreamer_subroutines.h \
	main.c \
	main.h \
	menu_subroutines.c \
	menu_subroutines.h \
	message_subroutines.c \
	message_subroutines.h \
	network_subroutines.c \
	network_subroutines.h \
	parse_net_subroutines.c \
	parse_net_subroutines.h \
	parse_xml_subroutines.c \
	parse_xml_subroutines.h \
	sequence_structure.h \
	sequence_subroutines.c \
	sequence_subroutines.h \
	signal_subroutines.c \
	signal_subroutines.h \
	sound_effects_player.c \
	sound_effects_player.h \
	sound_structure.h \
	sound_subroutines.c \
	sound_subroutines.h \
	timer_subroutines.c \
	timer_subroutines.h

sound_effects_player_LDFLAGS = \
	-Wl,--export-dynamic

sound_effects_player_LDADD = $(SOUND_EFFECTS_PLAYER_LIBS)

EXTRA_DIST = $(ui_DATA)

# Note: plugindir is set in configure

# These are application-specific Gstreamer plugins
plugin_LTLIBRARIES = libgstenvelope.la libgstlooper.la

# sources used to compile the application-specific plugins
libgstenvelope_la_SOURCES = gstenvelope.c gstenvelope.h
libgstlooper_la_SOURCES = gstlooper.c gstlooper.h

# compiler and linker flags used to compile these plugins, set in configure.ac
libgstenvelope_la_CFLAGS = $(GST_CFLAGS)
libgstenvelope_la_LIBADD = $(GST_LIBS)
libgstenvelope_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstenvelope_la_LIBTOOLFLAGS = --tag=disable-static
libgstlooper_la_CFLAGS = $(GST_CFLAGS)
libgstlooper_la_LIBADD = $(GST_LIBS)
libgstlooper_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstlooper_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = gstenvelope.h gstlooper.h

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)

