#!/usr/bin/env python3

import os
import struct
import sys

POLY = 0x04C11DB7

# Matches STM32C0 CRC peripheral defaults.
def crc32(data):
    crc = 0xFFFFFFFF
    for (word,) in struct.iter_unpack('<I', data):
        crc ^= word
        for _ in range(32):
            crc = ((crc << 1) ^ POLY if crc & 0x80000000 else crc << 1) & 0xFFFFFFFF
    return crc


def self_test():
    data = b'test implementation against known data\xff\xff'
    assert crc32(data) == 0x10fda4d3, hex(crc32(data))
    assert crc32(data + struct.pack('<I', crc32(data))) == 0


def main(argv):
    self_test()
    if len(argv) != 4:
        print(f'usage: {argv[0]} <in.bin> <target-size> <out.bin>')
        return 1

    _, in_path, target_size, out_path = argv
    target_size = int(target_size, 0)
    if target_size < 4 or target_size % 4 != 0:
        print(f'target size must be a positive multiple of 4: {target_size}')
        return 1

    if os.path.abspath(in_path) == os.path.abspath(out_path):
        print(f'refusing to overwrite input file: {in_path}')
        return 1

    body_size = target_size - 4
    with open(in_path, 'rb') as f:
        data = f.read()

    if len(data) > body_size:
        print(f'{in_path} too large: {len(data)} B (max {body_size})')
        return 1

    padded = data + b'\xff' * (body_size - len(data))
    with open(out_path, 'wb') as f:
        f.write(padded)
        f.write(struct.pack('<I', crc32(padded)))

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
