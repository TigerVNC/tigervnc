#!/usr/bin/python3

import os
import re

origin = os.path.realpath(os.path.dirname(__file__))
fn = os.path.join(origin, '..', 'common', 'rfb', 'keysymdef.h')

keys = {}
prog = re.compile('#define\s+XK_([^\s]+)\s*0x([0-9A-Fa-f]+)\s*/[*].\s*U[+]([0-9A-Fa-f]+)\s+([^*]+)\s*.[*]/')
with open(fn) as f:
    for line in f:
        m = prog.search(line)
        if m is None:
            continue

        (ksname, ks, ucs, ucsname) = m.group(1, 2, 3, 4)
        ks = int(ks, 16)
        ucs = int(ucs, 16)

        if (ks == ucs) and \
           (((ks >= 0x20) and (ks <= 0x7f)) or \
            ((ks >= 0xa0) and (ks <= 0xff))):
            continue
        if (ks & 0xff000000) == 0x01000000:
            assert ks == ucs | 0x01000000
            continue

        assert ks not in keys
        keys[ks] = { 'name': ksname, 'ucs': ucs, 'ucsname': ucsname }

print("""/*
 * This file is auto-generated from keysymdef.h
 */

struct codepair {
  unsigned short keysym;
  unsigned short ucs;
};

static const struct codepair keysymtab[] = {""")

maxlen = max([ len(keys[ks]['name']) for ks in keys ])
for ks in sorted(keys):
    key = keys[ks]
    if (key['ucs'] < 0x20) or key['ucs'] == 0x7f:
        ch = ' '
    else:
        ch = chr(key['ucs'])
    print("  { 0x%04x, 0x%04x }, /* %0*s %s %s */" %
          (ks, key['ucs'], maxlen, key['name'], ch, key['ucsname']))

print("};")
