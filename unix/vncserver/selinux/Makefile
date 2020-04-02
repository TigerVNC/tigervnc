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

all: vncsession.pp

%.pp: %.te
	make -f $(DATADIR)/selinux/devel/Makefile $@

clean:
	rm -f *.pp
	rm -rf tmp

install: vncsession.pp
	mkdir -p $(DESTDIR)$(DATADIR)/selinux/packages
	install vncsession.pp $(DESTDIR)$(DATADIR)/selinux/packages/vncsession.pp
