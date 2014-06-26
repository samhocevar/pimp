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
    Array2D<u8vec4> &data = other.Lock2D<PixelFormat::RGBA_8>();
    for (int y = 0; y < size.y; ++y)
    for (int x = 0; x < size.x; ++x)
    {
        /* Reduce red value */
        data[x][y].r *= 0.9f;
    }
    other.Unlock2D(data);

    /* Convert to YUV and back to RGB */
    other = other.RGBToYUV();
    other = other.Convolution(Image::GaussianKernel(vec2(2.f)));
    other = other.YUVToRGB();

    /* Filter */
    //other = other.Convolution(Image::HalftoneKernel(ivec2(2, 2)));
    //other = other.Convolution(Image::GaussianKernel(vec2(6.f, 0.5f)));
    //other = other.Median(ivec2(4, 4));
    //other = other.Brightness(0.5f);
    //other = other.Threshold(0.3f);
    //other = other.Erode().Erode().Erode().Erode();
    //other = other.Sharpen(Image::GaussianKernel(vec2(6.f, 0.5f)));
    other.Save("output1.jpeg");

    /* Dither to black and white */
    //other = other.DitherOstromoukhov(ScanMode::Serpentine);
    //other = other.DitherEdiff(Image::EdiffKernel(EdiffAlgorithm::Stucki), ScanMode::Serpentine);
    //other = other.DitherHalftone(20.f, lol::radians(22.f));
    //other = other.Convolution(Image::GaussianKernel(vec2(1.f)));
    //other = other.Resize(ivec2(256, 320), ResampleAlgorithm::Bicubic);
    other = other.Resize(ivec2(320, 400), ResampleAlgorithm::Bresenham);
    //other = other.DitherOrdered(Image::BayerKernel(ivec2(32, 32)));
    other = other.DitherDbs();
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
    tmp.RenderRandom(ivec2(320, 320));
    Array2D<float> ker = Image::GaussianKernel(vec2(3.f));
    float mymax = 0.0f;
    for (int j = 0; j < ker.GetSize().y; ++j)
        for (int i = 0; i < ker.GetSize().x; ++i)
            mymax = lol::max(ker[i][j], mymax);
    for (int j = 0; j < ker.GetSize().y; ++j)
        for (int i = 0; i < ker.GetSize().x; ++i)
            ker[i][j] = ker[i][j] > mymax / 8.0f ? 1.f : 0.f;
    tmp = tmp.Median(ker);
    //tmp = tmp.Median(Image::GaussianKernel(vec2(6.f)));
    //tmp = tmp.Median(Image::HalftoneKernel(ivec2(16, 16)));
    tmp = tmp.AutoContrast();
    tmp.Save("output3.jpeg");

    /* Test Oric support */
    tmp = tmp.Contrast(0.5f);
    tmp.Save("oric.tap");
    tmp.Load("oric.tap");
    tmp.Save("oric.png");
}

int main(int argc, char **argv)
{
    UNUSED(argc, argv);

    Image image;
    image.Load("input.jpeg");

    DoSomeSeriousShit(image);

    return 0;
}

