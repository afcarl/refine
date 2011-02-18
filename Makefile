
# WARNING this is a developer makefile, not generated by Autotools.

check : FORCE
	( cd `uname` && $(MAKE) check )

install : FORCE
	if [ -d `uname` ]; then ( cd `uname` && $(MAKE) install ) ; fi
	if [ -d `uname`64 ]; then ( cd `uname`64 && $(MAKE) install ) ; fi

clean : FORCE
	( cd src && $(MAKE) clean )
	( cd `uname` && $(MAKE) clean )

bu : FORCE
	echo `find . -name '*~'`
	rm -f `find . -name '*~'`

FORCE:


