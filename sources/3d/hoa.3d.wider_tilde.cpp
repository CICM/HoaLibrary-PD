/*
 // Copyright (c) 2012-2015 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

extern "C"
{
#include "../hoa.pd.h"
}
#include <Hoa.hpp>

typedef struct _hoa_3d_wider
{
    t_hoa_processor  f_obj;
    float               f_f;
    hoa::Wider<hoa::Hoa3d, t_sample>* f_processor;
    t_sample*               f_ins;
    t_sample*               f_outs;
} t_hoa_3d_wider;

static t_class *hoa_3d_wider_class;

static void *hoa_3d_wider_new(t_float f, t_symbol* s)
{
    t_hoa_3d_wider *x = (t_hoa_3d_wider *)pd_new(hoa_3d_wider_class);
    if(x)
    {
        const size_t order = (size_t)(f) < 1 ? 1 : (size_t)(f);
        x->f_processor = new hoa::Wider<hoa::Hoa3d, t_sample>(order);
        x->f_ins   = new t_sample[x->f_processor->getNumberOfHarmonics() * 81092];
        x->f_outs   = new t_sample[x->f_processor->getNumberOfHarmonics() * 81092];
        hoa_processor_init(x, x->f_processor->getNumberOfHarmonics() + 1, x->f_processor->getNumberOfHarmonics());
    }
    return x;
}

static void hoa_3d_wider_free(t_hoa_3d_wider *x)
{
    hoa_processor_clear(x);
    delete x->f_processor;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

static void hoa_3d_wider_perform(t_hoa_3d_wider *x, size_t sampleframes,
                                 size_t nins, t_sample **ins,
                                 size_t nouts, t_sample **outs)
{
    for(size_t i = 0; i < nins - 1; i++)
    {
        hoa::Signal<t_sample>::copy(sampleframes, ins[i], 1, x->f_ins+i, (nins - 1));
    }
    for(size_t i = 0; i < sampleframes; i++)
    {
        x->f_processor->setWidening(ins[nins - 1][i]);
        x->f_processor->process(x->f_ins + (nins - 1) * i, x->f_outs + nouts * i);
    }
    for(size_t i = 0; i < nouts; i++)
    {
        hoa::Signal<t_sample>::copy(sampleframes, x->f_outs+i, nouts, outs[i], 1);
    }
}

static void hoa_3d_wider_dsp(t_hoa_3d_wider *x, t_signal **sp)
{
    hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_3d_wider_perform, sp);
}

extern "C" void setup_hoa0x2e3d0x2ewider_tilde(void)
{
    t_class *c = class_new(gensym("hoa.3d.wider~"), (t_newmethod)hoa_3d_wider_new, (t_method)hoa_3d_wider_free,
                           (size_t)sizeof(t_hoa_3d_wider), CLASS_DEFAULT, A_FLOAT, A_DEFSYM, 0);
    if(c)
    {
        CLASS_MAINSIGNALIN(c, t_hoa_3d_wider, f_f);
        class_addmethod(c, (t_method)hoa_3d_wider_dsp, gensym("dsp"), A_CANT, 0);
        class_addcreator((t_newmethod)hoa_3d_wider_new, gensym("hoa.wider~"), A_FLOAT, A_DEFSYM, 0);
        class_sethelpsymbol(c, gensym("helps/hoa.3d.wider~"));
    }
    hoa_3d_wider_class = c;
}
