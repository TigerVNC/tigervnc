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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_NETTLE
#error "This header should not be compiled without HAVE_NETTLE defined"
#endif

#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <assert.h>

#include <nettle/bignum.h>
#include <nettle/sha1.h>
#include <nettle/sha2.h>
#include <rfb/CSecurityRSAAES.h>
#include <rfb/CConnection.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rfb/UserMsgBox.h>
#include <rfb/util.h>
#include <rdr/AESInStream.h>
#include <rdr/AESOutStream.h>
#include <os/os.h>

enum {
  ReadPublicKey,
  ReadRandom,
  ReadHash,
  ReadSubtype,
};

const int MinKeyLength = 1024;
const int MaxKeyLength = 8192;

using namespace rfb;

CSecurityRSAAES::CSecurityRSAAES(CConnection* cc_, uint32_t _secType,
                                 int _keySize, bool _isAllEncrypted)
  : CSecurity(cc_), state(ReadPublicKey),
    keySize(_keySize), isAllEncrypted(_isAllEncrypted), secType(_secType),
    clientKey(), clientPublicKey(), serverKey(),
    serverKeyN(nullptr), serverKeyE(nullptr),
    clientKeyN(nullptr), clientKeyE(nullptr),
    rais(nullptr), raos(nullptr), rawis(nullptr), rawos(nullptr)
{
  assert(keySize == 128 || keySize == 256);
}

CSecurityRSAAES::~CSecurityRSAAES()
{
  cleanup();
}

void CSecurityRSAAES::cleanup()
{
  if (serverKeyN)
    delete[] serverKeyN;
  if (serverKeyE)
    delete[] serverKeyE;
  if (clientKeyN)
    delete[] clientKeyN;
  if (clientKeyE)
    delete[] clientKeyE;
  if (clientKey.size)
    rsa_private_key_clear(&clientKey);
  if (clientPublicKey.size)
    rsa_public_key_clear(&clientPublicKey);
  if (serverKey.size)
    rsa_public_key_clear(&serverKey);
  if (isAllEncrypted && rawis && rawos)
    cc->setStreams(rawis, rawos);
  if (rais)
    delete rais;
  if (raos)
    delete raos;
}

bool CSecurityRSAAES::processMsg()
{
  switch (state) {
    case ReadPublicKey:
      if (!readPublicKey())
        return false;
      verifyServer();
      writePublicKey();
      writeRandom();
      state = ReadRandom;
      /* fall through */
    case ReadRandom:
      if (!readRandom())
        return false;
      setCipher();
      writeHash();
      state = ReadHash;
      /* fall through */
    case ReadHash:
      if (!readHash())
        return false;
      clearSecrets();
      state = ReadSubtype;
      /* fall through */
    case ReadSubtype:
      if (!readSubtype())
        return false;
      writeCredentials();
      return true;
  }
  assert(!"unreachable");
  return false;
}

static void random_func(void* ctx, size_t length, uint8_t* dst)
{
  rdr::RandomStream* rs = (rdr::RandomStream*)ctx;
  if (!rs->hasData(length))
    throw Exception("failed to generate random");
  rs->readBytes(dst, length);
}

void CSecurityRSAAES::writePublicKey()
{
  rdr::OutStream* os = cc->getOutStream();
  // generate client key
  rsa_public_key_init(&clientPublicKey);
  rsa_private_key_init(&clientKey);
  // match the server key size
  clientKeyLength = serverKeyLength;
  int rsaKeySize = (clientKeyLength + 7) / 8;
  // set key size to non-zero to allow clearing the keys when cleanup
  clientPublicKey.size = rsaKeySize;
  clientKey.size = rsaKeySize;
  // set e = 65537
  mpz_set_ui(clientPublicKey.e, 65537);
  if (!rsa_generate_keypair(&clientPublicKey, &clientKey,
                            &rs, random_func, nullptr, nullptr,
                            clientKeyLength, 0))
    throw Exception("failed to generate key");
  clientKeyN = new uint8_t[rsaKeySize];
  clientKeyE = new uint8_t[rsaKeySize];
  nettle_mpz_get_str_256(rsaKeySize, clientKeyN, clientPublicKey.n);
  nettle_mpz_get_str_256(rsaKeySize, clientKeyE, clientPublicKey.e);
  os->writeU32(clientKeyLength);
  os->writeBytes(clientKeyN, rsaKeySize);
  os->writeBytes(clientKeyE, rsaKeySize);
  os->flush();
}

bool CSecurityRSAAES::readPublicKey()
{
  rdr::InStream* is = cc->getInStream();
  if (!is->hasData(4))
    return false;
  is->setRestorePoint();
  serverKeyLength = is->readU32();
  if (serverKeyLength < MinKeyLength)
    throw Exception("server key is too short");
  if (serverKeyLength > MaxKeyLength)
    throw Exception("server key is too long");
  size_t size = (serverKeyLength + 7) / 8;
  if (!is->hasDataOrRestore(size * 2))
    return false;
  is->clearRestorePoint();
  serverKeyE = new uint8_t[size];
  serverKeyN = new uint8_t[size];
  is->readBytes(serverKeyN, size);
  is->readBytes(serverKeyE, size);
  rsa_public_key_init(&serverKey);
  nettle_mpz_set_str_256_u(serverKey.n, size, serverKeyN);
  nettle_mpz_set_str_256_u(serverKey.e, size, serverKeyE);
  if (!rsa_public_key_prepare(&serverKey))
    throw Exception("server key is invalid");
  return true;
}

void CSecurityRSAAES::verifyServer()
{
  uint8_t lenServerKey[4] = {
    (uint8_t)((serverKeyLength & 0xff000000) >> 24),
    (uint8_t)((serverKeyLength & 0xff0000) >> 16),
    (uint8_t)((serverKeyLength & 0xff00) >> 8),
    (uint8_t)(serverKeyLength & 0xff)
  };
  uint8_t f[8];
  struct sha1_ctx ctx;
  sha1_init(&ctx);
  sha1_update(&ctx, 4, lenServerKey);
  sha1_update(&ctx, serverKey.size, serverKeyN);
  sha1_update(&ctx, serverKey.size, serverKeyE);
  sha1_digest(&ctx, sizeof(f), f);
  const char *title = "Server key fingerprint";
  std::string text = format(
    "The server has provided the following identifying information:\n"
    "Fingerprint: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n"
    "Please verify that the information is correct and press \"Yes\". "
    "Otherwise press \"No\"", f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
  if (!msg->showMsgBox(UserMsgBox::M_YESNO, title, text.c_str()))
    throw Exception("server key mismatch");
}

void CSecurityRSAAES::writeRandom()
{
  rdr::OutStream* os = cc->getOutStream();
  if (!rs.hasData(keySize / 8))
    throw Exception("failed to generate random");
  rs.readBytes(clientRandom, keySize / 8);
  mpz_t x;
  mpz_init(x);
  int res;
  try {
    res = rsa_encrypt(&serverKey, &rs, random_func, keySize / 8,
                      clientRandom, x);
  } catch (...) {
    mpz_clear(x);
    throw;
  }
  if (!res) {
    mpz_clear(x);
    throw Exception("failed to encrypt random");
  }
  uint8_t* buffer = new uint8_t[serverKey.size];
  nettle_mpz_get_str_256(serverKey.size, buffer, x);
  mpz_clear(x);
  os->writeU16(serverKey.size);
  os->writeBytes(buffer, serverKey.size);
  os->flush();
  delete[] buffer;
}

bool CSecurityRSAAES::readRandom()
{
  rdr::InStream* is = cc->getInStream();
  if (!is->hasData(2))
    return false;
  is->setRestorePoint();
  size_t size = is->readU16();
  if (size != clientKey.size)
    throw Exception("client key length doesn't match");
  if (!is->hasDataOrRestore(size))
    return false;
  is->clearRestorePoint();
  uint8_t* buffer = new uint8_t[size];
  is->readBytes(buffer, size);
  size_t randomSize = keySize / 8;
  mpz_t x;
  nettle_mpz_init_set_str_256_u(x, size, buffer);
  delete[] buffer;
  if (!rsa_decrypt(&clientKey, &randomSize, serverRandom, x) ||
      randomSize != (size_t)keySize / 8) {
    mpz_clear(x);
    throw Exception("failed to decrypt server random");
  }
  mpz_clear(x);
  return true;
}

void CSecurityRSAAES::setCipher()
{
  rawis = cc->getInStream();
  rawos = cc->getOutStream();
  uint8_t key[32];
  if (keySize == 128) {
    struct sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, 16, clientRandom);
    sha1_update(&ctx, 16, serverRandom);
    sha1_digest(&ctx, 16, key);
    rais = new rdr::AESInStream(rawis, key, 128);
    sha1_init(&ctx);
    sha1_update(&ctx, 16, serverRandom);
    sha1_update(&ctx, 16, clientRandom);
    sha1_digest(&ctx, 16, key);
    raos = new rdr::AESOutStream(rawos, key, 128);
  } else {
    struct sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, 32, clientRandom);
    sha256_update(&ctx, 32, serverRandom);
    sha256_digest(&ctx, 32, key);
    rais = new rdr::AESInStream(rawis, key, 256);
    sha256_init(&ctx);
    sha256_update(&ctx, 32, serverRandom);
    sha256_update(&ctx, 32, clientRandom);
    sha256_digest(&ctx, 32, key);
    raos = new rdr::AESOutStream(rawos, key, 256);
  }
  if (isAllEncrypted)
    cc->setStreams(rais, raos);
}

void CSecurityRSAAES::writeHash()
{
  uint8_t hash[32];
  size_t len = serverKeyLength;
  uint8_t lenServerKey[4] = {
    (uint8_t)((len & 0xff000000) >> 24),
    (uint8_t)((len & 0xff0000) >> 16),
    (uint8_t)((len & 0xff00) >> 8),
    (uint8_t)(len & 0xff)
  };
  len = clientKeyLength;
  uint8_t lenClientKey[4] = {
    (uint8_t)((len & 0xff000000) >> 24),
    (uint8_t)((len & 0xff0000) >> 16),
    (uint8_t)((len & 0xff00) >> 8),
    (uint8_t)(len & 0xff)
  };
  int hashSize;
  if (keySize == 128) {
    hashSize = 20;
    struct sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, 4, lenClientKey);
    sha1_update(&ctx, clientKey.size, clientKeyN);
    sha1_update(&ctx, clientKey.size, clientKeyE);
    sha1_update(&ctx, 4, lenServerKey);
    sha1_update(&ctx, serverKey.size, serverKeyN);
    sha1_update(&ctx, serverKey.size, serverKeyE);
    sha1_digest(&ctx, hashSize, hash);
  } else {
    hashSize = 32;
    struct sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, 4, lenClientKey);
    sha256_update(&ctx, clientKey.size, clientKeyN);
    sha256_update(&ctx, clientKey.size, clientKeyE);
    sha256_update(&ctx, 4, lenServerKey);
    sha256_update(&ctx, serverKey.size, serverKeyN);
    sha256_update(&ctx, serverKey.size, serverKeyE);
    sha256_digest(&ctx, hashSize, hash);
  }
  raos->writeBytes(hash, hashSize);
  raos->flush();
}

bool CSecurityRSAAES::readHash()
{
  uint8_t hash[32];
  uint8_t realHash[32];
  int hashSize = keySize == 128 ? 20 : 32;
  if (!rais->hasData(hashSize))
    return false;
  rais->readBytes(hash, hashSize);
  size_t len = serverKeyLength;
  uint8_t lenServerKey[4] = {
    (uint8_t)((len & 0xff000000) >> 24),
    (uint8_t)((len & 0xff0000) >> 16),
    (uint8_t)((len & 0xff00) >> 8),
    (uint8_t)(len & 0xff)
  };
  len = clientKeyLength;
  uint8_t lenClientKey[4] = {
    (uint8_t)((len & 0xff000000) >> 24),
    (uint8_t)((len & 0xff0000) >> 16),
    (uint8_t)((len & 0xff00) >> 8),
    (uint8_t)(len & 0xff)
  };
  if (keySize == 128) {
    struct sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, 4, lenServerKey);
    sha1_update(&ctx, serverKey.size, serverKeyN);
    sha1_update(&ctx, serverKey.size, serverKeyE);
    sha1_update(&ctx, 4, lenClientKey);
    sha1_update(&ctx, clientKey.size, clientKeyN);
    sha1_update(&ctx, clientKey.size, clientKeyE);
    sha1_digest(&ctx, hashSize, realHash);
  } else {
    struct sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, 4, lenServerKey);
    sha256_update(&ctx, serverKey.size, serverKeyN);
    sha256_update(&ctx, serverKey.size, serverKeyE);
    sha256_update(&ctx, 4, lenClientKey);
    sha256_update(&ctx, clientKey.size, clientKeyN);
    sha256_update(&ctx, clientKey.size, clientKeyE);
    sha256_digest(&ctx, hashSize, realHash);
  }
  if (memcmp(hash, realHash, hashSize) != 0)
    throw Exception("hash doesn't match");
  return true;
}

void CSecurityRSAAES::clearSecrets()
{
  rsa_private_key_clear(&clientKey);
  rsa_public_key_clear(&clientPublicKey);
  rsa_public_key_clear(&serverKey);
  clientKey.size = 0;
  clientPublicKey.size = 0;
  serverKey.size = 0;
  delete[] serverKeyN;
  delete[] serverKeyE;
  delete[] clientKeyN;
  delete[] clientKeyE;
  serverKeyN = nullptr;
  serverKeyE = nullptr;
  clientKeyN = nullptr;
  clientKeyE = nullptr;
  memset(serverRandom, 0, sizeof(serverRandom));
  memset(clientRandom, 0, sizeof(clientRandom));
}

bool CSecurityRSAAES::readSubtype()
{
  if (!rais->hasData(1))
    return false;
  subtype = rais->readU8();
  if (subtype != secTypeRA2UserPass && subtype != secTypeRA2Pass)
    throw Exception("unknown RSA-AES subtype");
  return true;
}

void CSecurityRSAAES::writeCredentials()
{
  std::string username;
  std::string password;

  if (subtype == secTypeRA2UserPass)
    (CSecurity::upg)->getUserPasswd(isSecure(), &username, &password);
  else
    (CSecurity::upg)->getUserPasswd(isSecure(), nullptr, &password);

  if (subtype == secTypeRA2UserPass) {
    if (username.size() > 255)
      throw Exception("username is too long");
    raos->writeU8(username.size());
    raos->writeBytes((const uint8_t*)username.data(), username.size());
  } else {
    raos->writeU8(0);
  }

  if (password.size() > 255)
    throw Exception("password is too long");
  raos->writeU8(password.size());
  raos->writeBytes((const uint8_t*)password.data(), password.size());
  raos->flush();
}
