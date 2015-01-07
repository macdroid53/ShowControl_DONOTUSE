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
	button_clicked.c \
	button_clicked.h \
	gstreamer_utils.c \
	gstreamer_utils.h \
	message_handler.c \
	message_handler.h \
	update_display.c \
	update_display.h \
	menu_handler.c \
	menu_handler.h	


play_sound_LDFLAGS = \
	-Wl,--export-dynamic

play_sound_LDADD = $(PLAY_SOUND_LIBS)




EXTRA_DIST = $(ui_DATA)

# Remove ui directory on uninstall
uninstall-local:
	-rm -r $(uidir)
	-rm -r $(pkgdatadir)

