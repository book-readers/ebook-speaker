man_MANS = eBook-speaker.1

all: eBook-speaker

eBook-speaker:
	@which txt2man && txt2man -p ${srcdir}/../doc/$@.txt > ${srcdir}/$@.1
	@file ${srcdir}/$@.1 && which man2html && \
	  man2html ${srcdir}/$@.1.1 | tail -n +3 > ${srcdir}/../doc/$@.html

eBook-speaker.1:
