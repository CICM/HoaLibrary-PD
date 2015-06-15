/*
// Copyright (c) 2012-2015 Eliott Paris, Julien Colafrancesco, Thomas Le Meur & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.library.hpp"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_recomposer
{
    t_edspobj                                   f_obj;

    Recomposer<Hoa2d, t_sample, hoa::Fixe>*     f_fixe;
    Recomposer<Hoa2d, t_sample, hoa::Fisheye>*  f_fisheye;
    Line<t_sample>                              f_line;
    Recomposer<Hoa2d, t_sample, hoa::Free>*     f_free;
    PolarLines<Hoa2d,t_sample>*                 f_lines;
    t_sample*                                   f_lines_vector;

	t_sample*                                   f_ins;
    t_sample*                                   f_outs;
    t_symbol*                                   f_mode;
    t_sample                                    f_ramp;

} t_hoa_recomposer;

static t_eclass *hoa_recomposer_class;

static void *hoa_recomposer_new(t_symbol *s, int argc, t_atom *argv)
{
    ulong order = 1;
    ulong numberOfPlanewaves = 4;
    t_hoa_recomposer *x = (t_hoa_recomposer *)eobj_new(hoa_recomposer_class);
    t_binbuf *d         = binbuf_via_atoms(argc,argv);

	if(x && d)
	{
        x->f_mode = hoa_sym_fixe;
        x->f_ramp = 100;
		if(argc && argv && atom_gettype(argv) == A_LONG)
			order = ulong(pd_clip_minmax(atom_getlong(argv), 1, 63));
        if(argc > 1 && argv && atom_gettype(argv+1) == A_LONG)
			numberOfPlanewaves = ulong(pd_clip_min(atom_getlong(argv+1), order * 2 + 1));
        if(argc > 2 && argv && atom_gettype(argv+2) == A_SYM)
        {
            if(atom_getsym(argv+2) == hoa_sym_free)
                x->f_mode = hoa_sym_free;
            else if(atom_getsym(argv+2) == hoa_sym_fisheye)
                x->f_mode = hoa_sym_fisheye;
        }

        if(x->f_mode == hoa_sym_fixe)
        {
            x->f_fixe = new Recomposer<Hoa2d, t_sample, Fixe>(order, numberOfPlanewaves);
            x->f_ins  = new t_float[x->f_fixe->getNumberOfPlanewaves() * HOA_MAXBLKSIZE];
            x->f_outs = new t_float[x->f_fixe->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
            eobj_dspsetup(x, long(x->f_fixe->getNumberOfPlanewaves()), long(x->f_fixe->getNumberOfHarmonics()));
        }
        else if(x->f_mode == hoa_sym_fisheye)
        {
            x->f_fisheye    = new Recomposer<Hoa2d, t_sample, Fisheye>(order, numberOfPlanewaves);
            x->f_ins        = new t_float[x->f_fisheye->getNumberOfPlanewaves() * HOA_MAXBLKSIZE];
            x->f_outs       = new t_float[x->f_fisheye->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
            x->f_line.setRamp(0.1 * sys_getsr());
            x->f_line.setValue(0.f);
            eobj_dspsetup(x, long(x->f_fisheye->getNumberOfPlanewaves() + 1), long(x->f_fisheye->getNumberOfHarmonics()));
        }
        else if(x->f_mode == hoa_sym_free)
        {
            x->f_free       = new Recomposer<Hoa2d, t_sample, Free>(order, numberOfPlanewaves);
            x->f_lines      = new PolarLines<Hoa2d,t_sample>(x->f_free->getNumberOfPlanewaves());
            x->f_ins        = new t_float[x->f_free->getNumberOfPlanewaves() * HOA_MAXBLKSIZE];
            x->f_outs       = new t_float[x->f_free->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
            x->f_lines->setRamp(0.1 * sys_getsr());
            for(ulong i = 0; i < x->f_free->getNumberOfPlanewaves(); i++)
            {
                x->f_lines->setRadiusDirect(i, x->f_free->getWidening(i));
                x->f_lines->setAzimuthDirect(i, x->f_free->getAzimuth(i));
            }
            eobj_dspsetup(x, long(x->f_free->getNumberOfPlanewaves()), long(x->f_free->getNumberOfHarmonics()));
            x->f_lines_vector   = new float[x->f_free->getNumberOfPlanewaves() * 2];
        }

        ebox_attrprocess_viabinbuf(x, d);

        return x;
	}

   	return NULL;
}

static void hoa_recomposer_float(t_hoa_recomposer *x, float f)
{
    if(x->f_mode == hoa_sym_fisheye)
    {
        x->f_line.setValue(pd_clip_minmax(f, 0., 1.));
    }
}

static void hoa_recomposer_angle(t_hoa_recomposer *x, t_symbol *s, int ac, t_atom *av)
{
    if(ac && av && x->f_mode == hoa_sym_free)
    {
        for(ulong i = 0; i < x->f_free->getNumberOfPlanewaves() && i < ulong(ac); i++)
        {
            if(atom_gettype(av+i) == A_FLOAT)
            {
                x->f_lines->setAzimuth(i, atom_getfloat(av+i));
            }
        }
    }
}

static void hoa_recomposer_wide(t_hoa_recomposer *x, t_symbol *s, int ac, t_atom *av)
{
    if(ac && av && x->f_mode == hoa_sym_free)
    {
        for(ulong i = 0; i < x->f_free->getNumberOfPlanewaves() && i < ulong(ac); i++)
        {
            if(atom_gettype(av+i) == A_FLOAT)
            {
                x->f_lines->setRadius(i, atom_getfloat(av+i));
            }
        }
    }
}

static void hoa_recomposer_perform_fixe(t_hoa_recomposer *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(int i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, ulong(numins));
    }
    for(int i = 0; i < sampleframes; i++)
    {
        x->f_fixe->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(int i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_recomposer_perform_fisheye(t_hoa_recomposer *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    const ulong numberOfPlanewaves = x->f_fisheye->getNumberOfPlanewaves();
    for(ulong i = 0; i < numberOfPlanewaves; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, numberOfPlanewaves);
    }
    for(ulong i = 0; i < ulong(sampleframes); i++)
    {
        x->f_fisheye->setFisheye(ins[numberOfPlanewaves][i]);
        x->f_fisheye->process(x->f_ins + numberOfPlanewaves * i, x->f_outs + ulong(numouts) * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_recomposer_perform_fisheye_offset(t_hoa_recomposer *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    const ulong numberOfPlanewaves = x->f_fisheye->getNumberOfPlanewaves();
    for(ulong i = 0; i < numberOfPlanewaves; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, numberOfPlanewaves);
    }
    for(ulong i = 0; i < ulong(sampleframes); i++)
    {
        x->f_fisheye->setFisheye(x->f_line.process());
        x->f_fisheye->process(x->f_ins + numberOfPlanewaves * i, x->f_outs + ulong(numouts) * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_recomposer_perform_free(t_hoa_recomposer *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    const ulong numberOfPlanewaves = x->f_free->getNumberOfPlanewaves();
    for(ulong i = 0; i < numberOfPlanewaves && i < ulong(numins); i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, numberOfPlanewaves);
    }
    for(ulong i = 0; i < ulong(sampleframes); i++)
    {
        x->f_lines->process(x->f_lines_vector);
        for(ulong j = 0; j < numberOfPlanewaves; j++)
            x->f_free->setWidening(j, x->f_lines_vector[j]);
        for(ulong j = 0; j < numberOfPlanewaves; j++)
            x->f_free->setAzimuth(j, x->f_lines_vector[j + numberOfPlanewaves]);
        x->f_free->process(x->f_ins + numberOfPlanewaves * i, x->f_outs + ulong(numouts) * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_recomposer_dsp(t_hoa_recomposer *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    if(x->f_mode == hoa_sym_fixe)
    {
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_recomposer_perform_fixe, 0, NULL);
    }
    else if(x->f_mode == hoa_sym_fisheye)
    {
        x->f_line.setRamp(x->f_ramp / 1000. * samplerate);
        if(count[x->f_fisheye->getNumberOfPlanewaves()])
            object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_recomposer_perform_fisheye, 0, NULL);
        else
            object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_recomposer_perform_fisheye_offset, 0, NULL);
    }
    else if(x->f_mode == hoa_sym_free)
    {
        x->f_lines->setRamp(x->f_ramp / 1000. * samplerate);
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_recomposer_perform_free, 0, NULL);
    }
}

static void hoa_recomposer_free(t_hoa_recomposer *x)
{
	eobj_dspfree(x);
    if(x->f_mode == hoa_sym_fixe)
    {
        delete x->f_fixe;
    }
    else if(x->f_mode == hoa_sym_fisheye)
    {
        delete x->f_fisheye;
    }
    else if(x->f_mode == hoa_sym_free)
    {
        delete x->f_free;
        delete x->f_lines;
        delete [] x->f_lines_vector;
    }
    delete [] x->f_ins;
	delete [] x->f_outs;
}

static t_pd_err ramp_set(t_hoa_recomposer *x, t_object *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_LONG || atom_gettype(argv) == A_FLOAT)
        {
            x->f_ramp = pd_clip_min(atom_getfloat(argv), 0);
            x->f_lines->setRamp(x->f_ramp / 1000. * sys_getsr());
        }
    }

    return 0;
}

extern "C" void setup_hoa0x2e2d0x2erecomposer_tilde(void)
{
    t_eclass* c;

    c = eclass_new("hoa.2d.recomposer~", (method)hoa_recomposer_new, (method)hoa_recomposer_free, (short)sizeof(t_hoa_recomposer), CLASS_NOINLET, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_recomposer_new, gensym("hoa.recomposer~"), A_GIMME, 0);

    eclass_dspinit(c);
    
    eclass_addmethod(c, (method)hoa_recomposer_dsp,     "dsp",      A_CANT, 0);
    eclass_addmethod(c, (method)hoa_recomposer_angle,   "angle",    A_GIMME,0);
    eclass_addmethod(c, (method)hoa_recomposer_wide,    "wide",     A_GIMME,0);
    eclass_addmethod(c, (method)hoa_recomposer_float,   "float",    A_FLOAT,0);

    CLASS_ATTR_DOUBLE			(c,"ramp", 0, t_hoa_recomposer, f_ramp);
    CLASS_ATTR_LABEL			(c,"ramp", 0, "Ramp Time in milliseconds");
    CLASS_ATTR_CATEGORY			(c,"ramp", 0, "Behavior");
    CLASS_ATTR_ACCESSORS		(c,"ramp", NULL, ramp_set);
    CLASS_ATTR_ORDER			(c,"ramp", 0,  "2");
    CLASS_ATTR_DEFAULT          (c,"ramp", 0, "20");
    CLASS_ATTR_SAVE             (c,"ramp", 1);

    
    hoa_recomposer_class = c;
}



