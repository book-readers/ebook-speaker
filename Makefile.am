AUTOMAKE_OPTIONS = foreign

SUBDIRS = man doc po src

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = config.rpath m4/Makefile.in doc/COMPILE \
             doc/COPYING doc/Changelog doc/License doc/Readme doc/TODO \
             doc/ebook-speaker.* \
             doc/icons_copyright.txt doc/screenshot.html \
             man/ebook-speaker.1 \
             src/pactl/aclocal.m4 src/pactl/AGPL \
             src/pactl/compile src/pactl/config.guess \
             src/pactl/config.sub \
             src/pactl/config.h.in src/pactl/configure \
             src/pactl/configure.ac src/pactl/depcomp \
             src/pactl/GPL src/pactl/install-sh \
             src/pactl/LGPL src/pactl/LICENSE \
             src/pactl/m4 \
             src/pactl/Makefile.am src/pactl/Makefile.in \
             src/pactl/missing src/pactl/NEWS \
             src/pactl/README \
             src/pactl/src/core-error.h \
             src/pactl/src/core-util.h \
             src/pactl/src/i18n.h \
             src/pactl/src/install.sh \
             src/pactl/src/macro.h \
             src/pactl/src/Makefile.am \
             src/pactl/src/Makefile.in \
             src/pactl/src/pactl.c \
             src/pactl/src/socket.h \
             src/pactl/src/strbuf.h \
             src/pactl/src/strlist.h \
             src/pactl/todo

check-gettext:
	@if test x$(USE_NLS) != "xyes" ; then \
	  echo "Missing gettext. Rerun configure and check for" \"'checking whether to use NLS... yes'!" ; exit 1; fi

update-po: check-gettext
	@find $(srcdir)/src/ -name "*.cpp" -print | sort > $(srcdir)/po/POTFILES.in.2 ; \
	if diff $(srcdir)/po/POTFILES.in $(srcdir)/po/POTFILES.in.2 >/dev/null 2>&1 ; then \
	   rm -f $(srcdir)/po/POTFILES.in.2 ; \
	else \
	   mv $(srcdir)/po/POTFILES.in.2 $(srcdir)/po/POTFILES.in ; \
	fi
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po

update-gmo: check-gettext
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo

force-update-gmo: check-gettext
	touch po/*.po
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo
                          
force-update-gmo-%: check-gettext
	@language=`echo $@ | sed s/force-update-gmo-//` ; \
	if test ! -f po/$$language.po ; then \
	   echo "file po/$$language.po does not exist" ; exit 1 \
	fi ; \
	touch po/$$language.po ; \
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-gmo
	
checkinstall:
	@echo
	@echo "checkinstall..."
	@echo
	make distdir
	cd $(PACKAGE)-$(VERSION)
	checkinstall -D --fstrans --pkgname "$(PACKAGE)" \
	--pkgversion $(VERSION) --pkggroup audio \
	--maintainer "Jos Lemmens \<jos\@jlemmens.nl\>" \
	--provides "ebook-speaker" \
	--requires "unar,alsa-utils,pulseaudio-utils,sox,calibre,pandoc,unrtf" \
	--strip --stripso \
	--review-control

.PHONY: check-gettext update-po update-gmo force-update-gmo
