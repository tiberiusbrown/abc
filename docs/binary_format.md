# Binary Format

The compiled binary begins with a 256 byte header.

| Offset | Size | Description |
|---|---|---|
| 0x00 | 4 | Signature: `AB C0 0A BC` |
| 0x04 | 6 | Padding |
| 0x0a | 2 | Save Size (must be 0 if no save sector) |
| 0x0c | 1 | File Table Length |
| 0x0d | 3 | File Table Offset |
| 0x10 | 3 | Line Table Offset |
| 0x13 | 1 | Unused |
| 0x14 | 12 | Entry point for execution: `call $globinit`, `call main`, `jmp 20` |
| 0x20 | 224 | Unused |

Following the header is

1. Program non-bytecode data
2. Program bytecode
3. File Table
4. Line Table

Six additional bytes are reserved for the data page offset (for dev purposes only) and end-of-data signature (`AB CE EA BC`), and the length is then padded to 256 bytes. Finally, the dev data page and signature is placed in the final two bytes. The FX initialization logic is below.

```cpp
FX::begin(0x0000, 0xfff0);

// figure out data/save pages if we are in dev mode
if(FX::programDataPage == 0x0000)
{
    // look for signature
    uint24_t addr = uint24_t(16) * 1024 * 1024 - 4;
    FX::seekData(addr);
    uint32_t sig = FX::readPendingLastUInt32();
    if(sig != 0xABCEEABC)
    {
        // we might have a save page: look back one 4K sector
        addr -= 4096;
        FX::seekData(addr);
        sig = FX::readPendingLastUInt32();
        if(sig != 0xABCEEABC)
            vm_error(ards::ERR_SIG);
    }
    // read the data page from end-of-data
    FX::readDataBytes(
        addr - 2,
        (uint8_t*)&FX::programDataPage,
        2);
}
```