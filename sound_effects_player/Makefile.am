## Process this file with automake to produce Makefile.in
## Created originally by Anjuta

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src po doc icons

dist_doc_DATA = \
	README \
	COPYING \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS \
	listing/code.tex \
	listing/code.pdf


INTLTOOL_FILES = intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

EXTRA_DIST = \
	autogen.sh \
	sound_effects_player.desktop \
	icons/sound_effects_player_icon.png \
	icons/sound_effects_player_icon_hires.png \
	$(INTLTOOL_FILES)

iconsdir=${datadir}/icons/hicolor/48x48/apps
icons_DATA = icons/sound_effects_player_icon.png

desktopdir=${datadir}/applications
dist_desktop_DATA = sound_effects_player.desktop

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(docdir)

# Rules to create pdf and rtf files from odt files
doc/sound_effects_player_overview.pdf : doc/sound_effects_player_overview.odt
	libreoffice --headless --convert-to pdf doc/sound_effects_player_overview.odt --outdir doc

doc/sound_effects_player_overview.rtf : doc/sound_effects_player_overview.odt
	libreoffice --headless --convert-to rtf doc/sound_effects_player_overview.odt --outdir doc
