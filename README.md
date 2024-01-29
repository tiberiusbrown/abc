# ABC Compiler and Interpreter for the Arduboy FX

ABC is a C-like scripting language with an associated interpreter designed for the Arduboy FX. It includes runtime error checking and support for various FX assets, such as sprites, fonts, strings, and arbitrary data. Read the [language reference](docs/language.md) or try out the [online IDE](https://tiberiusbrown.github.io/abc/).

```c
void main()
{
    while(true)
        loop();
}

void loop()
{
    while(!$next_frame())
        ;
    $draw_text_P(29, 28, FONT_LEXIS, "Hello World!");
    $display();
}
```
