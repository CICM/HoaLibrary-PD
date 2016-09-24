/*
// Copyright (c) 2012-2015 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "hoa.pd.h"

size_t hoa_processor_clip_order(void* obj, size_t order)
{
    if(order < 1)
    {
        pd_error(obj, "%s: bad order of decomposition.", class_getname(*(t_pd *)obj));
        pd_error(obj, "%s: receive %i but expect at least 1.", class_getname(*(t_pd *)obj), (int)order);
        return 1;
    }
    return order;
}

void hoa_processor_init(void* obj, size_t nins, size_t nouts)
{
    size_t i;
    t_hoa_processor* x = (t_hoa_processor*)obj;
    x->f_nins      = nins;
    x->f_inputs    = (t_sample **)getbytes(x->f_nins * sizeof(t_sample*));
    x->f_nouts     = nouts;
    x->f_outputs   = (t_sample **)getbytes(x->f_nouts * sizeof(t_sample*));
    
    for(i = 0; i < nins - 1; ++i)
    {
        signalinlet_new((t_object *)obj, 0);
    }
    for(i = 0; i < nouts; ++i)
    {
        outlet_new((t_object *)x, &s_signal);
    }
}

void hoa_processor_clear(void* obj)
{
    t_hoa_processor* x = (t_hoa_processor*)obj;
    freebytes(x->f_inputs, x->f_nins * sizeof(t_sample*));
    freebytes(x->f_outputs, x->f_nouts * sizeof(t_sample*));
}

static t_int *hoa_processor_perform(t_int *w)
{
    t_hoa_processor *x = (t_hoa_processor *)(w[1]);
    x->f_method(x, x->f_vectorsize, x->f_nins, x->f_inputs, x->f_nouts, x->f_outputs);
    return (w+2);
}

void hoa_processor_prepare(void* obj, t_hoa_processor_perfm m, t_signal **sp)
{
    size_t i;
    t_hoa_processor *x = (t_hoa_processor *)(obj);
    x->f_method = m;
    x->f_vectorsize = sp[0]->s_n;
    for(i = 0; i < x->f_nins; ++i)
    {
        x->f_inputs[i] = sp[i]->s_vec;
    }
    for(i = 0; i < x->f_nouts; ++i)
    {
        x->f_outputs[i] = sp[i+x->f_nins]->s_vec;
    }
    dsp_add(hoa_processor_perform, 1, x);
}


