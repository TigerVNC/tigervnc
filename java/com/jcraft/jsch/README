
                                      JSch

                                Java Secure Channel
                         by ymnk@jcraft.com, JCraft,Inc.

                            http://www.jcraft.com/jsch/

Last modified: Thu Mar 18 13:58:16 UTC 2015


Description
===========
JSch is a pure Java implementation of SSH2.  JSch allows you to 
connect to an sshd server and use port forwarding, X11 forwarding, 
file transfer, etc., and you can integrate its functionality
into your own Java programs. JSch is licensed under BSD style license.


Documentation
=============
* README files all over the source tree have info related to the stuff
  in the directories. 
* ChangeLog: what changed from the previous version?


Directories & Files in the Source Tree
======================================
* src/com/ has source trees of JSch
* example/ has some samples, which demonstrate the usages.
* tools/ has scripts for Ant.


Why JSch?
==========
Our intension in developing this stuff is to enable users of our pure
java X servers, WiredX(http://wiredx.net/) and WeirdX, to enjoy secure X
sessions.  Our efforts have mostly targeted the SSH2 protocol in relation
to X Window System and X11 forwarding.  Of course, we are also interested in 
adding other functionality - port forward, file transfer, terminal emulation, etc.


Features
========
* JSch is in pure Java, but it depends on JavaTM Cryptography
  Extension (JCE).  JSch is know to work with:
  o J2SE 1.4.0 or later (no additional libraries required).
  o J2SE 1.3 and Sun's JCE reference implementation that can be
    obtained at http://java.sun.com/products/jce/
  o J2SE 1.2.2 and later and Bouncycastle's JCE implementation that
    can be obtained at http://www.bouncycastle.org/
* SSH2 protocol support.
* Key exchange: diffie-hellman-group-exchange-sha1,
                diffie-hellman-group1-sha1,
                diffie-hellman-group14-sha1,
                diffie-hellman-group-exchange-sha256,
                ecdh-sha2-nistp256,
                ecdh-sha2-nistp384,
                ecdh-sha2-nistp521
* Cipher: blowfish-cbc,3des-cbc,aes128-cbc,aes192-cbc,aes256-cbc
          3des-ctr,aes128-ctr,aes192-ctr,aes256-ctc,
          arcfour,arcfour128,arcfour256
* MAC: hmac-md5,hmac-md5-96,hmac-sha1,hmac-sha1-96
* Host key type: ssh-dss,ssh-rsa,
                 ecdsa-sha2-nistp256,ecdsa-sha2-nistp384,ecdsa-sha2-nistp521
* Userauth: password
* Userauth: publickey(DSA,RSA,ECDSA)
* Userauth: keyboard-interactive
* Userauth: gssapi-with-mic
* X11 forwarding.
* xauth spoofing.
* connection through HTTP proxy.
* connection through SOCKS5, SOCKS4 proxy.
* port forwarding.
* stream forwarding.
* signal sending.
  The unofficial patch for sshd of openssh will be find in the thread
  http://marc.theaimsgroup.com/?l=openssh-unix-dev&m=104295745607575&w=2
* envrironment variable passing.
* remote exec.
* generating DSA and RSA key pairs.
* supporting private keys in OpenSSL(traditional SSLeay) and PKCS#8 format.
* SSH File Transfer Protocol(version 0, 1, 2, 3)
* partial authentication
* packet compression: zlib, zlib@openssh.com
  JZlib(http://www.jcraft.com/jzlib/) has been used.
* hashed known_hosts file.
* NONE Cipher switching.
  http://www.psc.edu/networking/projects/hpn-ssh/none.php
* JSch is licensed under BSD style license(refer to LICENSE.txt).


How To Try
==========
This archive does not include java byte code, so please compile
the source code by your self.
  $ cd jsch-?.?.?/src
  $ javac com/jcraft/jsch/*java com/jcraft/jsch/jce/*java com/jcraft/jzlib/*.java
'/examples/' directory has included some samples to demonstrate what 
JSch can do.  Please refer to '/examples/README' file.


AES cipher
==========
JSch supports aes128-cbc,aes192-cbc,aes256-cbc,aes128-ctr,aes192-ctr,
aes256-ctr but you require AES support in your J2SE to choose some of them.  
If you are using Sun's J2SE, J2SE 1.4.2 or later is required.  
And then, J2SE 1.4.2(or later) does not support aes256 by the default, 
because of 'import control restrictions of some countries'.
We have confirmed that by applying
  "Java Cryptography Extension (JCE)
  Unlimited Strength Jurisdiction Policy Files 1.4.2"
on
  http://java.sun.com/j2se/1.4.2/download.html#docs
we can enjoy 'aes256-cbc,aes256-ctr'.


Stream Forwarding
=================
JSch has a unique functionality, Stream Forwarding.
Stream Forwarding allows you to plug Java I/O streams directly into a remote TCP
port without assigning and opening a local TCP port.
In port forwarding, as with the -L option of ssh command, you have to assign
and open a local TCP port and that port is also accessible by crackers
on localhost.  In some case, that local TCP port may be plugged to a
secret port via SSH session.
A sample program, /example/StreamForwarding.java , demonstrates
this functionality.


Generating Authentication Keys
==============================
JSch allows you to generate DSA and RSA key pairs, which are in OpenSSH format.
Please refer to 'examples/KeyGen.java'.


Packet Compression
==================
According to the draft from IETF sesch working group, the packet
compression can be applied to each data stream directions; from sshd
server to ssh client and from ssh client to sshd server.  So, jsch
allows you to choose which data stream direction will be compressed or not.
For example, in X11 forwarding session, the packet compression for data
stream from sshd to ssh client will save the network traffic, but
usually the traffic from ssh client to sshd is light, so by omitting
the compression for this direction, you may be able to save some CPU time.
Please refer to a sample program 'examples/Compression.java'.


Property
========
By setting properties, you can control the behavior of jsch.
Here is an example of enabling the packet compression,

      Session session=jsch.getSession(user, host, 22);
      java.util.Properties config=new java.util.Properties();
      config.put("compression.s2c", "zlib,none");
      config.put("compression.c2s", "zlib,none");
      session.setConfig(config);
      session.connect();

Current release has supported following properties,
* compression.s2c: zlib, none
  default: none
  Specifies whether to use compression for the data stream
  from sshd to jsch.  If "zlib,none" is given and the remote sshd does
  not allow the packet compression, compression will not be done.
* compression.c2s: zlib, none
  default: none
  Specifies whether to use compression for the data stream
  from jsch to sshd.
* StrictHostKeyChecking: ask | yes | no
  default: ask
  If this property is set to ``yes'', jsch will never automatically add
  host keys to the $HOME/.ssh/known_hosts file, and refuses to connect
  to hosts whose host key has changed.  This property forces the user
  to manually add all new hosts.  If this property is set to ``no'', 
  jsch will automatically add new host keys to the user known hosts
  files.  If this property is set to ``ask'', new  host keys will be
  added to the user known host files only after the user has confirmed 
  that is what they really want to do, and jsch will refuse to connect 
  to hosts whose host key has changed.


TODO
====
* re-implementation with java.nio.
* replacing cipher, hash by JCE with pure Java code.
* SSH File Transfer Protocol version 4.
* error handling.


Copyrights & Disclaimers
========================
JSch is copyrighted by ymnk, JCraft,Inc. and is licensed through BSD style license.
Read the LICENSE.txt file for the complete license.


Credits and Acknowledgments
============================
JSch has been developed by ymnk@jcraft.com and it can not be hacked
without several help.
* First of all, we want to thank JCE team at Sun Microsystems.
  For long time, we had planed to implement SSH2 in pure Java,
  but we had hesitated to do because tons of work must be done for
  implementing ciphers, hashes, etc., from the scratch.
  Thanks to newly added functionalities to J2SE 1.4.0, we could
  start this project.
* We appreciate the OpenSSH project.
  The options '-ddd' of sshd, '---vvv' of ssh and the compile options 
  '-DPACKET_DEBUG', '-DDEBUG_KEXDH' and  '-DDEBUG_KEX' were very
  useful in debugging JSch.
* We appreciate IETF sesch working group and SSH Communications Security Corp.
  Without the standardization of the protocol, we could not get the
  chance to implement JSch.
* We appreciate Seigo Haruyama(http://www.unixuser.org/~haruyama/),
  who are interpreting drafts of SSH2 protocol in Japanese.
  His works were very useful for us to understand the technical terms
  in our native language.
* We also appreciate SourceForge.net's awesome service to the 
  Open Source Community.


If you have any comments, suggestions and questions, write us 
at jsch@jcraft.com


``SSH is a registered trademark and Secure Shell is a trademark of
SSH Communications Security Corp (www.ssh.com)''.
