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

#ifndef OS_TLS_H
#define OS_TLS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*
 * Windows builds are build against fairly new GNUTLS, ignore compatibility
 * code.
 */
#if defined(HAVE_GNUTLS) && !defined(WIN32)
#include <gnutls/gnutls.h>

#ifndef HAVE_GNUTLS_DATUM_T
typedef gnutls_datum gnutls_datum_t;
#endif
#ifndef HAVE_GNUTLS_CRT_T
typedef gnutls_x509_crt gnutls_x509_crt_t;
#endif
#ifndef HAVE_GNUTLS_PK_ALGORITHM_T
typedef gnutls_pk_algorithm gnutls_pk_algorithm_t;
#endif
#ifndef HAVE_GNUTLS_SIGN_ALGORITHM_T
typedef gnutls_sign_algorithm gnutls_sign_algorithm_t;
#endif

#ifndef HAVE_GNUTLS_X509_CRT_PRINT

typedef enum {
	GNUTLS_CRT_PRINT_ONELINE = 1
} gnutls_certificate_print_formats_t;

/*
 * Prints certificate in human-readable form.
 */
int
gnutls_x509_crt_print(gnutls_x509_crt_t cert,
		      gnutls_certificate_print_formats_t format,
		      gnutls_datum_t * out);
#endif /* HAVE_GNUTLS_X509_CRT_PRINT */
#endif /* HAVE_GNUTLS */

#endif /* OS_TLS_H */

