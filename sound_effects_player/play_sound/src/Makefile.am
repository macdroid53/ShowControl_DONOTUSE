## Process this file with automake to produce Makefile.in

## Created by Anjuta


uidir = $(pkgdatadir)/ui
ui_DATA = play_sound.ui app-menu.ui


AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(PLAY_SOUND_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = play_sound

play_sound_SOURCES = \
	main.c \
	play_sound.h \
	play_sound.c \
	network_subroutines.c \
	network_subroutines.h \
	parse_subroutines.c \
	parse_subroutines.h \
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


play_sound_LDFLAGS = \
	-Wl,--export-dynamic

play_sound_LDADD = $(PLAY_SOUND_LIBS)




EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)

