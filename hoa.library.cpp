/*
// Copyright (c) 2012-2015 Eliott Paris, Julien Colafrancesco, Thomas Le Meur & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "hoa.library.h"

extern void hoa_initclass(t_eclass* c)
{
    char help[MAXPDSTRING];
    sprintf(help, "helps/%s-help", c->c_class.c_name->s_name);
    class_sethelpsymbol((t_class *)c, gensym(help));
}

char hoaversion[] = "Beta 2.2";
#ifdef PD_EXTENDED
char pdversion[] = "Pd-Extended";
#else
char pdversion[] = "Pd-Vanilla";
#endif

extern "C" void hoa_setup(void)
{
    post("HOA Library by Julien Colafrancesco, Pierre Guillot, Eliott Paris & Thomas Le Meur");
    post("© 2012 - 2015  CICM | Paris 8 University");
    post("Version %s (%s) for %s", hoaversion, __DATE__, pdversion);
    post("");
    
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
    
    // HOA 3D //
    setup_hoa0x2e3d0x2edecoder_tilde();
    setup_hoa0x2e3d0x2eencoder_tilde();
    setup_hoa0x2e3d0x2eoptim_tilde();
    setup_hoa0x2e3d0x2ewider_tilde();
    setup_hoa0x2e3d0x2emap_tilde();
	setup_hoa0x2e3d0x2emeter_tilde();
    setup_hoa0x2e3d0x2escope_tilde();  

    pd_library_add_folder("Hoa", "patchers");
    pd_library_add_folder("Hoa", "clippings");
    pd_library_add_folder("Hoa", "dependencies");
    pd_library_add_folder("Hoa", "media");
    pd_library_add_folder("Hoa", "misc");
}

