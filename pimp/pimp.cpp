//
//  Pimp - Pathetic Image Manipulation Program
//
//  Copyright (c) 2008-2014 Sam Hocevar <sam@hocevar.net>
//   This program is free software; you can redistribute it and/or
//   modify it under the terms of the Do What The Fuck You Want To
//   Public License, Version 2, as published by Sam Hocevar. See
//   http://www.wtfpl.net/ for more details.
//

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <lol/engine.h>

using namespace lol;


Image EHBConvert(Image src)
{
    ivec2 size = src.GetSize();
    Image dst(size);

    array2d<vec4> &srcdata = src.Lock2D<PixelFormat::RGBA_F32>();

    int const COLORS = 32;
    int const RECYCLED_COLORS = 16;
    int const FIXED_COLORS = 8;
    int const FREE_COLORS = RECYCLED_COLORS - FIXED_COLORS;
    static_assert(FREE_COLORS > 0, "no free colors!");

    /* Build the Amiga colour palette */
    array<vec3> hipal, lopal;
    for (int n = 0; n < 4096; ++n)
    {
        ivec3 col = ivec3(n % 16, (n / 16) % 16, (n / 256) % 16);
        hipal.push(vec3(col) / 15.f);
        lopal.push(vec3(col / 2) / 15.f);
    }

    /* Build clusters of pixels so that we know which colours are
     * important or not. */
    array2d<int> pixelcount(size);
    memset(pixelcount.data(), 0, pixelcount.bytes());
    for (int y = 0; y < size.y; ++y)
        for (int x = 0; x < size.x; ++x)
        {
            if (pixelcount[x][y])
                continue;
            ivec3 p = ivec3(srcdata[x][y].rgb * 255.999f);

            array<ivec2> visited, unvisited;
            unvisited.push(ivec2(x, y));

            while (unvisited.count())
            {
                ivec2 m = unvisited.pop();
                visited.push(m);

                for (ivec2 d : { ivec2(-1, 0), ivec2(1, 0),
                                 ivec2(0, -1), ivec2(0, 1) })
                {
                    if (visited.find(m + d) != -1)
                        continue;
                    if (m.x + d.x < 0 || m.x + d.x >= size.x
                         || m.y + d.y < 0 || m.y + d.y >= size.y)
                        continue;

                    ivec3 q = ivec3(srcdata[m.x + d.x][m.y + d.y].rgb * 255.999f);
                    if (p == q)
                        unvisited.push(m + d);
                }
            }

            for (ivec2 coord : visited)
                pixelcount[coord] = visited.count();
        }

#if 1
    /* Convert source image to Amiga colours -- do this if working on
     * pixel art rather than photographs. */
    for (int y = 0; y < size.y; ++y)
        for (int x = 0; x < size.x; ++x)
        {
            ivec3 p = ivec3(srcdata[x][y].rgb * 15.999f);
            srcdata[x][y] = vec4(hipal[p.r + p.g * 16 + p.b * 256], 1.f);
        }
#endif

    array<int> prev_line_palette;
    prev_line_palette.resize(COLORS);

    /* Treat our image, line by line. */
    for (int y = 0; y < size.y; ++y)
    {
        /* Count how important the pixels of the line are. */
        int histo[4096] = { 0 };
        for (int x = 0; x < size.x; ++x)
        {
            ivec3 p = ivec3(srcdata[x][y].rgb * 15.999f);

            /* Find desired color index. If the color is dark, pretend
             * we actually want a lighter version so we can take
             * advantage of EHB. */
            int index = p.r + p.g * 16 + p.b * 256;
            if (p < ivec3(8))
                index *= 2;

            histo[index] += pixelcount[ivec2(x, y)];
        }

        /* Count number of colours in this line */
        int count = 0;
        for (int i = 0; i < 4096; ++i)
            count += histo[i] ? 1 : 0;

#if 1
        printf("line %d: %d colors\n", y, count);
#endif

        /* Sort palette */
        array<int> sorted_palette;
        for (int n = 0; n < count; ++n)
        {
            int best = -1;
            for (int i = 0; i < 4096; ++i)
                if ((best == -1 || histo[i] >= histo[best])
                    && sorted_palette.find(i) == -1)
                    best = i;
            sorted_palette << best;
        }

        /* Recycle up to RECYCLED_COLORS from last line */
        array<int> recycled_palette;
        for (int i = 0; i < sorted_palette.count(); )
        {
            if (recycled_palette.count() >= RECYCLED_COLORS)
                break;
            if (prev_line_palette.find(sorted_palette[i]) == -1)
            {
                ++i;
            }
            else
            {
                recycled_palette << sorted_palette[i];
                sorted_palette.remove(i);
            }
        }

        /* Pad recycled palette. */
        while (recycled_palette.count() < RECYCLED_COLORS)
        {
            if (prev_line_palette.count())
                recycled_palette.push_unique(prev_line_palette.pop());
            else
                recycled_palette.push_unique(rand(4096));
        }

        /* Fix up to FIXED_COLORS colors for this line. We just pick
         * the most important ones from sorted_palette. */
        array<int> fixed_palette;
        while (sorted_palette.count()
                && fixed_palette.count() < FIXED_COLORS)
        {
            fixed_palette.push_unique(sorted_palette[0]);
            sorted_palette.remove(0);
        }

        while (fixed_palette.count() < FIXED_COLORS)
            fixed_palette.push_unique(rand(4096));

        /* The rest are free colours; if we have enough room for the
         * last colours we need, use these. Otherwise, we need to
         * search for the best combination. */
        array<int> free_palette;
        if (sorted_palette.count() <= FREE_COLORS)
        {
            free_palette += sorted_palette;
            while (free_palette.count() < FREE_COLORS)
                free_palette.push_unique(rand(4096));
        }
        else
        {
            float bestscore = (float)size.x;

            for (int iter = 0; iter < 2000; ++iter)
            {
                /* Pick FREE_COLORS colours at random */
                array<int> test_palette;
                while (test_palette.count() < FREE_COLORS)
                {
                    int index = rand(sorted_palette.count());
                    test_palette.push_unique(sorted_palette[index]);
                }

                array<int> palette = recycled_palette
                                          + fixed_palette + test_palette;
                /* Compute error */
                float error = 0.0f;
                for (int x = 0; x < size.x; ++x)
                {
                    vec3 p = srcdata[x][y].rgb;
                    float best_dist = 1.0f;

                    for (int c = 0; c < COLORS; ++c)
                    {
                        best_dist = min(best_dist,
                                        sqlength(p - hipal[palette[c]]));
                        best_dist = min(best_dist,
                                        sqlength(p - lopal[palette[c]]));
                    }

                    error += best_dist;
                }

                if (error < bestscore)
                {
                    bestscore = error;
                    free_palette = test_palette;
                }
            }
        }

#if 0
        printf("recycled palette:");
        for (auto color : recycled_palette)
            printf(" %03x(%d)", color, histo[color]);
        printf("\n");
#endif

        array<int> palette = recycled_palette + fixed_palette + free_palette;
        ASSERT(palette.count() == COLORS,
               "only %d colours in palette", (int)palette.count());

        /* Commit the best palette */
        array2d<vec4> &dstdata = dst.Lock2D<PixelFormat::RGBA_F32>();

        for (int x = 0; x < size.x; ++x)
        {
            vec3 pixel = srcdata[x][y].rgb;
            float best_dist = 1.0f;
            vec3 best_error(0.0f);

            for (int c = 0; c < COLORS * 2 /* EHB */; ++c)
            {
                vec3 color = c < COLORS ? hipal[palette[c]]
                                        : lopal[palette[c - COLORS]];
                float dist = sqlength(pixel - color);
                if (dist < best_dist)
                {
                     best_dist = dist;
                     best_error = pixel - color;
                     dstdata[x][y] = vec4(color, 1.f);
                }
            }

#if 0 /* Propagate error */
            if (x + 1 < size.x)
                srcdata[x + 1][y] = saturate(srcdata[x + 1][y]
                                            + 0.4375f * vec4(best_error, 0.f));
            if (y + 1 < size.y)
            {
                if (x - 1 >= 0)
                    srcdata[x - 1][y + 1] = saturate(srcdata[x - 1][y + 1]
                                            + 0.1875f * vec4(best_error, 0.f));
                srcdata[x][y + 1] = saturate(srcdata[x][y + 1]
                                            + 0.3125f * vec4(best_error, 0.f));
                if (x + 1 < size.x)
                    srcdata[x + 1][y + 1] = saturate(srcdata[x + 1][y + 1]
                                            + 0.0625f * vec4(best_error, 0.f));
            }
#endif
        }

        dst.Unlock2D(dstdata);
    }

    src.Unlock2D(srcdata);

    return dst;
}

int main(int argc, char **argv)
{
    UNUSED(argc, argv);

    if (argc >= 3)
    {
        /* Load images */
        Image im1, im2;
        im1.Load(argv[1]);
        im2.Load(argv[2]);
        ivec2 size = im1.GetSize();

        /* Ensure the size is the same */
        if (im2.GetSize() != size)
            im2 = im2.Resize(size, ResampleAlgorithm::Bicubic);

        /* Gaussian Blur */
        im1 = im1.Convolution(Image::GaussianKernel(vec2(2.f)));
        im2 = im2.Convolution(Image::GaussianKernel(vec2(2.f)));

        /* Count pixel differences */
        float delta = 0.f;
        array2d<vec4> &im1data = im1.Lock2D<PixelFormat::RGBA_F32>();
        array2d<vec4> &im2data = im2.Lock2D<PixelFormat::RGBA_F32>();
        for (int j = 0; j < size.y; ++j)
            for (int i = 0; i < size.x; ++i)
                delta += sqlength(im1data[i][j].rgb - im2data[i][j].rgb);
        im1.Unlock2D(im1data);
        im2.Unlock2D(im2data);

        printf("mean distance = %f\n", sqrt(delta / (size.x * size.y)));
    }

#if 0
    {
        Movie m("lol.gif", ivec2(128, 128), 10);

        for (int i = 0; i < 10; ++i)
        {
            Image im;
            im.RenderRandom(ivec2(128, 128));
            m.Feed(im);
        }
    }
#endif

#if 0
    Image image;
    image.Load("4bitfaces.png");
    image = EHBConvert(image);
    image.Save("ehb.png");
#endif

    return 0;
}

