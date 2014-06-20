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

void DoSomeSeriousShit(Image &image)
{
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

    /* Convolution */
    Array2D<float> kernel(3, 3);
    kernel[0][0] = -3; kernel[1][0] = -5; kernel[2][0] = -3;
    kernel[0][1] = 0;  kernel[1][1] = 0;  kernel[2][1] = 0;
    kernel[0][2] = 3;  kernel[1][2] = 5;  kernel[2][2] = 3;
    other = other.Convolution(kernel);

    other.Save("output1.jpeg");

    /* Dither to black and white */
    //Image kernel;
    //kernel.Stock("ediff:jajuni");
    //other = other.DitherEdiff(kernel, ScanMode::Serpentine);
    other = other.DitherOstromoukhov(ScanMode::Serpentine);
    other.Save("output2.jpeg");

#if 0
    /* Convert to braille! */
    {
        float *pixels = other.Lock<PixelFormat::Y_F32>();
        for (int y = 0; y < size.y; y += 4)
        {
            for (int x = 0; x < size.x; x += 2)
            {
                int ch = 0;
                ch += (int)(pixels[y * size.x + x] < 0.5f) << 0;
                ch += (int)(pixels[(y + 1) * size.x + x] < 0.5f) << 1;
                ch += (int)(pixels[(y + 2) * size.x + x] < 0.5f) << 2;
                ch += (int)(pixels[y * size.x + x + 1] < 0.5f) << 3;
                ch += (int)(pixels[(y + 1) * size.x + x + 1] < 0.5f) << 4;
                ch += (int)(pixels[(y + 2) * size.x + x + 1] < 0.5f) << 5;
                ch += (int)(pixels[(y + 3) * size.x + x] < 0.5f) << 6;
                ch += (int)(pixels[(y + 3) * size.x + x + 1] < 0.5f) << 7;
                printf("%c%c%c", 0xe2, 0xa0 + (ch >> 6), 0x80 + (ch & 0x3f));
            }
            printf("\n");
        }
        other.Unlock(pixels);
    }
#endif

    /* More tests */
    Image tmp;
    tmp.RenderRandom(ivec2(256, 256));
    // FIXME: this should not be necessary
    tmp.SetFormat(PixelFormat::RGBA_F32);
    tmp.Save("output3.jpeg");
}

int main(int argc, char **argv)
{
    UNUSED(argc, argv);

    Image image;
    image.Load("input.jpeg");
    DoSomeSeriousShit(image);

    return 0;
}

