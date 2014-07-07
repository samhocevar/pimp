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

#include <lol/main.h>

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
        hipal.Push(vec3(col) / 15.f);
        lopal.Push(vec3(col / 2) / 15.f);
    }

    /* Build clusters of pixels so that we know which colours are
     * important or not. */
    array2d<int> pixelcount(size);
    memset(pixelcount.Data(), 0, pixelcount.Bytes());
    for (int y = 0; y < size.y; ++y)
        for (int x = 0; x < size.x; ++x)
        {
            if (pixelcount[x][y])
                continue;
            ivec3 p = ivec3(srcdata[x][y].rgb * 255.999f);

            array<ivec2> visited, unvisited;
            unvisited.Push(ivec2(x, y));

            while (unvisited.Count())
            {
                ivec2 m = unvisited.Pop();
                visited.Push(m);

                for (ivec2 d : { ivec2(-1, 0), ivec2(1, 0),
                                 ivec2(0, -1), ivec2(0, 1) })
                {
                    if (visited.Find(m + d) != -1)
                        continue;
                    if (m.x + d.x < 0 || m.x + d.x >= size.x
                         || m.y + d.y < 0 || m.y + d.y >= size.y)
                        continue;

                    ivec3 q = ivec3(srcdata[m.x + d.x][m.y + d.y].rgb * 255.999f);
                    if (p == q)
                        unvisited.Push(m + d);
                }
            }

            for (ivec2 coord : visited)
                pixelcount[coord] = visited.Count();
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
    prev_line_palette.Resize(COLORS);

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
                    && sorted_palette.Find(i) == -1)
                    best = i;
            sorted_palette << best;
        }

        /* Recycle up to RECYCLED_COLORS from last line */
        array<int> recycled_palette;
        for (int i = 0; i < sorted_palette.Count(); )
        {
            if (recycled_palette.Count() >= RECYCLED_COLORS)
                break;
            if (prev_line_palette.Find(sorted_palette[i]) == -1)
            {
                ++i;
            }
            else
            {
                recycled_palette << sorted_palette[i];
                sorted_palette.Remove(i);
            }
        }

        /* Pad recycled palette. */
        while (recycled_palette.Count() < RECYCLED_COLORS)
        {
            if (prev_line_palette.Count())
                recycled_palette.PushUnique(prev_line_palette.Pop());
            else
                recycled_palette.PushUnique(rand(4096));
        }

        /* Fix up to FIXED_COLORS colors for this line. We just pick
         * the most important ones from sorted_palette. */
        array<int> fixed_palette;
        while (sorted_palette.Count()
                && fixed_palette.Count() < FIXED_COLORS)
        {
            fixed_palette.PushUnique(sorted_palette[0]);
            sorted_palette.Remove(0);
        }

        while (fixed_palette.Count() < FIXED_COLORS)
            fixed_palette.PushUnique(rand(4096));

        /* The rest are free colours; if we have enough room for the
         * last colours we need, use these. Otherwise, we need to
         * search for the best combination. */
        array<int> free_palette;
        if (sorted_palette.Count() <= FREE_COLORS)
        {
            free_palette += sorted_palette;
            while (free_palette.Count() < FREE_COLORS)
                free_palette.PushUnique(rand(4096));
        }
        else
        {
            float bestscore = (float)size.x;

            for (int iter = 0; iter < 2000; ++iter)
            {
                /* Pick FREE_COLORS colours at random */
                array<int> test_palette;
                while (test_palette.Count() < FREE_COLORS)
                {
                    int index = rand(sorted_palette.Count());
                    test_palette.PushUnique(sorted_palette[index]);
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
        ASSERT(palette.Count() == COLORS,
               "only %d colours in palette", palette.Count());

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

    Image image;
    image.Load("4bitfaces.png");
    image = EHBConvert(image);
    image.Save("ehb.png");

    return 0;
}

