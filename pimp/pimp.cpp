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
    int const COLORS = 32;
    int const RECYCLED_COLORS = 16;
    int const FIXED_COLORS = 12;

    static_assert(RECYCLED_COLORS + FIXED_COLORS < COLORS, "no free colors!");

    /* Build the Amiga colour palette */
    array<vec3> hipal, lopal;
    for (int n = 0; n < 4096; ++n)
    {
        ivec3 col = ivec3(n % 16, (n / 16) % 16, (n / 256) % 16);
        hipal.Push(vec3(col) / 15.f);
        lopal.Push(vec3(col / 2) / 15.f);
    }

    ivec2 size = src.GetSize();
    Image dst(size);

    array2d<vec4> &srcdata = src.Lock2D<PixelFormat::RGBA_F32>();

#if 1
    /* Convert source image to Amiga colours */
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
        array<int> super_palette;
        float super_msd = (float)size.x;

        /* Make a histogram of the line */
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

            ++histo[index];

            /* Count colours from adjacent lines, but only if they appear
             * in the current line. This tells us which colours are a bit
             * more important than the others. */
            for (int dy : { -3, -2, -1, 0, 1, 2, 3 })
            for (int dx : { -3, -2, -1, 0, 1, 2, 3 })
            {
                if (y + dy < 0 || y + dy >= size.y
                     || x + dx < 0 || x + dx >= size.x)
                    continue;

                ivec3 q = ivec3(vec3(srcdata[x + dx][y + dy].rgb) * 15.999f);
                if (p == q)
                    ++histo[index];
            }
        }

        /* Count number of colours in this line */
        int count = 0;
        for (int i = 0; i < 4096; ++i)
            count += histo[i] ? 1 : 0;

        printf("line %d: %d colors\n", y, count);

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
        for (auto c : sorted_palette)
        {
            if (recycled_palette.Count() >= RECYCLED_COLORS)
                break;
            if (prev_line_palette.Find(c) == -1)
                continue;
            recycled_palette << c;
        }

        /* Pad recycled palette with stuff from prev line. */
        for (auto c : prev_line_palette)
        {
            if (recycled_palette.Count() >= RECYCLED_COLORS)
                break;
            if (recycled_palette.Find(c) != -1)
                continue;
            recycled_palette << c;
        }

        /* Fix FIXED_COLORS colors for this line */
        array<int> fixed_palette;
        for (auto c : sorted_palette)
        {
            if (fixed_palette.Count() >= FIXED_COLORS)
                break;
            if (recycled_palette.Find(c) != -1)
                continue;
            recycled_palette << c;
        }

#if 0
        printf("recycled palette:");
        for (auto color : recycled_palette)
            printf(" %03x(%d)", color, histo[color]);
        printf("\n");
#endif

        for (int iter = 0; iter < 50; ++iter)
        {
            array<int> palette = recycled_palette + fixed_palette;
            palette.Resize(COLORS);

            /* Generate random palette */
            for (int c = RECYCLED_COLORS + FIXED_COLORS; c < COLORS; ++c)
                palette[c] = rand(4096);

            float best_msd = (float)size.x;
            int changed_index = 0;
            int changed_color = palette[0];

            for (int failures = 0; failures < 50; )
            {
                /* Dither one line */
                float msd = 0.0f;
                for (int x = 0; x < size.x; ++x)
                {
                    vec3 pixel = srcdata[x][y].rgb;
                    float dist_squared = 1.0f;

                    for (int c = 0; c < COLORS; ++c)
                    {
                        dist_squared = min(dist_squared,
                                           sqlength(hipal[palette[c]] - pixel));
                        dist_squared = min(dist_squared, /* EHB */
                                           sqlength(lopal[palette[c]] - pixel));
                    }

                    if (dist_squared < 0.5f / (16 * 16))
                        dist_squared = 0.f;
                    msd += dist_squared;
                }

                if (msd >= best_msd)
                {
                    /* If the result wasn't good, revert. */
                    ++failures;

                    palette[changed_index] = changed_color;
                }
                else
                {
                    failures = 0;

                    /* Otherwise, commit */
                    best_msd = msd;
                }

                /* Change one color */
                changed_index = rand(RECYCLED_COLORS + FIXED_COLORS, COLORS);
                changed_color = palette[changed_index];
                int new_color = changed_color;
                new_color += rand(-1, 2);
                new_color += rand(-1, 2) * 16;
                new_color += rand(-1, 2) * 256;
                palette[changed_index] = clamp(new_color, 0, 4095);

                if (best_msd > 5.f * super_msd)
                    break;
            }

            if (best_msd < super_msd)
            {
                super_msd = best_msd;
                super_palette = palette;
                printf("[%d] .. %f\n", y, best_msd);
            }
        }

        /* Commit the best palette */
        array2d<vec4> &dstdata = dst.Lock2D<PixelFormat::RGBA_F32>();

        for (int x = 0; x < size.x; ++x)
        {
            vec3 pixel = srcdata[x][y].rgb;
            float best_dist = 1.0f;
            vec3 best_error(0.0f);

            for (int c = 0; c < COLORS * 2 /* EHB */; ++c)
            {
                vec3 color = c < COLORS ? hipal[super_palette[c]]
                                        : lopal[super_palette[c - COLORS]];
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

        if (y % 10 == 9)
            dst.Save("ehb.png");

        printf("[%d] FINAL %f\n", y, super_msd);
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

