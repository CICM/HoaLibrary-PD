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

typedef struct _hoa_2d_projector
{
    t_hoa_processor  f_obj;
    float               f_f;
    hoa::Projector<hoa::Hoa2d, t_sample>* f_processor;
    t_sample*               f_ins;
    t_sample*               f_outs;
} t_hoa_2d_projector;

static t_class *hoa_2d_projector_class;

static void *hoa_2d_projector_new(t_float f1, t_float f2)
{
    t_hoa_2d_projector *x = (t_hoa_2d_projector *)pd_new(hoa_2d_projector_class);
    if(x)
    {
        const size_t order = (size_t)(f1) < 1 ? 1 : (size_t)(f1);
        const size_t nplws = (size_t)(f2) < (order * 2 + 1) ? (order * 2 + 1) : (size_t)(f2);
        x->f_processor = new hoa::Projector<hoa::Hoa2d, t_sample>(order, nplws);
        x->f_ins   = new t_sample[x->f_processor->getNumberOfHarmonics() * 81092];
        x->f_outs  = new t_sample[x->f_processor->getNumberOfPlanewaves() * 81092];
        hoa_processor_init(x, x->f_processor->getNumberOfHarmonics(), x->f_processor->getNumberOfPlanewaves());
    }
    return x;
}

static void hoa_2d_projector_free(t_hoa_2d_projector *x)
{
    hoa_processor_clear(x);
    delete x->f_processor;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

static void hoa_2d_projector_perform(t_hoa_2d_projector *x, size_t sampleframes,
                                 size_t nins, t_sample **ins,
                                 size_t nouts, t_sample **outs)
{
    for(size_t i = 0; i < nins; i++)
    {
        hoa::Signal<t_sample>::copy(sampleframes, ins[i], 1, x->f_ins+i, nins);
    }
    for(size_t i = 0; i < sampleframes; i++)
    {
        x->f_processor->process(x->f_ins + nins * i, x->f_outs + nouts * i);
    }
    for(size_t i = 0; i < nouts; i++)
    {
        hoa::Signal<t_sample>::copy(sampleframes, x->f_outs+i, nouts, outs[i], 1);
    }
}

static void hoa_2d_projector_dsp(t_hoa_2d_projector *x, t_signal **sp)
{
    hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_2d_projector_perform, sp);
}

extern "C" void setup_hoa0x2e2d0x2eprojector_tilde(void)
{
    t_class *c = class_new(gensym("hoa.2d.projector~"), (t_newmethod)hoa_2d_projector_new, (t_method)hoa_2d_projector_free,
                           (size_t)sizeof(t_hoa_2d_projector), CLASS_DEFAULT, A_FLOAT, A_DEFSYM, 0);
    if(c)
    {
        CLASS_MAINSIGNALIN(c, t_hoa_2d_projector, f_f);
        class_addmethod(c, (t_method)hoa_2d_projector_dsp, gensym("dsp"), A_CANT, 0);
        class_addcreator((t_newmethod)hoa_2d_projector_new, gensym("hoa.projector~"), A_FLOAT, A_DEFSYM, 0);
        class_sethelpsymbol(c, gensym("helps/hoa.2d.projector~"));
    }
    hoa_2d_projector_class = c;
}
