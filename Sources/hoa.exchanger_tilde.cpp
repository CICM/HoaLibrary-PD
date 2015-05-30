/*
// Copyright (c) 2012-2015 Eliott Paris, Julien Colafrancesco, Thomas Le Meur & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.library.h"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_exchanger
{
    t_edspobj                   f_obj;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
    Exchanger<Hoa2d, t_sample>* f_exchanger;
} t_hoa_exchanger;

static t_eclass *hoa_exchanger_class;

typedef struct _hoa_exchanger_3d
{
    t_edspobj                   f_obj;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
    Exchanger<Hoa3d, t_sample>* f_exchanger;
} t_hoa_exchanger_3d;

static t_eclass *hoa_exchanger_3d_class;

static void *hoa_exchanger_new(t_symbol *s, long argc, t_atom *argv)
{
	int	order = 1;
    t_hoa_exchanger *x = (t_hoa_exchanger *)eobj_new(hoa_exchanger_class);
	if (x)
	{
        if(atom_gettype(argv) == A_LONG)
            order = pd_clip_minmax(atom_getlong(argv), 1, 63);
        
        x->f_exchanger = new Exchanger<Hoa2d, t_sample>(order);
        eobj_dspsetup(x, x->f_exchanger->getNumberOfHarmonics(), x->f_exchanger->getNumberOfHarmonics());
        
        x->f_ins = new t_sample[x->f_exchanger->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        x->f_outs = new t_sample[x->f_exchanger->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        return x;
	}
	return NULL;
}

static void hoa_exchanger_perform(t_hoa_exchanger *x, t_object *dsp, t_sample **ins, long nins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < nins; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, ins[i], 1, x->f_ins+i, nins);
    }
    for(long i = 0; i < sampleframes; i++)
    {
         x->f_exchanger->process(x->f_ins + nins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, x->f_outs+i, numouts, outs[i], 1);
    }
}

static void hoa_exchanger_dsp(t_hoa_exchanger *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    object_method(dsp, gensym("dsp_add"), x, (method)hoa_exchanger_perform, 0, NULL);
}


static void hoa_exchanger_free(t_hoa_exchanger *x)
{
	eobj_dspfree(x);
	delete x->f_exchanger;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

extern "C" void setup_hoa0x2e2d0x2eexchanger_tilde(void)
{
    t_eclass *c;
    c = eclass_new("hoa.2d.exchanger~",(method)hoa_exchanger_new,(method)hoa_exchanger_free,sizeof(t_hoa_exchanger), 0L, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_exchanger_new, gensym("hoa.exchanger~"), A_GIMME, 0);
    
    eclass_dspinit(c);
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_exchanger_dsp,     "dsp",		A_CANT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_exchanger_class = c;
}

static void *hoa_exchanger_3d_new(t_symbol *s, long argc, t_atom *argv)
{
    int	order = 1;
    Exchanger<Hoa3d, t_sample>::Normalization   norm = Exchanger<Hoa3d, t_sample>::SN3D;
    Exchanger<Hoa3d, t_sample>::Numbering       numb = Exchanger<Hoa3d, t_sample>::ACN;
    t_hoa_exchanger_3d *x = (t_hoa_exchanger_3d *)eobj_new(hoa_exchanger_3d_class);
    if(x)
    {
        if(atom_gettype(argv) == A_LONG)
            order = pd_clip_minmax(atom_getlong(argv), 1, 10);
        for(int i = 1; i < 3; i++)
        {
            if(argc > i && atom_gettype(argv+i) == A_SYM)
            {
                if(atom_getsym(argv+i) == gensym("toMaxN"))
                {
                    norm = Exchanger<Hoa3d, t_sample>::toMaxN;
                }
                else if(atom_getsym(argv+i) == gensym("toN3D"))
                {
                    norm = Exchanger<Hoa3d, t_sample>::toN3D;
                }
                else if(atom_getsym(argv+i) == gensym("fromMaxN"))
                {
                    norm = Exchanger<Hoa3d, t_sample>::fromMaxN;
                }
                else if(atom_getsym(argv+i) == gensym("fromN3D"))
                {
                    norm = Exchanger<Hoa3d, t_sample>::fromN3D;
                }
                else if(atom_getsym(argv+i) == gensym("toFurseMalham"))
                {
                    numb = Exchanger<Hoa3d, t_sample>::toFurseMalham;
                }
                else if(atom_getsym(argv+i) == gensym("toSID"))
                {
                    numb = Exchanger<Hoa3d, t_sample>::toSID;
                }
                else if(atom_getsym(argv+i) == gensym("fromFurseMalham"))
                {
                    numb = Exchanger<Hoa3d, t_sample>::fromFurseMalham;
                }
                else if(atom_getsym(argv+i) == gensym("fromSID"))
                {
                    numb = Exchanger<Hoa3d, t_sample>::fromSID;
                }
                else if(atom_getsym(argv+i) == gensym("toBFormat"))
                {
                    norm = Exchanger<Hoa3d, t_sample>::toMaxN;
                    numb = Exchanger<Hoa3d, t_sample>::toFurseMalham;
                }
                else if(atom_getsym(argv+i) == gensym("fromBFormat"))
                {
                    norm = Exchanger<Hoa3d, t_sample>::fromMaxN;
                    numb = Exchanger<Hoa3d, t_sample>::fromFurseMalham;
                }
                else if(atom_getsym(argv+i) == gensym("toDaniel"))
                {
                    norm = Exchanger<Hoa3d, t_sample>::toN3D;
                    numb = Exchanger<Hoa3d, t_sample>::toSID;
                }
                else if(atom_getsym(argv+i) == gensym("fromDaniel"))
                {
                    norm = Exchanger<Hoa3d, t_sample>::fromN3D;
                    numb = Exchanger<Hoa3d, t_sample>::fromSID;
                }
                else
                {
                    pd_error(x, "hoa.exchanger : %s wrong symbol.", atom_getsym(argv+1)->s_name);
                }
            }
        }
        x->f_exchanger = new Exchanger<Hoa3d, t_sample>(order);
        x->f_exchanger->setNormalization(norm);
        x->f_exchanger->setNumbering(numb);
        eobj_dspsetup(x, x->f_exchanger->getNumberOfHarmonics(), x->f_exchanger->getNumberOfHarmonics());
        
        x->f_ins = new t_sample[x->f_exchanger->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        x->f_outs = new t_sample[x->f_exchanger->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
    }
    return x;
}

static void hoa_exchanger_3d_perform(t_hoa_exchanger_3d *x, t_object *dsp, float **ins, long numins, float **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, ins[i], 1, x->f_ins+i, numins);
    }
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_exchanger->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, x->f_outs+i, numouts, outs[i], 1);
    }
}

static void hoa_exchanger_3d_dsp(t_hoa_exchanger_3d *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    object_method(dsp, gensym("dsp_add"), x, (method)hoa_exchanger_3d_perform, 0, NULL);
}


static void hoa_exchanger_3d_free(t_hoa_exchanger_3d *x)
{
    eobj_dspfree(x);
    delete x->f_exchanger;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

extern "C" void setup_hoa0x2e3d0x2eexchanger_tilde(void)
{
    t_eclass *c;
    c = eclass_new("hoa.3d.exchanger~",(method)hoa_exchanger_3d_new,(method)hoa_exchanger_3d_free,sizeof(t_hoa_exchanger_3d), CLASS_NOINLET, A_GIMME, 0);
    
    eclass_dspinit(c);
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_exchanger_3d_dsp,     "dsp",		A_CANT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_exchanger_3d_class = c;
}
