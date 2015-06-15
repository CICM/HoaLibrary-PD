/*
// Copyright (c) 2012-2015 Eliott Paris, Julien Colafrancesco, Thomas Le Meur & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.library.hpp"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_projector
{
    t_edspobj                   f_obj;
    Projector<Hoa2d, t_sample>* f_projector;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
} t_hoa_projector;

static t_eclass *hoa_projector_class;

static void *hoa_projector_new(t_symbol *s, int argc, t_atom *argv)
{
    ulong	order = 1;
    ulong numberOfPlanewaves = 4;
    t_hoa_projector *x = NULL;
    x = (t_hoa_projector *)eobj_new(hoa_projector_class);
	if (x)
	{
		if(atom_gettype(argv) == A_LONG)
			order = ulong(pd_clip_minmax(atom_getlong(argv), 1, 63));
        if(atom_gettype(argv+1) == A_LONG)
			numberOfPlanewaves = ulong(pd_clip_min(atom_getlong(argv+1), order * 2 + 1));
		
		x->f_projector = new Projector<Hoa2d, t_sample>(order, numberOfPlanewaves);
		
        eobj_dspsetup(x, long(x->f_projector->getNumberOfHarmonics()), long(x->f_projector->getNumberOfPlanewaves()));
        
		x->f_ins = new t_sample[x->f_projector->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        x->f_outs = new t_sample[x->f_projector->getNumberOfPlanewaves() * HOA_MAXBLKSIZE];
	}
    
	return (x);
}

static void hoa_projector_perform(t_hoa_projector *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, ulong(numins));
    }
	for(long i = 0; i < sampleframes; i++)
    {
        x->f_projector->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_projector_dsp(t_hoa_projector *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    object_method(dsp, gensym("dsp_add"), x, (method)hoa_projector_perform, 0, NULL);
}

static void hoa_projector_free(t_hoa_projector *x)
{
	eobj_dspfree(x);
	delete x->f_projector;
    delete [] x->f_ins;
	delete [] x->f_outs;
}


extern "C" void setup_hoa0x2e2d0x2eprojector_tilde(void)
{
    t_eclass* c;
    
    c = eclass_new("hoa.2d.projector~", (method)hoa_projector_new, (method)hoa_projector_free, (short)sizeof(t_hoa_projector), 0L, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_projector_new, gensym("hoa.projector~"), A_GIMME, 0);
    
    eclass_dspinit(c);
    
    eclass_addmethod(c, (method)hoa_projector_dsp,     "dsp",      A_CANT, 0);
    
    hoa_projector_class = c;
}
