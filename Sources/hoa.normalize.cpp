/*
// Copyright (c) 2012-2015 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.library.hpp"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_normalize
{
    t_eobj      f_obj;
    t_float     f_normalization;
    t_float     f_legendre;
    t_outlet*   f_normalization_out;
    t_outlet*   f_legendre_out;
    t_outlet*   f_sum_out;
} t_hoa_normalize;

static t_eclass *hoa_normalize_class;

static inline long double hoa_normalize_factorial(long n)
{
    long double result = n;
    if(n == 0)
        return 1;
    while(--n > 0)
        result *= n;
    
    return result;
}

static void hoa_normalize_compute(t_hoa_normalize *x, ulong degree, long order)
{
    long i = 0;
    float prevl1, prevl2;
    if(order == 0)
    {
        x->f_normalization = (sqrt(hoa_normalize_factorial(long(degree) - long(abs(order))) / hoa_normalize_factorial(long(order) + long(abs(order))))) * (sqrt((1.) / (4. * 3.14159265358979323846264338327950288)));
    }
    else
    {
        x->f_normalization = (sqrt(hoa_normalize_factorial(long(degree) - abs(order)) / hoa_normalize_factorial(long(degree) + abs(order))) * sqrt((2.) / (4. * 3.14159265358979323846264338327950288)));
    }
    
    x->f_normalization = hoa::Harmonic<Hoa3d, t_float>::getSemiNormalization(degree, order);
    x->f_legendre      = 1;
    order = order < 0 ? -order : order;
    for(i = 0; i < order; i++)
    {
        x->f_legendre *= -(2.f * float(i) + 1.f);
    }
    prevl2 = x->f_legendre;
    if(order < degree)
    {
        i++;
        prevl1 = x->f_legendre = prevl2 * (2.f * float(i) + 1.f);
        if(order < degree)
        {
            for(; i < degree; i++)
            {
                x->f_legendre = (prevl1 * (2.f * float(i) + 1.f) - prevl2 * (i + float(order))) / (float(i) - float(order) + 1.f);
                prevl2 = prevl1;
                prevl1 = x->f_legendre;
            }
        }
    }
}

static void *hoa_normalize_new(t_symbol *s, int argc, t_atom *argv)
{
    t_hoa_normalize *x = (t_hoa_normalize *)eobj_new(hoa_normalize_class);
	if (x)
	{
        hoa_normalize_compute(x, ulong(atom_getfloatarg(1, argc, argv)), long(atom_getfloatarg(2, argc, argv)));
        x->f_sum_out            = outlet_new((t_object *)x, &s_float);
        x->f_normalization_out  = outlet_new((t_object *)x, &s_float);
        x->f_legendre_out       = outlet_new((t_object *)x, &s_float);
        return x;
	}
	return NULL;
}

static void hoa_normalize_list(t_hoa_normalize *x, t_symbol* s, int argc, t_atom *argv)
{
    hoa_normalize_compute(x, ulong(atom_getfloatarg(1, argc, argv)), long(atom_getfloatarg(2, argc, argv)));
    outlet_float(x->f_legendre_out, x->f_legendre);
    outlet_float(x->f_normalization_out, x->f_normalization);
    outlet_float(x->f_sum_out, x->f_normalization * x->f_legendre);
}

static void hoa_normalize_bang(t_hoa_normalize *x)
{
    outlet_float(x->f_legendre_out, x->f_legendre);
    outlet_float(x->f_normalization_out, x->f_normalization);
    outlet_float(x->f_sum_out, x->f_normalization * x->f_legendre);
}


extern "C" void setup_hoa0x2enormalize(void)
{
    t_eclass *c = eclass_new("hoa.normalize",(method)hoa_normalize_new,(method)eobj_free, sizeof(t_hoa_normalize), 0L, A_GIMME, 0);
    if(c)
    {
        eclass_addmethod(c, (method)hoa_normalize_bang,     "bang",		A_NULL, 0);
        eclass_addmethod(c, (method)hoa_normalize_list,     "list",		A_NULL, 0);
        eclass_register(CLASS_OBJ, c);
        hoa_normalize_class = c;
    }
}



