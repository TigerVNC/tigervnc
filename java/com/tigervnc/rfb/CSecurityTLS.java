/*
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 m-privacy GmbH
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2011-2012,2015 Brian P. Hinz
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
import java.security.KeyManagementException;
import java.security.KeyStore;
import java.security.NoSuchAlgorithmException;
import java.security.MessageDigest;
import java.security.cert.*;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import javax.swing.JOptionPane;
import javax.xml.bind.DatatypeConverter;

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
      manager.doHandshake();
    } catch(java.lang.Exception e) {
      throw new Exception(e.getMessage());
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

    String[] supported = engine.getSupportedProtocols();
    ArrayList<String> enabled = new ArrayList<String>();
    for (int i = 0; i < supported.length; i++)
      if (supported[i].matches("TLS.*"))
	      enabled.add(supported[i]);
    engine.setEnabledProtocols(enabled.toArray(new String[0]));

    if (anon) {
      supported = engine.getSupportedCipherSuites();
      enabled = new ArrayList<String>();
      // prefer ECDH over DHE
      for (int i = 0; i < supported.length; i++)
        if (supported[i].matches("TLS_ECDH_anon.*"))
	        enabled.add(supported[i]);
      for (int i = 0; i < supported.length; i++)
        if (supported[i].matches("TLS_DH_anon.*"))
	        enabled.add(supported[i]);
      engine.setEnabledCipherSuites(enabled.toArray(new String[0]));
    } else {
      engine.setEnabledCipherSuites(engine.getSupportedCipherSuites());
    }

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
      KeyStore ks = KeyStore.getInstance("JKS");
      CertificateFactory cf = CertificateFactory.getInstance("X.509");
      try {
        ks.load(null, null);
        String a = TrustManagerFactory.getDefaultAlgorithm();
        TrustManagerFactory tmf = TrustManagerFactory.getInstance(a);
        tmf.init((KeyStore)null);
        for (TrustManager m : tmf.getTrustManagers())
          if (m instanceof X509TrustManager)
            for (X509Certificate c : ((X509TrustManager)m).getAcceptedIssuers())
              ks.setCertificateEntry(c.getSubjectX500Principal().getName(), c);
        File castore = new File(FileUtils.getVncHomeDir()+"x509_savedcerts.pem");
        if (castore.exists() && castore.canRead()) {
          InputStream caStream = new MyFileInputStream(castore);
          Collection<? extends Certificate> cacerts =
            cf.generateCertificates(caStream);
          for (Certificate cert : cacerts) {
            String dn =
              ((X509Certificate)cert).getSubjectX500Principal().getName();
            ks.setCertificateEntry(dn, (X509Certificate)cert);
          }
        }
        File cacert = new File(cafile);
        if (cacert.exists() && cacert.canRead()) {
          InputStream caStream = new MyFileInputStream(cacert);
          Collection<? extends Certificate> cacerts =
            cf.generateCertificates(caStream);
          for (Certificate cert : cacerts) {
            String dn =
              ((X509Certificate)cert).getSubjectX500Principal().getName();
            ks.setCertificateEntry(dn, (X509Certificate)cert);
          }
        }
        PKIXBuilderParameters params =
          new PKIXBuilderParameters(ks, new X509CertSelector());
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
        tmf = TrustManagerFactory.getInstance("PKIX");
        tmf.init(new CertPathTrustManagerParameters(params));
        tm = (X509TrustManager)tmf.getTrustManagers()[0];
      } catch (java.lang.Exception e) {
        vlog.error(e.toString());
      }
    }

    public void checkClientTrusted(X509Certificate[] chain, String authType)
      throws CertificateException
    {
      tm.checkClientTrusted(chain, authType);
    }

    public void checkServerTrusted(X509Certificate[] chain, String authType)
      throws CertificateException
    {
      MessageDigest md = null;
      try {
        md = MessageDigest.getInstance("SHA-1");
        tm.checkServerTrusted(chain, authType);
      } catch (CertificateException e) {
        if (e.getCause() instanceof CertPathBuilderException) {
          Object[] answer = {"YES", "NO"};
          X509Certificate cert = chain[0];
          md.update(cert.getEncoded());
          String thumbprint =
            DatatypeConverter.printHexBinary(md.digest());
          thumbprint = thumbprint.replaceAll("..(?!$)", "$0 ");
          int ret = JOptionPane.showOptionDialog(null,
            "This certificate has been signed by an unknown authority\n"+
            "\n"+
            "  Subject: "+cert.getSubjectX500Principal().getName()+"\n"+
            "  Issuer: "+cert.getIssuerX500Principal().getName()+"\n"+
            "  Serial Number: "+cert.getSerialNumber()+"\n"+
            "  Version: "+cert.getVersion()+"\n"+
            "  Signature Algorithm: "+cert.getPublicKey().getAlgorithm()+"\n"+
            "  Not Valid Before: "+cert.getNotBefore()+"\n"+
            "  Not Valid After: "+cert.getNotAfter()+"\n"+
            "  SHA1 Fingerprint: "+thumbprint+"\n"+
            "\n"+
            "Do you want to save it and continue?",
            "Certificate Issuer Unknown",
            JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE,
            null, answer, answer[0]);
          if (ret == JOptionPane.YES_OPTION) {
            File vncDir = new File(FileUtils.getVncHomeDir());
            if (!vncDir.exists() && !vncDir.mkdir()) {
              vlog.info("Certificate save failed, unable to create ~/.vnc");
              return;
            }
            Collection<? extends X509Certificate> cacerts = null;
            String castore =
              FileUtils.getVncHomeDir()+"x509_savedcerts.pem";
            File caFile = new File(castore);
            try {
              caFile.createNewFile();
            } catch (IOException ioe) {
              vlog.error(ioe.getCause().getMessage());
              return;
            }
            InputStream caStream = new MyFileInputStream(caFile);
            CertificateFactory cf =
              CertificateFactory.getInstance("X.509");
            cacerts =
              (Collection <? extends X509Certificate>)cf.generateCertificates(caStream);
            for (int i = 0; i < chain.length; i++) {
              if (cacerts == null || !cacerts.contains(chain[i])) {
                byte[] der = chain[i].getEncoded();
                String pem = DatatypeConverter.printBase64Binary(der);
                pem = pem.replaceAll("(.{64})", "$1\n");
                FileWriter fw = null;
                try {
                  fw = new FileWriter(castore, true);
                  fw.write("-----BEGIN CERTIFICATE-----\n");
                  fw.write(pem+"\n");
                  fw.write("-----END CERTIFICATE-----\n");
                } catch (IOException ioe) {
                  throw new Exception(ioe.getCause().getMessage());
                } finally {
                  try {
                    if (fw != null)
                      fw.close();
                  } catch(IOException ioe2) {
                    throw new Exception(ioe2.getCause().getMessage());
                  }
                }
              }
            }
          } else {
            System.exit(1);
          }
        } else {
          throw new Exception(e.getCause().getMessage());
        }
      } catch (java.lang.Exception e) {
        throw new Exception(e.getCause().getMessage());
      }
    }

    public X509Certificate[] getAcceptedIssuers ()
    {
      return tm.getAcceptedIssuers();
    }

    private class MyFileInputStream extends InputStream {
      // Blank lines in a certificate file will cause Java 6 to throw a
      // "DerInputStream.getLength(): lengthTag=127, too big" exception.
      ByteBuffer buf;

      public MyFileInputStream(String name) {
        this(new File(name));
      }

      public MyFileInputStream(File file) {
        StringBuffer sb = new StringBuffer();
        BufferedReader reader = null;
        try {
          reader = new BufferedReader(new FileReader(file));
          String l;
          while ((l = reader.readLine()) != null) {
            if (l.trim().length() > 0 )
              sb.append(l+"\n");
          }
        } catch (java.lang.Exception e) {
          throw new Exception(e.toString());
        } finally {
          try {
            if (reader != null)
              reader.close();
          } catch(IOException ioe) {
            throw new Exception(ioe.getCause().getMessage());
          }
        }
        Charset utf8 = Charset.forName("UTF-8");
        buf = ByteBuffer.wrap(sb.toString().getBytes(utf8));
        buf.limit(buf.capacity());
      }

      @Override
      public int read(byte[] b) throws IOException {
        return this.read(b, 0, b.length);
      }

      @Override
      public int read(byte[] b, int off, int len) throws IOException {
        if (!buf.hasRemaining())
          return -1;
        len = Math.min(len, buf.remaining());
        buf.get(b, off, len);
        return len;
      }


      @Override
      public int read() throws IOException {
        if (!buf.hasRemaining())
          return -1;
        return buf.get() & 0xFF;
      }
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
