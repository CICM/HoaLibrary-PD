/*
// Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.library.h"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_encoder
{
    t_edspobj                 f_obj;
    t_sample*                 f_signals;
    Encoder<Hoa2d, t_sample>* f_encoder;
} t_hoa_encoder;

t_eclass *hoa_encoder_class;

typedef struct _hoa_encoder_3D
{
    t_edspobj                 f_obj;
    t_sample*                 f_signals;
    Encoder<Hoa3d, t_sample>* f_encoder;
} t_hoa_encoder_3D;

t_eclass *hoa_encoder_3D_class;

extern void *hoa_encoder_new(t_symbol *s, long argc, t_atom *argv)
{
	int	order = 1;
    t_hoa_encoder *x = (t_hoa_encoder *)eobj_new(hoa_encoder_class);
	if (x)
	{
        if(atom_gettype(argv) == A_LONG)
            order = pd_clip_min(atom_getlong(argv), 1);
        
		x->f_encoder = new Encoder<Hoa2d, t_sample>(order);
        eobj_dspsetup(x, 2, x->f_encoder->getNumberOfHarmonics());
        
        x->f_signals =  new t_sample[x->f_encoder->getNumberOfHarmonics() * 8192];
        
        return x;
	}
	return NULL;
}

extern void hoa_encoder_float(t_hoa_encoder *x, float f)
{
    x->f_encoder->setAzimuth(f);
}

extern void hoa_encoder_perform(t_hoa_encoder *x, t_object *dsp, t_sample **ins, long nins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_encoder->setAzimuth(ins[1][i]);
        x->f_encoder->process(ins[0]+i, x->f_signals + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_signals+i, numouts, outs[i], 1);
    }
}

extern void hoa_encoder_perform_offset(t_hoa_encoder *x, t_object *dsp, t_sample **ins, long nins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_encoder->process(ins[0]+i, x->f_signals + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_signals+i, numouts, outs[i], 1);
    }
}

extern void hoa_encoder_dsp(t_hoa_encoder *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    if(count[1])
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_perform, 0, NULL);
    else
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_perform_offset, 0, NULL);
}


extern void hoa_encoder_free(t_hoa_encoder *x)
{
	eobj_dspfree(x);
	delete x->f_encoder;
    delete [] x->f_signals;
}

extern "C" void setup_hoa0x2e2d0x2eencoder_tilde(void)
{
    t_eclass *c;
    c = eclass_new("hoa.2d.encoder~",(method)hoa_encoder_new,(method)hoa_encoder_free,sizeof(t_hoa_encoder), 0L, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_encoder_new, gensym("hoa.encoder~"), A_GIMME, 0);
    
    eclass_dspinit(c);
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_encoder_dsp,     "dsp",		A_CANT, 0);
    eclass_addmethod(c, (method)hoa_encoder_float,   "float",   A_FLOAT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_encoder_class = c;
}

extern void *hoa_encoder_3D_new(t_symbol *s, long argc, t_atom *argv)
{
    int	order = 1;
    t_hoa_encoder_3D *x = (t_hoa_encoder_3D *)eobj_new(hoa_encoder_3D_class);
    if(x)
    {
        if(atom_gettype(argv) == A_LONG)
            order = pd_clip_min(atom_getlong(argv), 1);
        
        x->f_encoder = new Encoder<Hoa3d, t_sample>(order);
        eobj_dspsetup(x, 3, x->f_encoder->getNumberOfHarmonics());
        
        x->f_signals =  new t_sample[x->f_encoder->getNumberOfHarmonics() * 8192];
    }
    return x;
}

extern void hoa_encoder_3D_float(t_hoa_encoder_3D *x, float f)
{
    if(eobj_getproxy(x) == 1)
        x->f_encoder->setAzimuth(f);
    else if(eobj_getproxy(x) == 2)
        x->f_encoder->setElevation(f);
}

extern void hoa_encoder_3D_perform(t_hoa_encoder_3D *x, t_object *dsp, float **ins, long nins, float **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_encoder->setAzimuth(ins[1][i]);
        x->f_encoder->setElevation(ins[2][i]);
        x->f_encoder->process(ins[0]+i, x->f_signals + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_signals+i, numouts, outs[i], 1);
    }
}

extern void hoa_encoder_3D_perform_azimuth(t_hoa_encoder_3D *x, t_object *dsp, t_sample **ins, long nins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_encoder->setAzimuth(ins[1][i]);
        x->f_encoder->process(ins[0]+i, x->f_signals + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_signals+i, numouts, outs[i], 1);
    }
}

extern void hoa_encoder_3D_perform_elevation(t_hoa_encoder_3D *x, t_object *dsp, t_sample **ins, long nins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_encoder->setElevation(ins[2][i]);
        x->f_encoder->process(ins[0]+i, x->f_signals + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_signals+i, numouts, outs[i], 1);
    }
}

extern void hoa_encoder_3D_perform_offset(t_hoa_encoder_3D *x, t_object *dsp, t_sample **ins, long nins, t_sample **outs, long numouts, long sampleframes, long f,void *up)
{
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_encoder->process(ins[0]+i, x->f_signals + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_signals+i, numouts, outs[i], 1);
    }
}

extern void hoa_encoder_3D_dsp(t_hoa_encoder_3D *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    if(count[1] && count[2])
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_3D_perform, 0, NULL);
    else if(count[1] && !count[2])
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_3D_perform_azimuth, 0, NULL);
    else if(!count[1] && count[2])
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_3D_perform_elevation, 0, NULL);
    else
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_3D_perform_offset, 0, NULL);
}


extern void hoa_encoder_3D_free(t_hoa_encoder_3D *x)
{
    eobj_dspfree(x);
    delete x->f_encoder;
    delete [] x->f_signals;
}

extern "C" void setup_hoa0x2e3d0x2eencoder_tilde(void)
{
    t_eclass *c;
    c = eclass_new("hoa.3d.encoder~",(method)hoa_encoder_3D_new,(method)hoa_encoder_3D_free,sizeof(t_hoa_encoder_3D), CLASS_NOINLET, A_GIMME, 0);
    
    eclass_dspinit(c);
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_encoder_3D_dsp,     "dsp",		A_CANT, 0);
    eclass_addmethod(c, (method)hoa_encoder_3D_float,   "float",   A_FLOAT, 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_encoder_3D_class = c;
}
