/* $XFree86$
 * This module converts keysym values into the corresponding ISO 10646
 * (UCS, Unicode) values.
 *
 * The keysym -> UTF-8 conversion will hopefully one day be provided
 * by Xlib via XmbLookupString() and should ideally not have to be
 * done in X applications. But we are not there yet.
 *
 * We allow to represent any UCS character in the range U-00000000 to
 * U-00FFFFFF by a keysym value in the range 0x01000000 to 0x01ffffff.
 * This admittedly does not cover the entire 31-bit space of UCS, but
 * it does cover all of the characters up to U-10FFFF, which can be
 * represented by UTF-16, and more, and it is very unlikely that higher
 * UCS codes will ever be assigned by ISO. So to get Unicode character
 * U+ABCD you can directly use keysym 0x0100abcd.
 *
 * Author: Markus G. Kuhn <http://www.cl.cam.ac.uk/~mgk25/>,
 *         University of Cambridge, April 2001
 *
 * Special thanks to Richard Verhoeven <river@win.tue.nl> for preparing
 * an initial draft of the mapping table.
 *
 * This software is in the public domain. Share and enjoy!
 */

#include "keysym2ucs.h"
#include "keyucsmap.h"

#define NoSymbol 0

struct combiningpair {
  unsigned short spacing;
  unsigned short combining;
};

static const struct codepair deadtab[] = {
  { 0xfe50, 0x0300 }, /*                  dead_grave ` COMBINING GRAVE ACCENT */
  { 0xfe51, 0x0301 }, /*                  dead_acute ´ COMBINING ACUTE ACCENT */
  { 0xfe52, 0x0302 }, /*             dead_circumflex ^ COMBINING CIRCUMFLEX ACCENT */
  { 0xfe53, 0x0303 }, /*                  dead_tilde ~ COMBINING TILDE */
  { 0xfe54, 0x0304 }, /*                 dead_macron ¯ COMBINING MACRON */
  { 0xfe55, 0x0306 }, /*                  dead_breve ˘ COMBINING BREVE */
  { 0xfe56, 0x0307 }, /*               dead_abovedot ˙ COMBINING DOT ABOVE */
  { 0xfe57, 0x0308 }, /*              dead_diaeresis ¨ COMBINING DIAERESIS */
  { 0xfe58, 0x030a }, /*              dead_abovering ˚ COMBINING RING ABOVE */
  { 0xfe59, 0x030b }, /*            dead_doubleacute ˝ COMBINING DOUBLE ACUTE ACCENT */
  { 0xfe5a, 0x030c }, /*                  dead_caron ˇ COMBINING CARON */
  { 0xfe5b, 0x0327 }, /*                dead_cedilla ¸ COMBINING CEDILLA */
  { 0xfe5c, 0x0328 }, /*                 dead_ogonek ¸ COMBINING OGONEK */
  { 0xfe5d, 0x0345 }, /*                   dead_iota ͺ COMBINING GREEK YPOGEGRAMMENI */
  { 0xfe5e, 0x3099 }, /*           dead_voiced_sound ゛ COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK */
  { 0xfe5f, 0x309a }, /*       dead_semivoiced_sound ゜ COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
  { 0xfe60, 0x0323 }, /*               dead_belowdot . COMBINING DOT BELOW */
};

static const struct combiningpair combinetab[] = {
  { 0x0060, 0x0300 }, /*                GRAVE ACCENT ` COMBINING GRAVE ACCENT */
  { 0x00b4, 0x0301 }, /*                ACUTE ACCENT ´ COMBINING ACUTE ACCENT */
  { 0x0027, 0x0301 }, /*                  APOSTROPHE ' COMBINING ACUTE ACCENT */
  { 0x0384, 0x0301 }, /*                 GREEK TONOS ΄ COMBINING ACUTE ACCENT */
  { 0x005e, 0x0302 }, /*           CIRCUMFLEX ACCENT ^ COMBINING CIRCUMFLEX ACCENT */
  { 0x007e, 0x0303 }, /*                       TILDE ~ COMBINING TILDE */
  { 0x00af, 0x0304 }, /*                      MACRON ¯ COMBINING MACRON */
  { 0x02d8, 0x0306 }, /*                       BREVE ˘ COMBINING BREVE */
  { 0x02d9, 0x0307 }, /*                   DOT ABOVE ˙ COMBINING DOT ABOVE */
  { 0x00a8, 0x0308 }, /*                   DIAERESIS ¨ COMBINING DIAERESIS */
  { 0x0022, 0x0308 }, /*              QUOTATION MARK " COMBINING DIAERESIS */
  { 0x02da, 0x030a }, /*                  RING ABOVE ˚ COMBINING RING ABOVE */
  { 0x00b0, 0x030a }, /*                 DEGREE SIGN ° COMBINING RING ABOVE */
  { 0x02dd, 0x030b }, /*         DOUBLE ACUTE ACCENT ˝ COMBINING DOUBLE ACUTE ACCENT */
  { 0x02c7, 0x030c }, /*                       CARON ˇ COMBINING CARON */
  { 0x00b8, 0x0327 }, /*                     CEDILLA ¸ COMBINING CEDILLA */
  { 0x02db, 0x0328 }, /*                      OGONEK ¸ COMBINING OGONEK */
  { 0x037a, 0x0345 }, /*         GREEK YPOGEGRAMMENI ͺ COMBINING GREEK YPOGEGRAMMENI */
  { 0x309b, 0x3099 }, /* KATAKANA-HIRAGANA VOICED SOUND MARK ゛COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK */
  { 0x309c, 0x309a }, /* KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK ゜COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
  { 0x002e, 0x0323 }, /*                   FULL STOP . COMBINING DOT BELOW */
  { 0x0385, 0x0344 }, /*       GREEK DIALYTIKA TONOS ΅ COMBINING GREEK DIALYTIKA TONOS */
};

static unsigned find_ucs(unsigned keysym,
                         const struct codepair *table, int entries)
{
  int min = 0;
  int max = entries - 1;
  int mid;

  /* binary search in table */
  while (max >= min) {
    mid = (min + max) / 2;
    if (table[mid].keysym < keysym)
      min = mid + 1;
    else if (table[mid].keysym > keysym)
      max = mid - 1;
    else {
      /* found it */
      return table[mid].ucs;
    }
  }

  return -1;
}

static unsigned find_sym(unsigned ucs,
                         const struct codepair *table, int entries)
{
  int cur = 0;
  int max = entries - 1;

  /* linear search in table */
  while (cur <= max) {
    if (table[cur].ucs == ucs)
      return table[cur].keysym;
    cur++;
  }

  return NoSymbol;
}

unsigned keysym2ucs(unsigned keysym)
{
  unsigned ucs;

  /* first check for Latin-1 characters (1:1 mapping) */
  if ((keysym >= 0x0020 && keysym <= 0x007e) ||
      (keysym >= 0x00a0 && keysym <= 0x00ff))
    return keysym;

  /* also check for directly encoded 24-bit UCS characters */
  if ((keysym & 0xff000000) == 0x01000000)
    return keysym & 0x00ffffff;

  /* normal key? */
  ucs = find_ucs(keysym, keysymtab,
                 sizeof(keysymtab) / sizeof(struct codepair));
  if (ucs != (unsigned)-1)
    return ucs;

  /* dead key? */
  ucs = find_ucs(keysym, deadtab,
                 sizeof(deadtab) / sizeof(struct codepair));
  if (ucs != (unsigned)-1)
    return ucs;

  /* no matching Unicode value found */
  return -1;
}

unsigned ucs2keysym(unsigned ucs)
{
  unsigned keysym;

  /* first check for Latin-1 characters (1:1 mapping) */
  if ((ucs >= 0x0020 && ucs <= 0x007e) ||
      (ucs >= 0x00a0 && ucs <= 0x00ff))
    return ucs;

  /* normal key? */
  keysym = find_sym(ucs, keysymtab,
                    sizeof(keysymtab) / sizeof(struct codepair));
  if (keysym != NoSymbol)
    return keysym;

  /* dead key? */
  keysym = find_sym(ucs, deadtab,
                    sizeof(deadtab) / sizeof(struct codepair));
  if (keysym != NoSymbol)
    return keysym;

  /* us the directly encoded 24-bit UCS character */
  if ((ucs & 0xff000000) == 0)
    return ucs | 0x01000000;

  /* no matching keysym value found */
  return NoSymbol;
}

unsigned ucs2combining(unsigned spacing)
{
  int cur = 0;
  int max = sizeof(combinetab) / sizeof(struct combiningpair) - 1;

  /* linear search in table */
  while (cur <= max) {
    if (combinetab[cur].spacing == spacing)
      return combinetab[cur].combining;
    cur++;
  }

  /* no matching Unicode value found */
  return -1;
}
