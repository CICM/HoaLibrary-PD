/*
 // Copyright (c) 2012-2016 Eliott Paris & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include <m_pd.h>
#include <math.h>

#ifndef M_PI
#define M_PI  3.14159265358979323846264338327950288
#endif

typedef struct _hoa_pi
{	
	t_object    p_obj;
	t_float     p_value;
    t_outlet   *p_outlet;
} t_hoa_pi;

static t_class *hoa_pi_class;

static void *hoa_pi_new(t_symbol *s, int argc, t_atom* argv)
{
    t_hoa_pi *x = (t_hoa_pi *)pd_new(hoa_pi_class);
    if(x)
    {
        x->p_value = (t_float)M_PI;
        if(argc && argv)
        {
            if(argv[0].a_type == A_FLOAT)
            {
                x->p_value *= atom_getfloat(argv);
            }
            else
            {
                pd_error(x, "hoa.pi: bad argument.");
            }
            if(argc > 1)
            {
                pd_error(x, "hoa.pi: extra arguments ignored.");
            }
        }
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
	x->p_value = (t_float)M_PI * f;
	hoa_pi_bang(x);
}

extern void setup_hoa0x2epi(void)
{
    t_class* c = class_new(gensym("hoa.pi"), (t_newmethod)hoa_pi_new, (t_method)NULL,
                           sizeof(t_hoa_pi), CLASS_DEFAULT, A_GIMME, 0);
    if(c)
    {
        class_addbang(c, (t_method)hoa_pi_bang);
        class_addfloat(c, (t_method)hoa_pi_float);
    }
    
    hoa_pi_class = c;
}

