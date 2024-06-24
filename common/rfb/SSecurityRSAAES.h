/* Copyright (C) 2022 Dinglan Peng
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

#ifndef __S_SECURITY_RSAAES_H__
#define __S_SECURITY_RSAAES_H__

#ifndef HAVE_NETTLE
#error "This header should not be included without HAVE_NETTLE defined"
#endif

#include <nettle/rsa.h>
#include <rfb/SSecurity.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rdr/RandomStream.h>

namespace rfb {

  class SSecurityRSAAES : public SSecurity {
  public:
    SSecurityRSAAES(SConnection* sc, uint32_t secType,
                    int keySize, bool isAllEncrypted);
    virtual ~SSecurityRSAAES();
    bool processMsg() override;
    const char* getUserName() const override;
    int getType() const override {return secType;}
    AccessRights getAccessRights() const override
    {
      return accessRights;
    }

    static StringParameter keyFile;
    static BoolParameter requireUsername;

  private:
    void cleanup();
    void loadPrivateKey();
    void loadPKCS1Key(const uint8_t* data, size_t size);
    void loadPKCS8Key(const uint8_t* data, size_t size);
    void writePublicKey();
    bool readPublicKey();
    void writeRandom();
    bool readRandom();
    void setCipher();
    void writeHash();
    bool readHash();
    void clearSecrets();
    void writeSubtype();
    bool readCredentials();
    void verifyUserPass();
    void verifyPass();

    int state;
    int keySize;
    bool isAllEncrypted;
    uint32_t secType;
    struct rsa_private_key serverKey;
    struct rsa_public_key clientKey;
    uint32_t serverKeyLength;
    uint8_t* serverKeyN;
    uint8_t* serverKeyE;
    uint32_t clientKeyLength;
    uint8_t* clientKeyN;
    uint8_t* clientKeyE;
    uint8_t serverRandom[32];
    uint8_t clientRandom[32];

    char username[256];
    char password[256];
    AccessRights accessRights;

    rdr::InStream* rais;
    rdr::OutStream* raos;

    rdr::InStream* rawis;
    rdr::OutStream* rawos;

    rdr::RandomStream rs;
  };

}

#endif
