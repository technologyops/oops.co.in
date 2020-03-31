/*
 * Copyright (C) 2004-2005 Pawel Foremski <pjf@asn.pl>
 *
 * This program is free software; you can redistribute it and/or 
 * modify it under the terms of the GNU General Public License 
 * as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later 
 * version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <sys/types.h>
#include <sys/stat.h>
#include "stralloc.h"
#include "fmt.h"
#include "auto_break.h"
#include "auto_qmail.h"
#include "auto_uids.h"
#include "auto_usera.h"
#include "dirqmail.h"

static stralloc home = {0};
static char num[FMT_ULONG];
static struct stat st;

static int dq_clean(str)
char *str;
{
  unsigned int i = 0;

  for(;;) switch (str[i++]) {
    case 0: return 1;
    case '/': return 0;
    case '.': if (str[i] == '.') return 0;
  }
}

static void dq_save(dest, local, dash, ext)
stralloc *dest; char *local, *dash, *ext;
{
  if (!stralloc_copys(dest, "")) return;
  if (!stralloc_cats(dest, local)) return;
  if (!stralloc_0(dest)) return;
  if (!stralloc_cats(dest, num, fmt_ulong(num, (unsigned long) auto_uida))) return;
  if (!stralloc_0(dest)) return;
  if (!stralloc_cats(dest, num, fmt_ulong(num, (unsigned long) auto_gidn))) return;
  if (!stralloc_0(dest)) return;
  if (!stralloc_cats(dest, home.s)) return;
  if (!stralloc_0(dest)) return;
  if (!stralloc_cats(dest, dash)) return;
  if (!stralloc_0(dest)) return;
  if (!stralloc_cats(dest, ext)) return;
  if (!stralloc_0(dest)) return;
}

static int dq_checkhome()
{
  if (stat(home.s, &st) == 0) {
#ifndef NOAUTOMAILDIR
    if (st.st_uid != auto_uida || st.st_gid != auto_gidn) chown(home.s, auto_uida, auto_gidn);
    if ((st.st_mode & S_IRWXG) || (st.st_mode & S_IRWXO)) chmod(home.s, S_IRWXU);
#endif
    return 1;
  }

  return 0;
}

int dq_localb(host, len)
char *host; int len;
{
  static stralloc host_0 = {0};
  if (!stralloc_copyb(&host_0, host, len)) return 0;
  if (!stralloc_0(&host_0)) return 0;
  return dq_locals(host_0.s);
}

int dq_locals(host)
char *host;
{
  if (!dq_clean(host)) return 0;

  if (!stralloc_copys(&home, auto_qmail)) return 0;
  if (!stralloc_cats(&home, "/mail/")) return 0;
  if (!stralloc_cats(&home, host)) return 0;
  if (!stralloc_0(&home)) return 0;

  if (stat(home.s, &st) == -1)
    return 0;
  else
    return 1;
}

int dq_get(dest, local, host)
stralloc *dest; char *local, *host;
{
  static int ext, l;

  if (!dq_clean(local)) return 0;
  if (!dq_locals(host)) return 0; /* sets home */

  home.len--; /* set by dq_local */
  if (!stralloc_cats(&home, "/")) return 0;

  /** local@domain **/
  l = home.len; /* point on local */
  if (!stralloc_cats(&home, local)) return 0;
  if (!stralloc_0(&home)) return 0;

  if (dq_checkhome()) { dq_save(dest, home.s + l, "", ""); return 1; }

  /* need to find the extension */
  ext = home.len - 1;
  do {
    if (home.s[ext] != *auto_break) continue;
    home.s[ext] = 0;
    if (dq_checkhome()) { dq_save(dest, home.s + l, "-", home.s + ext + 1); return 1; }
    home.s[ext] = *auto_break;
  } while (ext-- > l);

  /** alias@domain **/
  home.len = l; /* point on local */
  if (!stralloc_cats(&home, "alias")) return 0;
  if (!stralloc_0(&home)) return 0;

  if (dq_checkhome()) { dq_save(dest, auto_usera, "-", local); return 1; }

  return 0;
}
