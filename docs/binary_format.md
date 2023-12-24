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
5. (Optional) 8192 bytes for save data (ensures 4K-alignment)

Four additional bytes are reserved for the data and save page offset, and the length is then padded to 256 bytes. Finally, the data page is placed in the final two bytes, and the save page in the two bytes preceeding it. The FX initialization logic is below.

```cpp
FX::begin(0);

// read data page if we are in dev mode
if(FX::programDataPage == 0)
{
    FX::readDataBytes(
        uint24_t(16) * 1024 * 1024 - 2,
        (uint8_t*)&FX::programDataPage,
        2);
}

// figure out save page
{
    uint16_t save_page;
    FX::readDataBytes(
        uint24_t(16) * 1024 * 1024 - 4,
        (uint8_t*)&save_page,
        2);
    if(save_page != 0)
        FX::programSavePage = (FX::programDataPage + save_page) & 0xfff0;
}
```