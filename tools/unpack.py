#!/usr/bin/env python3
"""The game, unpacked by LOADER's own decruncher.

    unpack.py           game/!JamesPond/JamesCode -> unpacked/jamescode (the
                        image LOADER runs, at &B000)

!run loads LOADER at &8000 and runs it. LOADER shows the Krisalis logo, then
OS_File-loads JamesCode at &AFFC and unpacks it to &B000 (&9768: r9 the
packed file, r10 where it goes; &8284 is a backwards decruncher from the
Amiga original, as in Chuck Rock: a big-endian length, the data, then its
unpacked length and a checksum, so the file unpacks in place over itself)
and branches to &B000 (r5 is then 0: the checksum). Here that routine runs
under SuperSpider's tools/arm2.py.
"""
import os
import struct
import sys

SS = os.environ.get('SUPERSPIDER', os.path.expanduser('~/stuff/superspider'))
sys.path.insert(0, os.path.join(SS, 'tools'))
import arm2  # noqa: E402

TOP = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..')
APP = os.path.join(TOP, 'game', '!JamesPond')
OUT = os.path.join(TOP, 'unpacked')
LOADER, PACKED, GAME, UNPACK = 0x8000, 0xAFFC, 0xB000, 0x8284
STACK, DONE = 0x7F000, 0x3FFFFF0


def find(d, name):
    """a RISC OS file in a host directory: any case, any ,load-exec suffix"""
    for f in os.listdir(d):
        if f.split(',')[0].lower() == name.lower():
            return os.path.join(d, f)
    raise FileNotFoundError(os.path.join(d, name))


def unpack(loader, packed):
    m = arm2.ARM(memsize=0x4000000)
    m.mem[LOADER:LOADER + len(loader)] = loader
    m.mem[PACKED:PACKED + len(packed)] = packed
    m.r[9], m.r[10] = PACKED, GAME
    m.r[13], m.r[14] = STACK - 4, DONE
    struct.pack_into('<I', m.mem, STACK - 4, DONE)      # (it returns with ldmia sp!,{pc})
    m.r[15] = UNPACK
    while m.r[15] != DONE:
        m.step()
        if m.steps > 50_000_000:
            raise RuntimeError('unpack: no end, pc=&%X' % m.r[15])
    if m.r[5]:                                          # the checksum, eored down to 0
        sys.exit('unpack: JamesCode fails its checksum (a damaged dump?)')
    n = struct.unpack_from('>I', packed, 4 + struct.unpack_from('>I', packed, 0)[0] - 4)[0]
    return bytes(m.mem[GAME:GAME + n])


def main():
    os.makedirs(OUT, exist_ok=True)
    data = unpack(open(find(APP, 'LOADER'), 'rb').read(), open(find(APP, 'JamesCode'), 'rb').read())
    open(os.path.join(OUT, 'jamescode'), 'wb').write(data)
    print('jamescode: %d bytes at &%X' % (len(data), GAME))


if __name__ == '__main__':
    main()
