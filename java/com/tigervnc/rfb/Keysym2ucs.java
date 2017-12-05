/* Copyright (C) 2017 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 *
 * This module converts keysym values into the corresponding ISO 10646
 * (UCS, Unicode) values.
 *
 * The array keysymtab[] contains pairs of X11 keysym values for graphical
 * characters and the corresponding Unicode value. The function
 * keysym2ucs() maps a keysym onto a Unicode value using a binary search,
 * therefore keysymtab[] must remain SORTED by keysym value.
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
 * NOTE: The comments in the table below contain the actual character
 * encoded in UTF-8, so for viewing and editing best use an editor in
 * UTF-8 mode.
 *
 * Derived from keysym2ucs.c, originally authored by Markus G, Kuhn
 *
 */

package com.tigervnc.rfb;

public class Keysym2ucs {

  private static class codepair {
    public codepair(int keysym_, int ucs_) {
      keysym = keysym_;
      ucs = ucs_;
    }
    int keysym;
    int ucs;
  }

  private static class combiningpair {
    public combiningpair(int spacing_, int combining_) {
      spacing = spacing_;
      combining = combining_;
    }
    int spacing;
    int combining;
  }

  public static codepair[] keysymtab = {
    new codepair(0x01a1, 0x0104), /*                     Aogonek Ą LATIN CAPITAL LETTER A WITH OGONEK */
    new codepair(0x01a2, 0x02d8), /*                       breve ˘ BREVE */
    new codepair(0x01a3, 0x0141), /*                     Lstroke Ł LATIN CAPITAL LETTER L WITH STROKE */
    new codepair(0x01a5, 0x013d), /*                      Lcaron Ľ LATIN CAPITAL LETTER L WITH CARON */
    new codepair(0x01a6, 0x015a), /*                      Sacute Ś LATIN CAPITAL LETTER S WITH ACUTE */
    new codepair(0x01a9, 0x0160), /*                      Scaron Š LATIN CAPITAL LETTER S WITH CARON */
    new codepair(0x01aa, 0x015e), /*                    Scedilla Ş LATIN CAPITAL LETTER S WITH CEDILLA */
    new codepair(0x01ab, 0x0164), /*                      Tcaron Ť LATIN CAPITAL LETTER T WITH CARON */
    new codepair(0x01ac, 0x0179), /*                      Zacute Ź LATIN CAPITAL LETTER Z WITH ACUTE */
    new codepair(0x01ae, 0x017d), /*                      Zcaron Ž LATIN CAPITAL LETTER Z WITH CARON */
    new codepair(0x01af, 0x017b), /*                   Zabovedot Ż LATIN CAPITAL LETTER Z WITH DOT ABOVE */
    new codepair(0x01b1, 0x0105), /*                     aogonek ą LATIN SMALL LETTER A WITH OGONEK */
    new codepair(0x01b2, 0x02db), /*                      ogonek ˛ OGONEK */
    new codepair(0x01b3, 0x0142), /*                     lstroke ł LATIN SMALL LETTER L WITH STROKE */
    new codepair(0x01b5, 0x013e), /*                      lcaron ľ LATIN SMALL LETTER L WITH CARON */
    new codepair(0x01b6, 0x015b), /*                      sacute ś LATIN SMALL LETTER S WITH ACUTE */
    new codepair(0x01b7, 0x02c7), /*                       caron ˇ CARON */
    new codepair(0x01b9, 0x0161), /*                      scaron š LATIN SMALL LETTER S WITH CARON */
    new codepair(0x01ba, 0x015f), /*                    scedilla ş LATIN SMALL LETTER S WITH CEDILLA */
    new codepair(0x01bb, 0x0165), /*                      tcaron ť LATIN SMALL LETTER T WITH CARON */
    new codepair(0x01bc, 0x017a), /*                      zacute ź LATIN SMALL LETTER Z WITH ACUTE */
    new codepair(0x01bd, 0x02dd), /*                 doubleacute ˝ DOUBLE ACUTE ACCENT */
    new codepair(0x01be, 0x017e), /*                      zcaron ž LATIN SMALL LETTER Z WITH CARON */
    new codepair(0x01bf, 0x017c), /*                   zabovedot ż LATIN SMALL LETTER Z WITH DOT ABOVE */
    new codepair(0x01c0, 0x0154), /*                      Racute Ŕ LATIN CAPITAL LETTER R WITH ACUTE */
    new codepair(0x01c3, 0x0102), /*                      Abreve Ă LATIN CAPITAL LETTER A WITH BREVE */
    new codepair(0x01c5, 0x0139), /*                      Lacute Ĺ LATIN CAPITAL LETTER L WITH ACUTE */
    new codepair(0x01c6, 0x0106), /*                      Cacute Ć LATIN CAPITAL LETTER C WITH ACUTE */
    new codepair(0x01c8, 0x010c), /*                      Ccaron Č LATIN CAPITAL LETTER C WITH CARON */
    new codepair(0x01ca, 0x0118), /*                     Eogonek Ę LATIN CAPITAL LETTER E WITH OGONEK */
    new codepair(0x01cc, 0x011a), /*                      Ecaron Ě LATIN CAPITAL LETTER E WITH CARON */
    new codepair(0x01cf, 0x010e), /*                      Dcaron Ď LATIN CAPITAL LETTER D WITH CARON */
    new codepair(0x01d0, 0x0110), /*                     Dstroke Đ LATIN CAPITAL LETTER D WITH STROKE */
    new codepair(0x01d1, 0x0143), /*                      Nacute Ń LATIN CAPITAL LETTER N WITH ACUTE */
    new codepair(0x01d2, 0x0147), /*                      Ncaron Ň LATIN CAPITAL LETTER N WITH CARON */
    new codepair(0x01d5, 0x0150), /*                Odoubleacute Ő LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
    new codepair(0x01d8, 0x0158), /*                      Rcaron Ř LATIN CAPITAL LETTER R WITH CARON */
    new codepair(0x01d9, 0x016e), /*                       Uring Ů LATIN CAPITAL LETTER U WITH RING ABOVE */
    new codepair(0x01db, 0x0170), /*                Udoubleacute Ű LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
    new codepair(0x01de, 0x0162), /*                    Tcedilla Ţ LATIN CAPITAL LETTER T WITH CEDILLA */
    new codepair(0x01e0, 0x0155), /*                      racute ŕ LATIN SMALL LETTER R WITH ACUTE */
    new codepair(0x01e3, 0x0103), /*                      abreve ă LATIN SMALL LETTER A WITH BREVE */
    new codepair(0x01e5, 0x013a), /*                      lacute ĺ LATIN SMALL LETTER L WITH ACUTE */
    new codepair(0x01e6, 0x0107), /*                      cacute ć LATIN SMALL LETTER C WITH ACUTE */
    new codepair(0x01e8, 0x010d), /*                      ccaron č LATIN SMALL LETTER C WITH CARON */
    new codepair(0x01ea, 0x0119), /*                     eogonek ę LATIN SMALL LETTER E WITH OGONEK */
    new codepair(0x01ec, 0x011b), /*                      ecaron ě LATIN SMALL LETTER E WITH CARON */
    new codepair(0x01ef, 0x010f), /*                      dcaron ď LATIN SMALL LETTER D WITH CARON */
    new codepair(0x01f0, 0x0111), /*                     dstroke đ LATIN SMALL LETTER D WITH STROKE */
    new codepair(0x01f1, 0x0144), /*                      nacute ń LATIN SMALL LETTER N WITH ACUTE */
    new codepair(0x01f2, 0x0148), /*                      ncaron ň LATIN SMALL LETTER N WITH CARON */
    new codepair(0x01f5, 0x0151), /*                odoubleacute ő LATIN SMALL LETTER O WITH DOUBLE ACUTE */
    new codepair(0x01f8, 0x0159), /*                      rcaron ř LATIN SMALL LETTER R WITH CARON */
    new codepair(0x01f9, 0x016f), /*                       uring ů LATIN SMALL LETTER U WITH RING ABOVE */
    new codepair(0x01fb, 0x0171), /*                udoubleacute ű LATIN SMALL LETTER U WITH DOUBLE ACUTE */
    new codepair(0x01fe, 0x0163), /*                    tcedilla ţ LATIN SMALL LETTER T WITH CEDILLA */
    new codepair(0x01ff, 0x02d9), /*                    abovedot ˙ DOT ABOVE */
    new codepair(0x02a1, 0x0126), /*                     Hstroke Ħ LATIN CAPITAL LETTER H WITH STROKE */
    new codepair(0x02a6, 0x0124), /*                 Hcircumflex Ĥ LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
    new codepair(0x02a9, 0x0130), /*                   Iabovedot İ LATIN CAPITAL LETTER I WITH DOT ABOVE */
    new codepair(0x02ab, 0x011e), /*                      Gbreve Ğ LATIN CAPITAL LETTER G WITH BREVE */
    new codepair(0x02ac, 0x0134), /*                 Jcircumflex Ĵ LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
    new codepair(0x02b1, 0x0127), /*                     hstroke ħ LATIN SMALL LETTER H WITH STROKE */
    new codepair(0x02b6, 0x0125), /*                 hcircumflex ĥ LATIN SMALL LETTER H WITH CIRCUMFLEX */
    new codepair(0x02b9, 0x0131), /*                    idotless ı LATIN SMALL LETTER DOTLESS I */
    new codepair(0x02bb, 0x011f), /*                      gbreve ğ LATIN SMALL LETTER G WITH BREVE */
    new codepair(0x02bc, 0x0135), /*                 jcircumflex ĵ LATIN SMALL LETTER J WITH CIRCUMFLEX */
    new codepair(0x02c5, 0x010a), /*                   Cabovedot Ċ LATIN CAPITAL LETTER C WITH DOT ABOVE */
    new codepair(0x02c6, 0x0108), /*                 Ccircumflex Ĉ LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
    new codepair(0x02d5, 0x0120), /*                   Gabovedot Ġ LATIN CAPITAL LETTER G WITH DOT ABOVE */
    new codepair(0x02d8, 0x011c), /*                 Gcircumflex Ĝ LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
    new codepair(0x02dd, 0x016c), /*                      Ubreve Ŭ LATIN CAPITAL LETTER U WITH BREVE */
    new codepair(0x02de, 0x015c), /*                 Scircumflex Ŝ LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
    new codepair(0x02e5, 0x010b), /*                   cabovedot ċ LATIN SMALL LETTER C WITH DOT ABOVE */
    new codepair(0x02e6, 0x0109), /*                 ccircumflex ĉ LATIN SMALL LETTER C WITH CIRCUMFLEX */
    new codepair(0x02f5, 0x0121), /*                   gabovedot ġ LATIN SMALL LETTER G WITH DOT ABOVE */
    new codepair(0x02f8, 0x011d), /*                 gcircumflex ĝ LATIN SMALL LETTER G WITH CIRCUMFLEX */
    new codepair(0x02fd, 0x016d), /*                      ubreve ŭ LATIN SMALL LETTER U WITH BREVE */
    new codepair(0x02fe, 0x015d), /*                 scircumflex ŝ LATIN SMALL LETTER S WITH CIRCUMFLEX */
    new codepair(0x03a2, 0x0138), /*                         kra ĸ LATIN SMALL LETTER KRA */
    new codepair(0x03a3, 0x0156), /*                    Rcedilla Ŗ LATIN CAPITAL LETTER R WITH CEDILLA */
    new codepair(0x03a5, 0x0128), /*                      Itilde Ĩ LATIN CAPITAL LETTER I WITH TILDE */
    new codepair(0x03a6, 0x013b), /*                    Lcedilla Ļ LATIN CAPITAL LETTER L WITH CEDILLA */
    new codepair(0x03aa, 0x0112), /*                     Emacron Ē LATIN CAPITAL LETTER E WITH MACRON */
    new codepair(0x03ab, 0x0122), /*                    Gcedilla Ģ LATIN CAPITAL LETTER G WITH CEDILLA */
    new codepair(0x03ac, 0x0166), /*                      Tslash Ŧ LATIN CAPITAL LETTER T WITH STROKE */
    new codepair(0x03b3, 0x0157), /*                    rcedilla ŗ LATIN SMALL LETTER R WITH CEDILLA */
    new codepair(0x03b5, 0x0129), /*                      itilde ĩ LATIN SMALL LETTER I WITH TILDE */
    new codepair(0x03b6, 0x013c), /*                    lcedilla ļ LATIN SMALL LETTER L WITH CEDILLA */
    new codepair(0x03ba, 0x0113), /*                     emacron ē LATIN SMALL LETTER E WITH MACRON */
    new codepair(0x03bb, 0x0123), /*                    gcedilla ģ LATIN SMALL LETTER G WITH CEDILLA */
    new codepair(0x03bc, 0x0167), /*                      tslash ŧ LATIN SMALL LETTER T WITH STROKE */
    new codepair(0x03bd, 0x014a), /*                         ENG Ŋ LATIN CAPITAL LETTER ENG */
    new codepair(0x03bf, 0x014b), /*                         eng ŋ LATIN SMALL LETTER ENG */
    new codepair(0x03c0, 0x0100), /*                     Amacron Ā LATIN CAPITAL LETTER A WITH MACRON */
    new codepair(0x03c7, 0x012e), /*                     Iogonek Į LATIN CAPITAL LETTER I WITH OGONEK */
    new codepair(0x03cc, 0x0116), /*                   Eabovedot Ė LATIN CAPITAL LETTER E WITH DOT ABOVE */
    new codepair(0x03cf, 0x012a), /*                     Imacron Ī LATIN CAPITAL LETTER I WITH MACRON */
    new codepair(0x03d1, 0x0145), /*                    Ncedilla Ņ LATIN CAPITAL LETTER N WITH CEDILLA */
    new codepair(0x03d2, 0x014c), /*                     Omacron Ō LATIN CAPITAL LETTER O WITH MACRON */
    new codepair(0x03d3, 0x0136), /*                    Kcedilla Ķ LATIN CAPITAL LETTER K WITH CEDILLA */
    new codepair(0x03d9, 0x0172), /*                     Uogonek Ų LATIN CAPITAL LETTER U WITH OGONEK */
    new codepair(0x03dd, 0x0168), /*                      Utilde Ũ LATIN CAPITAL LETTER U WITH TILDE */
    new codepair(0x03de, 0x016a), /*                     Umacron Ū LATIN CAPITAL LETTER U WITH MACRON */
    new codepair(0x03e0, 0x0101), /*                     amacron ā LATIN SMALL LETTER A WITH MACRON */
    new codepair(0x03e7, 0x012f), /*                     iogonek į LATIN SMALL LETTER I WITH OGONEK */
    new codepair(0x03ec, 0x0117), /*                   eabovedot ė LATIN SMALL LETTER E WITH DOT ABOVE */
    new codepair(0x03ef, 0x012b), /*                     imacron ī LATIN SMALL LETTER I WITH MACRON */
    new codepair(0x03f1, 0x0146), /*                    ncedilla ņ LATIN SMALL LETTER N WITH CEDILLA */
    new codepair(0x03f2, 0x014d), /*                     omacron ō LATIN SMALL LETTER O WITH MACRON */
    new codepair(0x03f3, 0x0137), /*                    kcedilla ķ LATIN SMALL LETTER K WITH CEDILLA */
    new codepair(0x03f9, 0x0173), /*                     uogonek ų LATIN SMALL LETTER U WITH OGONEK */
    new codepair(0x03fd, 0x0169), /*                      utilde ũ LATIN SMALL LETTER U WITH TILDE */
    new codepair(0x03fe, 0x016b), /*                     umacron ū LATIN SMALL LETTER U WITH MACRON */
    new codepair(0x047e, 0x203e), /*                    overline ‾ OVERLINE */
    new codepair(0x04a1, 0x3002), /*               kana_fullstop 。 IDEOGRAPHIC FULL STOP */
    new codepair(0x04a2, 0x300c), /*         kana_openingbracket 「 LEFT CORNER BRACKET */
    new codepair(0x04a3, 0x300d), /*         kana_closingbracket 」 RIGHT CORNER BRACKET */
    new codepair(0x04a4, 0x3001), /*                  kana_comma 、 IDEOGRAPHIC COMMA */
    new codepair(0x04a5, 0x30fb), /*            kana_conjunctive ・ KATAKANA MIDDLE DOT */
    new codepair(0x04a6, 0x30f2), /*                     kana_WO ヲ KATAKANA LETTER WO */
    new codepair(0x04a7, 0x30a1), /*                      kana_a ァ KATAKANA LETTER SMALL A */
    new codepair(0x04a8, 0x30a3), /*                      kana_i ィ KATAKANA LETTER SMALL I */
    new codepair(0x04a9, 0x30a5), /*                      kana_u ゥ KATAKANA LETTER SMALL U */
    new codepair(0x04aa, 0x30a7), /*                      kana_e ェ KATAKANA LETTER SMALL E */
    new codepair(0x04ab, 0x30a9), /*                      kana_o ォ KATAKANA LETTER SMALL O */
    new codepair(0x04ac, 0x30e3), /*                     kana_ya ャ KATAKANA LETTER SMALL YA */
    new codepair(0x04ad, 0x30e5), /*                     kana_yu ュ KATAKANA LETTER SMALL YU */
    new codepair(0x04ae, 0x30e7), /*                     kana_yo ョ KATAKANA LETTER SMALL YO */
    new codepair(0x04af, 0x30c3), /*                    kana_tsu ッ KATAKANA LETTER SMALL TU */
    new codepair(0x04b0, 0x30fc), /*              prolongedsound ー KATAKANA-HIRAGANA PROLONGED SOUND MARK */
    new codepair(0x04b1, 0x30a2), /*                      kana_A ア KATAKANA LETTER A */
    new codepair(0x04b2, 0x30a4), /*                      kana_I イ KATAKANA LETTER I */
    new codepair(0x04b3, 0x30a6), /*                      kana_U ウ KATAKANA LETTER U */
    new codepair(0x04b4, 0x30a8), /*                      kana_E エ KATAKANA LETTER E */
    new codepair(0x04b5, 0x30aa), /*                      kana_O オ KATAKANA LETTER O */
    new codepair(0x04b6, 0x30ab), /*                     kana_KA カ KATAKANA LETTER KA */
    new codepair(0x04b7, 0x30ad), /*                     kana_KI キ KATAKANA LETTER KI */
    new codepair(0x04b8, 0x30af), /*                     kana_KU ク KATAKANA LETTER KU */
    new codepair(0x04b9, 0x30b1), /*                     kana_KE ケ KATAKANA LETTER KE */
    new codepair(0x04ba, 0x30b3), /*                     kana_KO コ KATAKANA LETTER KO */
    new codepair(0x04bb, 0x30b5), /*                     kana_SA サ KATAKANA LETTER SA */
    new codepair(0x04bc, 0x30b7), /*                    kana_SHI シ KATAKANA LETTER SI */
    new codepair(0x04bd, 0x30b9), /*                     kana_SU ス KATAKANA LETTER SU */
    new codepair(0x04be, 0x30bb), /*                     kana_SE セ KATAKANA LETTER SE */
    new codepair(0x04bf, 0x30bd), /*                     kana_SO ソ KATAKANA LETTER SO */
    new codepair(0x04c0, 0x30bf), /*                     kana_TA タ KATAKANA LETTER TA */
    new codepair(0x04c1, 0x30c1), /*                    kana_CHI チ KATAKANA LETTER TI */
    new codepair(0x04c2, 0x30c4), /*                    kana_TSU ツ KATAKANA LETTER TU */
    new codepair(0x04c3, 0x30c6), /*                     kana_TE テ KATAKANA LETTER TE */
    new codepair(0x04c4, 0x30c8), /*                     kana_TO ト KATAKANA LETTER TO */
    new codepair(0x04c5, 0x30ca), /*                     kana_NA ナ KATAKANA LETTER NA */
    new codepair(0x04c6, 0x30cb), /*                     kana_NI ニ KATAKANA LETTER NI */
    new codepair(0x04c7, 0x30cc), /*                     kana_NU ヌ KATAKANA LETTER NU */
    new codepair(0x04c8, 0x30cd), /*                     kana_NE ネ KATAKANA LETTER NE */
    new codepair(0x04c9, 0x30ce), /*                     kana_NO ノ KATAKANA LETTER NO */
    new codepair(0x04ca, 0x30cf), /*                     kana_HA ハ KATAKANA LETTER HA */
    new codepair(0x04cb, 0x30d2), /*                     kana_HI ヒ KATAKANA LETTER HI */
    new codepair(0x04cc, 0x30d5), /*                     kana_FU フ KATAKANA LETTER HU */
    new codepair(0x04cd, 0x30d8), /*                     kana_HE ヘ KATAKANA LETTER HE */
    new codepair(0x04ce, 0x30db), /*                     kana_HO ホ KATAKANA LETTER HO */
    new codepair(0x04cf, 0x30de), /*                     kana_MA マ KATAKANA LETTER MA */
    new codepair(0x04d0, 0x30df), /*                     kana_MI ミ KATAKANA LETTER MI */
    new codepair(0x04d1, 0x30e0), /*                     kana_MU ム KATAKANA LETTER MU */
    new codepair(0x04d2, 0x30e1), /*                     kana_ME メ KATAKANA LETTER ME */
    new codepair(0x04d3, 0x30e2), /*                     kana_MO モ KATAKANA LETTER MO */
    new codepair(0x04d4, 0x30e4), /*                     kana_YA ヤ KATAKANA LETTER YA */
    new codepair(0x04d5, 0x30e6), /*                     kana_YU ユ KATAKANA LETTER YU */
    new codepair(0x04d6, 0x30e8), /*                     kana_YO ヨ KATAKANA LETTER YO */
    new codepair(0x04d7, 0x30e9), /*                     kana_RA ラ KATAKANA LETTER RA */
    new codepair(0x04d8, 0x30ea), /*                     kana_RI リ KATAKANA LETTER RI */
    new codepair(0x04d9, 0x30eb), /*                     kana_RU ル KATAKANA LETTER RU */
    new codepair(0x04da, 0x30ec), /*                     kana_RE レ KATAKANA LETTER RE */
    new codepair(0x04db, 0x30ed), /*                     kana_RO ロ KATAKANA LETTER RO */
    new codepair(0x04dc, 0x30ef), /*                     kana_WA ワ KATAKANA LETTER WA */
    new codepair(0x04dd, 0x30f3), /*                      kana_N ン KATAKANA LETTER N */
    new codepair(0x04de, 0x309b), /*                 voicedsound ゛ KATAKANA-HIRAGANA VOICED SOUND MARK */
    new codepair(0x04df, 0x309c), /*             semivoicedsound ゜ KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
    new codepair(0x05ac, 0x060c), /*                Arabic_comma ، ARABIC COMMA */
    new codepair(0x05bb, 0x061b), /*            Arabic_semicolon ؛ ARABIC SEMICOLON */
    new codepair(0x05bf, 0x061f), /*        Arabic_question_mark ؟ ARABIC QUESTION MARK */
    new codepair(0x05c1, 0x0621), /*                Arabic_hamza ء ARABIC LETTER HAMZA */
    new codepair(0x05c2, 0x0622), /*          Arabic_maddaonalef آ ARABIC LETTER ALEF WITH MADDA ABOVE */
    new codepair(0x05c3, 0x0623), /*          Arabic_hamzaonalef أ ARABIC LETTER ALEF WITH HAMZA ABOVE */
    new codepair(0x05c4, 0x0624), /*           Arabic_hamzaonwaw ؤ ARABIC LETTER WAW WITH HAMZA ABOVE */
    new codepair(0x05c5, 0x0625), /*       Arabic_hamzaunderalef إ ARABIC LETTER ALEF WITH HAMZA BELOW */
    new codepair(0x05c6, 0x0626), /*           Arabic_hamzaonyeh ئ ARABIC LETTER YEH WITH HAMZA ABOVE */
    new codepair(0x05c7, 0x0627), /*                 Arabic_alef ا ARABIC LETTER ALEF */
    new codepair(0x05c8, 0x0628), /*                  Arabic_beh ب ARABIC LETTER BEH */
    new codepair(0x05c9, 0x0629), /*           Arabic_tehmarbuta ة ARABIC LETTER TEH MARBUTA */
    new codepair(0x05ca, 0x062a), /*                  Arabic_teh ت ARABIC LETTER TEH */
    new codepair(0x05cb, 0x062b), /*                 Arabic_theh ث ARABIC LETTER THEH */
    new codepair(0x05cc, 0x062c), /*                 Arabic_jeem ج ARABIC LETTER JEEM */
    new codepair(0x05cd, 0x062d), /*                  Arabic_hah ح ARABIC LETTER HAH */
    new codepair(0x05ce, 0x062e), /*                 Arabic_khah خ ARABIC LETTER KHAH */
    new codepair(0x05cf, 0x062f), /*                  Arabic_dal د ARABIC LETTER DAL */
    new codepair(0x05d0, 0x0630), /*                 Arabic_thal ذ ARABIC LETTER THAL */
    new codepair(0x05d1, 0x0631), /*                   Arabic_ra ر ARABIC LETTER REH */
    new codepair(0x05d2, 0x0632), /*                 Arabic_zain ز ARABIC LETTER ZAIN */
    new codepair(0x05d3, 0x0633), /*                 Arabic_seen س ARABIC LETTER SEEN */
    new codepair(0x05d4, 0x0634), /*                Arabic_sheen ش ARABIC LETTER SHEEN */
    new codepair(0x05d5, 0x0635), /*                  Arabic_sad ص ARABIC LETTER SAD */
    new codepair(0x05d6, 0x0636), /*                  Arabic_dad ض ARABIC LETTER DAD */
    new codepair(0x05d7, 0x0637), /*                  Arabic_tah ط ARABIC LETTER TAH */
    new codepair(0x05d8, 0x0638), /*                  Arabic_zah ظ ARABIC LETTER ZAH */
    new codepair(0x05d9, 0x0639), /*                  Arabic_ain ع ARABIC LETTER AIN */
    new codepair(0x05da, 0x063a), /*                Arabic_ghain غ ARABIC LETTER GHAIN */
    new codepair(0x05e0, 0x0640), /*              Arabic_tatweel ـ ARABIC TATWEEL */
    new codepair(0x05e1, 0x0641), /*                  Arabic_feh ف ARABIC LETTER FEH */
    new codepair(0x05e2, 0x0642), /*                  Arabic_qaf ق ARABIC LETTER QAF */
    new codepair(0x05e3, 0x0643), /*                  Arabic_kaf ك ARABIC LETTER KAF */
    new codepair(0x05e4, 0x0644), /*                  Arabic_lam ل ARABIC LETTER LAM */
    new codepair(0x05e5, 0x0645), /*                 Arabic_meem م ARABIC LETTER MEEM */
    new codepair(0x05e6, 0x0646), /*                 Arabic_noon ن ARABIC LETTER NOON */
    new codepair(0x05e7, 0x0647), /*                   Arabic_ha ه ARABIC LETTER HEH */
    new codepair(0x05e8, 0x0648), /*                  Arabic_waw و ARABIC LETTER WAW */
    new codepair(0x05e9, 0x0649), /*          Arabic_alefmaksura ى ARABIC LETTER ALEF MAKSURA */
    new codepair(0x05ea, 0x064a), /*                  Arabic_yeh ي ARABIC LETTER YEH */
    new codepair(0x05eb, 0x064b), /*             Arabic_fathatan ً ARABIC FATHATAN */
    new codepair(0x05ec, 0x064c), /*             Arabic_dammatan ٌ ARABIC DAMMATAN */
    new codepair(0x05ed, 0x064d), /*             Arabic_kasratan ٍ ARABIC KASRATAN */
    new codepair(0x05ee, 0x064e), /*                Arabic_fatha َ ARABIC FATHA */
    new codepair(0x05ef, 0x064f), /*                Arabic_damma ُ ARABIC DAMMA */
    new codepair(0x05f0, 0x0650), /*                Arabic_kasra ِ ARABIC KASRA */
    new codepair(0x05f1, 0x0651), /*               Arabic_shadda ّ ARABIC SHADDA */
    new codepair(0x05f2, 0x0652), /*                Arabic_sukun ْ ARABIC SUKUN */
    new codepair(0x06a1, 0x0452), /*                 Serbian_dje ђ CYRILLIC SMALL LETTER DJE */
    new codepair(0x06a2, 0x0453), /*               Macedonia_gje ѓ CYRILLIC SMALL LETTER GJE */
    new codepair(0x06a3, 0x0451), /*                 Cyrillic_io ё CYRILLIC SMALL LETTER IO */
    new codepair(0x06a4, 0x0454), /*                Ukrainian_ie є CYRILLIC SMALL LETTER UKRAINIAN IE */
    new codepair(0x06a5, 0x0455), /*               Macedonia_dse ѕ CYRILLIC SMALL LETTER DZE */
    new codepair(0x06a6, 0x0456), /*                 Ukrainian_i і CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
    new codepair(0x06a7, 0x0457), /*                Ukrainian_yi ї CYRILLIC SMALL LETTER YI */
    new codepair(0x06a8, 0x0458), /*                 Cyrillic_je ј CYRILLIC SMALL LETTER JE */
    new codepair(0x06a9, 0x0459), /*                Cyrillic_lje љ CYRILLIC SMALL LETTER LJE */
    new codepair(0x06aa, 0x045a), /*                Cyrillic_nje њ CYRILLIC SMALL LETTER NJE */
    new codepair(0x06ab, 0x045b), /*                Serbian_tshe ћ CYRILLIC SMALL LETTER TSHE */
    new codepair(0x06ac, 0x045c), /*               Macedonia_kje ќ CYRILLIC SMALL LETTER KJE */
    new codepair(0x06ae, 0x045e), /*         Byelorussian_shortu ў CYRILLIC SMALL LETTER SHORT U */
    new codepair(0x06af, 0x045f), /*               Cyrillic_dzhe џ CYRILLIC SMALL LETTER DZHE */
    new codepair(0x06b0, 0x2116), /*                  numerosign № NUMERO SIGN */
    new codepair(0x06b1, 0x0402), /*                 Serbian_DJE Ђ CYRILLIC CAPITAL LETTER DJE */
    new codepair(0x06b2, 0x0403), /*               Macedonia_GJE Ѓ CYRILLIC CAPITAL LETTER GJE */
    new codepair(0x06b3, 0x0401), /*                 Cyrillic_IO Ё CYRILLIC CAPITAL LETTER IO */
    new codepair(0x06b4, 0x0404), /*                Ukrainian_IE Є CYRILLIC CAPITAL LETTER UKRAINIAN IE */
    new codepair(0x06b5, 0x0405), /*               Macedonia_DSE Ѕ CYRILLIC CAPITAL LETTER DZE */
    new codepair(0x06b6, 0x0406), /*                 Ukrainian_I І CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
    new codepair(0x06b7, 0x0407), /*                Ukrainian_YI Ї CYRILLIC CAPITAL LETTER YI */
    new codepair(0x06b8, 0x0408), /*                 Cyrillic_JE Ј CYRILLIC CAPITAL LETTER JE */
    new codepair(0x06b9, 0x0409), /*                Cyrillic_LJE Љ CYRILLIC CAPITAL LETTER LJE */
    new codepair(0x06ba, 0x040a), /*                Cyrillic_NJE Њ CYRILLIC CAPITAL LETTER NJE */
    new codepair(0x06bb, 0x040b), /*                Serbian_TSHE Ћ CYRILLIC CAPITAL LETTER TSHE */
    new codepair(0x06bc, 0x040c), /*               Macedonia_KJE Ќ CYRILLIC CAPITAL LETTER KJE */
    new codepair(0x06be, 0x040e), /*         Byelorussian_SHORTU Ў CYRILLIC CAPITAL LETTER SHORT U */
    new codepair(0x06bf, 0x040f), /*               Cyrillic_DZHE Џ CYRILLIC CAPITAL LETTER DZHE */
    new codepair(0x06c0, 0x044e), /*                 Cyrillic_yu ю CYRILLIC SMALL LETTER YU */
    new codepair(0x06c1, 0x0430), /*                  Cyrillic_a а CYRILLIC SMALL LETTER A */
    new codepair(0x06c2, 0x0431), /*                 Cyrillic_be б CYRILLIC SMALL LETTER BE */
    new codepair(0x06c3, 0x0446), /*                Cyrillic_tse ц CYRILLIC SMALL LETTER TSE */
    new codepair(0x06c4, 0x0434), /*                 Cyrillic_de д CYRILLIC SMALL LETTER DE */
    new codepair(0x06c5, 0x0435), /*                 Cyrillic_ie е CYRILLIC SMALL LETTER IE */
    new codepair(0x06c6, 0x0444), /*                 Cyrillic_ef ф CYRILLIC SMALL LETTER EF */
    new codepair(0x06c7, 0x0433), /*                Cyrillic_ghe г CYRILLIC SMALL LETTER GHE */
    new codepair(0x06c8, 0x0445), /*                 Cyrillic_ha х CYRILLIC SMALL LETTER HA */
    new codepair(0x06c9, 0x0438), /*                  Cyrillic_i и CYRILLIC SMALL LETTER I */
    new codepair(0x06ca, 0x0439), /*             Cyrillic_shorti й CYRILLIC SMALL LETTER SHORT I */
    new codepair(0x06cb, 0x043a), /*                 Cyrillic_ka к CYRILLIC SMALL LETTER KA */
    new codepair(0x06cc, 0x043b), /*                 Cyrillic_el л CYRILLIC SMALL LETTER EL */
    new codepair(0x06cd, 0x043c), /*                 Cyrillic_em м CYRILLIC SMALL LETTER EM */
    new codepair(0x06ce, 0x043d), /*                 Cyrillic_en н CYRILLIC SMALL LETTER EN */
    new codepair(0x06cf, 0x043e), /*                  Cyrillic_o о CYRILLIC SMALL LETTER O */
    new codepair(0x06d0, 0x043f), /*                 Cyrillic_pe п CYRILLIC SMALL LETTER PE */
    new codepair(0x06d1, 0x044f), /*                 Cyrillic_ya я CYRILLIC SMALL LETTER YA */
    new codepair(0x06d2, 0x0440), /*                 Cyrillic_er р CYRILLIC SMALL LETTER ER */
    new codepair(0x06d3, 0x0441), /*                 Cyrillic_es с CYRILLIC SMALL LETTER ES */
    new codepair(0x06d4, 0x0442), /*                 Cyrillic_te т CYRILLIC SMALL LETTER TE */
    new codepair(0x06d5, 0x0443), /*                  Cyrillic_u у CYRILLIC SMALL LETTER U */
    new codepair(0x06d6, 0x0436), /*                Cyrillic_zhe ж CYRILLIC SMALL LETTER ZHE */
    new codepair(0x06d7, 0x0432), /*                 Cyrillic_ve в CYRILLIC SMALL LETTER VE */
    new codepair(0x06d8, 0x044c), /*           Cyrillic_softsign ь CYRILLIC SMALL LETTER SOFT SIGN */
    new codepair(0x06d9, 0x044b), /*               Cyrillic_yeru ы CYRILLIC SMALL LETTER YERU */
    new codepair(0x06da, 0x0437), /*                 Cyrillic_ze з CYRILLIC SMALL LETTER ZE */
    new codepair(0x06db, 0x0448), /*                Cyrillic_sha ш CYRILLIC SMALL LETTER SHA */
    new codepair(0x06dc, 0x044d), /*                  Cyrillic_e э CYRILLIC SMALL LETTER E */
    new codepair(0x06dd, 0x0449), /*              Cyrillic_shcha щ CYRILLIC SMALL LETTER SHCHA */
    new codepair(0x06de, 0x0447), /*                Cyrillic_che ч CYRILLIC SMALL LETTER CHE */
    new codepair(0x06df, 0x044a), /*           Cyrillic_hardsign ъ CYRILLIC SMALL LETTER HARD SIGN */
    new codepair(0x06e0, 0x042e), /*                 Cyrillic_YU Ю CYRILLIC CAPITAL LETTER YU */
    new codepair(0x06e1, 0x0410), /*                  Cyrillic_A А CYRILLIC CAPITAL LETTER A */
    new codepair(0x06e2, 0x0411), /*                 Cyrillic_BE Б CYRILLIC CAPITAL LETTER BE */
    new codepair(0x06e3, 0x0426), /*                Cyrillic_TSE Ц CYRILLIC CAPITAL LETTER TSE */
    new codepair(0x06e4, 0x0414), /*                 Cyrillic_DE Д CYRILLIC CAPITAL LETTER DE */
    new codepair(0x06e5, 0x0415), /*                 Cyrillic_IE Е CYRILLIC CAPITAL LETTER IE */
    new codepair(0x06e6, 0x0424), /*                 Cyrillic_EF Ф CYRILLIC CAPITAL LETTER EF */
    new codepair(0x06e7, 0x0413), /*                Cyrillic_GHE Г CYRILLIC CAPITAL LETTER GHE */
    new codepair(0x06e8, 0x0425), /*                 Cyrillic_HA Х CYRILLIC CAPITAL LETTER HA */
    new codepair(0x06e9, 0x0418), /*                  Cyrillic_I И CYRILLIC CAPITAL LETTER I */
    new codepair(0x06ea, 0x0419), /*             Cyrillic_SHORTI Й CYRILLIC CAPITAL LETTER SHORT I */
    new codepair(0x06eb, 0x041a), /*                 Cyrillic_KA К CYRILLIC CAPITAL LETTER KA */
    new codepair(0x06ec, 0x041b), /*                 Cyrillic_EL Л CYRILLIC CAPITAL LETTER EL */
    new codepair(0x06ed, 0x041c), /*                 Cyrillic_EM М CYRILLIC CAPITAL LETTER EM */
    new codepair(0x06ee, 0x041d), /*                 Cyrillic_EN Н CYRILLIC CAPITAL LETTER EN */
    new codepair(0x06ef, 0x041e), /*                  Cyrillic_O О CYRILLIC CAPITAL LETTER O */
    new codepair(0x06f0, 0x041f), /*                 Cyrillic_PE П CYRILLIC CAPITAL LETTER PE */
    new codepair(0x06f1, 0x042f), /*                 Cyrillic_YA Я CYRILLIC CAPITAL LETTER YA */
    new codepair(0x06f2, 0x0420), /*                 Cyrillic_ER Р CYRILLIC CAPITAL LETTER ER */
    new codepair(0x06f3, 0x0421), /*                 Cyrillic_ES С CYRILLIC CAPITAL LETTER ES */
    new codepair(0x06f4, 0x0422), /*                 Cyrillic_TE Т CYRILLIC CAPITAL LETTER TE */
    new codepair(0x06f5, 0x0423), /*                  Cyrillic_U У CYRILLIC CAPITAL LETTER U */
    new codepair(0x06f6, 0x0416), /*                Cyrillic_ZHE Ж CYRILLIC CAPITAL LETTER ZHE */
    new codepair(0x06f7, 0x0412), /*                 Cyrillic_VE В CYRILLIC CAPITAL LETTER VE */
    new codepair(0x06f8, 0x042c), /*           Cyrillic_SOFTSIGN Ь CYRILLIC CAPITAL LETTER SOFT SIGN */
    new codepair(0x06f9, 0x042b), /*               Cyrillic_YERU Ы CYRILLIC CAPITAL LETTER YERU */
    new codepair(0x06fa, 0x0417), /*                 Cyrillic_ZE З CYRILLIC CAPITAL LETTER ZE */
    new codepair(0x06fb, 0x0428), /*                Cyrillic_SHA Ш CYRILLIC CAPITAL LETTER SHA */
    new codepair(0x06fc, 0x042d), /*                  Cyrillic_E Э CYRILLIC CAPITAL LETTER E */
    new codepair(0x06fd, 0x0429), /*              Cyrillic_SHCHA Щ CYRILLIC CAPITAL LETTER SHCHA */
    new codepair(0x06fe, 0x0427), /*                Cyrillic_CHE Ч CYRILLIC CAPITAL LETTER CHE */
    new codepair(0x06ff, 0x042a), /*           Cyrillic_HARDSIGN Ъ CYRILLIC CAPITAL LETTER HARD SIGN */
    new codepair(0x07a1, 0x0386), /*           Greek_ALPHAaccent Ά GREEK CAPITAL LETTER ALPHA WITH TONOS */
    new codepair(0x07a2, 0x0388), /*         Greek_EPSILONaccent Έ GREEK CAPITAL LETTER EPSILON WITH TONOS */
    new codepair(0x07a3, 0x0389), /*             Greek_ETAaccent Ή GREEK CAPITAL LETTER ETA WITH TONOS */
    new codepair(0x07a4, 0x038a), /*            Greek_IOTAaccent Ί GREEK CAPITAL LETTER IOTA WITH TONOS */
    new codepair(0x07a5, 0x03aa), /*         Greek_IOTAdiaeresis Ϊ GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
    new codepair(0x07a7, 0x038c), /*         Greek_OMICRONaccent Ό GREEK CAPITAL LETTER OMICRON WITH TONOS */
    new codepair(0x07a8, 0x038e), /*         Greek_UPSILONaccent Ύ GREEK CAPITAL LETTER UPSILON WITH TONOS */
    new codepair(0x07a9, 0x03ab), /*       Greek_UPSILONdieresis Ϋ GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
    new codepair(0x07ab, 0x038f), /*           Greek_OMEGAaccent Ώ GREEK CAPITAL LETTER OMEGA WITH TONOS */
    new codepair(0x07ae, 0x0385), /*        Greek_accentdieresis ΅ GREEK DIALYTIKA TONOS */
    new codepair(0x07af, 0x2015), /*              Greek_horizbar ― HORIZONTAL BAR */
    new codepair(0x07b1, 0x03ac), /*           Greek_alphaaccent ά GREEK SMALL LETTER ALPHA WITH TONOS */
    new codepair(0x07b2, 0x03ad), /*         Greek_epsilonaccent έ GREEK SMALL LETTER EPSILON WITH TONOS */
    new codepair(0x07b3, 0x03ae), /*             Greek_etaaccent ή GREEK SMALL LETTER ETA WITH TONOS */
    new codepair(0x07b4, 0x03af), /*            Greek_iotaaccent ί GREEK SMALL LETTER IOTA WITH TONOS */
    new codepair(0x07b5, 0x03ca), /*          Greek_iotadieresis ϊ GREEK SMALL LETTER IOTA WITH DIALYTIKA */
    new codepair(0x07b6, 0x0390), /*    Greek_iotaaccentdieresis ΐ GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
    new codepair(0x07b7, 0x03cc), /*         Greek_omicronaccent ό GREEK SMALL LETTER OMICRON WITH TONOS */
    new codepair(0x07b8, 0x03cd), /*         Greek_upsilonaccent ύ GREEK SMALL LETTER UPSILON WITH TONOS */
    new codepair(0x07b9, 0x03cb), /*       Greek_upsilondieresis ϋ GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
    new codepair(0x07ba, 0x03b0), /* Greek_upsilonaccentdieresis ΰ GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
    new codepair(0x07bb, 0x03ce), /*           Greek_omegaaccent ώ GREEK SMALL LETTER OMEGA WITH TONOS */
    new codepair(0x07c1, 0x0391), /*                 Greek_ALPHA Α GREEK CAPITAL LETTER ALPHA */
    new codepair(0x07c2, 0x0392), /*                  Greek_BETA Β GREEK CAPITAL LETTER BETA */
    new codepair(0x07c3, 0x0393), /*                 Greek_GAMMA Γ GREEK CAPITAL LETTER GAMMA */
    new codepair(0x07c4, 0x0394), /*                 Greek_DELTA Δ GREEK CAPITAL LETTER DELTA */
    new codepair(0x07c5, 0x0395), /*               Greek_EPSILON Ε GREEK CAPITAL LETTER EPSILON */
    new codepair(0x07c6, 0x0396), /*                  Greek_ZETA Ζ GREEK CAPITAL LETTER ZETA */
    new codepair(0x07c7, 0x0397), /*                   Greek_ETA Η GREEK CAPITAL LETTER ETA */
    new codepair(0x07c8, 0x0398), /*                 Greek_THETA Θ GREEK CAPITAL LETTER THETA */
    new codepair(0x07c9, 0x0399), /*                  Greek_IOTA Ι GREEK CAPITAL LETTER IOTA */
    new codepair(0x07ca, 0x039a), /*                 Greek_KAPPA Κ GREEK CAPITAL LETTER KAPPA */
    new codepair(0x07cb, 0x039b), /*                Greek_LAMBDA Λ GREEK CAPITAL LETTER LAMDA */
    new codepair(0x07cc, 0x039c), /*                    Greek_MU Μ GREEK CAPITAL LETTER MU */
    new codepair(0x07cd, 0x039d), /*                    Greek_NU Ν GREEK CAPITAL LETTER NU */
    new codepair(0x07ce, 0x039e), /*                    Greek_XI Ξ GREEK CAPITAL LETTER XI */
    new codepair(0x07cf, 0x039f), /*               Greek_OMICRON Ο GREEK CAPITAL LETTER OMICRON */
    new codepair(0x07d0, 0x03a0), /*                    Greek_PI Π GREEK CAPITAL LETTER PI */
    new codepair(0x07d1, 0x03a1), /*                   Greek_RHO Ρ GREEK CAPITAL LETTER RHO */
    new codepair(0x07d2, 0x03a3), /*                 Greek_SIGMA Σ GREEK CAPITAL LETTER SIGMA */
    new codepair(0x07d4, 0x03a4), /*                   Greek_TAU Τ GREEK CAPITAL LETTER TAU */
    new codepair(0x07d5, 0x03a5), /*               Greek_UPSILON Υ GREEK CAPITAL LETTER UPSILON */
    new codepair(0x07d6, 0x03a6), /*                   Greek_PHI Φ GREEK CAPITAL LETTER PHI */
    new codepair(0x07d7, 0x03a7), /*                   Greek_CHI Χ GREEK CAPITAL LETTER CHI */
    new codepair(0x07d8, 0x03a8), /*                   Greek_PSI Ψ GREEK CAPITAL LETTER PSI */
    new codepair(0x07d9, 0x03a9), /*                 Greek_OMEGA Ω GREEK CAPITAL LETTER OMEGA */
    new codepair(0x07e1, 0x03b1), /*                 Greek_alpha α GREEK SMALL LETTER ALPHA */
    new codepair(0x07e2, 0x03b2), /*                  Greek_beta β GREEK SMALL LETTER BETA */
    new codepair(0x07e3, 0x03b3), /*                 Greek_gamma γ GREEK SMALL LETTER GAMMA */
    new codepair(0x07e4, 0x03b4), /*                 Greek_delta δ GREEK SMALL LETTER DELTA */
    new codepair(0x07e5, 0x03b5), /*               Greek_epsilon ε GREEK SMALL LETTER EPSILON */
    new codepair(0x07e6, 0x03b6), /*                  Greek_zeta ζ GREEK SMALL LETTER ZETA */
    new codepair(0x07e7, 0x03b7), /*                   Greek_eta η GREEK SMALL LETTER ETA */
    new codepair(0x07e8, 0x03b8), /*                 Greek_theta θ GREEK SMALL LETTER THETA */
    new codepair(0x07e9, 0x03b9), /*                  Greek_iota ι GREEK SMALL LETTER IOTA */
    new codepair(0x07ea, 0x03ba), /*                 Greek_kappa κ GREEK SMALL LETTER KAPPA */
    new codepair(0x07eb, 0x03bb), /*                Greek_lambda λ GREEK SMALL LETTER LAMDA */
    new codepair(0x07ec, 0x03bc), /*                    Greek_mu μ GREEK SMALL LETTER MU */
    new codepair(0x07ed, 0x03bd), /*                    Greek_nu ν GREEK SMALL LETTER NU */
    new codepair(0x07ee, 0x03be), /*                    Greek_xi ξ GREEK SMALL LETTER XI */
    new codepair(0x07ef, 0x03bf), /*               Greek_omicron ο GREEK SMALL LETTER OMICRON */
    new codepair(0x07f0, 0x03c0), /*                    Greek_pi π GREEK SMALL LETTER PI */
    new codepair(0x07f1, 0x03c1), /*                   Greek_rho ρ GREEK SMALL LETTER RHO */
    new codepair(0x07f2, 0x03c3), /*                 Greek_sigma σ GREEK SMALL LETTER SIGMA */
    new codepair(0x07f3, 0x03c2), /*       Greek_finalsmallsigma ς GREEK SMALL LETTER FINAL SIGMA */
    new codepair(0x07f4, 0x03c4), /*                   Greek_tau τ GREEK SMALL LETTER TAU */
    new codepair(0x07f5, 0x03c5), /*               Greek_upsilon υ GREEK SMALL LETTER UPSILON */
    new codepair(0x07f6, 0x03c6), /*                   Greek_phi φ GREEK SMALL LETTER PHI */
    new codepair(0x07f7, 0x03c7), /*                   Greek_chi χ GREEK SMALL LETTER CHI */
    new codepair(0x07f8, 0x03c8), /*                   Greek_psi ψ GREEK SMALL LETTER PSI */
    new codepair(0x07f9, 0x03c9), /*                 Greek_omega ω GREEK SMALL LETTER OMEGA */
    new codepair(0x08a1, 0x23b7), /*                 leftradical ⎷ ??? */
    new codepair(0x08a2, 0x250c), /*              topleftradical ┌ BOX DRAWINGS LIGHT DOWN AND RIGHT */
    new codepair(0x08a3, 0x2500), /*              horizconnector ─ BOX DRAWINGS LIGHT HORIZONTAL */
    new codepair(0x08a4, 0x2320), /*                 topintegral ⌠ TOP HALF INTEGRAL */
    new codepair(0x08a5, 0x2321), /*                 botintegral ⌡ BOTTOM HALF INTEGRAL */
    new codepair(0x08a6, 0x2502), /*               vertconnector │ BOX DRAWINGS LIGHT VERTICAL */
    new codepair(0x08a7, 0x23a1), /*            topleftsqbracket ⎡ ??? */
    new codepair(0x08a8, 0x23a3), /*            botleftsqbracket ⎣ ??? */
    new codepair(0x08a9, 0x23a4), /*           toprightsqbracket ⎤ ??? */
    new codepair(0x08aa, 0x23a6), /*           botrightsqbracket ⎦ ??? */
    new codepair(0x08ab, 0x239b), /*               topleftparens ⎛ ??? */
    new codepair(0x08ac, 0x239d), /*               botleftparens ⎝ ??? */
    new codepair(0x08ad, 0x239e), /*              toprightparens ⎞ ??? */
    new codepair(0x08ae, 0x23a0), /*              botrightparens ⎠ ??? */
    new codepair(0x08af, 0x23a8), /*        leftmiddlecurlybrace ⎨ ??? */
    new codepair(0x08b0, 0x23ac), /*       rightmiddlecurlybrace ⎬ ??? */
  /*  0x08b1                          topleftsummation ? ??? */
  /*  0x08b2                          botleftsummation ? ??? */
  /*  0x08b3                 topvertsummationconnector ? ??? */
  /*  0x08b4                 botvertsummationconnector ? ??? */
  /*  0x08b5                         toprightsummation ? ??? */
  /*  0x08b6                         botrightsummation ? ??? */
  /*  0x08b7                      rightmiddlesummation ? ??? */
    new codepair(0x08bc, 0x2264), /*               lessthanequal ≤ LESS-THAN OR EQUAL TO */
    new codepair(0x08bd, 0x2260), /*                    notequal ≠ NOT EQUAL TO */
    new codepair(0x08be, 0x2265), /*            greaterthanequal ≥ GREATER-THAN OR EQUAL TO */
    new codepair(0x08bf, 0x222b), /*                    integral ∫ INTEGRAL */
    new codepair(0x08c0, 0x2234), /*                   therefore ∴ THEREFORE */
    new codepair(0x08c1, 0x221d), /*                   variation ∝ PROPORTIONAL TO */
    new codepair(0x08c2, 0x221e), /*                    infinity ∞ INFINITY */
    new codepair(0x08c5, 0x2207), /*                       nabla ∇ NABLA */
    new codepair(0x08c8, 0x223c), /*                 approximate ∼ TILDE OPERATOR */
    new codepair(0x08c9, 0x2243), /*                similarequal ≃ ASYMPTOTICALLY EQUAL TO */
    new codepair(0x08cd, 0x21d4), /*                    ifonlyif ⇔ LEFT RIGHT DOUBLE ARROW */
    new codepair(0x08ce, 0x21d2), /*                     implies ⇒ RIGHTWARDS DOUBLE ARROW */
    new codepair(0x08cf, 0x2261), /*                   identical ≡ IDENTICAL TO */
    new codepair(0x08d6, 0x221a), /*                     radical √ SQUARE ROOT */
    new codepair(0x08da, 0x2282), /*                  includedin ⊂ SUBSET OF */
    new codepair(0x08db, 0x2283), /*                    includes ⊃ SUPERSET OF */
    new codepair(0x08dc, 0x2229), /*                intersection ∩ INTERSECTION */
    new codepair(0x08dd, 0x222a), /*                       union ∪ UNION */
    new codepair(0x08de, 0x2227), /*                  logicaland ∧ LOGICAL AND */
    new codepair(0x08df, 0x2228), /*                   logicalor ∨ LOGICAL OR */
    new codepair(0x08ef, 0x2202), /*           partialderivative ∂ PARTIAL DIFFERENTIAL */
    new codepair(0x08f6, 0x0192), /*                    function ƒ LATIN SMALL LETTER F WITH HOOK */
    new codepair(0x08fb, 0x2190), /*                   leftarrow ← LEFTWARDS ARROW */
    new codepair(0x08fc, 0x2191), /*                     uparrow ↑ UPWARDS ARROW */
    new codepair(0x08fd, 0x2192), /*                  rightarrow → RIGHTWARDS ARROW */
    new codepair(0x08fe, 0x2193), /*                   downarrow ↓ DOWNWARDS ARROW */
  /*  0x09df                                     blank ? ??? */
    new codepair(0x09e0, 0x25c6), /*                soliddiamond ◆ BLACK DIAMOND */
    new codepair(0x09e1, 0x2592), /*                checkerboard ▒ MEDIUM SHADE */
    new codepair(0x09e2, 0x2409), /*                          ht ␉ SYMBOL FOR HORIZONTAL TABULATION */
    new codepair(0x09e3, 0x240c), /*                          ff ␌ SYMBOL FOR FORM FEED */
    new codepair(0x09e4, 0x240d), /*                          cr ␍ SYMBOL FOR CARRIAGE RETURN */
    new codepair(0x09e5, 0x240a), /*                          lf ␊ SYMBOL FOR LINE FEED */
    new codepair(0x09e8, 0x2424), /*                          nl ␤ SYMBOL FOR NEWLINE */
    new codepair(0x09e9, 0x240b), /*                          vt ␋ SYMBOL FOR VERTICAL TABULATION */
    new codepair(0x09ea, 0x2518), /*              lowrightcorner ┘ BOX DRAWINGS LIGHT UP AND LEFT */
    new codepair(0x09eb, 0x2510), /*               uprightcorner ┐ BOX DRAWINGS LIGHT DOWN AND LEFT */
    new codepair(0x09ec, 0x250c), /*                upleftcorner ┌ BOX DRAWINGS LIGHT DOWN AND RIGHT */
    new codepair(0x09ed, 0x2514), /*               lowleftcorner └ BOX DRAWINGS LIGHT UP AND RIGHT */
    new codepair(0x09ee, 0x253c), /*               crossinglines ┼ BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
    new codepair(0x09ef, 0x23ba), /*              horizlinescan1 ⎺ HORIZONTAL SCAN LINE-1 (Unicode 3.2 draft) */
    new codepair(0x09f0, 0x23bb), /*              horizlinescan3 ⎻ HORIZONTAL SCAN LINE-3 (Unicode 3.2 draft) */
    new codepair(0x09f1, 0x2500), /*              horizlinescan5 ─ BOX DRAWINGS LIGHT HORIZONTAL */
    new codepair(0x09f2, 0x23bc), /*              horizlinescan7 ⎼ HORIZONTAL SCAN LINE-7 (Unicode 3.2 draft) */
    new codepair(0x09f3, 0x23bd), /*              horizlinescan9 ⎽ HORIZONTAL SCAN LINE-9 (Unicode 3.2 draft) */
    new codepair(0x09f4, 0x251c), /*                       leftt ├ BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
    new codepair(0x09f5, 0x2524), /*                      rightt ┤ BOX DRAWINGS LIGHT VERTICAL AND LEFT */
    new codepair(0x09f6, 0x2534), /*                        bott ┴ BOX DRAWINGS LIGHT UP AND HORIZONTAL */
    new codepair(0x09f7, 0x252c), /*                        topt ┬ BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
    new codepair(0x09f8, 0x2502), /*                     vertbar │ BOX DRAWINGS LIGHT VERTICAL */
    new codepair(0x0aa1, 0x2003), /*                     emspace   EM SPACE */
    new codepair(0x0aa2, 0x2002), /*                     enspace   EN SPACE */
    new codepair(0x0aa3, 0x2004), /*                    em3space   THREE-PER-EM SPACE */
    new codepair(0x0aa4, 0x2005), /*                    em4space   FOUR-PER-EM SPACE */
    new codepair(0x0aa5, 0x2007), /*                  digitspace   FIGURE SPACE */
    new codepair(0x0aa6, 0x2008), /*                  punctspace   PUNCTUATION SPACE */
    new codepair(0x0aa7, 0x2009), /*                   thinspace   THIN SPACE */
    new codepair(0x0aa8, 0x200a), /*                   hairspace   HAIR SPACE */
    new codepair(0x0aa9, 0x2014), /*                      emdash — EM DASH */
    new codepair(0x0aaa, 0x2013), /*                      endash – EN DASH */
  /*  0x0aac                               signifblank ? ??? */
    new codepair(0x0aae, 0x2026), /*                    ellipsis … HORIZONTAL ELLIPSIS */
    new codepair(0x0aaf, 0x2025), /*             doubbaselinedot ‥ TWO DOT LEADER */
    new codepair(0x0ab0, 0x2153), /*                    onethird ⅓ VULGAR FRACTION ONE THIRD */
    new codepair(0x0ab1, 0x2154), /*                   twothirds ⅔ VULGAR FRACTION TWO THIRDS */
    new codepair(0x0ab2, 0x2155), /*                    onefifth ⅕ VULGAR FRACTION ONE FIFTH */
    new codepair(0x0ab3, 0x2156), /*                   twofifths ⅖ VULGAR FRACTION TWO FIFTHS */
    new codepair(0x0ab4, 0x2157), /*                 threefifths ⅗ VULGAR FRACTION THREE FIFTHS */
    new codepair(0x0ab5, 0x2158), /*                  fourfifths ⅘ VULGAR FRACTION FOUR FIFTHS */
    new codepair(0x0ab6, 0x2159), /*                    onesixth ⅙ VULGAR FRACTION ONE SIXTH */
    new codepair(0x0ab7, 0x215a), /*                  fivesixths ⅚ VULGAR FRACTION FIVE SIXTHS */
    new codepair(0x0ab8, 0x2105), /*                      careof ℅ CARE OF */
    new codepair(0x0abb, 0x2012), /*                     figdash ‒ FIGURE DASH */
    new codepair(0x0abc, 0x2329), /*            leftanglebracket 〈 LEFT-POINTING ANGLE BRACKET */
  /*  0x0abd                              decimalpoint ? ??? */
    new codepair(0x0abe, 0x232a), /*           rightanglebracket 〉 RIGHT-POINTING ANGLE BRACKET */
  /*  0x0abf                                    marker ? ??? */
    new codepair(0x0ac3, 0x215b), /*                   oneeighth ⅛ VULGAR FRACTION ONE EIGHTH */
    new codepair(0x0ac4, 0x215c), /*                threeeighths ⅜ VULGAR FRACTION THREE EIGHTHS */
    new codepair(0x0ac5, 0x215d), /*                 fiveeighths ⅝ VULGAR FRACTION FIVE EIGHTHS */
    new codepair(0x0ac6, 0x215e), /*                seveneighths ⅞ VULGAR FRACTION SEVEN EIGHTHS */
    new codepair(0x0ac9, 0x2122), /*                   trademark ™ TRADE MARK SIGN */
    new codepair(0x0aca, 0x2613), /*               signaturemark ☓ SALTIRE */
  /*  0x0acb                         trademarkincircle ? ??? */
    new codepair(0x0acc, 0x25c1), /*            leftopentriangle ◁ WHITE LEFT-POINTING TRIANGLE */
    new codepair(0x0acd, 0x25b7), /*           rightopentriangle ▷ WHITE RIGHT-POINTING TRIANGLE */
    new codepair(0x0ace, 0x25cb), /*                emopencircle ○ WHITE CIRCLE */
    new codepair(0x0acf, 0x25af), /*             emopenrectangle ▯ WHITE VERTICAL RECTANGLE */
    new codepair(0x0ad0, 0x2018), /*         leftsinglequotemark ‘ LEFT SINGLE QUOTATION MARK */
    new codepair(0x0ad1, 0x2019), /*        rightsinglequotemark ’ RIGHT SINGLE QUOTATION MARK */
    new codepair(0x0ad2, 0x201c), /*         leftdoublequotemark “ LEFT DOUBLE QUOTATION MARK */
    new codepair(0x0ad3, 0x201d), /*        rightdoublequotemark ” RIGHT DOUBLE QUOTATION MARK */
    new codepair(0x0ad4, 0x211e), /*                prescription ℞ PRESCRIPTION TAKE */
    new codepair(0x0ad6, 0x2032), /*                     minutes ′ PRIME */
    new codepair(0x0ad7, 0x2033), /*                     seconds ″ DOUBLE PRIME */
    new codepair(0x0ad9, 0x271d), /*                  latincross ✝ LATIN CROSS */
  /*  0x0ada                                  hexagram ? ??? */
    new codepair(0x0adb, 0x25ac), /*            filledrectbullet ▬ BLACK RECTANGLE */
    new codepair(0x0adc, 0x25c0), /*         filledlefttribullet ◀ BLACK LEFT-POINTING TRIANGLE */
    new codepair(0x0add, 0x25b6), /*        filledrighttribullet ▶ BLACK RIGHT-POINTING TRIANGLE */
    new codepair(0x0ade, 0x25cf), /*              emfilledcircle ● BLACK CIRCLE */
    new codepair(0x0adf, 0x25ae), /*                emfilledrect ▮ BLACK VERTICAL RECTANGLE */
    new codepair(0x0ae0, 0x25e6), /*            enopencircbullet ◦ WHITE BULLET */
    new codepair(0x0ae1, 0x25ab), /*          enopensquarebullet ▫ WHITE SMALL SQUARE */
    new codepair(0x0ae2, 0x25ad), /*              openrectbullet ▭ WHITE RECTANGLE */
    new codepair(0x0ae3, 0x25b3), /*             opentribulletup △ WHITE UP-POINTING TRIANGLE */
    new codepair(0x0ae4, 0x25bd), /*           opentribulletdown ▽ WHITE DOWN-POINTING TRIANGLE */
    new codepair(0x0ae5, 0x2606), /*                    openstar ☆ WHITE STAR */
    new codepair(0x0ae6, 0x2022), /*          enfilledcircbullet • BULLET */
    new codepair(0x0ae7, 0x25aa), /*            enfilledsqbullet ▪ BLACK SMALL SQUARE */
    new codepair(0x0ae8, 0x25b2), /*           filledtribulletup ▲ BLACK UP-POINTING TRIANGLE */
    new codepair(0x0ae9, 0x25bc), /*         filledtribulletdown ▼ BLACK DOWN-POINTING TRIANGLE */
    new codepair(0x0aea, 0x261c), /*                 leftpointer ☜ WHITE LEFT POINTING INDEX */
    new codepair(0x0aeb, 0x261e), /*                rightpointer ☞ WHITE RIGHT POINTING INDEX */
    new codepair(0x0aec, 0x2663), /*                        club ♣ BLACK CLUB SUIT */
    new codepair(0x0aed, 0x2666), /*                     diamond ♦ BLACK DIAMOND SUIT */
    new codepair(0x0aee, 0x2665), /*                       heart ♥ BLACK HEART SUIT */
    new codepair(0x0af0, 0x2720), /*                maltesecross ✠ MALTESE CROSS */
    new codepair(0x0af1, 0x2020), /*                      dagger † DAGGER */
    new codepair(0x0af2, 0x2021), /*                doubledagger ‡ DOUBLE DAGGER */
    new codepair(0x0af3, 0x2713), /*                   checkmark ✓ CHECK MARK */
    new codepair(0x0af4, 0x2717), /*                 ballotcross ✗ BALLOT X */
    new codepair(0x0af5, 0x266f), /*                musicalsharp ♯ MUSIC SHARP SIGN */
    new codepair(0x0af6, 0x266d), /*                 musicalflat ♭ MUSIC FLAT SIGN */
    new codepair(0x0af7, 0x2642), /*                  malesymbol ♂ MALE SIGN */
    new codepair(0x0af8, 0x2640), /*                femalesymbol ♀ FEMALE SIGN */
    new codepair(0x0af9, 0x260e), /*                   telephone ☎ BLACK TELEPHONE */
    new codepair(0x0afa, 0x2315), /*           telephonerecorder ⌕ TELEPHONE RECORDER */
    new codepair(0x0afb, 0x2117), /*         phonographcopyright ℗ SOUND RECORDING COPYRIGHT */
    new codepair(0x0afc, 0x2038), /*                       caret ‸ CARET */
    new codepair(0x0afd, 0x201a), /*          singlelowquotemark ‚ SINGLE LOW-9 QUOTATION MARK */
    new codepair(0x0afe, 0x201e), /*          doublelowquotemark „ DOUBLE LOW-9 QUOTATION MARK */
  /*  0x0aff                                    cursor ? ??? */
    new codepair(0x0ba3, 0x003c), /*                   leftcaret < LESS-THAN SIGN */
    new codepair(0x0ba6, 0x003e), /*                  rightcaret > GREATER-THAN SIGN */
    new codepair(0x0ba8, 0x2228), /*                   downcaret ∨ LOGICAL OR */
    new codepair(0x0ba9, 0x2227), /*                     upcaret ∧ LOGICAL AND */
    new codepair(0x0bc0, 0x00af), /*                     overbar ¯ MACRON */
    new codepair(0x0bc2, 0x22a5), /*                    downtack ⊥ UP TACK */
    new codepair(0x0bc3, 0x2229), /*                      upshoe ∩ INTERSECTION */
    new codepair(0x0bc4, 0x230a), /*                   downstile ⌊ LEFT FLOOR */
    new codepair(0x0bc6, 0x005f), /*                    underbar _ LOW LINE */
    new codepair(0x0bca, 0x2218), /*                         jot ∘ RING OPERATOR */
    new codepair(0x0bcc, 0x2395), /*                        quad ⎕ APL FUNCTIONAL SYMBOL QUAD */
    new codepair(0x0bce, 0x22a4), /*                      uptack ⊤ DOWN TACK */
    new codepair(0x0bcf, 0x25cb), /*                      circle ○ WHITE CIRCLE */
    new codepair(0x0bd3, 0x2308), /*                     upstile ⌈ LEFT CEILING */
    new codepair(0x0bd6, 0x222a), /*                    downshoe ∪ UNION */
    new codepair(0x0bd8, 0x2283), /*                   rightshoe ⊃ SUPERSET OF */
    new codepair(0x0bda, 0x2282), /*                    leftshoe ⊂ SUBSET OF */
    new codepair(0x0bdc, 0x22a2), /*                    lefttack ⊢ RIGHT TACK */
    new codepair(0x0bfc, 0x22a3), /*                   righttack ⊣ LEFT TACK */
    new codepair(0x0cdf, 0x2017), /*        hebrew_doublelowline ‗ DOUBLE LOW LINE */
    new codepair(0x0ce0, 0x05d0), /*                hebrew_aleph א HEBREW LETTER ALEF */
    new codepair(0x0ce1, 0x05d1), /*                  hebrew_bet ב HEBREW LETTER BET */
    new codepair(0x0ce2, 0x05d2), /*                hebrew_gimel ג HEBREW LETTER GIMEL */
    new codepair(0x0ce3, 0x05d3), /*                hebrew_dalet ד HEBREW LETTER DALET */
    new codepair(0x0ce4, 0x05d4), /*                   hebrew_he ה HEBREW LETTER HE */
    new codepair(0x0ce5, 0x05d5), /*                  hebrew_waw ו HEBREW LETTER VAV */
    new codepair(0x0ce6, 0x05d6), /*                 hebrew_zain ז HEBREW LETTER ZAYIN */
    new codepair(0x0ce7, 0x05d7), /*                 hebrew_chet ח HEBREW LETTER HET */
    new codepair(0x0ce8, 0x05d8), /*                  hebrew_tet ט HEBREW LETTER TET */
    new codepair(0x0ce9, 0x05d9), /*                  hebrew_yod י HEBREW LETTER YOD */
    new codepair(0x0cea, 0x05da), /*            hebrew_finalkaph ך HEBREW LETTER FINAL KAF */
    new codepair(0x0ceb, 0x05db), /*                 hebrew_kaph כ HEBREW LETTER KAF */
    new codepair(0x0cec, 0x05dc), /*                hebrew_lamed ל HEBREW LETTER LAMED */
    new codepair(0x0ced, 0x05dd), /*             hebrew_finalmem ם HEBREW LETTER FINAL MEM */
    new codepair(0x0cee, 0x05de), /*                  hebrew_mem מ HEBREW LETTER MEM */
    new codepair(0x0cef, 0x05df), /*             hebrew_finalnun ן HEBREW LETTER FINAL NUN */
    new codepair(0x0cf0, 0x05e0), /*                  hebrew_nun נ HEBREW LETTER NUN */
    new codepair(0x0cf1, 0x05e1), /*               hebrew_samech ס HEBREW LETTER SAMEKH */
    new codepair(0x0cf2, 0x05e2), /*                 hebrew_ayin ע HEBREW LETTER AYIN */
    new codepair(0x0cf3, 0x05e3), /*              hebrew_finalpe ף HEBREW LETTER FINAL PE */
    new codepair(0x0cf4, 0x05e4), /*                   hebrew_pe פ HEBREW LETTER PE */
    new codepair(0x0cf5, 0x05e5), /*            hebrew_finalzade ץ HEBREW LETTER FINAL TSADI */
    new codepair(0x0cf6, 0x05e6), /*                 hebrew_zade צ HEBREW LETTER TSADI */
    new codepair(0x0cf7, 0x05e7), /*                 hebrew_qoph ק HEBREW LETTER QOF */
    new codepair(0x0cf8, 0x05e8), /*                 hebrew_resh ר HEBREW LETTER RESH */
    new codepair(0x0cf9, 0x05e9), /*                 hebrew_shin ש HEBREW LETTER SHIN */
    new codepair(0x0cfa, 0x05ea), /*                  hebrew_taw ת HEBREW LETTER TAV */
    new codepair(0x0da1, 0x0e01), /*                  Thai_kokai ก THAI CHARACTER KO KAI */
    new codepair(0x0da2, 0x0e02), /*                Thai_khokhai ข THAI CHARACTER KHO KHAI */
    new codepair(0x0da3, 0x0e03), /*               Thai_khokhuat ฃ THAI CHARACTER KHO KHUAT */
    new codepair(0x0da4, 0x0e04), /*               Thai_khokhwai ค THAI CHARACTER KHO KHWAI */
    new codepair(0x0da5, 0x0e05), /*                Thai_khokhon ฅ THAI CHARACTER KHO KHON */
    new codepair(0x0da6, 0x0e06), /*             Thai_khorakhang ฆ THAI CHARACTER KHO RAKHANG */
    new codepair(0x0da7, 0x0e07), /*                 Thai_ngongu ง THAI CHARACTER NGO NGU */
    new codepair(0x0da8, 0x0e08), /*                Thai_chochan จ THAI CHARACTER CHO CHAN */
    new codepair(0x0da9, 0x0e09), /*               Thai_choching ฉ THAI CHARACTER CHO CHING */
    new codepair(0x0daa, 0x0e0a), /*               Thai_chochang ช THAI CHARACTER CHO CHANG */
    new codepair(0x0dab, 0x0e0b), /*                   Thai_soso ซ THAI CHARACTER SO SO */
    new codepair(0x0dac, 0x0e0c), /*                Thai_chochoe ฌ THAI CHARACTER CHO CHOE */
    new codepair(0x0dad, 0x0e0d), /*                 Thai_yoying ญ THAI CHARACTER YO YING */
    new codepair(0x0dae, 0x0e0e), /*                Thai_dochada ฎ THAI CHARACTER DO CHADA */
    new codepair(0x0daf, 0x0e0f), /*                Thai_topatak ฏ THAI CHARACTER TO PATAK */
    new codepair(0x0db0, 0x0e10), /*                Thai_thothan ฐ THAI CHARACTER THO THAN */
    new codepair(0x0db1, 0x0e11), /*          Thai_thonangmontho ฑ THAI CHARACTER THO NANGMONTHO */
    new codepair(0x0db2, 0x0e12), /*             Thai_thophuthao ฒ THAI CHARACTER THO PHUTHAO */
    new codepair(0x0db3, 0x0e13), /*                  Thai_nonen ณ THAI CHARACTER NO NEN */
    new codepair(0x0db4, 0x0e14), /*                  Thai_dodek ด THAI CHARACTER DO DEK */
    new codepair(0x0db5, 0x0e15), /*                  Thai_totao ต THAI CHARACTER TO TAO */
    new codepair(0x0db6, 0x0e16), /*               Thai_thothung ถ THAI CHARACTER THO THUNG */
    new codepair(0x0db7, 0x0e17), /*              Thai_thothahan ท THAI CHARACTER THO THAHAN */
    new codepair(0x0db8, 0x0e18), /*               Thai_thothong ธ THAI CHARACTER THO THONG */
    new codepair(0x0db9, 0x0e19), /*                   Thai_nonu น THAI CHARACTER NO NU */
    new codepair(0x0dba, 0x0e1a), /*               Thai_bobaimai บ THAI CHARACTER BO BAIMAI */
    new codepair(0x0dbb, 0x0e1b), /*                  Thai_popla ป THAI CHARACTER PO PLA */
    new codepair(0x0dbc, 0x0e1c), /*               Thai_phophung ผ THAI CHARACTER PHO PHUNG */
    new codepair(0x0dbd, 0x0e1d), /*                   Thai_fofa ฝ THAI CHARACTER FO FA */
    new codepair(0x0dbe, 0x0e1e), /*                Thai_phophan พ THAI CHARACTER PHO PHAN */
    new codepair(0x0dbf, 0x0e1f), /*                  Thai_fofan ฟ THAI CHARACTER FO FAN */
    new codepair(0x0dc0, 0x0e20), /*             Thai_phosamphao ภ THAI CHARACTER PHO SAMPHAO */
    new codepair(0x0dc1, 0x0e21), /*                   Thai_moma ม THAI CHARACTER MO MA */
    new codepair(0x0dc2, 0x0e22), /*                  Thai_yoyak ย THAI CHARACTER YO YAK */
    new codepair(0x0dc3, 0x0e23), /*                  Thai_rorua ร THAI CHARACTER RO RUA */
    new codepair(0x0dc4, 0x0e24), /*                     Thai_ru ฤ THAI CHARACTER RU */
    new codepair(0x0dc5, 0x0e25), /*                 Thai_loling ล THAI CHARACTER LO LING */
    new codepair(0x0dc6, 0x0e26), /*                     Thai_lu ฦ THAI CHARACTER LU */
    new codepair(0x0dc7, 0x0e27), /*                 Thai_wowaen ว THAI CHARACTER WO WAEN */
    new codepair(0x0dc8, 0x0e28), /*                 Thai_sosala ศ THAI CHARACTER SO SALA */
    new codepair(0x0dc9, 0x0e29), /*                 Thai_sorusi ษ THAI CHARACTER SO RUSI */
    new codepair(0x0dca, 0x0e2a), /*                  Thai_sosua ส THAI CHARACTER SO SUA */
    new codepair(0x0dcb, 0x0e2b), /*                  Thai_hohip ห THAI CHARACTER HO HIP */
    new codepair(0x0dcc, 0x0e2c), /*                Thai_lochula ฬ THAI CHARACTER LO CHULA */
    new codepair(0x0dcd, 0x0e2d), /*                   Thai_oang อ THAI CHARACTER O ANG */
    new codepair(0x0dce, 0x0e2e), /*               Thai_honokhuk ฮ THAI CHARACTER HO NOKHUK */
    new codepair(0x0dcf, 0x0e2f), /*              Thai_paiyannoi ฯ THAI CHARACTER PAIYANNOI */
    new codepair(0x0dd0, 0x0e30), /*                  Thai_saraa ะ THAI CHARACTER SARA A */
    new codepair(0x0dd1, 0x0e31), /*             Thai_maihanakat ั THAI CHARACTER MAI HAN-AKAT */
    new codepair(0x0dd2, 0x0e32), /*                 Thai_saraaa า THAI CHARACTER SARA AA */
    new codepair(0x0dd3, 0x0e33), /*                 Thai_saraam ำ THAI CHARACTER SARA AM */
    new codepair(0x0dd4, 0x0e34), /*                  Thai_sarai ิ THAI CHARACTER SARA I */
    new codepair(0x0dd5, 0x0e35), /*                 Thai_saraii ี THAI CHARACTER SARA II */
    new codepair(0x0dd6, 0x0e36), /*                 Thai_saraue ึ THAI CHARACTER SARA UE */
    new codepair(0x0dd7, 0x0e37), /*                Thai_sarauee ื THAI CHARACTER SARA UEE */
    new codepair(0x0dd8, 0x0e38), /*                  Thai_sarau ุ THAI CHARACTER SARA U */
    new codepair(0x0dd9, 0x0e39), /*                 Thai_sarauu ู THAI CHARACTER SARA UU */
    new codepair(0x0dda, 0x0e3a), /*                Thai_phinthu ฺ THAI CHARACTER PHINTHU */
  /*  0x0dde                    Thai_maihanakat_maitho ? ??? */
    new codepair(0x0ddf, 0x0e3f), /*                   Thai_baht ฿ THAI CURRENCY SYMBOL BAHT */
    new codepair(0x0de0, 0x0e40), /*                  Thai_sarae เ THAI CHARACTER SARA E */
    new codepair(0x0de1, 0x0e41), /*                 Thai_saraae แ THAI CHARACTER SARA AE */
    new codepair(0x0de2, 0x0e42), /*                  Thai_sarao โ THAI CHARACTER SARA O */
    new codepair(0x0de3, 0x0e43), /*          Thai_saraaimaimuan ใ THAI CHARACTER SARA AI MAIMUAN */
    new codepair(0x0de4, 0x0e44), /*         Thai_saraaimaimalai ไ THAI CHARACTER SARA AI MAIMALAI */
    new codepair(0x0de5, 0x0e45), /*            Thai_lakkhangyao ๅ THAI CHARACTER LAKKHANGYAO */
    new codepair(0x0de6, 0x0e46), /*               Thai_maiyamok ๆ THAI CHARACTER MAIYAMOK */
    new codepair(0x0de7, 0x0e47), /*              Thai_maitaikhu ็ THAI CHARACTER MAITAIKHU */
    new codepair(0x0de8, 0x0e48), /*                  Thai_maiek ่ THAI CHARACTER MAI EK */
    new codepair(0x0de9, 0x0e49), /*                 Thai_maitho ้ THAI CHARACTER MAI THO */
    new codepair(0x0dea, 0x0e4a), /*                 Thai_maitri ๊ THAI CHARACTER MAI TRI */
    new codepair(0x0deb, 0x0e4b), /*            Thai_maichattawa ๋ THAI CHARACTER MAI CHATTAWA */
    new codepair(0x0dec, 0x0e4c), /*            Thai_thanthakhat ์ THAI CHARACTER THANTHAKHAT */
    new codepair(0x0ded, 0x0e4d), /*               Thai_nikhahit ํ THAI CHARACTER NIKHAHIT */
    new codepair(0x0df0, 0x0e50), /*                 Thai_leksun ๐ THAI DIGIT ZERO */
    new codepair(0x0df1, 0x0e51), /*                Thai_leknung ๑ THAI DIGIT ONE */
    new codepair(0x0df2, 0x0e52), /*                Thai_leksong ๒ THAI DIGIT TWO */
    new codepair(0x0df3, 0x0e53), /*                 Thai_leksam ๓ THAI DIGIT THREE */
    new codepair(0x0df4, 0x0e54), /*                  Thai_leksi ๔ THAI DIGIT FOUR */
    new codepair(0x0df5, 0x0e55), /*                  Thai_lekha ๕ THAI DIGIT FIVE */
    new codepair(0x0df6, 0x0e56), /*                 Thai_lekhok ๖ THAI DIGIT SIX */
    new codepair(0x0df7, 0x0e57), /*                Thai_lekchet ๗ THAI DIGIT SEVEN */
    new codepair(0x0df8, 0x0e58), /*                Thai_lekpaet ๘ THAI DIGIT EIGHT */
    new codepair(0x0df9, 0x0e59), /*                 Thai_lekkao ๙ THAI DIGIT NINE */
    new codepair(0x0ea1, 0x3131), /*               Hangul_Kiyeog ㄱ HANGUL LETTER KIYEOK */
    new codepair(0x0ea2, 0x3132), /*          Hangul_SsangKiyeog ㄲ HANGUL LETTER SSANGKIYEOK */
    new codepair(0x0ea3, 0x3133), /*           Hangul_KiyeogSios ㄳ HANGUL LETTER KIYEOK-SIOS */
    new codepair(0x0ea4, 0x3134), /*                Hangul_Nieun ㄴ HANGUL LETTER NIEUN */
    new codepair(0x0ea5, 0x3135), /*           Hangul_NieunJieuj ㄵ HANGUL LETTER NIEUN-CIEUC */
    new codepair(0x0ea6, 0x3136), /*           Hangul_NieunHieuh ㄶ HANGUL LETTER NIEUN-HIEUH */
    new codepair(0x0ea7, 0x3137), /*               Hangul_Dikeud ㄷ HANGUL LETTER TIKEUT */
    new codepair(0x0ea8, 0x3138), /*          Hangul_SsangDikeud ㄸ HANGUL LETTER SSANGTIKEUT */
    new codepair(0x0ea9, 0x3139), /*                Hangul_Rieul ㄹ HANGUL LETTER RIEUL */
    new codepair(0x0eaa, 0x313a), /*          Hangul_RieulKiyeog ㄺ HANGUL LETTER RIEUL-KIYEOK */
    new codepair(0x0eab, 0x313b), /*           Hangul_RieulMieum ㄻ HANGUL LETTER RIEUL-MIEUM */
    new codepair(0x0eac, 0x313c), /*           Hangul_RieulPieub ㄼ HANGUL LETTER RIEUL-PIEUP */
    new codepair(0x0ead, 0x313d), /*            Hangul_RieulSios ㄽ HANGUL LETTER RIEUL-SIOS */
    new codepair(0x0eae, 0x313e), /*           Hangul_RieulTieut ㄾ HANGUL LETTER RIEUL-THIEUTH */
    new codepair(0x0eaf, 0x313f), /*          Hangul_RieulPhieuf ㄿ HANGUL LETTER RIEUL-PHIEUPH */
    new codepair(0x0eb0, 0x3140), /*           Hangul_RieulHieuh ㅀ HANGUL LETTER RIEUL-HIEUH */
    new codepair(0x0eb1, 0x3141), /*                Hangul_Mieum ㅁ HANGUL LETTER MIEUM */
    new codepair(0x0eb2, 0x3142), /*                Hangul_Pieub ㅂ HANGUL LETTER PIEUP */
    new codepair(0x0eb3, 0x3143), /*           Hangul_SsangPieub ㅃ HANGUL LETTER SSANGPIEUP */
    new codepair(0x0eb4, 0x3144), /*            Hangul_PieubSios ㅄ HANGUL LETTER PIEUP-SIOS */
    new codepair(0x0eb5, 0x3145), /*                 Hangul_Sios ㅅ HANGUL LETTER SIOS */
    new codepair(0x0eb6, 0x3146), /*            Hangul_SsangSios ㅆ HANGUL LETTER SSANGSIOS */
    new codepair(0x0eb7, 0x3147), /*                Hangul_Ieung ㅇ HANGUL LETTER IEUNG */
    new codepair(0x0eb8, 0x3148), /*                Hangul_Jieuj ㅈ HANGUL LETTER CIEUC */
    new codepair(0x0eb9, 0x3149), /*           Hangul_SsangJieuj ㅉ HANGUL LETTER SSANGCIEUC */
    new codepair(0x0eba, 0x314a), /*                Hangul_Cieuc ㅊ HANGUL LETTER CHIEUCH */
    new codepair(0x0ebb, 0x314b), /*               Hangul_Khieuq ㅋ HANGUL LETTER KHIEUKH */
    new codepair(0x0ebc, 0x314c), /*                Hangul_Tieut ㅌ HANGUL LETTER THIEUTH */
    new codepair(0x0ebd, 0x314d), /*               Hangul_Phieuf ㅍ HANGUL LETTER PHIEUPH */
    new codepair(0x0ebe, 0x314e), /*                Hangul_Hieuh ㅎ HANGUL LETTER HIEUH */
    new codepair(0x0ebf, 0x314f), /*                    Hangul_A ㅏ HANGUL LETTER A */
    new codepair(0x0ec0, 0x3150), /*                   Hangul_AE ㅐ HANGUL LETTER AE */
    new codepair(0x0ec1, 0x3151), /*                   Hangul_YA ㅑ HANGUL LETTER YA */
    new codepair(0x0ec2, 0x3152), /*                  Hangul_YAE ㅒ HANGUL LETTER YAE */
    new codepair(0x0ec3, 0x3153), /*                   Hangul_EO ㅓ HANGUL LETTER EO */
    new codepair(0x0ec4, 0x3154), /*                    Hangul_E ㅔ HANGUL LETTER E */
    new codepair(0x0ec5, 0x3155), /*                  Hangul_YEO ㅕ HANGUL LETTER YEO */
    new codepair(0x0ec6, 0x3156), /*                   Hangul_YE ㅖ HANGUL LETTER YE */
    new codepair(0x0ec7, 0x3157), /*                    Hangul_O ㅗ HANGUL LETTER O */
    new codepair(0x0ec8, 0x3158), /*                   Hangul_WA ㅘ HANGUL LETTER WA */
    new codepair(0x0ec9, 0x3159), /*                  Hangul_WAE ㅙ HANGUL LETTER WAE */
    new codepair(0x0eca, 0x315a), /*                   Hangul_OE ㅚ HANGUL LETTER OE */
    new codepair(0x0ecb, 0x315b), /*                   Hangul_YO ㅛ HANGUL LETTER YO */
    new codepair(0x0ecc, 0x315c), /*                    Hangul_U ㅜ HANGUL LETTER U */
    new codepair(0x0ecd, 0x315d), /*                  Hangul_WEO ㅝ HANGUL LETTER WEO */
    new codepair(0x0ece, 0x315e), /*                   Hangul_WE ㅞ HANGUL LETTER WE */
    new codepair(0x0ecf, 0x315f), /*                   Hangul_WI ㅟ HANGUL LETTER WI */
    new codepair(0x0ed0, 0x3160), /*                   Hangul_YU ㅠ HANGUL LETTER YU */
    new codepair(0x0ed1, 0x3161), /*                   Hangul_EU ㅡ HANGUL LETTER EU */
    new codepair(0x0ed2, 0x3162), /*                   Hangul_YI ㅢ HANGUL LETTER YI */
    new codepair(0x0ed3, 0x3163), /*                    Hangul_I ㅣ HANGUL LETTER I */
    new codepair(0x0ed4, 0x11a8), /*             Hangul_J_Kiyeog ᆨ HANGUL JONGSEONG KIYEOK */
    new codepair(0x0ed5, 0x11a9), /*        Hangul_J_SsangKiyeog ᆩ HANGUL JONGSEONG SSANGKIYEOK */
    new codepair(0x0ed6, 0x11aa), /*         Hangul_J_KiyeogSios ᆪ HANGUL JONGSEONG KIYEOK-SIOS */
    new codepair(0x0ed7, 0x11ab), /*              Hangul_J_Nieun ᆫ HANGUL JONGSEONG NIEUN */
    new codepair(0x0ed8, 0x11ac), /*         Hangul_J_NieunJieuj ᆬ HANGUL JONGSEONG NIEUN-CIEUC */
    new codepair(0x0ed9, 0x11ad), /*         Hangul_J_NieunHieuh ᆭ HANGUL JONGSEONG NIEUN-HIEUH */
    new codepair(0x0eda, 0x11ae), /*             Hangul_J_Dikeud ᆮ HANGUL JONGSEONG TIKEUT */
    new codepair(0x0edb, 0x11af), /*              Hangul_J_Rieul ᆯ HANGUL JONGSEONG RIEUL */
    new codepair(0x0edc, 0x11b0), /*        Hangul_J_RieulKiyeog ᆰ HANGUL JONGSEONG RIEUL-KIYEOK */
    new codepair(0x0edd, 0x11b1), /*         Hangul_J_RieulMieum ᆱ HANGUL JONGSEONG RIEUL-MIEUM */
    new codepair(0x0ede, 0x11b2), /*         Hangul_J_RieulPieub ᆲ HANGUL JONGSEONG RIEUL-PIEUP */
    new codepair(0x0edf, 0x11b3), /*          Hangul_J_RieulSios ᆳ HANGUL JONGSEONG RIEUL-SIOS */
    new codepair(0x0ee0, 0x11b4), /*         Hangul_J_RieulTieut ᆴ HANGUL JONGSEONG RIEUL-THIEUTH */
    new codepair(0x0ee1, 0x11b5), /*        Hangul_J_RieulPhieuf ᆵ HANGUL JONGSEONG RIEUL-PHIEUPH */
    new codepair(0x0ee2, 0x11b6), /*         Hangul_J_RieulHieuh ᆶ HANGUL JONGSEONG RIEUL-HIEUH */
    new codepair(0x0ee3, 0x11b7), /*              Hangul_J_Mieum ᆷ HANGUL JONGSEONG MIEUM */
    new codepair(0x0ee4, 0x11b8), /*              Hangul_J_Pieub ᆸ HANGUL JONGSEONG PIEUP */
    new codepair(0x0ee5, 0x11b9), /*          Hangul_J_PieubSios ᆹ HANGUL JONGSEONG PIEUP-SIOS */
    new codepair(0x0ee6, 0x11ba), /*               Hangul_J_Sios ᆺ HANGUL JONGSEONG SIOS */
    new codepair(0x0ee7, 0x11bb), /*          Hangul_J_SsangSios ᆻ HANGUL JONGSEONG SSANGSIOS */
    new codepair(0x0ee8, 0x11bc), /*              Hangul_J_Ieung ᆼ HANGUL JONGSEONG IEUNG */
    new codepair(0x0ee9, 0x11bd), /*              Hangul_J_Jieuj ᆽ HANGUL JONGSEONG CIEUC */
    new codepair(0x0eea, 0x11be), /*              Hangul_J_Cieuc ᆾ HANGUL JONGSEONG CHIEUCH */
    new codepair(0x0eeb, 0x11bf), /*             Hangul_J_Khieuq ᆿ HANGUL JONGSEONG KHIEUKH */
    new codepair(0x0eec, 0x11c0), /*              Hangul_J_Tieut ᇀ HANGUL JONGSEONG THIEUTH */
    new codepair(0x0eed, 0x11c1), /*             Hangul_J_Phieuf ᇁ HANGUL JONGSEONG PHIEUPH */
    new codepair(0x0eee, 0x11c2), /*              Hangul_J_Hieuh ᇂ HANGUL JONGSEONG HIEUH */
    new codepair(0x0eef, 0x316d), /*     Hangul_RieulYeorinHieuh ㅭ HANGUL LETTER RIEUL-YEORINHIEUH */
    new codepair(0x0ef0, 0x3171), /*    Hangul_SunkyeongeumMieum ㅱ HANGUL LETTER KAPYEOUNMIEUM */
    new codepair(0x0ef1, 0x3178), /*    Hangul_SunkyeongeumPieub ㅸ HANGUL LETTER KAPYEOUNPIEUP */
    new codepair(0x0ef2, 0x317f), /*              Hangul_PanSios ㅿ HANGUL LETTER PANSIOS */
    new codepair(0x0ef3, 0x3181), /*    Hangul_KkogjiDalrinIeung ㆁ HANGUL LETTER YESIEUNG */
    new codepair(0x0ef4, 0x3184), /*   Hangul_SunkyeongeumPhieuf ㆄ HANGUL LETTER KAPYEOUNPHIEUPH */
    new codepair(0x0ef5, 0x3186), /*          Hangul_YeorinHieuh ㆆ HANGUL LETTER YEORINHIEUH */
    new codepair(0x0ef6, 0x318d), /*                Hangul_AraeA ㆍ HANGUL LETTER ARAEA */
    new codepair(0x0ef7, 0x318e), /*               Hangul_AraeAE ㆎ HANGUL LETTER ARAEAE */
    new codepair(0x0ef8, 0x11eb), /*            Hangul_J_PanSios ᇫ HANGUL JONGSEONG PANSIOS */
    new codepair(0x0ef9, 0x11f0), /*  Hangul_J_KkogjiDalrinIeung ᇰ HANGUL JONGSEONG YESIEUNG */
    new codepair(0x0efa, 0x11f9), /*        Hangul_J_YeorinHieuh ᇹ HANGUL JONGSEONG YEORINHIEUH */
    new codepair(0x0eff, 0x20a9), /*                  Korean_Won ₩ WON SIGN */
    new codepair(0x13bc, 0x0152), /*                          OE Œ LATIN CAPITAL LIGATURE OE */
    new codepair(0x13bd, 0x0153), /*                          oe œ LATIN SMALL LIGATURE OE */
    new codepair(0x13be, 0x0178), /*                  Ydiaeresis Ÿ LATIN CAPITAL LETTER Y WITH DIAERESIS */
    new codepair(0x20ac, 0x20ac), /*                    EuroSign € EURO SIGN */
    new codepair(0xfe50, 0x0300), /*                               COMBINING GRAVE ACCENT */
    new codepair(0xfe51, 0x0301), /*                               COMBINING ACUTE ACCENT */
    new codepair(0xfe52, 0x0302), /*                               COMBINING CIRCUMFLEX ACCENT */
    new codepair(0xfe53, 0x0303), /*                               COMBINING TILDE */
    new codepair(0xfe54, 0x0304), /*                               COMBINING MACRON */
    new codepair(0xfe55, 0x0306), /*                               COMBINING BREVE */
    new codepair(0xfe56, 0x0307), /*                               COMBINING DOT ABOVE */
    new codepair(0xfe57, 0x0308), /*                               COMBINING DIAERESIS */
    new codepair(0xfe58, 0x030a), /*                               COMBINING RING ABOVE */
    new codepair(0xfe59, 0x030b), /*                               COMBINING DOUBLE ACUTE ACCENT */
    new codepair(0xfe5a, 0x030c), /*                               COMBINING CARON */
    new codepair(0xfe5b, 0x0327), /*                               COMBINING CEDILLA */
    new codepair(0xfe5c, 0x0328), /*                               COMBINING OGONEK */
    new codepair(0xfe5d, 0x1da5), /*                               MODIFIER LETTER SMALL IOTA */
    new codepair(0xfe5e, 0x3099), /*                               COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK */
    new codepair(0xfe5f, 0x309a), /*                               COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
    new codepair(0xfe60, 0x0323), /*                               COMBINING DOT BELOW */
  };

  public static combiningpair[] combinetab = {
    new combiningpair(0x0060, 0x0300), /*                GRAVE ACCENT ` COMBINING GRAVE ACCENT */
    new combiningpair(0x00b4, 0x0301), /*                ACUTE ACCENT ´ COMBINING ACUTE ACCENT */
    new combiningpair(0x0027, 0x0301), /*                  APOSTROPHE ' COMBINING ACUTE ACCENT */
    new combiningpair(0x0384, 0x0301), /*                 GREEK TONOS ΄ COMBINING ACUTE ACCENT */
    new combiningpair(0x005e, 0x0302), /*           CIRCUMFLEX ACCENT ^ COMBINING CIRCUMFLEX ACCENT */
    new combiningpair(0x007e, 0x0303), /*                       TILDE ~ COMBINING TILDE */
    new combiningpair(0x00af, 0x0304), /*                      MACRON ¯ COMBINING MACRON */
    new combiningpair(0x02d8, 0x0306), /*                       BREVE ˘ COMBINING BREVE */
    new combiningpair(0x02d9, 0x0307), /*                   DOT ABOVE ˙ COMBINING DOT ABOVE */
    new combiningpair(0x00a8, 0x0308), /*                   DIAERESIS ¨ COMBINING DIAERESIS */
    new combiningpair(0x0022, 0x0308), /*              QUOTATION MARK " COMBINING DIAERESIS */
    new combiningpair(0x02da, 0x030a), /*                  RING ABOVE ˚ COMBINING RING ABOVE */
    new combiningpair(0x00b0, 0x030a), /*                 DEGREE SIGN ° COMBINING RING ABOVE */
    new combiningpair(0x02dd, 0x030b), /*         DOUBLE ACUTE ACCENT ˝ COMBINING DOUBLE ACUTE ACCENT */
    new combiningpair(0x02c7, 0x030c), /*                       CARON ˇ COMBINING CARON */
    new combiningpair(0x00b8, 0x0327), /*                     CEDILLA ¸ COMBINING CEDILLA */
    new combiningpair(0x02db, 0x0328), /*                      OGONEK ¸ COMBINING OGONEK */
    new combiningpair(0x037a, 0x0345), /*         GREEK YPOGEGRAMMENI ͺ COMBINING GREEK YPOGEGRAMMENI */
    new combiningpair(0x309b, 0x3099), /* KATAKANA-HIRAGANA VOICED SOUND MARK ゛COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK */
    new combiningpair(0x309c, 0x309a), /* KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK ゜COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
    new combiningpair(0x002e, 0x0323), /*                   FULL STOP . COMBINING DOT BELOW */
    new combiningpair(0x0385, 0x0344), /*       GREEK DIALYTIKA TONOS ΅ COMBINING GREEK DIALYTIKA TONOS */
  };

  public static int keysym2ucs(int keysym)
  {
    int min = 0;
    int max = keysymtab.length - 1;
    int mid;

    /* first check for Latin-1 characters (1:1 mapping) */
    if ((keysym >= 0x0020 && keysym <= 0x007e) ||
        (keysym >= 0x00a0 && keysym <= 0x00ff))
      return keysym;

    /* also check for directly encoded 24-bit UCS characters */
    if ((keysym & 0xff000000) == 0x01000000)
      return keysym & 0x00ffffff;

    /* binary search in table */
    while (max >= min) {
      mid = (min + max) / 2;
      if (keysymtab[mid].keysym < keysym)
        min = mid + 1;
      else if (keysymtab[mid].keysym > keysym)
        max = mid - 1;
      else {
        /* found it */
        return keysymtab[mid].ucs;
      }
    }

    /* no matching Unicode value found */
    return -1;
  }

  public static int ucs2keysym(int ucs) {
    int cur = 0;
    int max = keysymtab.length - 1;

    /* first check for Latin-1 characters (1:1 mapping) */
    if ((ucs >= 0x0020 && ucs <= 0x007e) ||
        (ucs >= 0x00a0 && ucs <= 0x00ff))
      return ucs;

    /* linear search in table */
    while (cur <= max) {
      if (keysymtab[cur].ucs == ucs)
        return keysymtab[cur].keysym;
      cur++;
    }

    /* use the directly encoded 24-bit UCS character */
    if ((ucs & 0xff000000) == 0)
      return ucs | 0x01000000;

    /* no matching Unicode value found */
    return NoSymbol;
  }

  public static int ucs2combining(int spacing)
  {
    int cur = 0;
    int max = combinetab.length - 1;

    /* linear search in table */
    while (cur <= max) {
      if (combinetab[cur].spacing == spacing)
        return combinetab[cur].combining;
      cur++;
    }

    /* no matching Unicode value found */
    return -1;
  }

  private static final int NoSymbol = 0;
}
