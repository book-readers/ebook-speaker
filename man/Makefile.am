man_MANS = eBook-speaker.1

all: eBook-speaker

eBook-speaker:
	@which txt2man > /dev/null && txt2man -p ${srcdir}/../doc/$@.txt > \
	       ${srcdir}/$@.1
	@file ${srcdir}/$@.1 && \
	  man2html ${srcdir}/$@.1 | tail -n +3 > ${srcdir}/../doc/$@.html

eBook-speaker.1:
