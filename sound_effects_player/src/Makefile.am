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
	 -g

bin_PROGRAMS = sound_effects_player

sound_effects_player_SOURCES = \
	main.c \
	sound_effects_player.h \
	sound_effects_player.c \
	network_subroutines.c \
	network_subroutines.h \
	parse_net_subroutines.c \
	parse_net_subroutines.h \
	parse_xml_subroutines.c \
	parse_xml_subroutines.h \
	button_subroutines.c \
	button_subroutines.h \
	gstreamer_subroutines.c \
	gstreamer_subroutines.h \
	menu_subroutines.c \
	menu_subroutines.h \
	message_subroutines.c \
	message_subroutines.h \
	display_subroutines.c \
	display_subroutines.h	

sound_effects_player_LDFLAGS = \
	-Wl,--export-dynamic

sound_effects_player_LDADD = $(SOUND_EFFECTS_PLAYER_LIBS)

EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)

