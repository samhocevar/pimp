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

    Image *image = Image::Create("input.jpeg");

    ivec2 size = image->GetSize();
    uint8_t *data = image->GetData();

    image->Save("output.jpeg");

    Image::Destroy(image);

    return 0;
}

