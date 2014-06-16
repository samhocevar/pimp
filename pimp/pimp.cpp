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

    /* Test lock */
    Image tmp;
    tmp.SetSize(ivec2(256, 256));
    u8vec4 *p = tmp.Lock<PixelFormat::RGBA_8>();
    tmp.Unlock();

    Image *image = Image::Create("input.jpeg");

    ivec2 size = image->GetSize();
    u8vec4 *data = image->Lock<PixelFormat::RGBA_8>();
    image->Unlock();

    image->Save("output.jpeg");

    image->Destroy();

    return 0;
}

