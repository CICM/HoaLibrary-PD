/*
 // Copyright (c) 2012-2015 Eliott Paris & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <m_pd.h>
#include <math.h>

typedef struct _hoa_pi_tilde
{
    t_object    p_ob;
    t_sample    p_value;
} t_hoa_pi_tilde;

static t_class *hoa_pi_tilde_class;

static void *hoa_pi_tilde_new(t_symbol* s, int argc, t_atom* argv)
{
    t_hoa_pi_tilde *x = (t_hoa_pi_tilde *)pd_new(hoa_pi_tilde_class);
    if(x)
    {
        outlet_new((t_object *)x, &s_signal);
        if(!argc)
        {
            x->p_value = 1;
        }
        else if(argv[0].a_type != A_FLOAT)
        {
            x->p_value = 1;
            pd_error(x, "hoa.pi~: argument type error, expected float.");
        }
        else
        {
            x->p_value = atom_getfloat(argv);
        }
        if(argc > 1)
        {
            pd_error(x, "hoa.pi~: extra argument ignored.");
        }
    }
    return(x);
}

static t_int *hoa_pi_tilde_perform(t_int *w)
{
    t_sample *in        = (t_sample *)(w[1]);
    t_sample const g    = (*(t_sample *)(w[2])) * (t_sample)M_PI;
    t_sample *out       = (t_sample *)(w[3]);
    int n               = (int)(w[4]);
    while (n--) *out++ = *in++ * g;
    return (w+5);
}

static t_int *hoa_pi_tilde_perform8(t_int *w)
{
    t_sample *in        = (t_sample *)(w[1]);
    t_sample const g    = (*(t_sample *)(w[2])) * (t_sample)M_PI;
    t_sample *out       = (t_sample *)(w[3]);
    int n               = (int)(w[4]);
    
    for (; n; n -= 8, in += 8, out += 8)
    {
        t_sample f0 = in[0], f1 = in[1], f2 = in[2], f3 = in[3];
        t_sample f4 = in[4], f5 = in[5], f6 = in[6], f7 = in[7];
        
        out[0] = f0 * g; out[1] = f1 * g;
        out[2] = f2 * g; out[3] = f3 * g;
        out[4] = f4 * g; out[5] = f5 * g;
        out[6] = f6 * g; out[7] = f7 * g;
    }
    return (w+5);
}

static void hoa_pi_tilde_dsp(t_hoa_pi_tilde *x, t_signal **sp)
{
    if(sp[0]->s_n&7)
    {
        dsp_add(hoa_pi_tilde_perform, 4, sp[0]->s_vec, &x->p_value, sp[1]->s_vec, sp[0]->s_n);
    }
    else
    {
        dsp_add(hoa_pi_tilde_perform8, 4, sp[0]->s_vec, &x->p_value, sp[1]->s_vec, sp[0]->s_n);
    }
}

void setup_hoa0x2epi_tilde(void)
{
    t_class* c  = class_new(gensym("hoa.pi~"), (t_newmethod)hoa_pi_tilde_new, (t_method)NULL,
                           sizeof(t_hoa_pi_tilde), CLASS_DEFAULT, A_GIMME, 0);
    if(c)
    {
        CLASS_MAINSIGNALIN(c, t_hoa_pi_tilde, p_value);
        class_addmethod(c, (t_method)hoa_pi_tilde_dsp, gensym("dsp"), A_CANT, 0);
    }
    
    hoa_pi_tilde_class = c;
}
