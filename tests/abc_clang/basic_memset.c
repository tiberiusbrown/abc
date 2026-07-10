
typedef unsigned char u8;
typedef unsigned short u16;

extern void *abc_memset(void *dst, int value, u16 n) asm("$memset");

static char buf[8];

void main()
{
    abc_memset(buf, 0, sizeof(buf));
}
