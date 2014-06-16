//
//  Pimp - Pathetic Image Manipulation Program
//
//  Copyright (c) 2008-2014 Sam Hocevar <sam@hocevar.net>
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the Do What The Fuck You Want To
//   Public License, Version 2, as published by Sam Hocevar. See
//   http://www.wtfpl.net/ for more details.
//

#if defined HAVE_CONFIG_H
#   include "config.h"
#endif

#include "core.h"

using namespace lol;

int main(int argc, char **argv)
{
    UNUSED(argc, argv);

    Image image;
    image.Load("input.jpeg");
    ivec2 size = image.GetSize();
    u8vec4 *data = image.Lock<PixelFormat::RGBA_8>();

    for (int n = 0; n < size.x * size.y; ++n)
    {
        data[n] = data[n].bgra;
    }

    image.Unlock();
    image.Save("output.jpeg");

    return 0;
}

