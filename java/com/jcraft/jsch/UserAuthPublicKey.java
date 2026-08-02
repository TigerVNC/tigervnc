/*
 * Copyright (c) 2002-2018 ymnk, JCraft,Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided with
 * the distribution.
 *
 * 3. The names of the authors may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL JCRAFT, INC. OR ANY CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.jcraft.jsch;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import java.util.Vector;

class UserAuthPublicKey extends UserAuth {

  @Override
  public boolean start(Session session) throws Exception {
    super.start(session);

    Vector<Identity> identities = session.getIdentityRepository().getIdentities();

    synchronized (identities) {
      if (identities.size() <= 0) {
        return false;
      }

      String pkmethodstr = session.getConfig("PubkeyAcceptedAlgorithms");
      if (session.getLogger().isEnabled(Logger.DEBUG)) {
        session.getLogger().log(Logger.DEBUG, "PubkeyAcceptedAlgorithms = " + pkmethodstr);
      }

      String[] not_available_pka = session.getUnavailableSignatures();
      List<String> not_available_pks = (not_available_pka != null && not_available_pka.length > 0
          ? Arrays.asList(not_available_pka)
          : Collections.emptyList());
      if (!not_available_pks.isEmpty()) {
        if (session.getLogger().isEnabled(Logger.DEBUG)) {
          session.getLogger().log(Logger.DEBUG,
              "Signature algorithms unavailable for non-agent identities = " + not_available_pks);
        }
      }

      List<String> pkmethods = Arrays.asList(Util.split(pkmethodstr, ","));
      if (pkmethods.isEmpty()) {
        return false;
      }

      String[] server_sig_algs = session.getServerSigAlgs();
      if (server_sig_algs != null && server_sig_algs.length > 0) {
        List<String> _known = new ArrayList<>();
        List<String> _unknown = new ArrayList<>();
        for (String pkmethod : pkmethods) {
          boolean add = false;
          for (String server_sig_alg : server_sig_algs) {
            // This cover the case of the public key is in Openssh certificate format.
            String pkRawMethod = OpenSshCertificateUtil.getRawKeyType(pkmethod);
            if (pkmethod.equals(server_sig_alg)
                || (pkRawMethod != null && pkRawMethod.equals(server_sig_alg))) {
              add = true;
              break;
            }
          }

          if (add) {
            _known.add(pkmethod);
          } else {
            _unknown.add(pkmethod);
          }
        }

        if (!_known.isEmpty()) {
          if (session.getLogger().isEnabled(Logger.DEBUG)) {
            session.getLogger().log(Logger.DEBUG,
                "PubkeyAcceptedAlgorithms in server-sig-algs = " + _known);
          }
        }

        if (!_unknown.isEmpty()) {
          if (session.getLogger().isEnabled(Logger.DEBUG)) {
            session.getLogger().log(Logger.DEBUG,
                "PubkeyAcceptedAlgorithms not in server-sig-algs = " + _unknown);
          }
        }

        if (!_known.isEmpty() && !_unknown.isEmpty()) {
          boolean success = _start(session, identities, _known, not_available_pks);
          if (success) {
            return true;
          }

          return _start(session, identities, _unknown, not_available_pks);
        }
      } else {
        if (session.getLogger().isEnabled(Logger.DEBUG)) {
          session.getLogger().log(Logger.DEBUG,
              "No server-sig-algs found, using PubkeyAcceptedAlgorithms = " + pkmethods);
        }
      }

      return _start(session, identities, pkmethods, not_available_pks);
    }
  }

  private boolean _start(Session session, List<Identity> identities, List<String> pkmethods,
      List<String> not_available_pks) throws Exception {
    if (session.auth_failures >= session.max_auth_tries) {
      return false;
    }

    boolean use_pk_auth_query = session.getConfig("enable_pubkey_auth_query").equals("yes");
    boolean try_other_pkmethods =
        session.getConfig("try_additional_pubkey_algorithms").equals("yes");

    List<String> rsamethods = new ArrayList<>();
    List<String> rsacertmethods = new ArrayList<>();
    List<String> nonrsamethods = new ArrayList<>();
    for (String pkmethod : pkmethods) {
      if (pkmethod.equals("ssh-rsa") || pkmethod.equals("rsa-sha2-256")
          || pkmethod.equals("rsa-sha2-512") || pkmethod.equals("ssh-rsa-sha224@ssh.com")
          || pkmethod.equals("ssh-rsa-sha256@ssh.com") || pkmethod.equals("ssh-rsa-sha384@ssh.com")
          || pkmethod.equals("ssh-rsa-sha512@ssh.com")) {
        rsamethods.add(pkmethod);
      } else if (pkmethod.equals("ssh-rsa-cert-v01@openssh.com")
          || pkmethod.equals("rsa-sha2-256-cert-v01@openssh.com")
          || pkmethod.equals("rsa-sha2-512-cert-v01@openssh.com")) {
        rsacertmethods.add(pkmethod);
      } else {
        nonrsamethods.add(pkmethod);
      }
    }

    byte[] _username = Util.str2byte(username);

    int command;

    iloop: for (Identity identity : identities) {

      if (session.auth_failures >= session.max_auth_tries) {
        return false;
      }

      // System.err.println("UserAuthPublicKey: identity.isEncrypted()="+identity.isEncrypted());
      decryptKey(session, identity);
      // System.err.println("UserAuthPublicKey: identity.isEncrypted()="+identity.isEncrypted());

      String _ipkmethod = identity.getAlgName();
      List<String> ipkmethods = null;
      if (_ipkmethod.equals("ssh-rsa")) {
        ipkmethods = new ArrayList<>(rsamethods);
      } else if (_ipkmethod.equals("ssh-rsa-cert-v01@openssh.com")) {
        ipkmethods = new ArrayList<>(rsacertmethods);
      } else if (nonrsamethods.contains(_ipkmethod)) {
        ipkmethods = new ArrayList<>(1);
        ipkmethods.add(_ipkmethod);
      }
      if (ipkmethods == null || ipkmethods.isEmpty()) {
        if (session.getLogger().isEnabled(Logger.DEBUG)) {
          session.getLogger().log(Logger.DEBUG,
              _ipkmethod + " cannot be used as public key type for identity " + identity.getName());
        }
        continue iloop;
      }

      ipkmethodloop: while (!ipkmethods.isEmpty()
          && session.auth_failures < session.max_auth_tries) {
        byte[] pubkeyblob = identity.getPublicKeyBlob();
        List<String> pkmethodsuccesses = null;

        if (pubkeyblob != null && use_pk_auth_query) {
          command = SSH_MSG_USERAUTH_FAILURE;
          Iterator<String> it = ipkmethods.iterator();
          loop3: while (it.hasNext()) {
            String ipkmethod = it.next();
            it.remove();
            // Map certificate type to base algorithm for availability check
            if (isAlgorithmUnavailable(ipkmethod, not_available_pks, identity, session)) {
              continue loop3;
            }

            // send
            // byte SSH_MSG_USERAUTH_REQUEST(50)
            // string user name
            // string service name ("ssh-connection")
            // string "publickey"
            // boolen FALSE
            // string public key algorithm name
            // string public key blob
            packet.reset();
            buf.putByte((byte) SSH_MSG_USERAUTH_REQUEST);
            buf.putString(_username);
            buf.putString(Util.str2byte("ssh-connection"));
            buf.putString(Util.str2byte("publickey"));
            buf.putByte((byte) 0);
            buf.putString(Util.str2byte(ipkmethod));
            buf.putString(pubkeyblob);
            session.write(packet);

            loop1: while (true) {
              buf = session.read(buf);
              command = buf.getCommand() & 0xff;

              if (command == SSH_MSG_USERAUTH_PK_OK) {
                if (session.getLogger().isEnabled(Logger.DEBUG)) {
                  session.getLogger().log(Logger.DEBUG, ipkmethod + " preauth success");
                }
                pkmethodsuccesses = new ArrayList<>(1);
                pkmethodsuccesses.add(ipkmethod);
                break loop3;
              } else if (command == SSH_MSG_USERAUTH_FAILURE) {
                if (session.getLogger().isEnabled(Logger.DEBUG)) {
                  session.getLogger().log(Logger.DEBUG, ipkmethod + " preauth failure");
                }
                continue loop3;
              } else if (command == SSH_MSG_USERAUTH_BANNER) {
                buf.getInt();
                buf.getByte();
                buf.getByte();
                byte[] _message = buf.getString();
                byte[] lang = buf.getString();
                String message = Util.byte2str(_message);
                if (userinfo != null) {
                  userinfo.showMessage(message);
                }
                continue loop1;
              } else {
                // System.err.println("USERAUTH fail ("+command+")");
                // throw new JSchException("USERAUTH fail ("+command+")");
                if (session.getLogger().isEnabled(Logger.DEBUG)) {
                  session.getLogger().log(Logger.DEBUG,
                      ipkmethod + " preauth failure command (" + command + ")");
                }
                continue loop3;
              }
            }
          }

          if (command != SSH_MSG_USERAUTH_PK_OK) {
            continue iloop;
          }
        }

        if (identity.isEncrypted())
          continue iloop;
        if (pubkeyblob == null)
          pubkeyblob = identity.getPublicKeyBlob();

        // System.err.println("UserAuthPublicKey: pubkeyblob="+pubkeyblob);

        if (pubkeyblob == null)
          continue iloop;
        if (pkmethodsuccesses == null)
          pkmethodsuccesses = ipkmethods;
        if (pkmethodsuccesses.isEmpty())
          continue iloop;

        Iterator<String> it = pkmethodsuccesses.iterator();
        loop4: while (it.hasNext() && session.auth_failures < session.max_auth_tries) {
          String pkmethodsuccess = it.next();
          it.remove();
          // Map certificate type to base algorithm for availability check
          if (isAlgorithmUnavailable(pkmethodsuccess, not_available_pks, identity, session)) {
            continue loop4;
          }

          // send
          // byte SSH_MSG_USERAUTH_REQUEST(50)
          // string user name
          // string service name ("ssh-connection")
          // string "publickey"
          // boolen TRUE
          // string public key algorithm name
          // string public key blob
          // string signature
          packet.reset();
          buf.putByte((byte) SSH_MSG_USERAUTH_REQUEST);
          buf.putString(_username);
          buf.putString(Util.str2byte("ssh-connection"));
          buf.putString(Util.str2byte("publickey"));
          buf.putByte((byte) 1);
          buf.putString(Util.str2byte(pkmethodsuccess));
          buf.putString(pubkeyblob);

          // byte[] tmp=new byte[buf.index-5];
          // System.arraycopy(buf.buffer, 5, tmp, 0, tmp.length);
          // buf.putString(signature);

          byte[] sid = session.getSessionId();
          int sidlen = sid.length;
          byte[] tmp = new byte[4 + sidlen + buf.index - 5];
          tmp[0] = (byte) (sidlen >>> 24);
          tmp[1] = (byte) (sidlen >>> 16);
          tmp[2] = (byte) (sidlen >>> 8);
          tmp[3] = (byte) (sidlen);
          System.arraycopy(sid, 0, tmp, 4, sidlen);
          System.arraycopy(buf.buffer, 5, tmp, 4 + sidlen, buf.index - 5);
          byte[] signature = identity.getSignature(tmp, pkmethodsuccess);
          if (signature == null) { // for example, too long key length.
            if (session.getLogger().isEnabled(Logger.DEBUG)) {
              session.getLogger().log(Logger.DEBUG, pkmethodsuccess + " signature failure");
            }
            continue loop4;
          }
          buf.putString(signature);
          session.write(packet);

          loop2: while (true) {
            buf = session.read(buf);
            command = buf.getCommand() & 0xff;

            if (command == SSH_MSG_USERAUTH_SUCCESS) {
              if (session.getLogger().isEnabled(Logger.DEBUG)) {
                session.getLogger().log(Logger.DEBUG, pkmethodsuccess + " auth success");
              }
              return true;
            } else if (command == SSH_MSG_USERAUTH_BANNER) {
              buf.getInt();
              buf.getByte();
              buf.getByte();
              byte[] _message = buf.getString();
              byte[] lang = buf.getString();
              String message = Util.byte2str(_message);
              if (userinfo != null) {
                userinfo.showMessage(message);
              }
              continue loop2;
            } else if (command == SSH_MSG_USERAUTH_FAILURE) {
              buf.getInt();
              buf.getByte();
              buf.getByte();
              byte[] foo = buf.getString();
              int partial_success = buf.getByte();
              // System.err.println(new String(foo)+
              // " partial_success:"+(partial_success!=0));
              if (partial_success != 0) {
                throw new JSchPartialAuthException(Util.byte2str(foo));
              }
              session.auth_failures++;
              if (session.getLogger().isEnabled(Logger.DEBUG)) {
                session.getLogger().log(Logger.DEBUG, pkmethodsuccess + " auth failure");
              }

              if (session.auth_failures >= session.max_auth_tries) {
                return false;
              } else if (try_other_pkmethods) {
                break loop2;
              } else {
                continue iloop;
              }
            }
            // System.err.println("USERAUTH fail ("+command+")");
            // throw new JSchException("USERAUTH fail ("+command+")");
            if (session.getLogger().isEnabled(Logger.DEBUG)) {
              session.getLogger().log(Logger.DEBUG,
                  pkmethodsuccess + " auth failure command (" + command + ")");
            }
            break loop2;
          }
        }
      }
    }
    return false;
  }

  /**
   * Checks if a public key algorithm is unavailable for a non-agent identity.
   * <p>
   * For certificate key types, this checks the availability of the base algorithm.
   * </p>
   *
   * @param pkmethod the public key method/algorithm to check
   * @param not_available_pks list of unavailable algorithms
   * @param identity the identity being used
   * @param session the current session (for logging)
   * @return true if the algorithm is unavailable and should be skipped, false otherwise
   */
  private static boolean isAlgorithmUnavailable(String pkmethod, List<String> not_available_pks,
      Identity identity, Session session) {
    String baseAlgorithm = OpenSshCertificateKeyTypes.getBaseKeyType(pkmethod);
    if (not_available_pks.contains(baseAlgorithm) && !(identity instanceof AgentIdentity)) {
      if (session.getLogger().isEnabled(Logger.DEBUG)) {
        session.getLogger().log(Logger.DEBUG,
            pkmethod + " not available for identity " + identity.getName());
      }
      return true;
    }
    return false;
  }

  private void decryptKey(Session session, Identity identity) throws JSchException {
    byte[] passphrase = null;
    int count = 5;
    while (true) {
      if ((identity.isEncrypted() && passphrase == null)) {
        if (userinfo == null)
          throw new JSchException("USERAUTH fail");
        if (identity.isEncrypted()
            && !userinfo.promptPassphrase("Passphrase for " + identity.getName())) {
          throw new JSchAuthCancelException("publickey");
          // throw new JSchException("USERAUTH cancel");
          // break;
        }
        String _passphrase = userinfo.getPassphrase();
        if (_passphrase != null) {
          passphrase = Util.str2byte(_passphrase);
        }
      }

      if (!identity.isEncrypted() || passphrase != null) {
        if (identity.setPassphrase(passphrase)) {
          if (passphrase != null
              && (session.getIdentityRepository() instanceof IdentityRepositoryWrapper)) {
            ((IdentityRepositoryWrapper) session.getIdentityRepository()).check();
          }
          break;
        }
      }
      Util.bzero(passphrase);
      passphrase = null;
      count--;
      if (count == 0)
        break;
    }

    Util.bzero(passphrase);
    passphrase = null;
  }
}
