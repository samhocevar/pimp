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

    /* Load image */
    Image image;
    image.Load("input.jpeg");

    /* Copy image to another variable */
    Image other = image;
    ivec2 size = other.GetSize();
    u8vec4 *data = other.Lock<PixelFormat::RGBA_8>();

    for (int n = 0; n < size.x * size.y; ++n)
    {
        data[n] = data[n].bgra;
    }

    other.Unlock(data);
    other.Save("output1.jpeg");

    /* More tests */
    Image tmp;
    tmp.RenderHalftone(ivec2(128, 128));
    // FIXME: this should not be necessary
    tmp.SetFormat(PixelFormat::RGBA_F32);
    tmp.Save("output2.jpeg");

    return 0;
}

