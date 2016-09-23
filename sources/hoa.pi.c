/*
 // Copyright (c) 2012-2015 Eliott Paris & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <m_pd.h>

typedef struct _hoa_pi
{	
	t_object    p_obj;
	double      p_value;
    t_outlet   *p_outlet;
} t_hoa_pi;

static t_class *hoa_pi_class;

static void *hoa_pi_new(t_symbol *s, t_float f)
{
    t_hoa_pi *x = (t_hoa_pi *)pd_new(hoa_pi_class);
    if(x)
    {
        x->p_value  = (t_float)3.14159265358979323846264338327950288 * f;
        x->p_outlet = outlet_new((t_object *)x, &s_float);
    }
	return(x);
}

static void hoa_pi_bang(t_hoa_pi *x)
{
    outlet_float(x->p_outlet, x->p_value);
}

static void hoa_pi_float(t_hoa_pi *x, t_float f)
{
	x->p_value = (t_float)3.14159265358979323846264338327950288 * f;
	hoa_pi_bang(x);
}

void setup_hoa0x2epi(void)
{
    t_class* c = class_new(gensym("hoa.pi"), (t_newmethod)hoa_pi_new, (t_method)NULL,
                           sizeof(t_hoa_pi), CLASS_DEFAULT, A_FLOAT, 0);
    if(c)
    {
        class_addbang(c, (t_method)hoa_pi_bang);
        class_addfloat(c, (t_method)hoa_pi_float);
        class_sethelpsymbol(c, gensym("helps/hoa.pi"));
    }
    
    hoa_pi_class = c;
}

