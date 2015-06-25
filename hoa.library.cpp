/*
// Copyright (c) 2012-2015 Eliott Paris, Julien Colafrancesco, Thomas Le Meur & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "hoa.library.hpp"

char hoaversion[] = "Beta 2.2";

static t_eclass *cream_class;

static void *hoa_new(t_symbol *s)
{
    t_eobj *x = (t_eobj *)eobj_new(cream_class);
    if(x)
    {
        logpost(x, 3, "HOA Library by Julien Colafrancesco, Pierre Guillot, Eliott Paris & Thomas Le Meur\n© 2013 - 2015  CICM | Paris 8 University\nVersion %s (%s) for PureData %i.%i\n",hoaversion, __DATE__, PD_MAJOR_VERSION, PD_MINOR_VERSION);
    }
    return (x);
}


extern "C" void hoa_setup(void)
{
    cream_class = eclass_new("hoa", (method)hoa_new, (method)eobj_free, (short)sizeof(t_eobj), CLASS_PD, A_NULL, 0);
    cream_class = eclass_new("Hoa", (method)hoa_new, (method)eobj_free, (short)sizeof(t_eobj), CLASS_PD, A_NULL, 0);
    t_eobj* obj = (t_eobj *)hoa_new(NULL);
    if(!obj)
    {
        verbose(3, "HOA Library by Julien Colafrancesco, Pierre Guillot, Eliott Paris & Thomas Le Meur\n© 2013 - 2015  CICM | Paris 8 University\nVersion %s (%s) for PureData %i.%i\n",hoaversion, __DATE__, PD_MAJOR_VERSION, PD_MINOR_VERSION);
        eobj_free(obj);
    }

    // HOA COMMON //
    setup_hoa0x2econnect();
    setup_hoa0x2edac_tilde();
    setup_hoa0x2ein();
    setup_hoa0x2ein_tilde();
    setup_hoa0x2eout();
    setup_hoa0x2eout_tilde();
    setup_hoa0x2epi();
    setup_hoa0x2epi_tilde();
    setup_hoa0x2eprocess_tilde();
    setup_hoa0x2ethisprocess_tilde();
	setup_hoa0x2emap();

    // HOA 2D //
    setup_hoa0x2e2d0x2edecoder_tilde();
    setup_hoa0x2e2d0x2eencoder_tilde();
    setup_hoa0x2e2d0x2emap_tilde();
    setup_hoa0x2e2d0x2emeter_tilde();
    setup_hoa0x2e2d0x2eoptim_tilde();
    setup_hoa0x2e2d0x2eprojector_tilde();
    setup_hoa0x2e2d0x2erecomposer_tilde();
    setup_hoa0x2e2d0x2erotate_tilde();
    setup_hoa0x2e2d0x2escope_tilde();
    setup_hoa0x2e2d0x2espace();
    setup_hoa0x2e2d0x2ewider_tilde();
    setup_hoa0x2e2d0x2eexchanger_tilde();

    // HOA 3D //
    setup_hoa0x2e3d0x2edecoder_tilde();
    setup_hoa0x2e3d0x2eencoder_tilde();
    setup_hoa0x2e3d0x2eoptim_tilde();
    setup_hoa0x2e3d0x2ewider_tilde();
    setup_hoa0x2e3d0x2emap_tilde();
	setup_hoa0x2e3d0x2emeter_tilde();
    setup_hoa0x2e3d0x2escope_tilde();
    setup_hoa0x2e3d0x2eexchanger_tilde();

    epd_add_folder("Hoa", "patchers");
    epd_add_folder("Hoa", "clippings");
    epd_add_folder("Hoa", "dependencies");
    epd_add_folder("Hoa", "media");
    epd_add_folder("Hoa", "misc");
}

extern "C" void Hoa_setup(void)
{
    hoa_setup();
}

