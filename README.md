# binaryreader

binaryreader is a faster and more elegant alternative to ``struct`` for parsing binary data in Python.

## Example

```python
from binaryreader import BinaryReader

data: bytes|bytearray
read_little_endian: bool = True
reader = BinaryReader(data, read_little_endian)

text = reader.readStringC()
```

## Benchmark

TODO


## Documentation

### Notes:
- kwargs won't be accepted, only args are useable
- all readArray functions accept a langth as optional argument
- bytes can be read by using readUInt8Array


### Init
- ``BinaryReader(data: bytes|bytearray, is_little_endian: bool)``

### Properties

- ``.endian: bool``\[get,set\] - endianness of the reader (True - little, False - big)
- ``.position: int``\[get,set\] - position of the cursor within the data
- ``.size: int``\[get\] - size of underlying/passed object
- ``.obj: bytes|bytearray``\[get\] - underlying/passed object

### Functions
- ``.readBool(): bool`` - reads a bool
- ``.readInt8(): int`` - reads an int8
- ``.readUInt8(): int`` - reads an uint8
- ``.readInt16(): int`` - reads an int16
- ``.readUInt16(): int`` - reads an uint16
- ``.readInt32(): int`` - reads an int32
- ``.readUInt32(): int`` - reads an uint32
- ``.readInt64(): int`` - reads an int64
- ``.readUInt64(): int`` - reads an uint64
- ``.readHalf(): float`` - reads a half
- ``.readFloat(): float`` - reads a float
- ``.readDouble(): float`` - reads a double
- ``.readBoolArray(): [bool]`` - reads a bool array
- ``.readInt8Array(): [int]`` - reads a array of int8
- ``.readUInt8Array(): bytearray`` - reads a array of uint8
- ``.readInt16Array(): [int]`` - reads a array of int16
- ``.readUInt16Array(): [int]`` - reads a array of uint16
- ``.readInt32Array(): [int]`` - reads a array of int32
- ``.readUInt32Array(): [int]`` - reads a array of uint32
- ``.readInt64Array(): [int]`` - reads a array of int64
- ``.readUInt64Array(): [int]`` - reads a array of uint64
- ``.readHalfArray(): [float]`` - reads a array of half
- ``.readFloatArray(): [float]`` - reads a array of float
- ``.readDoubleArray(): [float]`` - reads a array of double
- ``.readStringC(): str`` - reads a null terminated string
- ``.readStringCArray(): [str]`` - reads an array of null terminated strings
- ``.readString(): str`` - reads a string (if length is not passed as arg, read an int as length)
- ``.readStringArray(): [str]`` - reads an array of strings
- ``.readStringAligned(): str`` - same as readString but aligned to 4 bytes after reading the string
- ``.readStringAlignedArray(): [str]`` - reads an array of aligned strings
- ``.align(align_by: int): int`` - aligns the cursor to the given input and returns the position after the alignment
- ``.readVarInt(): int`` - reads a varint
- ``.readLSB(): bytearray`` - reads the lsb data of the given size (in bytes to read -> output length is 1/8 of that)
