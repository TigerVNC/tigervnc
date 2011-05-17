/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
  public static final int secTypeManaged   = 20;

  /* VeNCrypt subtypes */
  public static final int secTypePlain     = 256;
  public static final int secTypeTLSNone   = 257;
  public static final int secTypeTLSVnc    = 258;
  public static final int secTypeTLSPlain  = 259;
  public static final int secTypeX509None  = 260;
  public static final int secTypeX509Vnc   = 261;
  public static final int secTypeX509Plain = 262;

  // result types

  public static final int secResultOK = 0;
  public static final int secResultFailed = 1;
  public static final int secResultTooMany = 2; // deprecated

  public Security(StringParameter secTypes)
  {
    String secTypesStr;

    secTypesStr = secTypes.getData();
    enabledSecTypes = parseSecTypes(secTypesStr);

    secTypesStr = null;
  }

  public static List<Integer> enabledSecTypes = new ArrayList<Integer>();

  public static final List<Integer> GetEnabledSecTypes()
  {
    List<Integer> result = new ArrayList<Integer>();

    result.add(secTypeVeNCrypt);
    for (Iterator i = enabledSecTypes.iterator(); i.hasNext(); ) {
      int refType = (Integer)i.next();
      if (refType < 0x100)
        result.add(refType);
    }
    
    return (result);
  }

  public static final List<Integer> GetEnabledExtSecTypes()
  {
    List<Integer> result = new ArrayList<Integer>();

    for (Iterator i = enabledSecTypes.iterator(); i.hasNext(); ) {
      int refType = (Integer)i.next();
      if (refType != secTypeVeNCrypt) /* Do not include VeNCrypt to avoid loops */
        result.add(refType);
    }

    return (result);
  }

  public static final void EnableSecType(int secType)
  {

    for (Iterator i = enabledSecTypes.iterator(); i.hasNext(); )
      if ((Integer)i.next() == secType)
        return;

    enabledSecTypes.add(secType);
  }

  public boolean IsSupported(int secType)
  {
    Iterator i;
  
    for (i = enabledSecTypes.iterator(); i.hasNext(); )
     if ((Integer)i.next() == secType)
       return true;
    if (secType == secTypeVeNCrypt)
     return true;
  
    return false;
  }

  public static void DisableSecType(int secType) { enabledSecTypes.remove((Object)secType); }

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
    if (name.equalsIgnoreCase("Managed"))	  return secTypeManaged;

    /* VeNCrypt subtypes */
    if (name.equalsIgnoreCase("Plain"))	    return secTypePlain;
    if (name.equalsIgnoreCase("TLSNone"))	  return secTypeTLSNone;
    if (name.equalsIgnoreCase("TLSVnc"))	  return secTypeTLSVnc;
    if (name.equalsIgnoreCase("TLSPlain"))	return secTypeTLSPlain;
    if (name.equalsIgnoreCase("X509None"))	return secTypeX509None;
    if (name.equalsIgnoreCase("X509Vnc"))	  return secTypeX509Vnc;
    if (name.equalsIgnoreCase("X509Plain")) return secTypeX509Plain;

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
    case secTypeManaged:    return "Managed";

    /* VeNCrypt subtypes */
    case secTypePlain:      return "Plain";
    case secTypeTLSNone:    return "TLSNone";
    case secTypeTLSVnc:     return "TLSVnc";
    case secTypeTLSPlain:   return "TLSPlain";
    case secTypeX509None:   return "X509None";
    case secTypeX509Vnc:    return "X509Vnc";
    case secTypeX509Plain:  return "X509Plain";
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

  public final void SetSecTypes(List<Integer> secTypes) { enabledSecTypes = secTypes; }

  static LogWriter vlog = new LogWriter("Security");
}
