# DuhView

DuhView is an ANSI art viewer written in C and SDL.

ANSI art is a low resolution art form constructed from IBM code page 437
symbols and ANSI escape sequences that color text with 16 foreground and 8
background colors. You can find lots of past and present ANSI art packs at
[Sixteen Colors][Sixteen Colors].

## Building DuhView

To build DuhView, run:

```
$ make
```

To view an ANSI:

```
$ ./duhview examples/skull.ans
```

## Credits and License

CP437 font from [escape.js][escape.js].

Source code is

```
Copyright (c) 2011-2012  Pekka Enberg

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

[Sixteen Colors]: http://sixteencolors.net/
[escape.js]: http://256.io/escapes.js/
