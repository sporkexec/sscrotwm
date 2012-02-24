/*
 * Copyright (c) 2012 Jacob Courtneay <jacob@sporkexec.com>
 * Copyright (c) 2011 Conformal Systems LLC <info@conformal.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#ifndef SSCROTWM_VERSION_H
#define SSCROTWM_VERSION_H

#define SSCROTWM_STR(x)		#x
#define SSCROTWM_STRINGIZE(x)	SSCROTWM_STR(x)

#define SSCROTWM_MAJOR		1
#define SSCROTWM_MINOR		0
#define SSCROTWM_PATCH		0
#define SSCROTWM_VERSION		SSCROTWM_STRINGIZE(SSCROTWM_MAJOR) "." \
				SSCROTWM_STRINGIZE(SSCROTWM_MINOR) "." \
				SSCROTWM_STRINGIZE(SSCROTWM_PATCH)

#endif /* SSCROTWM_VERSION_H */

