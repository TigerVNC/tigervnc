/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2011-2015 Brian P. Hinz
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

//
// SecTypes.java - constants for the various security types.
//

package com.tigervnc.rfb;
import java.util.*;

public class Security {

  public static final int secTypeInvalid   = 0;
  public static final int secTypeNone      = 1;
  public static final int secTypeVncAuth   = 2;

  public static final int secTypeRA2       = 5;
  public static final int secTypeRA2ne     = 6;

  public static final int secTypeSSPI      = 7;
  public static final int secTypeSSPIne    = 8;

  public static final int secTypeTight     = 16;
  public static final int secTypeUltra     = 17;
  public static final int secTypeTLS       = 18;
  public static final int secTypeVeNCrypt  = 19;

  /* VeNCrypt subtypes */
  public static final int secTypePlain     = 256;
  public static final int secTypeTLSNone   = 257;
  public static final int secTypeTLSVnc    = 258;
  public static final int secTypeTLSPlain  = 259;
  public static final int secTypeX509None  = 260;
  public static final int secTypeX509Vnc   = 261;
  public static final int secTypeX509Plain = 262;
  public static final int secTypeIdent     = 265;
  public static final int secTypeTLSIdent  = 266;
  public static final int secTypeX509Ident = 267;

  // result types

  public static final int secResultOK = 0;
  public static final int secResultFailed = 1;
  public static final int secResultTooMany = 2; // deprecated

  public Security() { }

  public Security(StringParameter secTypes)
  {
    String secTypesStr;

    secTypesStr = secTypes.getData();
    enabledSecTypes = parseSecTypes(secTypesStr);

    secTypesStr = null;
  }

  private List<Integer> enabledSecTypes = new ArrayList<Integer>();

  public final List<Integer> GetEnabledSecTypes()
  {
    List<Integer> result = new ArrayList<Integer>();

   /* Partial workaround for Vino's stupid behaviour. It doesn't allow
    * the basic authentication types as part of the VeNCrypt handshake,
    * making it impossible for a client to do opportunistic encryption.
    * At least make it possible to connect when encryption is explicitly
    * disabled. */
    for (Iterator<Integer> i = enabledSecTypes.iterator(); i.hasNext(); ) {
      int refType = (Integer)i.next();
      if (refType >= 0x100) {
        result.add(secTypeVeNCrypt);
        break;
      }
    }

    for (Iterator<Integer> i = enabledSecTypes.iterator(); i.hasNext(); ) {
      int refType = (Integer)i.next();
      if (refType < 0x100)
        result.add(refType);
    }

    return (result);
  }

  public final List<Integer> GetEnabledExtSecTypes()
  {
    List<Integer> result = new ArrayList<Integer>();

    for (Iterator<Integer> i = enabledSecTypes.iterator(); i.hasNext(); ) {
      int refType = (Integer)i.next();
      if (refType != secTypeVeNCrypt) /* Do not include VeNCrypt to avoid loops */
        result.add(refType);
    }

    return (result);
  }

  public final void EnableSecType(int secType)
  {

    for (Iterator<Integer> i = enabledSecTypes.iterator(); i.hasNext(); )
      if ((Integer)i.next() == secType)
        return;

    enabledSecTypes.add(secType);
  }

  public boolean IsSupported(int secType)
  {
    Iterator<Integer> i;

    for (i = enabledSecTypes.iterator(); i.hasNext(); )
     if ((Integer)i.next() == secType)
       return true;
    if (secType == secTypeVeNCrypt)
     return true;

    return false;
  }

  public String ToString()
  {
    Iterator<Integer> i;
    String out = new String("");
    boolean firstpass = true;
    String name;

    for (i = enabledSecTypes.iterator(); i.hasNext(); ) {
      name = secTypeName((Integer)i.next());
      if (name.startsWith("[")) /* Unknown security type */
        continue;

      if (!firstpass)
        out = out.concat(",");
      else
        firstpass = false;
      out = out.concat(name);
    }

    return out;
  }

  public void DisableSecType(int secType) { enabledSecTypes.remove((Object)secType); }

  public static int secTypeNum(String name) {
    if (name.equalsIgnoreCase("None"))      return secTypeNone;
    if (name.equalsIgnoreCase("VncAuth"))   return secTypeVncAuth;
    if (name.equalsIgnoreCase("Tight"))	    return secTypeTight;
    if (name.equalsIgnoreCase("RA2"))       return secTypeRA2;
    if (name.equalsIgnoreCase("RA2ne"))	    return secTypeRA2ne;
    if (name.equalsIgnoreCase("SSPI"))      return secTypeSSPI;
    if (name.equalsIgnoreCase("SSPIne"))	  return secTypeSSPIne;
    //if (name.equalsIgnoreCase("ultra"))	    return secTypeUltra;
    //if (name.equalsIgnoreCase("TLS"))	      return secTypeTLS;
    if (name.equalsIgnoreCase("VeNCrypt"))  return secTypeVeNCrypt;

    /* VeNCrypt subtypes */
    if (name.equalsIgnoreCase("Plain"))	    return secTypePlain;
    if (name.equalsIgnoreCase("Ident"))	    return secTypeIdent;
    if (name.equalsIgnoreCase("TLSNone"))	  return secTypeTLSNone;
    if (name.equalsIgnoreCase("TLSVnc"))	  return secTypeTLSVnc;
    if (name.equalsIgnoreCase("TLSPlain"))	return secTypeTLSPlain;
    if (name.equalsIgnoreCase("TLSIdent"))	return secTypeTLSIdent;
    if (name.equalsIgnoreCase("X509None"))	return secTypeX509None;
    if (name.equalsIgnoreCase("X509Vnc"))	  return secTypeX509Vnc;
    if (name.equalsIgnoreCase("X509Plain")) return secTypeX509Plain;
    if (name.equalsIgnoreCase("X509Ident")) return secTypeX509Ident;

    return secTypeInvalid;
  }

  public static String secTypeName(int num) {
    switch (num) {
    case secTypeNone:       return "None";
    case secTypeVncAuth:    return "VncAuth";
    case secTypeTight:      return "Tight";
    case secTypeRA2:        return "RA2";
    case secTypeRA2ne:      return "RA2ne";
    case secTypeSSPI:       return "SSPI";
    case secTypeSSPIne:     return "SSPIne";
    //case secTypeUltra:      return "Ultra";
    //case secTypeTLS:        return "TLS";
    case secTypeVeNCrypt:   return "VeNCrypt";

    /* VeNCrypt subtypes */
    case secTypePlain:      return "Plain";
    case secTypeIdent:      return "Ident";
    case secTypeTLSNone:    return "TLSNone";
    case secTypeTLSVnc:     return "TLSVnc";
    case secTypeTLSPlain:   return "TLSPlain";
    case secTypeTLSIdent:   return "TLSIdent";
    case secTypeX509None:   return "X509None";
    case secTypeX509Vnc:    return "X509Vnc";
    case secTypeX509Plain:  return "X509Plain";
    case secTypeX509Ident:  return "X509Ident";
    default:                return "[unknown secType]";
    }
  }

  public final static List<Integer> parseSecTypes(String types_)
  {
    List<Integer> result = new ArrayList<Integer>();
    String[] types = types_.split(",");
    for (int i = 0; i < types.length; i++) {
      int typeNum = secTypeNum(types[i]);
      if (typeNum != secTypeInvalid)
        result.add(typeNum);
    }
    return (result);
  }

  public final void SetSecTypes(List<Integer> secTypes) {
    enabledSecTypes = secTypes;
  }

  static LogWriter vlog = new LogWriter("Security");
}
