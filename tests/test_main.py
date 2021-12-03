from struct import unpack_from, Struct, unpack, pack
from binaryreader import BinaryReader

TESTS = [
    ("Bool", "?", 1),
    ("Int8", "b", -8),
    ("UInt8", "B", 8),
    ("Int16", "h", -16),
    ("UInt16", "H", 16),
    ("Int32", "i", -32),
    ("UInt32", "I", 32),
    ("Int64", "q", -64),
    ("UInt64", "Q", 64),
    ("Half", "e", 2.0),
    ("Float", "f", 4.0),
    ("Double", "d", 8.0),
]

# generate the tests
for name, fmt, value in TESTS:
    if 1:
        exec(
            f"""
def test_{name}():
    print("Test {name}")
    for endian in ["<", ">"]:
        data = Struct(endian + "{fmt}").pack({value})
        br = BinaryReader(data, endian == "<")
        br_value = br.read{name}()
        print({value}, br_value)
        assert(br_value == {value})
    return br
"""
        )
    if 1:
        exec(
            f"""
def test_{name}Array():
    print("Test {name}Array")
    if isinstance({value}, (int, float)):
        array = [({value}**i)%127 for i in range(10)]
    elif isinstance({value}, (str, bytes)):
        array = [{value}*i for i in range(10)]
    for endian in ["<", ">"]:
        data = Struct(endian + "i").pack(10)
        data += Struct(endian + "{fmt}"*10).pack(*array)
        br = BinaryReader(data, endian == "<")
        br_array = br.read{name}Array()
        print(array)
        print(br_array)
        assert(all(x == y for x,y in zip(br_array, array)))
    return br
"""
        )


def test_stringC():
    print("Test stringC")
    value = "StringC"
    data = value.encode("utf-8") + b"\x00"
    br_value = BinaryReader(data).readStringC()
    print(value, br_value)
    assert br_value == value


def test_string():
    print("Test string")
    value = "StringLengthDelimited"
    data = Struct("<i").pack(len(value)) + value.encode("utf-8")
    br_value = BinaryReader(data, True).readString()
    print(value, br_value)
    assert br_value == value
    data = value.encode("utf-8")
    br_value = BinaryReader(data, True).readString(len(data))
    print(value, br_value)
    assert br_value == value


def test_string_aligned():
    print("Test string aligned")
    value = "StringAlligned"
    data = value.encode("utf-8")
    data = Struct("<i").pack(len(value)) + data
    # align data to multiple of 4 and add 1 to check if the allignment is correct
    data += b"\x00" * (4 - (len(data) % 4))
    data += b"\x01"
    br = BinaryReader(data, True)
    br_value = br.readStringAligned()
    print(value, br_value)
    assert br_value == value
    assert br.readUInt8() == 1


def run_tests():
    print("Running tests")
    for key, value in list(globals().items()):
        if key.startswith("test_"):
            br = value()
            print()


if __name__ == "__main__":
    run_tests()
