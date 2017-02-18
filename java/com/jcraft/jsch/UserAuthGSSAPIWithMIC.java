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
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION)HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT(INCLUDING
NEGLIGENCE OR OTHERWISE)ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

package com.jcraft.jsch;

public class UserAuthGSSAPIWithMIC extends UserAuth {
  private static final int SSH_MSG_USERAUTH_GSSAPI_RESPONSE=         60;
  private static final int SSH_MSG_USERAUTH_GSSAPI_TOKEN=            61;
  private static final int SSH_MSG_USERAUTH_GSSAPI_EXCHANGE_COMPLETE=63;
  private static final int SSH_MSG_USERAUTH_GSSAPI_ERROR=            64;
  private static final int SSH_MSG_USERAUTH_GSSAPI_ERRTOK=           65;
  private static final int SSH_MSG_USERAUTH_GSSAPI_MIC=              66;

  private static final byte[][] supported_oid={
    // OID 1.2.840.113554.1.2.2 in DER
    {(byte)0x6,(byte)0x9,(byte)0x2a,(byte)0x86,(byte)0x48,
     (byte)0x86,(byte)0xf7,(byte)0x12,(byte)0x1,(byte)0x2,
     (byte)0x2}
  };

  private static final String[] supported_method={
    "gssapi-with-mic.krb5"
  };

  public boolean start(Session session)throws Exception{
    super.start(session);

    byte[] _username=Util.str2byte(username);

    packet.reset();

    // byte            SSH_MSG_USERAUTH_REQUEST(50)
    // string          user name(in ISO-10646 UTF-8 encoding)
    // string          service name(in US-ASCII)
    // string          "gssapi"(US-ASCII)
    // uint32          n, the number of OIDs client supports
    // string[n]       mechanism OIDS
    buf.putByte((byte)SSH_MSG_USERAUTH_REQUEST);
    buf.putString(_username);
    buf.putString(Util.str2byte("ssh-connection"));
    buf.putString(Util.str2byte("gssapi-with-mic"));
    buf.putInt(supported_oid.length);
    for(int i=0; i<supported_oid.length; i++){
      buf.putString(supported_oid[i]);
    }
    session.write(packet);

    String method=null;
    int command;
    while(true){
      buf=session.read(buf);
      command=buf.getCommand()&0xff;

      if(command==SSH_MSG_USERAUTH_FAILURE){
        return false;
      }
      
      if(command==SSH_MSG_USERAUTH_GSSAPI_RESPONSE){
        buf.getInt(); buf.getByte(); buf.getByte();
        byte[] message=buf.getString();

        for(int i=0; i<supported_oid.length; i++){
          if(Util.array_equals(message, supported_oid[i])){
            method=supported_method[i];
            break;
          }
        }

        if(method==null){
          return false;
        }

        break; // success
      }

      if(command==SSH_MSG_USERAUTH_BANNER){
        buf.getInt(); buf.getByte(); buf.getByte();
        byte[] _message=buf.getString();
        byte[] lang=buf.getString();
        String message=Util.byte2str(_message);
        if(userinfo!=null){
          userinfo.showMessage(message);
        }
        continue;
      }
      return false;
    }

    GSSContext context=null;
    try{
      Class c=Class.forName(session.getConfig(method));
      context=(GSSContext)(c.newInstance());
    }
    catch(Exception e){ 
      return false;
    }

    try{
      context.create(username, session.host);
    }
    catch(JSchException e){
      return false;
    }

    byte[] token=new byte[0];

    while(!context.isEstablished()){
      try{
        token=context.init(token, 0, token.length);
      }
      catch(JSchException e){
        // TODO
        // ERRTOK should be sent?
        // byte        SSH_MSG_USERAUTH_GSSAPI_ERRTOK
        // string      error token
        return false;
      }

      if(token!=null){
        packet.reset();
        buf.putByte((byte)SSH_MSG_USERAUTH_GSSAPI_TOKEN);
        buf.putString(token);
        session.write(packet);
      }

      if(!context.isEstablished()){
        buf=session.read(buf);
        command=buf.getCommand()&0xff;
        if(command==SSH_MSG_USERAUTH_GSSAPI_ERROR){
          // uint32    major_status
          // uint32    minor_status
          // string    message
          // string    language tag

          buf=session.read(buf);
          command=buf.getCommand()&0xff;
          //return false;
        }
        else if(command==SSH_MSG_USERAUTH_GSSAPI_ERRTOK){
          // string error token

          buf=session.read(buf);
          command=buf.getCommand()&0xff;
          //return false;
        }

        if(command==SSH_MSG_USERAUTH_FAILURE){
          return false;
        }

        buf.getInt(); buf.getByte(); buf.getByte();
        token=buf.getString();
      }
    }

    Buffer mbuf=new Buffer();
    // string    session identifier
    // byte      SSH_MSG_USERAUTH_REQUEST
    // string    user name
    // string    service
    // string    "gssapi-with-mic"
    mbuf.putString(session.getSessionId());
    mbuf.putByte((byte)SSH_MSG_USERAUTH_REQUEST);
    mbuf.putString(_username);
    mbuf.putString(Util.str2byte("ssh-connection"));
    mbuf.putString(Util.str2byte("gssapi-with-mic"));

    byte[] mic=context.getMIC(mbuf.buffer, 0, mbuf.getLength());

    if(mic==null){
      return false;
    }

    packet.reset();
    buf.putByte((byte)SSH_MSG_USERAUTH_GSSAPI_MIC);
    buf.putString(mic);
    session.write(packet);

    context.dispose();

    buf=session.read(buf);
    command=buf.getCommand()&0xff;

    if(command==SSH_MSG_USERAUTH_SUCCESS){
      return true;
    }
    else if(command==SSH_MSG_USERAUTH_FAILURE){
      buf.getInt(); buf.getByte(); buf.getByte(); 
      byte[] foo=buf.getString();
      int partial_success=buf.getByte();
      //System.err.println(new String(foo)+
      //		 " partial_success:"+(partial_success!=0));
      if(partial_success!=0){
        throw new JSchPartialAuthException(Util.byte2str(foo));
      }
    }
    return false;
  }
}


