import random

random.seed(1234);

def test(f, a, b):
    neg = (1 if a < 0 else 0) ^ (1 if b < 0 else 0)
    q = int(abs(a) / abs(b))
    if neg != 0: q = -q
    f.write(
        '    a = 0x%08x; b = 0x%08x; $assert(a / b == 0x%08x);\n' %
        (a & 0xffffffff, b & 0xffffffff, q & 0xffffffff))

def test_range(f, signed, astop, bstop = None):
    for n in range(20):
        a = random.randrange(-astop if signed else 2, astop);
        stop = int(abs(a) if bstop is None else bstop)
        if stop < 2:
            stop = 2
        b = random.randrange(-stop if signed else 1, stop);
        if b == 0:
            b = 1
        test(f, a, b);

with open('div.abc', 'w') as f:
    f.write('void main()\n{\n');
    f.write('    $debug_break();\n');
    
    f.write('    {\n');
    f.write('    u32 a, b;\n');
    test_range(f, False, 1 << 8)
    test_range(f, False, 1 << 16)
    test_range(f, False, 1 << 24)
    test_range(f, False, 1 << 32)
    test_range(f, False, 1 << 16, 1 << 8)
    test_range(f, False, 1 << 24, 1 << 16)
    test_range(f, False, 1 << 24, 1 << 8)
    test_range(f, False, 1 << 32, 1 << 24)
    test_range(f, False, 1 << 32, 1 << 16)
    test_range(f, False, 1 << 32, 1 << 8)
    f.write('    }\n');
    
    f.write('    {\n');
    f.write('    i32 a, b;\n');
    test_range(f, True, 1 << 8)
    test_range(f, True, 1 << 16)
    test_range(f, True, 1 << 24)
    test_range(f, True, 1 << 31)
    test_range(f, True, 1 << 16, 1 << 8)
    test_range(f, True, 1 << 24, 1 << 16)
    test_range(f, True, 1 << 24, 1 << 8)
    test_range(f, True, 1 << 31, 1 << 24)
    test_range(f, True, 1 << 31, 1 << 16)
    test_range(f, True, 1 << 31, 1 << 8)
    f.write('    }\n');
    
    f.write('    {\n');
    f.write('    u16 a, b;\n');
    test_range(f, False, 1 << 8)
    test_range(f, False, 1 << 16)
    test_range(f, False, 1 << 16, 1 << 8)
    f.write('    }\n');
    
    f.write('    {\n');
    f.write('    i16 a, b;\n');
    test_range(f, True, 1 << 8)
    test_range(f, True, 1 << 15)
    test_range(f, True, 1 << 15, 1 << 8)
    f.write('    }\n');
    
    f.write('    $debug_break();\n');
    f.write('}\n');
    