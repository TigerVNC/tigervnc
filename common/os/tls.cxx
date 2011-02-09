/* Copyright (C) 2011 TightVNC Team.  All Rights Reserved.
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

#include <os/tls.h>

#include <iomanip>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <sys/types.h>
#include <time.h>

using namespace std;

#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#include <gnutls/x509.h>

#ifndef HAVE_GNUTLS_X509_CRT_PRINT

#define UNKNOWN_SUBJECT(err) \
	do { \
		ss << "unknown subject (" << gnutls_strerror(err) << "), "; \
	} while (0)

#define UNKNOWN_ISSUER(err) \
	do { \
		ss << "unknown issuer (" << gnutls_strerror(err) << "), "; \
	} while (0)


static void
hexprint(ostringstream &ss, const char *data, size_t len)
{
	size_t j;
	char tmp[3];

	if (len == 0)
		ss << "00";
	else {
		for (j = 0; j < len; j++) {
			snprintf(tmp, sizeof(tmp), "%.2x", (unsigned char) data[j]);
			ss << tmp;
		}
	}
}

/* Implementation based on gnutls_x509_crt_print from GNUTLS */
int
gnutls_x509_crt_print(gnutls_x509_crt_t cert,
		      gnutls_certificate_print_formats_t format,
		      gnutls_datum_t * out)
{
	ostringstream ss;
	
	int err;

	char *dn;
	size_t dn_size = 0;

	/* Subject */
	err = gnutls_x509_crt_get_dn(cert, NULL, &dn_size);
	if (err != GNUTLS_E_SHORT_MEMORY_BUFFER)
		UNKNOWN_SUBJECT(err);
	else {
		dn = (char *)malloc(dn_size);
		if (dn == NULL) {
			UNKNOWN_SUBJECT(GNUTLS_E_MEMORY_ERROR);
		} else {
			err = gnutls_x509_crt_get_dn(cert, dn, &dn_size);
			if (err < 0) {
				UNKNOWN_SUBJECT(err);
			} else
				ss << "subject `" << dn << "', ";
			free(dn);
		}
	}

	/* Issuer */
	dn = NULL;
	dn_size = 0;
	err = gnutls_x509_crt_get_issuer_dn(cert, NULL, &dn_size);
	if (err != GNUTLS_E_SHORT_MEMORY_BUFFER)
		UNKNOWN_ISSUER(err);
	else {
		dn = (char *)malloc(dn_size);
		if (dn == NULL) {
			UNKNOWN_ISSUER(GNUTLS_E_MEMORY_ERROR);
		} else {
			err = gnutls_x509_crt_get_issuer_dn(cert, dn, &dn_size);
			if (err < 0)
				UNKNOWN_ISSUER(err);
			else
				ss << "issuer `" << dn << "', ";
			free(dn);
		}
	}

	/* Key algorithm and size */
	unsigned int bits;
	const char *name;
	name = gnutls_pk_algorithm_get_name( (gnutls_pk_algorithm_t)
		gnutls_x509_crt_get_pk_algorithm(cert, &bits));
	if (name == NULL)
		name = "Unknown";
	ss << name << " key " << bits << " bits, ";

	/* Signature algorithm */
	err = gnutls_x509_crt_get_signature_algorithm(cert);
	if (err < 0) {
		ss << "unknown signature algorithm (" << gnutls_strerror(err)
		   << "), ";
	} else {
		const char *name;
		name = gnutls_sign_algorithm_get_name((gnutls_sign_algorithm_t)err);
		if (name == NULL)
			name = "Unknown";

		ss << "signed using " << name;
		if (err == GNUTLS_SIGN_RSA_MD5 || err == GNUTLS_SIGN_RSA_MD2)
			ss << " (broken!)";
		ss << ", ";
	}

	/* Validity */
	time_t tim;
	char s[42];
	size_t max = sizeof(s);
	struct tm t;

	tim = gnutls_x509_crt_get_activation_time(cert);
	if (gmtime_r(&tim, &t) == NULL)
		ss << "unknown activation (" << (unsigned long) tim << ")";
	else if (strftime(s, max, "%Y-%m-%d %H:%M:%S UTC", &t) == 0)
		ss << "failed activation (" << (unsigned long) tim << ")";
	else
		ss << "activated `" << s << "'";
	ss << ", ";

	tim = gnutls_x509_crt_get_expiration_time(cert);
	if (gmtime_r(&tim, &t) == NULL)
		ss << "unknown expiry (" << (unsigned long) tim << ")";
	else if (strftime(s, max, "%Y-%m-%d %H:%M:%S UTC", &t) == 0)
		ss << "failed expiry (" << (unsigned long) tim << ")";
	else
		ss << "expires `" << s << "'";
	ss << ", ";

	/* Fingerprint */
	char buffer[20];
	size_t size = sizeof(buffer);

	err = gnutls_x509_crt_get_fingerprint(cert, GNUTLS_DIG_SHA1, buffer, &size);
	if (err < 0)
		ss << "unknown fingerprint (" << gnutls_strerror(err) << ")";
	else {
		ss << "SHA-1 fingerprint `";
		hexprint(ss, buffer, size);
		ss << "'";
	}

	out->data = (unsigned char *) strdup(ss.str().c_str());
	if (out->data == NULL)
		return GNUTLS_E_MEMORY_ERROR;
	out->size = strlen((char *)out->data);

	return 0;
}

#endif /* HAVE_GNUTLS_X509_CRT_PRINT */

#endif /* HAVE_GNUTLS */

