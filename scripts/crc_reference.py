"""Generate independently checked CRC vectors and per-byte host-frame traces."""
import binascii

def crc_arithmetic(data):
    crc = 65535
    for byte in data:
        crc ^= byte * 256
        for _ in range(8):
            top = crc >= 32768
            crc = (crc * 2) % 65536
            if top:
                crc ^= 4129
    return crc

vectors = {
    'empty': b'',
    'ASCII 123456789': bytes.fromhex('31 32 33 34 35 36 37 38 39'),
    'HEAD pen20': bytes.fromhex('02 10 78 56 34 12 03 00 00 00 14 00'),
    'HEAD pen20B': bytes.fromhex('02 10 78 56 34 12 03 00 00 00 14 42'),
    'all byte values 00..FF': bytes(range(256)),
}
for name, data in vectors.items():
    value = crc_arithmetic(data)
    assert value == binascii.crc_hqx(data, 0xffff)
    print(f'{name}: CRC={value:04X}; decimal={value}; wire={value%256:02X} {value//256:02X}')
body = vectors['HEAD pen20']
print('Frame:', (bytes.fromhex('AA 55')+body+crc_arithmetic(body).to_bytes(2, 'little')).hex(' ').upper())
print('Per-byte trace: index byte CRC')
crc = 0xffff
for index, byte in enumerate(body, 1):
    crc = binascii.crc_hqx(bytes([byte]), crc)
    print(f'{index:02d} {byte:02X} {crc:04X}')
print('First byte bit trace after initial XOR: FDFF')
crc = 0xffff ^ (body[0] << 8)
for bit in range(1, 9):
    top = crc >= 0x8000
    crc = (crc * 2) % 65536
    if top: crc ^= 0x1021
    print(f'{bit}: MSB={int(top)} CRC={crc:04X}')
