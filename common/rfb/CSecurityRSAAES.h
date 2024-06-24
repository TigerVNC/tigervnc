/* 
 * Copyright (C) 2022 Dinglan Peng
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

#ifndef __C_SECURITY_RSAAES_H__
#define __C_SECURITY_RSAAES_H__

#ifndef HAVE_NETTLE
#error "This header should not be compiled without HAVE_NETTLE defined"
#endif

#include <nettle/rsa.h>
#include <rfb/CSecurity.h>
#include <rfb/Security.h>
#include <rfb/UserMsgBox.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rdr/RandomStream.h>

namespace rfb {
  class UserMsgBox;
  class CSecurityRSAAES : public CSecurity {
  public:
    CSecurityRSAAES(CConnection* cc, uint32_t secType,
                    int keySize, bool isAllEncrypted);
    virtual ~CSecurityRSAAES();
    bool processMsg() override;
    int getType() const override { return secType; }
    bool isSecure() const override { return secType == secTypeRA256; }

    static IntParameter RSAKeyLength;

  private:
    void cleanup();
    void writePublicKey();
    bool readPublicKey();
    void verifyServer();
    void writeRandom();
    bool readRandom();
    void setCipher();
    void writeHash();
    bool readHash();
    void clearSecrets();
    bool readSubtype();
    void writeCredentials();

    int state;
    int keySize;
    bool isAllEncrypted;
    uint32_t secType;
    uint8_t subtype;
    struct rsa_private_key clientKey;
    struct rsa_public_key clientPublicKey;
    struct rsa_public_key serverKey;
    uint32_t serverKeyLength;
    uint8_t* serverKeyN;
    uint8_t* serverKeyE;
    uint32_t clientKeyLength;
    uint8_t* clientKeyN;
    uint8_t* clientKeyE;
    uint8_t serverRandom[32];
    uint8_t clientRandom[32];

    rdr::InStream* rais;
    rdr::OutStream* raos;

    rdr::InStream* rawis;
    rdr::OutStream* rawos;

    rdr::RandomStream rs;
  };
}

#endif
