/*
 // Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../hoa.library.h"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_wider
{
    t_edspobj               f_obj;
    Wider<Hoa2d, t_sample>* f_wider;
    t_sample*               f_ins;
    t_sample*               f_outs;
} t_hoa_wider;

static t_eclass *hoa_wider_class;

typedef struct _hoa_wider_3d
{
    t_edspobj               f_obj;
    Wider<Hoa3d, t_sample>* f_wider;
    t_sample*               f_ins;
    t_sample*               f_outs;
} t_hoa_wider_3d;

static t_eclass *hoa_wider_3d_class;

static void *hoa_wider_new(t_symbol *s, long argc, t_atom *argv)
{
    int	order = 1;
    t_hoa_wider *x = (t_hoa_wider *)eobj_new(hoa_wider_class);
	if(x)
	{
		if(atom_gettype(argv) == A_LONG)
			order = pd_clip_minmax(atom_getlong(argv), 1, 63);
        
		x->f_wider = new Wider<Hoa2d, t_sample>(order);
        eobj_dspsetup(x, x->f_wider->getNumberOfHarmonics() + 1, x->f_wider->getNumberOfHarmonics());
        
		x->f_ins = new t_sample[x->f_wider->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        x->f_outs = new t_sample[x->f_wider->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
	}
    
	return (x);
}

static void *hoa_wider_3d_new(t_symbol *s, long argc, t_atom *argv)
{
    int	order = 1;
    t_hoa_wider_3d *x = (t_hoa_wider_3d *)eobj_new(hoa_wider_3d_class);
    if(x)
    {
        if(atom_gettype(argv) == A_LONG)
            order = pd_clip_minmax(atom_getlong(argv), 1, 10);
        
        x->f_wider = new Wider<Hoa3d, t_sample>(order);
        eobj_dspsetup(x, x->f_wider->getNumberOfHarmonics() + 1, x->f_wider->getNumberOfHarmonics());
        
        x->f_ins = new t_sample[x->f_wider->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        x->f_outs = new t_sample[x->f_wider->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
    }
    
    return (x);
}

static void hoa_wider_float(t_hoa_wider *x, float f)
{
    x->f_wider->setWidening(f);
}

static void hoa_wider_3d_float(t_hoa_wider_3d *x, float f)
{
    x->f_wider->setWidening(f);
}

static void hoa_wider_perform(t_hoa_wider *x, t_object *dsp, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
	for(long i = 0; i < numins - 1; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, ins[i], 1, x->f_ins+i, numins - 1);
    }
	for(long i = 0; i < sampleframes; i++)
    {
        x->f_wider->setWidening(ins[numins-1][i]);
        x->f_wider->process(x->f_ins + (numins - 1) * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, x->f_outs+i, numouts, outs[i], 1);
    }
}

static void hoa_wider_3d_perform(t_hoa_wider_3d *x, t_object *dsp, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < numins - 1; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, ins[i], 1, x->f_ins+i, numins - 1);
    }
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_wider->setWidening(ins[numins-1][i]);
        x->f_wider->process(x->f_ins + (numins - 1) * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, x->f_outs+i, numouts, outs[i], 1);
    }
}

static void hoa_wider_perform_offset(t_hoa_wider *x, t_object *dsp, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
	for(long i = 0; i < numins - 1; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, ins[i], 1, x->f_ins+i, numins - 1);
    }
	for(long i = 0; i < sampleframes; i++)
    {
        x->f_wider->process(x->f_ins + (numins - 1) * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, x->f_outs+i, numouts, outs[i], 1);
    }
}

static void hoa_wider_3d_perform_offset(t_hoa_wider_3d *x, t_object *dsp, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < numins - 1; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, ins[i], 1, x->f_ins+i, numins - 1);
    }
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_wider->process(x->f_ins + (numins - 1) * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::vector_copy(sampleframes, x->f_outs+i, numouts, outs[i], 1);
    }
}

static void hoa_wider_dsp(t_hoa_wider *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    if(count[x->f_wider->getNumberOfHarmonics()])
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_wider_perform, 0, NULL);
    else
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_wider_perform_offset, 0, NULL);
}

static void hoa_wider_3d_dsp(t_hoa_wider_3d *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    if(count[x->f_wider->getNumberOfHarmonics()])
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_wider_3d_perform, 0, NULL);
    else
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_wider_3d_perform_offset, 0, NULL);
}

static void hoa_wider_free(t_hoa_wider *x)
{
    eobj_dspfree(x);
	delete x->f_wider;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

static void hoa_wider_3d_free(t_hoa_wider_3d *x)
{
    eobj_dspfree(x);
    delete x->f_wider;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

extern "C" void setup_hoa0x2e2d0x2ewider_tilde(void)
{
    t_eclass* c;
    c = eclass_new("hoa.2d.wider~", (method)hoa_wider_new, (method)hoa_wider_free, (short)sizeof(t_hoa_wider), 0, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_wider_new, gensym("hoa.wider~"), A_GIMME, 0);
    
    eclass_dspinit(c);
    hoa_initclass(c);;
    eclass_addmethod(c, (method)hoa_wider_dsp,       "dsp",      A_CANT, 0);
    eclass_addmethod(c, (method)hoa_wider_float,    "float",    A_FLOAT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_wider_class = c;
}

extern "C" void setup_hoa0x2e3d0x2ewider_tilde(void)
{
    t_eclass* c;
    c = eclass_new("hoa.3d.wider~", (method)hoa_wider_3d_new, (method)hoa_wider_3d_free, (short)sizeof(t_hoa_wider_3d), 0, A_GIMME, 0);
    
    eclass_dspinit(c);
    hoa_initclass(c);;
    eclass_addmethod(c, (method)hoa_wider_3d_dsp,       "dsp",      A_CANT, 0);
    eclass_addmethod(c, (method)hoa_wider_3d_float,    "float",    A_FLOAT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_wider_3d_class = c;
}

