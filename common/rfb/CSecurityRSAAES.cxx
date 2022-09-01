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

CSecurityRSAAES::CSecurityRSAAES(CConnection* cc, rdr::U32 _secType,
                                 int _keySize, bool _isAllEncrypted)
  : CSecurity(cc), state(ReadPublicKey),
    keySize(_keySize), isAllEncrypted(_isAllEncrypted), secType(_secType),
    clientKey(), clientPublicKey(), serverKey(),
    serverKeyN(NULL), serverKeyE(NULL),
    clientKeyN(NULL), clientKeyE(NULL),
    rais(NULL), raos(NULL), rawis(NULL), rawos(NULL)
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
      if (readPublicKey()) {
        verifyServer();
        writePublicKey();
        writeRandom();
        state = ReadRandom;
      }
      return false;
    case ReadRandom:
      if (readRandom()) {
        setCipher();
        writeHash();
        state = ReadHash;
      }
      return false;
    case ReadHash:
      if (readHash()) {
        clearSecrets();
        state = ReadSubtype;
      }
    case ReadSubtype:
      if (readSubtype()) {
        writeCredentials();
        return true;
      }
      return false;
  }
  assert(!"unreachable");
  return false;
}

static void random_func(void* ctx, size_t length, uint8_t* dst)
{
  rdr::RandomStream* rs = (rdr::RandomStream*)ctx;
  if (!rs->hasData(length))
    throw ConnFailedException("failed to generate random");
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
                            &rs, random_func, NULL, NULL, clientKeyLength, 0))
    throw AuthFailureException("failed to generate key");
  clientKeyN = new rdr::U8[rsaKeySize];
  clientKeyE = new rdr::U8[rsaKeySize];
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
    throw AuthFailureException("server key is too short");
  if (serverKeyLength > MaxKeyLength)
    throw AuthFailureException("server key is too long");
  size_t size = (serverKeyLength + 7) / 8;
  if (!is->hasDataOrRestore(size * 2))
    return false;
  is->clearRestorePoint();
  serverKeyE = new rdr::U8[size];
  serverKeyN = new rdr::U8[size];
  is->readBytes(serverKeyN, size);
  is->readBytes(serverKeyE, size);
  rsa_public_key_init(&serverKey);
  nettle_mpz_set_str_256_u(serverKey.n, size, serverKeyN);
  nettle_mpz_set_str_256_u(serverKey.e, size, serverKeyE);
  if (!rsa_public_key_prepare(&serverKey))
    throw AuthFailureException("server key is invalid");
  return true;
}

void CSecurityRSAAES::verifyServer()
{
  rdr::U8 lenServerKey[4] = {
    (rdr::U8)((serverKeyLength & 0xff000000) >> 24),
    (rdr::U8)((serverKeyLength & 0xff0000) >> 16),
    (rdr::U8)((serverKeyLength & 0xff00) >> 8),
    (rdr::U8)(serverKeyLength & 0xff)
  };
  rdr::U8 f[8];
  struct sha1_ctx ctx;
  sha1_init(&ctx);
  sha1_update(&ctx, 4, lenServerKey);
  sha1_update(&ctx, serverKey.size, serverKeyN);
  sha1_update(&ctx, serverKey.size, serverKeyE);
  sha1_digest(&ctx, sizeof(f), f);
  const char *title = "Server key fingerprint";
  CharArray text;
  text.format(
    "The server has provided the following identifying information:\n"
    "Fingerprint: %02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x\n"
    "Please verify that the information is correct and press \"Yes\". "
    "Otherwise press \"No\"", f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7]);
  if (!msg->showMsgBox(UserMsgBox::M_YESNO, title, text.buf))
    throw AuthFailureException("server key mismatch");
}

void CSecurityRSAAES::writeRandom()
{
  rdr::OutStream* os = cc->getOutStream();
  if (!rs.hasData(keySize / 8))
    throw ConnFailedException("failed to generate random");
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
    throw AuthFailureException("failed to encrypt random");
  }
  rdr::U8* buffer = new rdr::U8[serverKey.size];
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
    throw AuthFailureException("client key length doesn't match");
  if (!is->hasDataOrRestore(size))
    return false;
  is->clearRestorePoint();
  rdr::U8* buffer = new rdr::U8[size];
  is->readBytes(buffer, size);
  size_t randomSize = keySize / 8;
  mpz_t x;
  nettle_mpz_init_set_str_256_u(x, size, buffer);
  delete[] buffer;
  if (!rsa_decrypt(&clientKey, &randomSize, serverRandom, x) ||
      randomSize != (size_t)keySize / 8) {
    mpz_clear(x);
    throw AuthFailureException("failed to decrypt server random");
  }
  mpz_clear(x);
  return true;
}

void CSecurityRSAAES::setCipher()
{
  rawis = cc->getInStream();
  rawos = cc->getOutStream();
  rdr::U8 key[32];
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
  rdr::U8 hash[32];
  size_t len = serverKeyLength;
  rdr::U8 lenServerKey[4] = {
    (rdr::U8)((len & 0xff000000) >> 24),
    (rdr::U8)((len & 0xff0000) >> 16),
    (rdr::U8)((len & 0xff00) >> 8),
    (rdr::U8)(len & 0xff)
  };
  len = clientKeyLength;
  rdr::U8 lenClientKey[4] = {
    (rdr::U8)((len & 0xff000000) >> 24),
    (rdr::U8)((len & 0xff0000) >> 16),
    (rdr::U8)((len & 0xff00) >> 8),
    (rdr::U8)(len & 0xff)
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
  rdr::U8 hash[32];
  rdr::U8 realHash[32];
  int hashSize = keySize == 128 ? 20 : 32;
  if (!rais->hasData(hashSize))
    return false;
  rais->readBytes(hash, hashSize);
  size_t len = serverKeyLength;
  rdr::U8 lenServerKey[4] = {
    (rdr::U8)((len & 0xff000000) >> 24),
    (rdr::U8)((len & 0xff0000) >> 16),
    (rdr::U8)((len & 0xff00) >> 8),
    (rdr::U8)(len & 0xff)
  };
  len = clientKeyLength;
  rdr::U8 lenClientKey[4] = {
    (rdr::U8)((len & 0xff000000) >> 24),
    (rdr::U8)((len & 0xff0000) >> 16),
    (rdr::U8)((len & 0xff00) >> 8),
    (rdr::U8)(len & 0xff)
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
    throw AuthFailureException("hash doesn't match");
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
  serverKeyN = NULL;
  serverKeyE = NULL;
  clientKeyN = NULL;
  clientKeyE = NULL;
  memset(serverRandom, 0, sizeof(serverRandom));
  memset(clientRandom, 0, sizeof(clientRandom));
}

bool CSecurityRSAAES::readSubtype()
{
  if (!rais->hasData(1))
    return false;
  subtype = rais->readU8();
  if (subtype != secTypeRA2UserPass && subtype != secTypeRA2Pass)
    throw AuthFailureException("unknown RSA-AES subtype");
  return true;
}

void CSecurityRSAAES::writeCredentials()
{
  CharArray username;
  CharArray password;

  (CSecurity::upg)->getUserPasswd(
    isSecure(),
    subtype == secTypeRA2UserPass ? &username.buf : NULL, &password.buf
  );
  size_t len;
  if (username.buf) {
    len = strlen(username.buf);
    if (len > 255)
      throw AuthFailureException("username is too long");
    raos->writeU8(len);
    if (len)
      raos->writeBytes(username.buf, len);
  } else {
    raos->writeU8(0);
  }
  len = strlen(password.buf);
  if (len > 255)
    throw AuthFailureException("password is too long");
  raos->writeU8(len);
  if (len)
    raos->writeBytes(password.buf, len);
  raos->flush();
}