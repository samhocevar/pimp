//
//  Pimp — Pathetic Image Manipulation Program
//
//  Copyright © 2008—2018 Sam Hocevar <sam@hocevar.net>
//
//  This program is free software. It comes without any warranty, to
//  the extent permitted by applicable law. You can redistribute it
//  and/or modify it under the terms of the Do What the Fuck You Want
//  to Public License, Version 2, as published by the WTFPL Task Force.
//  See http://www.wtfpl.net/ for more details.
//

#if HAVE_CONFIG_H
#   include "config.h"
#endif

#include <lol/engine.h>

using namespace lol;

/* Image processing kernels */
struct dither : array2d<float>
{
    ivec2 base_a, base_b;

    // Reduce the lattice base
    std::tuple<ivec2, ivec2> reduce_base() const
    {
        ivec2 new_a(base_a), new_b(base_b);

        for (bool changed = true; changed; )
        {
            changed = false;
            for (int s : { -1, 1 })
            {
                if (lol::sqlength(new_a + s * new_b) < lol::sqlength(new_a))
                {
                    new_a += s * new_b;
                    changed = true;
                }

                if (lol::sqlength(new_b + s * new_a) < lol::sqlength(new_b))
                {
                    new_b += s * new_a;
                    changed = true;
                }
            }
        }

        return std::make_tuple(new_a, new_b);
    }
};



int main(int argc, char **argv)
{
    UNUSED(argc, argv);

    dither d;
    d.base_a = ivec2(1, 1);
    d.base_b = ivec2(4, -1);
    d.resize(ivec2(4, 2));
    d[0][0] = d[1][0] = d[2][0] = d[3][0] = d[0][1] = 0.5f;

    printf("Reducing base...\n");
    auto r = d.reduce_base();
    auto new_a = std::get<0>(r);
    auto new_b = std::get<1>(r);
    printf("new a: %d %d\n", new_a.x, new_a.y);
    printf("new b: %d %d\n", new_b.x, new_b.y);

    return 0;
}

