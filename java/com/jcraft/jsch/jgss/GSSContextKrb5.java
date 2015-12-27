/* -*-mode:java; c-basic-offset:2; indent-tabs-mode:nil -*- */
/*
Copyright (c) 2006-2015 ymnk, JCraft,Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in 
     the documentation and/or other materials provided with the distribution.

  3. The names of the authors may not be used to endorse or promote products
     derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES,
INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL JCRAFT,
INC. OR ANY CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package com.jcraft.jsch.jgss;

import com.jcraft.jsch.JSchException;

import java.net.InetAddress;
import java.net.UnknownHostException;
import org.ietf.jgss.GSSContext;
import org.ietf.jgss.GSSCredential;
import org.ietf.jgss.GSSException;
import org.ietf.jgss.GSSManager;
import org.ietf.jgss.GSSName;
import org.ietf.jgss.MessageProp;
import org.ietf.jgss.Oid;

public class GSSContextKrb5 implements com.jcraft.jsch.GSSContext{

  private static final String pUseSubjectCredsOnly = 
    "javax.security.auth.useSubjectCredsOnly";
  private static String useSubjectCredsOnly = 
    getSystemProperty(pUseSubjectCredsOnly);

  private GSSContext context=null;
  public void create(String user, String host) throws JSchException{
    try{
      // RFC 1964
      Oid krb5=new Oid("1.2.840.113554.1.2.2");
      // Kerberos Principal Name Form
      Oid principalName=new Oid("1.2.840.113554.1.2.2.1");

      GSSManager mgr=GSSManager.getInstance();

      GSSCredential crd=null;
      /*
      try{
        GSSName _user=mgr.createName(user, principalName);
        crd=mgr.createCredential(_user,
                                 GSSCredential.DEFAULT_LIFETIME,
                                 krb5,
                                 GSSCredential.INITIATE_ONLY);
      }
      catch(GSSException crdex){
      }
      */

      String cname=host;
      try{
        cname=InetAddress.getByName(cname).getCanonicalHostName();
      }
      catch(UnknownHostException e){
      }
      GSSName _host=mgr.createName("host/"+cname, principalName);

      context=mgr.createContext(_host,
                                krb5,
                                crd,
                                GSSContext.DEFAULT_LIFETIME);

      // RFC4462  3.4.  GSS-API Session
      //
      // When calling GSS_Init_sec_context(), the client MUST set
      // integ_req_flag to "true" to request that per-message integrity
      // protection be supported for this context.  In addition,
      // deleg_req_flag MAY be set to "true" to request access delegation, if
      // requested by the user.
      //
      // Since the user authentication process by its nature authenticates
      // only the client, the setting of mutual_req_flag is not needed for
      // this process.  This flag SHOULD be set to "false".

      // TODO: OpenSSH's sshd does accepts 'false' for mutual_req_flag
      //context.requestMutualAuth(false);
      context.requestMutualAuth(true);
      context.requestConf(true);
      context.requestInteg(true);             // for MIC
      context.requestCredDeleg(true);
      context.requestAnonymity(false);

      return;
    }
    catch(GSSException ex){
      throw new JSchException(ex.toString());
    }
  }

  public boolean isEstablished(){
    return context.isEstablished();
  }

  public byte[] init(byte[] token, int s, int l) throws JSchException {
    try{
      // Without setting "javax.security.auth.useSubjectCredsOnly" to "false",
      // Sun's JVM for Un*x will show messages to stderr in
      // processing context.initSecContext().
      // This hack is not thread safe ;-<.
      // If that property is explicitly given as "true" or "false",
      // this hack must not be invoked.
      if(useSubjectCredsOnly==null){
        setSystemProperty(pUseSubjectCredsOnly, "false");
      }
      return context.initSecContext(token, 0, l);
    }
    catch(GSSException ex){
      throw new JSchException(ex.toString());
    }
    catch(java.lang.SecurityException ex){
      throw new JSchException(ex.toString());
    }
    finally{
      if(useSubjectCredsOnly==null){
        // By the default, it must be "true".
        setSystemProperty(pUseSubjectCredsOnly, "true");
      }
    }
  }

  public byte[] getMIC(byte[] message, int s, int l){
    try{
      MessageProp prop =  new MessageProp(0, true);
      return context.getMIC(message, s, l, prop);
    }
    catch(GSSException ex){
      return null;
    }
  }

  public void dispose(){
    try{
      context.dispose();
    }
    catch(GSSException ex){
    }
  }

  private static String getSystemProperty(String key){
    try{ return System.getProperty(key); }
    catch(Exception e){ 
      // We are not allowed to get the System properties.
      return null; 
    } 
  }

  private static void setSystemProperty(String key, String value){
    try{ System.setProperty(key, value); }
    catch(Exception e){ 
      // We are not allowed to set the System properties.
    }
  }
}
