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

    /* Perform some per-pixel operations */
    u8vec4 *data = other.Lock<PixelFormat::RGBA_8>();
    for (int n = 0; n < size.x * size.y; ++n)
    {
        /* Reduce red value */
        data[n].r *= 0.9f;
    }
    other.Unlock(data);

    /* Convert to YUV and back to RGB */
    other = other.RGBToYUV();
    other = other.YUVToRGB();

    /* Convolution */
    //other = other.Convolution(Image::HalftoneKernel(ivec2(2, 2)));
    //other = other.Convolution(Image::GaussianKernel(vec2(6.f, 0.5f), radians(22.f), vec2(0.f)));
    other = other.Median(ivec2(4, 4));
    other.Save("output1.jpeg");

    /* Dither to black and white */
    //other = other.DitherOstromoukhov(ScanMode::Serpentine);
    //other = other.DitherEdiff(Image::EdiffKernel(EdiffAlgorithm::Stucki), ScanMode::Serpentine);
    other = other.DitherHalftone(20.f, lol::radians(22.f));
    //other = other.DitherOrdered(Image::BayerKernel(ivec2(32, 32)));
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
    tmp = tmp.Median(ivec2(15, 15));
    tmp = tmp.AutoContrast();
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

