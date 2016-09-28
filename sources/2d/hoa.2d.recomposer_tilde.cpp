/*
// Copyright (c) 2012-2016 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

extern "C"
{
#include "../hoa.pd.h"
}
#include <Hoa.hpp>

typedef struct _hoa_2d_recomposer
{
    t_hoa_processor  f_obj;
    t_float                                 f_f;
    hoa::Recomposer<hoa::Hoa2d, t_sample>*  f_processor;
    hoa::PolarLines<hoa::Hoa2d,t_sample>*   f_lines;
    t_sample*                               f_lines_vector;
	t_sample*                               f_ins;
    t_sample*                               f_outs;
    t_symbol*                               f_mode;
    t_sample                                f_ramp;

} t_hoa_2d_recomposer;

static t_class *hoa_2d_recomposer_class;
static t_symbol* hoa_sym_fixe;
static t_symbol* hoa_sym_fisheye;
static t_symbol* hoa_sym_free;

static void *hoa_2d_recomposer_new(t_symbol *s, int argc, t_atom *argv)
{
    t_hoa_2d_recomposer *x = (t_hoa_2d_recomposer *)pd_new(hoa_2d_recomposer_class);
    if(x)
	{
        const size_t order   = hoa_processor_clip_order(x, (size_t)atom_getfloatarg(0, argc, argv));
        size_t nplws = atom_getfloatarg(1, argc, argv);
        if(nplws < order * 2 + 1)
        {
            pd_error(x, "hoa.2d.recomposer~: bad argument : number of planewaves must be at least 2 * order + 1.");
            nplws = order * 2 + 1;
        }
        x->f_mode = atom_getsymbolarg(2, argc, argv);
        if(x->f_mode == hoa_sym_fixe)
        {
            x->f_processor = new hoa::RecomposerFixe<hoa::Hoa2d, t_sample>(order, nplws);
        }
        else if(x->f_mode == hoa_sym_fisheye)
        {
            x->f_processor = new hoa::RecomposerFisheye<hoa::Hoa2d, t_sample>(order, nplws);
        }
        else
        {
            hoa::RecomposerFree<hoa::Hoa2d, t_sample>* temp = new hoa::RecomposerFree<hoa::Hoa2d, t_sample>(order, nplws);
            x->f_processor = temp;
            x->f_lines = new hoa::PolarLines<hoa::Hoa2d,t_sample>(nplws);
            x->f_lines->setRamp(0.1 * sys_getsr());
            for(size_t i = 0; i < x->f_processor->getNumberOfPlanewaves(); i++)
            {
                x->f_lines->setRadiusDirect(i, temp->getWidening(i));
                x->f_lines->setAzimuthDirect(i, temp->getAzimuth(i));
            }
            
            x->f_lines_vector = hoa::Signal<t_sample>::alloc(temp->getNumberOfPlanewaves() * 2);
            if(x->f_mode != hoa_sym_free)
            {
                pd_error(x, "hoa.2d.recomposer~: bad argument : mode must be fixe, free or fisheye.");
                x->f_mode = hoa_sym_free;
            }
        }
        
        x->f_ramp = 100;
        x->f_ins   = new t_sample[x->f_processor->getNumberOfHarmonics() * 81092];
        x->f_outs  = new t_sample[x->f_processor->getNumberOfPlanewaves() * 81092];
        hoa_processor_init(x,
                           x->f_processor->getNumberOfPlanewaves() + static_cast<size_t>(x->f_mode == hoa_sym_fisheye),
                           x->f_processor->getNumberOfHarmonics());
	}

   	return x;
}

static void hoa_2d_recomposer_free(t_hoa_2d_recomposer *x)
{
    hoa_processor_clear(x);
    if(x->f_mode == hoa_sym_free)
    {
        delete x->f_lines;
        hoa::Signal<t_sample>::free(x->f_lines_vector);
    }
    delete x->f_processor;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

static void hoa_2d_recomposer_angle(t_hoa_2d_recomposer *x, t_symbol *s, int argc, t_atom *argv)
{
    for(size_t i = 0; i < x->f_processor->getNumberOfPlanewaves() && i < size_t(argc); i++)
    {
        x->f_lines->setAzimuth(i, atom_getfloatarg(i, argc, argv));
    }
}

static void hoa_2d_recomposer_wide(t_hoa_2d_recomposer *x, t_symbol *s, int argc, t_atom *argv)
{
    for(size_t i = 0; i < x->f_processor->getNumberOfPlanewaves() && i < size_t(argc); i++)
    {
        x->f_lines->setRadius(i, atom_getfloatarg(i, argc, argv));
    }
}

static void hoa_2d_recomposer_perform_fixe(t_hoa_2d_recomposer *x, size_t sampleframes,
                                           size_t nins, t_sample **ins,
                                           size_t nouts, t_sample **outs)
{
    hoa::RecomposerFixe<hoa::Hoa2d, t_sample>* proc = static_cast< hoa::RecomposerFixe<hoa::Hoa2d, t_sample>* >(x->f_processor);
    for(size_t i = 0; i < nins; i++)
    {
        hoa::Signal<t_sample>::copy(size_t(sampleframes), ins[i], 1, x->f_ins+i, size_t(nins));
    }
    for(size_t i = 0; i < sampleframes; i++)
    {
        proc->process(x->f_ins + nins * i, x->f_outs + nouts * i);
    }
    for(size_t i = 0; i < nouts; i++)
    {
        hoa::Signal<t_sample>::copy(size_t(sampleframes), x->f_outs+i, size_t(nouts), outs[i], 1);
    }
}

static void hoa_2d_recomposer_perform_fisheye(t_hoa_2d_recomposer *x, size_t sampleframes,
                                              size_t nins, t_sample **ins,
                                              size_t nouts, t_sample **outs)
{
    const size_t numberOfPlanewaves = nins - 1;
    hoa::RecomposerFisheye<hoa::Hoa2d, t_sample>* proc = static_cast< hoa::RecomposerFisheye<hoa::Hoa2d, t_sample>* >(x->f_processor);
    for(size_t i = 0; i < numberOfPlanewaves; i++)
    {
        hoa::Signal<t_sample>::copy(size_t(sampleframes), ins[i], 1, x->f_ins+i, numberOfPlanewaves);
    }
    for(size_t i = 0; i < sampleframes; i++)
    {
        proc->setFisheye(ins[numberOfPlanewaves][i]);
        proc->process(x->f_ins + numberOfPlanewaves * i, x->f_outs + size_t(nouts) * i);
    }
    for(long i = 0; i < nouts; i++)
    {
        hoa::Signal<t_sample>::copy(size_t(sampleframes), x->f_outs+i, size_t(nouts), outs[i], 1);
    }
}

static void hoa_2d_recomposer_perform_free(t_hoa_2d_recomposer *x, size_t sampleframes,
                                           size_t nins, t_sample **ins,
                                           size_t nouts, t_sample **outs)
{
    hoa::RecomposerFree<hoa::Hoa2d, t_sample>* proc = static_cast< hoa::RecomposerFree<hoa::Hoa2d, t_sample>* >(x->f_processor);
    for(size_t i = 0; i < size_t(nins); i++)
    {
        hoa::Signal<t_sample>::copy(size_t(sampleframes), ins[i], 1, x->f_ins+i, nins);
    }
    for(size_t i = 0; i < size_t(sampleframes); i++)
    {
        x->f_lines->process(x->f_lines_vector);
        for(size_t j = 0; j < nins; j++)
            proc->setWidening(j, x->f_lines_vector[j]);
        for(size_t j = 0; j < nins; j++)
            proc->setAzimuth(j, x->f_lines_vector[j + nins]);
        proc->process(x->f_ins + nins * i, x->f_outs + size_t(nouts) * i);
    }
    for(size_t i = 0; i < nouts; i++)
    {
        hoa::Signal<t_sample>::copy(size_t(sampleframes), x->f_outs+i, size_t(nouts), outs[i], 1);
    }
}

static void hoa_2d_recomposer_dsp(t_hoa_2d_recomposer *x, t_signal **sp)
{
    if(x->f_mode == hoa_sym_fixe)
    {
        hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_2d_recomposer_perform_fixe, sp);
    }
    else if(x->f_mode == hoa_sym_fisheye)
    {
        hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_2d_recomposer_perform_fisheye, sp);
    }
    else if(x->f_mode == hoa_sym_free)
    {
        x->f_lines->setRamp(x->f_ramp / 1000. * sp[0]->s_sr);
        hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_2d_recomposer_perform_free, sp);
    }
}

static void hoa_2d_recomposer_ramp(t_hoa_2d_recomposer *x, t_float f)
{
    x->f_ramp = (f < 0.) ? 0. : f;
    x->f_lines->setRamp(x->f_ramp / 1000. * sys_getsr());
}

extern "C" void setup_hoa0x2e2d0x2erecomposer_tilde(void)
{
    t_class* c = class_new(gensym("hoa.2d.recomposer~"), (t_newmethod)hoa_2d_recomposer_new, (t_method)hoa_2d_recomposer_free,
                            (size_t)sizeof(t_hoa_2d_recomposer), CLASS_DEFAULT, A_GIMME, 0);
    if(c)
    {
        CLASS_MAINSIGNALIN(c, t_hoa_2d_recomposer, f_f);
        class_addcreator((t_newmethod)hoa_2d_recomposer_new, gensym("hoa.recomposer~"), A_GIMME, 0);
        class_addmethod(c, (t_method)hoa_2d_recomposer_dsp,     gensym("dsp"),      A_CANT, 0);
        class_addmethod(c, (t_method)hoa_2d_recomposer_angle,   gensym("angle"),    A_GIMME,0);
        class_addmethod(c, (t_method)hoa_2d_recomposer_wide,    gensym("wide"),     A_GIMME,0);
        class_addmethod(c, (t_method)hoa_2d_recomposer_ramp,    gensym("ramp"),     A_FLOAT,0);
    }
    
    hoa_sym_fixe    = gensym("fixe");
    hoa_sym_fisheye = gensym("fisheye");
    hoa_sym_free    = gensym("free");
    hoa_2d_recomposer_class = c;
}



