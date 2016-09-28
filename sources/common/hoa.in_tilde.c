/*
 // Copyright (c) 2012-2016 Eliott Paris & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../hoa.pd.h"

static t_class*     hoa_in_tilde_class;
static t_symbol*    hoa_sym_extra;

static void *hoa_in_tilde_new(t_float f)
{
    t_hoa_io_tilde *x = (t_hoa_io_tilde *)pd_new(hoa_in_tilde_class);
    if(x)
    {
        x->f_extra = 0;
        x->f_signal = NULL;
        x->f_extra = f;
        outlet_new((t_object *)x, &s_signal);
        if(x->f_extra < 0)
        {
            pd_error(x, "hoa.in: bad argument, extra index must be at least 1.");
            pd_free((t_pd *)x);
            return NULL;
        }
    }

    return x;
}

static void hoa_in_tilde_dsp(t_hoa_io_tilde *x, t_signal **sp)
{
    if(x->f_signal)
    {
        dsp_add_copy(x->f_signal, sp[0]->s_vec, sp[0]->s_n);
    }
    else
    {
        dsp_add_zero(sp[0]->s_vec, sp[0]->s_n);
    }
}

extern void setup_hoa0x2ein_tilde(void)
{
    t_class* c = class_new(gensym("hoa.in~"), (t_newmethod)hoa_in_tilde_new, (t_method)NULL,
                           (size_t)sizeof(t_hoa_io_tilde), CLASS_NOINLET, A_DEFFLOAT, 0);

    if(c)
    {
        class_sethelpsymbol((t_class *)c, gensym("help/hoa.io"));
        class_addmethod(c, (t_method)hoa_in_tilde_dsp, gensym("dsp"), A_CANT, 0);
    }

    hoa_in_tilde_class = c;
    hoa_sym_extra = gensym("extra");
}


