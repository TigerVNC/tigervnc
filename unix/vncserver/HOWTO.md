# Changes in recent TigerVNC versions
Previous TigerVNC versions had a wrapper script called `vncserver`. This
script could be run as a user manually to start *Xvnc* process. The
usage was quite simple as you just run:
```
$ vncserver :x [vncserver options] [Xvnc options]
```
and that was it. It worked fine for some cases, but far from all. There
were issues when users wanted to use it in combination with *systemd*.
Therefore, the implementation had to be changed to comply with *SELinux*
and *systemd* rules.

# How to start TigerVNC server
## Add a user mapping
With this you can map a user to a particular port. The mapping should be
done in `vncserver.users` configuration file. It should be pretty
straightforward. Once you open the file you will see there are some
examples, but basically the mapping is in form:
```
:x=user
```
For example you can have
```
:1=test
:2=vncuser
```

## Configure Xvnc options
To configure Xvnc parameters, you need to go to the same directory where
you did the user mapping and open `vncserver-config-defaults`
configuration file. This file is for the default Xvnc configuration and
will be applied to every user unless any of the following applies:
* The user has its own configuration in `$HOME/.vnc/config`.
* The same option with different value is configured in 
  `vncserver-config-mandatory` configuration file, which replaces the
  default configuration and has even a higher priority than the per-user
  configuration. This option is for system administrators when they want
  to force particular *Xvnc* options.

Format of the configuration file is also quite simple as the
configuration is in form of:
```
option=value
option
```
for example:
```
session=gnome
securitytypes=vncauth,tlsvnc
geometry=2000x1200
localhost
alwaysshared
```
See the following manpage for more details: Xvnc(1).

### Note:
It is recommended to set option specifying the session you want to
start. E.g. when you want to start GNOME desktop, then you have to use:
```
session=gnome
```
This should match the name of a session desktop file from
`/usr/share/xsessions` directory. If you don't specify the session,
TigerVNC will try to use the first one it finds, which may or may not
work correctly.

## Set VNC password
You need to set a password for each user in order to be able to start
the TigerVNC server. In order to create a password, you just run:
```
$ vncpasswd
```
You need to run it as the user who will run the server. 

### Note:
If you used TigerVNC before with your user and you already created a
password, then you have to make sure the `$HOME/.vnc` folder created by
`vncpasswd` have the correct *SELinux* context. You either can delete
this folder and recreate it again by creating the password one more
time, or alternatively you can run:
```
$ restorecon -RFv /home/<USER>/.vnc
```

## Start the TigerVNC server
Finally you can start the server using systemd service. To do so just
run:
```
$ systemctl start vncserver@:x
```
Run this as the root user or:
```
$ sudo systemctl start vncserver@:x
```
Run it as a regular user in case the user has permissions to run `sudo`.
Don't forget to replace the `:x` by the actual number you configured in
the user mapping file. For example:
```
$ systemctl start vncserver@:1
```
This starts a TigerVNC server for user `test` with GNOME session.

In case you want your server to be automatically started at boot, you
can run:
```
$ systemctl enable vncserver@:1
```

### Note:
If you previously used TigerVNC and you were used to start it by using
*systemd*, then you might need to remove previous *systemd*
configuration files placed in `/etc/systemd/system/vncserver@.service`,
in order to avoid them being prioritized by the new systemd service
files from latest TigerVNC.

# Limitations
You will not be able to start a TigerVNC server for a user who is
already logged into a graphical session. Avoid running the server as the
`root` user as it's not a safe thing to do. While running the server as
the `root` should work in general, it's not recommended to do so and
there might be some things which are not working properly.
