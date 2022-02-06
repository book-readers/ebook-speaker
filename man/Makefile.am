man_MANS = eBook-speaker.1

all: eBook-speaker

eBook-speaker:
#	@which txt2man > /dev/null && txt2man -p ${srcdir}/../doc/$@.txt > \
#	       ${srcdir}/$@.1
#	@file ${srcdir}/$@.1 && \
#	  man2html ${srcdir}/$@.1 | tail -n +3 > ${srcdir}/../doc/$@.html

#Date: Sun, 13 Nov 2016 16:14:13 +0000
#From: Daniel Shahaf <danielsh@apache.org>
#Subject: Bug#844228: ebook-speaker: man page title is wrong if build directory
#        isn't named ${package}-${version}
#To: Debian Bug Tracking System <submit@bugs.debian.org>

	@which txt2man > /dev/null && txt2man \
	      -t 'eBook-speaker' -s 1 -v "Linux Reference Manual" \
	      -r "ebook-speaker-$(VERSION)" \
	      ${srcdir}/../doc/$@.txt > \
	      ${srcdir}/$@.1
	@file ${srcdir}/$@.1 && \
	      man2html ${srcdir}/$@.1 | tail -n +3 > ${srcdir}/../doc/$@.html

eBook-speaker.1:
