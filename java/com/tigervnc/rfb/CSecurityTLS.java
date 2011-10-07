/*
 * Copyright (C) 2003 Sun Microsystems, Inc.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

package com.tigervnc.rfb;

import javax.net.ssl.*;
import java.security.*;
import java.security.cert.*;
import java.security.KeyStore;
import java.io.File;
import java.io.InputStream;
import java.io.FileInputStream;
import java.util.ArrayList;
import java.util.Collection;
import javax.swing.JOptionPane;

import com.tigervnc.vncviewer.UserPrefs;
import com.tigervnc.rdr.*;

public class CSecurityTLS extends CSecurity {

  public static StringParameter x509ca
  = new StringParameter("x509ca",
                        "X509 CA certificate", "");
  public static StringParameter x509crl
  = new StringParameter("x509crl",
                        "X509 CRL file", "");

  private void initGlobal() 
  {
    try {
      SSLSocketFactory sslfactory;
      SSLContext ctx = SSLContext.getInstance("TLS");
      if (anon) {
        ctx.init(null, null, null);
      } else {
        TrustManager[] myTM = new TrustManager[] { 
          new MyX509TrustManager() 
        };
        ctx.init (null, myTM, null);
      }
      sslfactory = ctx.getSocketFactory();
      try {
        ssl = (SSLSocket)sslfactory.createSocket(cc.sock,
						  cc.sock.getInetAddress().getHostName(),
						  cc.sock.getPort(), true);
      } catch (java.io.IOException e) { 
        throw new Exception(e.toString());
      }

      if (anon) {
        String[] supported;
        ArrayList<String> enabled = new ArrayList<String>();

        supported = ssl.getSupportedCipherSuites();

        for (int i = 0; i < supported.length; i++)
          if (supported[i].matches("TLS_DH_anon.*"))
	          enabled.add(supported[i]);

        ssl.setEnabledCipherSuites(enabled.toArray(new String[0]));
      } else {
        ssl.setEnabledCipherSuites(ssl.getSupportedCipherSuites());
      }

      ssl.setEnabledProtocols(new String[]{"SSLv3","TLSv1"});
      ssl.addHandshakeCompletedListener(new MyHandshakeListener());
    }
    catch (java.security.GeneralSecurityException e)
    {
      vlog.error ("TLS handshake failed " + e.toString ());
      return;
    }
  }

  public CSecurityTLS(boolean _anon) 
  {
    anon = _anon;
    setDefaults();
    cafile = x509ca.getData(); 
    crlfile = x509crl.getData(); 
  }

  public static void setDefaults()
  {
    String homeDir = null;
    
    if ((homeDir=UserPrefs.getHomeDir()) == null) {
      vlog.error("Could not obtain VNC home directory path");
      return;
    }

    String vnchomedir = homeDir+UserPrefs.getFileSeparator()+".vnc"+
                        UserPrefs.getFileSeparator();
    String caDefault = new String(vnchomedir+"x509_ca.pem");
    String crlDefault = new String(vnchomedir+"x509_crl.pem");

    if (new File(caDefault).exists())
      x509ca.setDefaultStr(caDefault);
    if (new File(crlDefault).exists())
      x509crl.setDefaultStr(crlDefault);
  }

  public boolean processMsg(CConnection cc) {
    is = cc.getInStream();
    os = cc.getOutStream();

    initGlobal();

    if (!is.checkNoWait(1))
      return false;

    if (is.readU8() == 0) {
      int result = is.readU32();
      String reason;
      if (result == Security.secResultFailed ||
          result == Security.secResultTooMany)
        reason = is.readString();
      else
        reason = new String("Authentication failure (protocol error)");
      throw new AuthFailureException(reason);
    }

    // SSLSocket.getSession blocks until the handshake is complete
    session = ssl.getSession();
    if (!session.isValid())
      throw new Exception("TLS Handshake failed!");

    try {
      cc.setStreams(new JavaInStream(ssl.getInputStream()),
		                new JavaOutStream(ssl.getOutputStream()));
    } catch (java.io.IOException e) { 
      throw new Exception("Failed to set streams");
    }

    return true;
  }

  class MyHandshakeListener implements HandshakeCompletedListener {
   public void handshakeCompleted(HandshakeCompletedEvent e) {
     vlog.info("Handshake succesful!");
     vlog.info("Using cipher suite: " + e.getCipherSuite());
   }
  }

  class MyX509TrustManager implements X509TrustManager
  {

    X509TrustManager tm;

    MyX509TrustManager() throws java.security.GeneralSecurityException
    {
      TrustManagerFactory tmf =
        TrustManagerFactory.getInstance("PKIX");
      KeyStore ks = KeyStore.getInstance("JKS");
      CertificateFactory cf = CertificateFactory.getInstance("X.509");
      try {
        ks.load(null, null);
        File cacert = new File(cafile);
        if (!cacert.exists() || !cacert.canRead())
          return;
        InputStream caStream = new FileInputStream(cafile);
        X509Certificate ca = (X509Certificate)cf.generateCertificate(caStream);
        ks.setCertificateEntry("CA", ca);
        PKIXBuilderParameters params = new PKIXBuilderParameters(ks, new X509CertSelector());
        File crlcert = new File(crlfile);
        if (!crlcert.exists() || !crlcert.canRead()) {
          params.setRevocationEnabled(false);
        } else {
          InputStream crlStream = new FileInputStream(crlfile);
          Collection<? extends CRL> crls = cf.generateCRLs(crlStream);
          CertStoreParameters csp = new CollectionCertStoreParameters(crls);
          CertStore store = CertStore.getInstance("Collection", csp);
          params.addCertStore(store);
          params.setRevocationEnabled(true);
        }
        tmf.init(new CertPathTrustManagerParameters(params));
      } catch (java.io.FileNotFoundException e) { 
        vlog.error(e.toString());
      } catch (java.io.IOException e) {
        vlog.error(e.toString());
      }
      tm = (X509TrustManager)tmf.getTrustManagers()[0];
    }

    public void checkClientTrusted(X509Certificate[] chain, String authType) 
      throws CertificateException
    {
      tm.checkClientTrusted(chain, authType);
    }

    public void checkServerTrusted(X509Certificate[] chain, String authType)
      throws CertificateException
    {
      try {
	      tm.checkServerTrusted(chain, authType);
      } catch (CertificateException e) {
        Object[] answer = {"Proceed", "Exit"};
        int ret = JOptionPane.showOptionDialog(null,
          e.getCause().getLocalizedMessage()+"\n"+
          "Continue connecting to this host?",
          "Confirm certificate exception?",
          JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE,
          null, answer, answer[0]);
        if (ret == JOptionPane.NO_OPTION)
          System.exit(1);
      } catch (java.lang.Exception e) {
        throw new Exception(e.toString());
      }
    }

    public X509Certificate[] getAcceptedIssuers ()
    {
      return tm.getAcceptedIssuers();
    }
  }

  public final int getType() { return anon ? Security.secTypeTLSNone : Security.secTypeX509None; }
  public final String description() 
    { return anon ? "TLS Encryption without VncAuth" : "X509 Encryption without VncAuth"; }


  //protected void setParam();
  //protected void checkSession();
  protected CConnection cc;

  private boolean anon;
  private SSLSession session;
  private String cafile, crlfile;
  private InStream is;
  private OutStream os;
  private SSLSocket ssl;

  static LogWriter vlog = new LogWriter("CSecurityTLS");
}
