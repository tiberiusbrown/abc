# ABC Compiler and Interpreter for the Arduboy FX

**Note: ABC is currently unstable! There have been no releases yet.**

ABC is a C-like scripting language with an associated interpreter designed for the Arduboy FX. It includes runtime error checking and support for various FX assets, such as sprites, fonts, strings, and arbitrary data. Read the [language reference](docs/language.md) or try out the [online IDE](https://tiberiusbrown.github.io/abc/).

```c
int x = 20, y = 30;
int dx = 1, dy = -1;

void loop()
{
    // draw some text with one of the built-in fonts
    $draw_text(29, 36, "Hello World!");
        
    x += dx;
    y += dy;
    if(x < 0 || x > 120) { dx = -dx; x += dx * 2; }
    if(y < 0 || y >  56) { dy = -dy; y += dy * 2; }
    
    // Draw an 8x8 sprite with a sprite literal: sprites{ WxH [pixels] }
    //     '-' is a transparent pixel
    //     '.' is a black pixel
    //     all other non-whitespace characters are white pixels
    $draw_sprite(x, y, sprites{
        8x8
        --XXXX--
        -X....X-
        X......X
        X......X
        X......X
        X......X
        -X....X-
        --XXXX-- }, 0);
    
    // push display buffer to display and clear it
    $display();
}

void main()
{
    while(true)
        loop();
}
```

![Hello World](docs/recording_helloworld.gif)
