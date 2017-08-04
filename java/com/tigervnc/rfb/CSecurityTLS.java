/*
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 m-privacy GmbH
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2011-2017 Brian P. Hinz
 * Copyright (C) 2015 D. R. Commander.  All Rights Reserved.
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
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.InputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.List;
import javax.naming.InvalidNameException;
import javax.naming.ldap.LdapName;
import javax.naming.ldap.Rdn;
import javax.net.ssl.HostnameVerifier;
import javax.swing.JOptionPane;
import javax.xml.bind.DatatypeConverter;

import com.tigervnc.rdr.*;
import com.tigervnc.network.*;
import com.tigervnc.vncviewer.*;

public class CSecurityTLS extends CSecurity {

  public static StringParameter X509CA
  = new StringParameter("X509CA",
                        "X509 CA certificate", "", Configuration.ConfigurationObject.ConfViewer);
  public static StringParameter X509CRL
  = new StringParameter("X509CRL",
                        "X509 CRL file", "", Configuration.ConfigurationObject.ConfViewer);

  private void initGlobal()
  {
    try {
      ctx = SSLContext.getInstance("TLS");
    } catch(NoSuchAlgorithmException e) {
      throw new Exception(e.toString());
    }
  }

  public CSecurityTLS(boolean _anon)
  {
    anon = _anon;
    manager = null;

    setDefaults();
    cafile = X509CA.getData();
    crlfile = X509CRL.getData();
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
      X509CA.setDefaultStr(getDefaultCA());
    if (new File(getDefaultCRL()).exists())
      X509CRL.setDefaultStr(getDefaultCRL());
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

    if (manager == null) {
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
      throw new SystemException(e.toString());
    }

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
              ks.setCertificateEntry(getThumbprint((X509Certificate)c), c);
        File cacert = new File(cafile);
        if (cacert.exists() && cacert.canRead()) {
          InputStream caStream = new MyFileInputStream(cacert);
          Collection<? extends Certificate> cacerts =
            cf.generateCertificates(caStream);
          for (Certificate cert : cacerts) {
            String thumbprint = getThumbprint((X509Certificate)cert);
            ks.setCertificateEntry(thumbprint, (X509Certificate)cert);
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
        throw new Exception(e.getMessage());
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
      Collection<? extends Certificate> certs = null;
      X509Certificate cert = chain[0];
      String thumbprint = getThumbprint(cert);
      File vncDir = new File(FileUtils.getVncHomeDir());
      File certFile = new File(vncDir, "x509_savedcerts.pem");
      CertificateFactory cf = CertificateFactory.getInstance("X.509");
      if (vncDir.exists() && certFile.exists() && certFile.canRead()) {
        InputStream certStream = new MyFileInputStream(certFile);
        certs = cf.generateCertificates(certStream);
        for (Certificate c : certs)
          if (thumbprint.equals(getThumbprint((X509Certificate)c)))
            return;
      }
      try {
        verifyHostname(cert);
        tm.checkServerTrusted(chain, authType);
      } catch (java.lang.Exception e) {
        if (e.getCause() instanceof CertPathBuilderException) {
          Object[] answer = {"YES", "NO"};
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
            "  SHA1 Fingerprint: "+getThumbprint(cert)+"\n"+
            "\n"+
            "Do you want to save it and continue?",
            "Certificate Issuer Unknown",
            JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE,
            null, answer, answer[0]);
          if (ret == JOptionPane.YES_OPTION) {
            try {
              if (!vncDir.exists())
                vncDir.mkdir();
              if (!certFile.exists() && !certFile.createNewFile()) {
                vlog.error("Certificate save failed.");
                return;
              }
            } catch (java.lang.Exception ioe) {
              // skip save if security settings prohibit access to filesystem
              vlog.error("Certificate save failed: "+ioe.getMessage());
              return;
            }
            if (certs == null || !certs.contains(cert)) {
              byte[] der = cert.getEncoded();
              String pem = DatatypeConverter.printBase64Binary(der);
              pem = pem.replaceAll("(.{64})", "$1\n");
              FileWriter fw = null;
              try {
                fw = new FileWriter(certFile.getAbsolutePath(), true);
                fw.write("-----BEGIN CERTIFICATE-----\n");
                fw.write(pem+"\n");
                fw.write("-----END CERTIFICATE-----\n");
              } catch (IOException ioe) {
                throw new Exception(ioe.getMessage());
              } finally {
                try {
                  if (fw != null)
                    fw.close();
                } catch(IOException ioe2) {
                  throw new Exception(ioe2.getMessage());
                }
              }
            }
          } else {
            throw new WarningException("Peer certificate verification failed.");
          }
        } else {
          throw new SystemException(e.getMessage());
        }
      }
    }

    public X509Certificate[] getAcceptedIssuers ()
    {
      return tm.getAcceptedIssuers();
    }

    private String getThumbprint(X509Certificate cert)
    {
      String thumbprint = null;
      try {
        MessageDigest md = MessageDigest.getInstance("SHA-1");
        md.update(cert.getEncoded());
        thumbprint = DatatypeConverter.printHexBinary(md.digest());
        thumbprint = thumbprint.replaceAll("..(?!$)", "$0 ");
      } catch(CertificateEncodingException e) {
        throw new SystemException(e.getMessage());
      } catch(NoSuchAlgorithmException e) {
        throw new SystemException(e.getMessage());
      }
      return thumbprint;
    }

    private void verifyHostname(X509Certificate cert)
      throws CertificateParsingException
    {
      try {
        Collection sans = cert.getSubjectAlternativeNames();
        if (sans == null) {
          String dn = cert.getSubjectX500Principal().getName();
          LdapName ln = new LdapName(dn);
          for (Rdn rdn : ln.getRdns()) {
            if (rdn.getType().equalsIgnoreCase("CN")) {
              String peer =
                ((CConn)client).getSocket().getPeerName().toLowerCase();
              if (peer.equals(((String)rdn.getValue()).toLowerCase()))
                return;
            }
          }
        } else {
          Iterator i = sans.iterator();
          while (i.hasNext()) {
            List nxt = (List)i.next();
            if (((Integer)nxt.get(0)).intValue() == 2) {
              String peer =
                ((CConn)client).getSocket().getPeerName().toLowerCase();
              if (peer.equals(((String)nxt.get(1)).toLowerCase()))
                return;
            } else if (((Integer)nxt.get(0)).intValue() == 7) {
              String peer = ((CConn)client).getSocket().getPeerAddress();
              if (peer.equals(((String)nxt.get(1)).toLowerCase()))
                return;
            }
          }
        }
        Object[] answer = {"YES", "NO"};
        int ret = JOptionPane.showOptionDialog(null,
          "Hostname verification failed. Do you want to continue?",
          "Hostname Verification Failure",
          JOptionPane.YES_NO_OPTION, JOptionPane.WARNING_MESSAGE,
          null, answer, answer[0]);
        if (ret != JOptionPane.YES_OPTION)
          throw new WarningException("Hostname verification failed.");
      } catch (CertificateParsingException e) {
        throw new SystemException(e.getMessage());
      } catch (InvalidNameException e) {
        throw new SystemException(e.getMessage());
      }
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
            throw new Exception(ioe.getMessage());
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

  protected CConnection client;

  private SSLContext ctx;
  private SSLEngine engine;
  private SSLEngineManager manager;
  private boolean anon;

  private String cafile, crlfile;
  private FdInStream is;
  private FdOutStream os;

  static LogWriter vlog = new LogWriter("CSecurityTLS");
}
