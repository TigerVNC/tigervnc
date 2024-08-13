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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_NETTLE
#error "This source should not be compiled without HAVE_NETTLE defined"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <vector>

#include <nettle/bignum.h>
#include <nettle/sha1.h>
#include <nettle/sha2.h>
#include <nettle/base64.h>
#include <nettle/asn1.h>
#include <rfb/SSecurityRSAAES.h>
#include <rfb/SConnection.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rdr/AESInStream.h>
#include <rdr/AESOutStream.h>
#if !defined(WIN32) && !defined(__APPLE__)
#include <rfb/UnixPasswordValidator.h>
#endif
#ifdef WIN32
#include <rfb/WinPasswdValidator.h>
#endif
#include <rfb/SSecurityVncAuth.h>

enum {
  SendPublicKey,
  ReadPublicKey,
  ReadRandom,
  ReadHash,
  ReadCredentials,
};

const int MinKeyLength = 1024;
const int MaxKeyLength = 8192;
const size_t MaxKeyFileSize = 32 * 1024;

using namespace rfb;

StringParameter SSecurityRSAAES::keyFile
("RSAKey", "Path to the RSA key for the RSA-AES security types in "
           "PEM format", "", ConfServer);
BoolParameter SSecurityRSAAES::requireUsername
("RequireUsername", "Require username for the RSA-AES security types",
 false, ConfServer);

SSecurityRSAAES::SSecurityRSAAES(SConnection* sc_, uint32_t _secType,
                                 int _keySize, bool _isAllEncrypted)
  : SSecurity(sc_), state(SendPublicKey),
    keySize(_keySize), isAllEncrypted(_isAllEncrypted), secType(_secType),
    serverKey(), clientKey(),
    serverKeyN(nullptr), serverKeyE(nullptr),
    clientKeyN(nullptr), clientKeyE(nullptr),
    accessRights(AccessDefault),
    rais(nullptr), raos(nullptr), rawis(nullptr), rawos(nullptr)
{
  assert(keySize == 128 || keySize == 256);
}

SSecurityRSAAES::~SSecurityRSAAES()
{
  cleanup();
}

void SSecurityRSAAES::cleanup()
{
  if (serverKeyN)
    delete[] serverKeyN;
  if (serverKeyE)
    delete[] serverKeyE;
  if (clientKeyN)
    delete[] clientKeyN;
  if (clientKeyE)
    delete[] clientKeyE;
  if (serverKey.size)
    rsa_private_key_clear(&serverKey);
  if (clientKey.size)
    rsa_public_key_clear(&clientKey);
  if (isAllEncrypted && rawis && rawos)
    sc->setStreams(rawis, rawos);
  if (rais)
    delete rais;
  if (raos)
    delete raos;
}

static inline ssize_t findSubstr(uint8_t* data, size_t size, const char *pattern)
{
  size_t patternLength = strlen(pattern);
  for (size_t i = 0; i + patternLength < size; ++i) {
    for (size_t j = 0; j < patternLength; ++j)
      if (data[i + j] != pattern[j])
        goto next;
    return i;
next:
    continue;
  }
  return -1;
}

static bool loadPEM(uint8_t* data, size_t size, const char *begin,
                    const char *end, std::vector<uint8_t> *der)
{
  ssize_t pos1 = findSubstr(data, size, begin);
  if (pos1 == -1)
    return false;
  pos1 += strlen(begin);
  ssize_t base64Size = findSubstr(data + pos1, size - pos1, end);
  if (base64Size == -1)
    return false;
  char *derBase64 = (char *)data + pos1;
  if (!base64Size)
    return false;
  der->resize(BASE64_DECODE_LENGTH(base64Size));
  struct base64_decode_ctx ctx;
  size_t derSize;
  base64_decode_init(&ctx);
  if (!base64_decode_update(&ctx, &derSize, der->data(),
                            base64Size, derBase64))
    return false;
  if (!base64_decode_final(&ctx))
    return false;
  assert(derSize <= der->size());
  der->resize(derSize);
  return true;
}

void SSecurityRSAAES::loadPrivateKey()
{
  FILE* file = fopen(keyFile, "rb");
  if (!file)
    throw Exception("failed to open key file");
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  if (size == 0 || size > MaxKeyFileSize) {
    fclose(file);
    throw Exception("size of key file is zero or too big");
  }
  fseek(file, 0, SEEK_SET);
  std::vector<uint8_t> data(size);
  if (fread(data.data(), 1, data.size(), file) != size) {
    fclose(file);
    throw Exception("failed to read key");
  }
  fclose(file);

  std::vector<uint8_t> der;
  if (loadPEM(data.data(), data.size(),
              "-----BEGIN RSA PRIVATE KEY-----\n",
              "-----END RSA PRIVATE KEY-----", &der)) {
    loadPKCS1Key(der.data(), der.size());
    return;
  }
  if (loadPEM(data.data(), data.size(),
              "-----BEGIN PRIVATE KEY-----\n",
              "-----END PRIVATE KEY-----", &der)) {
    loadPKCS8Key(der.data(), der.size());
    return;
  }
  throw Exception("failed to import key");
}

void SSecurityRSAAES::loadPKCS1Key(const uint8_t* data, size_t size)
{
  struct rsa_public_key pub;
  rsa_private_key_init(&serverKey);
  rsa_public_key_init(&pub);
  if (!rsa_keypair_from_der(&pub, &serverKey, 0, size, data)) {
    rsa_private_key_clear(&serverKey);
    rsa_public_key_clear(&pub);
    throw Exception("failed to import key");
  }
  serverKeyLength = serverKey.size * 8;
  serverKeyN = new uint8_t[serverKey.size];
  serverKeyE = new uint8_t[serverKey.size];
  nettle_mpz_get_str_256(serverKey.size, serverKeyN, pub.n);
  nettle_mpz_get_str_256(serverKey.size, serverKeyE, pub.e);
  rsa_public_key_clear(&pub);
}

void SSecurityRSAAES::loadPKCS8Key(const uint8_t* data, size_t size)
{
  struct asn1_der_iterator i, j;
  uint32_t version;
  const char* rsaIdentifier = "\x2a\x86\x48\x86\xf7\x0d\x01\x01\x01";
  const size_t rsaIdentifierLength = 9;
  enum asn1_iterator_result res = asn1_der_iterator_first(&i, size, data);
  if (res != ASN1_ITERATOR_CONSTRUCTED)
    goto failed;
  if (i.type != ASN1_SEQUENCE)
    goto failed;
  if (asn1_der_decode_constructed_last(&i) != ASN1_ITERATOR_PRIMITIVE)
    goto failed;
  if (!(i.type == ASN1_INTEGER &&
        asn1_der_get_uint32(&i, &version) &&
        version == 0))
    goto failed;
  if (!(asn1_der_iterator_next(&i) == ASN1_ITERATOR_CONSTRUCTED &&
        i.type == ASN1_SEQUENCE &&
        asn1_der_decode_constructed(&i, &j) == ASN1_ITERATOR_PRIMITIVE &&
        j.type == ASN1_IDENTIFIER &&
        j.length == rsaIdentifierLength &&
        memcmp(j.data, rsaIdentifier, rsaIdentifierLength) == 0))
    goto failed;
  if (!(asn1_der_iterator_next(&i) == ASN1_ITERATOR_PRIMITIVE &&
        i.type == ASN1_OCTETSTRING && i.length))
    goto failed;
  loadPKCS1Key(i.data, i.length);
  return;
failed:
  throw Exception("failed to import key");
}

bool SSecurityRSAAES::processMsg()
{
  switch (state) {
    case SendPublicKey:
      loadPrivateKey();
      writePublicKey();
      state = ReadPublicKey;
      /* fall through */
    case ReadPublicKey:
      if (!readPublicKey())
        return false;
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
      writeSubtype();
      state = ReadCredentials;
      /* fall through */
    case ReadCredentials:
      if (!readCredentials())
        return false;
      if (requireUsername)
        verifyUserPass();
      else
        verifyPass();
      return true;
  }
  assert(!"unreachable");
  return false;
}

void SSecurityRSAAES::writePublicKey()
{
  rdr::OutStream* os = sc->getOutStream();
  os->writeU32(serverKeyLength);
  os->writeBytes(serverKeyN, serverKey.size);
  os->writeBytes(serverKeyE, serverKey.size);
  os->flush();
}

bool SSecurityRSAAES::readPublicKey()
{
  rdr::InStream* is = sc->getInStream();
  if (!is->hasData(4))
    return false;
  is->setRestorePoint();
  clientKeyLength = is->readU32();
  if (clientKeyLength < MinKeyLength)
    throw ConnFailedException("client key is too short");
  if (clientKeyLength > MaxKeyLength)
    throw ConnFailedException("client key is too long");
  size_t size = (clientKeyLength + 7) / 8;
  if (!is->hasDataOrRestore(size * 2))
    return false;
  is->clearRestorePoint();
  clientKeyE = new uint8_t[size];
  clientKeyN = new uint8_t[size];
  is->readBytes(clientKeyN, size);
  is->readBytes(clientKeyE, size);
  rsa_public_key_init(&clientKey);
  nettle_mpz_set_str_256_u(clientKey.n, size, clientKeyN);
  nettle_mpz_set_str_256_u(clientKey.e, size, clientKeyE);
  if (!rsa_public_key_prepare(&clientKey))
    throw ConnFailedException("client key is invalid");
  return true;
}

static void random_func(void* ctx, size_t length, uint8_t* dst)
{
  rdr::RandomStream* rs = (rdr::RandomStream*)ctx;
  if (!rs->hasData(length))
    throw ConnFailedException("failed to encrypt random");
  rs->readBytes(dst, length);
}

void SSecurityRSAAES::writeRandom()
{
  rdr::OutStream* os = sc->getOutStream();
  if (!rs.hasData(keySize / 8))
    throw ConnFailedException("failed to generate random");
  rs.readBytes(serverRandom, keySize / 8);
  mpz_t x;
  mpz_init(x);
  int res;
  try {
    res = rsa_encrypt(&clientKey, &rs, random_func, keySize / 8,
                      serverRandom, x);
  } catch (...) {
    mpz_clear(x);
    throw;
  }
  if (!res) {
    mpz_clear(x);
    throw ConnFailedException("failed to encrypt random");
  }
  uint8_t* buffer = new uint8_t[clientKey.size];
  nettle_mpz_get_str_256(clientKey.size, buffer, x);
  mpz_clear(x);
  os->writeU16(clientKey.size);
  os->writeBytes(buffer, clientKey.size);
  os->flush();
  delete[] buffer;
}

bool SSecurityRSAAES::readRandom()
{
  rdr::InStream* is = sc->getInStream();
  if (!is->hasData(2))
    return false;
  is->setRestorePoint();
  size_t size = is->readU16();
  if (size != serverKey.size)
    throw ConnFailedException("server key length doesn't match");
  if (!is->hasDataOrRestore(size))
    return false;
  is->clearRestorePoint();
  uint8_t* buffer = new uint8_t[size];
  is->readBytes(buffer, size);
  size_t randomSize = keySize / 8;
  mpz_t x;
  nettle_mpz_init_set_str_256_u(x, size, buffer);
  delete[] buffer;
  if (!rsa_decrypt(&serverKey, &randomSize, clientRandom, x) ||
    randomSize != (size_t)keySize / 8) {
    mpz_clear(x);
    throw ConnFailedException("failed to decrypt client random");
  }
  mpz_clear(x);
  return true;
}

void SSecurityRSAAES::setCipher()
{
  rawis = sc->getInStream();
  rawos = sc->getOutStream();
  uint8_t key[32];
  if (keySize == 128) {
    struct sha1_ctx ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, 16, serverRandom);
    sha1_update(&ctx, 16, clientRandom);
    sha1_digest(&ctx, 16, key);
    rais = new rdr::AESInStream(rawis, key, 128);
    sha1_init(&ctx);
    sha1_update(&ctx, 16, clientRandom);
    sha1_update(&ctx, 16, serverRandom);
    sha1_digest(&ctx, 16, key);
    raos = new rdr::AESOutStream(rawos, key, 128);
  } else {
    struct sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, 32, serverRandom);
    sha256_update(&ctx, 32, clientRandom);
    sha256_digest(&ctx, 32, key);
    rais = new rdr::AESInStream(rawis, key, 256);
    sha256_init(&ctx);
    sha256_update(&ctx, 32, clientRandom);
    sha256_update(&ctx, 32, serverRandom);
    sha256_digest(&ctx, 32, key);
    raos = new rdr::AESOutStream(rawos, key, 256);
  }
  if (isAllEncrypted)
    sc->setStreams(rais, raos);
}

void SSecurityRSAAES::writeHash()
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
    sha1_update(&ctx, 4, lenServerKey);
    sha1_update(&ctx, serverKey.size, serverKeyN);
    sha1_update(&ctx, serverKey.size, serverKeyE);
    sha1_update(&ctx, 4, lenClientKey);
    sha1_update(&ctx, clientKey.size, clientKeyN);
    sha1_update(&ctx, clientKey.size, clientKeyE);
    sha1_digest(&ctx, hashSize, hash);
  } else {
    hashSize = 32;
    struct sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, 4, lenServerKey);
    sha256_update(&ctx, serverKey.size, serverKeyN);
    sha256_update(&ctx, serverKey.size, serverKeyE);
    sha256_update(&ctx, 4, lenClientKey);
    sha256_update(&ctx, clientKey.size, clientKeyN);
    sha256_update(&ctx, clientKey.size, clientKeyE);
    sha256_digest(&ctx, hashSize, hash);
  }
  raos->writeBytes(hash, hashSize);
  raos->flush();
}

bool SSecurityRSAAES::readHash()
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
    sha1_update(&ctx, 4, lenClientKey);
    sha1_update(&ctx, clientKey.size, clientKeyN);
    sha1_update(&ctx, clientKey.size, clientKeyE);
    sha1_update(&ctx, 4, lenServerKey);
    sha1_update(&ctx, serverKey.size, serverKeyN);
    sha1_update(&ctx, serverKey.size, serverKeyE);
    sha1_digest(&ctx, hashSize, realHash);
  } else {
    struct sha256_ctx ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, 4, lenClientKey);
    sha256_update(&ctx, clientKey.size, clientKeyN);
    sha256_update(&ctx, clientKey.size, clientKeyE);
    sha256_update(&ctx, 4, lenServerKey);
    sha256_update(&ctx, serverKey.size, serverKeyN);
    sha256_update(&ctx, serverKey.size, serverKeyE);
    sha256_digest(&ctx, hashSize, realHash);
  }
  if (memcmp(hash, realHash, hashSize) != 0)
    throw ConnFailedException("hash doesn't match");
  return true;
}

void SSecurityRSAAES::clearSecrets()
{
  rsa_private_key_clear(&serverKey);
  rsa_public_key_clear(&clientKey);
  serverKey.size = 0;
  clientKey.size = 0;
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

void SSecurityRSAAES::writeSubtype()
{
  if (requireUsername)
    raos->writeU8(secTypeRA2UserPass);
  else
    raos->writeU8(secTypeRA2Pass);
  raos->flush();
}

bool SSecurityRSAAES::readCredentials()
{
  rais->setRestorePoint();
  if (!rais->hasData(1))
    return false;
  uint8_t lenUsername = rais->readU8();
  if (!rais->hasDataOrRestore(lenUsername + 1))
    return false;
  rais->readBytes((uint8_t*)username, lenUsername);
  username[lenUsername] = 0;
  uint8_t lenPassword = rais->readU8();
  if (!rais->hasDataOrRestore(lenPassword))
    return false;
  rais->readBytes((uint8_t*)password, lenPassword);
  password[lenPassword] = 0;
  rais->clearRestorePoint();
  return true;
}

void SSecurityRSAAES::verifyUserPass()
{
#ifndef __APPLE__
#ifdef WIN32
  WinPasswdValidator* valid = new WinPasswdValidator();
#elif !defined(__APPLE__)
  UnixPasswordValidator *valid = new UnixPasswordValidator();
#endif
  if (!valid->validate(sc, username, password)) {
    delete valid;
    throw AuthFailureException("Authentication failed");
  }
  delete valid;
#else
  throw Exception("No password validator configured");
#endif
}

void SSecurityRSAAES::verifyPass()
{
  VncAuthPasswdGetter* pg = &SSecurityVncAuth::vncAuthPasswd;
  std::string passwd, passwdReadOnly;
  pg->getVncAuthPasswd(&passwd, &passwdReadOnly);

  if (passwd.empty())
    throw Exception("No password configured");

  if (password == passwd) {
    accessRights = AccessDefault;
    return;
  }

  if (!passwdReadOnly.empty() && password == passwdReadOnly) {
    accessRights = AccessView;
    return;
  }

  throw AuthFailureException("Authentication failed");
}

const char* SSecurityRSAAES::getUserName() const
{
  return username;
}
