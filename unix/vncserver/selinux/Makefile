# SELinux module for TigerVNC's vncsession
#
# This will install the policy module, but not load it. To apply
# it you should also run:
#
#     sudo semodule -i /usr/share/selinux/packages/vncsession.pp
#     sudo restorecon /usr/sbin/vncsession /usr/libexec/vncsession-start
#

PREFIX=/usr
DATADIR=$(PREFIX)/share

all: vncsession.pp.bz2

%.pp.bz2: %.pp
	bzip2 -9 $^

%.pp: %.te
	make -f $(DATADIR)/selinux/devel/Makefile $@

clean:
	rm -f *.pp *.pp.bz2
	rm -rf tmp

install: vncsession.pp.bz2
	mkdir -p $(DESTDIR)$(DATADIR)/selinux/packages/targeted/
	install vncsession.pp.bz2 $(DESTDIR)$(DATADIR)/selinux/packages/targeted/vncsession.pp.bz2
