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
        /* Swap R and B */
        data[n] = data[n].bgra;

        /* Reduce red value */
        data[n].r *= 0.9f;
    }
    other.Unlock(data);
    other.Save("output1.jpeg");

    /* Dither to black and white */
    //Image kernel;
    //kernel.Stock("ediff:jajuni");
    //other = other.DitherEdiff(kernel, ScanMode::Serpentine);
    other = other.DitherOstromoukhov(ScanMode::Serpentine);
    other.Save("output2.jpeg");

    /* More tests */
    Image tmp;
    tmp.RenderRandom(ivec2(256, 256));
    // FIXME: this should not be necessary
    tmp.SetFormat(PixelFormat::RGBA_F32);
    tmp.Save("output3.jpeg");

    return 0;
}

