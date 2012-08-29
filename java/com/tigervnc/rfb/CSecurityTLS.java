/*
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 m-privacy GmbH
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2011-2012 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
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

import com.tigervnc.rdr.*;
import com.tigervnc.network.*;
import com.tigervnc.vncviewer.*;

public class CSecurityTLS extends CSecurity {

  public static StringParameter x509ca
  = new StringParameter("x509ca",
                        "X509 CA certificate", "", Configuration.ConfigurationObject.ConfViewer);
  public static StringParameter x509crl
  = new StringParameter("x509crl",
                        "X509 CRL file", "", Configuration.ConfigurationObject.ConfViewer);

  private void initGlobal() 
  {
    boolean globalInitDone = false;

    if (!globalInitDone) {
      try {
        ctx = SSLContext.getInstance("TLS");
      } catch(NoSuchAlgorithmException e) {
        throw new Exception(e.toString());
      }

      globalInitDone = true;
    }
  }

  public CSecurityTLS(boolean _anon) 
  {
    anon = _anon;
    session = null;
    
    setDefaults();
    cafile = x509ca.getData(); 
    crlfile = x509crl.getData(); 
  }

  public static String getDefaultCA() {
    if (UserPreferences.get("viewer", "x509ca") != null)
      return UserPreferences.get("viewer", "x509ca");
    return FileUtils.getVncHomeDir()+"x509_ca.pem";
  }

  public static String getDefaultCRL() {
    if (UserPreferences.get("viewer", "x509crl") != null)
      return UserPreferences.get("viewer", "x509crl");
    return FileUtils.getVncHomeDir()+"x509_crl.pem";
  }

  public static void setDefaults()
  {
    if (new File(getDefaultCA()).exists())
      x509ca.setDefaultStr(getDefaultCA());
    if (new File(getDefaultCRL()).exists())
      x509crl.setDefaultStr(getDefaultCRL());
  }

// FIXME:
// Need to shutdown the connection cleanly

// FIXME?
// add a finalizer method that calls shutdown

  public boolean processMsg(CConnection cc) {
    is = (FdInStream)cc.getInStream();
    os = (FdOutStream)cc.getOutStream();
    client = cc;

    initGlobal();

    if (session == null) {
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
      
      setParam();

    }

    try {
      manager = new SSLEngineManager(engine, is, os);
    } catch(java.lang.Exception e) {
      throw new Exception(e.toString());
    }

    try {
      manager.doHandshake();
    } catch (java.lang.Exception e) {
      throw new Exception(e.toString());
    }

    //checkSession();

    cc.setStreams(new TLSInStream(is, manager),
		              new TLSOutStream(os, manager));
    return true;
  }

  private void setParam() {

    if (anon) {
      try {
        ctx.init(null, null, null);
      } catch(KeyManagementException e) {
        throw new AuthFailureException(e.toString());
      }
    } else {
      try {
        TrustManager[] myTM = new TrustManager[] { 
          new MyX509TrustManager() 
        };
        ctx.init (null, myTM, null);
      } catch (java.security.GeneralSecurityException e) {
        throw new AuthFailureException(e.toString());
      }
    }
    SSLSocketFactory sslfactory = ctx.getSocketFactory();
    engine = ctx.createSSLEngine(client.getServerName(),
                                 client.getServerPort());
    engine.setUseClientMode(true);

    if (anon) {
      String[] supported;
      ArrayList<String> enabled = new ArrayList<String>();

      supported = engine.getSupportedCipherSuites();

      for (int i = 0; i < supported.length; i++)
        if (supported[i].matches("TLS_DH_anon.*"))
	          enabled.add(supported[i]);

      engine.setEnabledCipherSuites(enabled.toArray(new String[0]));
    } else {
      engine.setEnabledCipherSuites(engine.getSupportedCipherSuites());
    }

    engine.setEnabledProtocols(new String[]{"SSLv3","TLSv1"});
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

  //protected void checkSession();
  protected CConnection client;



  private SSLContext ctx;
  private SSLSession session;
  private SSLEngine engine;
  private SSLEngineManager manager;
  private boolean anon;

  private String cafile, crlfile;
  private FdInStream is;
  private FdOutStream os;

  static LogWriter vlog = new LogWriter("CSecurityTLS");
}
