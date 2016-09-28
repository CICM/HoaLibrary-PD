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

typedef struct _hoa_3d_optim
{
    t_hoa_processor  f_obj;
    float               f_f;
    hoa::Optim<hoa::Hoa3d, t_sample>* f_processor;
    t_sample*               f_ins;
    t_sample*               f_outs;
} t_hoa_3d_optim;

static t_class *hoa_3d_optim_class;
static t_symbol* hoa_sym_basic;
static t_symbol* hoa_sym_maxRe;
static t_symbol* hoa_sym_maxre;
static t_symbol* hoa_sym_inPhase;
static t_symbol* hoa_sym_inphase;

static void *hoa_3d_optim_new(t_symbol* s, int argc, t_atom* argv)
{
    t_hoa_3d_optim *x = (t_hoa_3d_optim *)pd_new(hoa_3d_optim_class);
    if(x)
    {
        const size_t order = hoa_processor_clip_order(x, (size_t)atom_getfloatarg(0, argc, argv));
        s = atom_getsymbolarg(1, argc, argv);
        x->f_processor = new hoa::Optim<hoa::Hoa3d, t_sample>(order);
        if(s == hoa_sym_basic)
        {
            x->f_processor->setMode(hoa::Optim<hoa::Hoa3d, t_sample>::Basic);
        }
        else if(s == hoa_sym_maxRe || s == hoa_sym_maxre)
        {
            x->f_processor->setMode(hoa::Optim<hoa::Hoa3d, t_sample>::MaxRe);
        }
        else
        {
            x->f_processor->setMode(hoa::Optim<hoa::Hoa3d, t_sample>::InPhase);
            if(s != hoa_sym_inPhase || s == hoa_sym_inphase)
            {
                pd_error(x, "hoa.3d.optim: bad argument.");
            }
        }
        
        x->f_ins   = new t_sample[x->f_processor->getNumberOfHarmonics() * 81092];
        x->f_outs   = new t_sample[x->f_processor->getNumberOfHarmonics() * 81092];
        hoa_processor_init(x, x->f_processor->getNumberOfHarmonics(), x->f_processor->getNumberOfHarmonics());
    }
    return x;
}

static void hoa_3d_optim_free(t_hoa_3d_optim *x)
{
    hoa_processor_clear(x);
    delete x->f_processor;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

static void hoa_3d_optim_perform(t_hoa_3d_optim *x, size_t sampleframes,
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

static void hoa_3d_optim_dsp(t_hoa_3d_optim *x, t_signal **sp)
{
    hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_3d_optim_perform, sp);
}

extern "C" void setup_hoa0x2e3d0x2eoptim_tilde(void)
{
    t_class *c = class_new(gensym("hoa.3d.optim~"), (t_newmethod)hoa_3d_optim_new, (t_method)hoa_3d_optim_free,
                           (size_t)sizeof(t_hoa_3d_optim), CLASS_DEFAULT, A_GIMME, 0); // A_FLOAT A_SYMBOL not supported
    if(c)
    {
        CLASS_MAINSIGNALIN(c, t_hoa_3d_optim, f_f);
        class_addmethod(c, (t_method)hoa_3d_optim_dsp, gensym("dsp"), A_CANT, 0);
    }
    hoa_3d_optim_class = c;
    hoa_sym_basic   = gensym("basic");
    hoa_sym_maxRe   = gensym("maxRe");
    hoa_sym_maxre   = gensym("maxre");
    hoa_sym_inPhase = gensym("inPhase");
    hoa_sym_inphase = gensym("inphase");
}

