/*
 // Copyright (c) 2012-2016 Eliott Paris & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../hoa.pd.h"

static t_class*     hoa_out_class;
static t_symbol*    hoa_sym_extra;

static void *hoa_out_new(t_symbol *s, int argc, t_atom *argv)
{
    t_hoa_out *x = (t_hoa_out *)pd_new(hoa_out_class);
    if(x)
    {
        x->f_extra  = 0;
        x->f_outlet = NULL;
        if(atom_getsymbolarg(0, argc, argv) == hoa_sym_extra)
        {
            x->f_extra = atom_getfloatarg(1, argc, argv);
            if(x->f_extra < 1)
            {
                pd_error(x, "hoa.in: bad argument, extra index must be at least 1.");
                pd_free((t_pd *)x);
                return NULL;
            }
        }
        else if(argc && argv)
        {
            pd_error(x, "hoa.in: bad argument.");
            pd_free((t_pd *)x);
            return NULL;
        }
    }
    return x;
}

static void hoa_out_bang(t_hoa_out *x)
{
    if(x->f_outlet)
    {
        outlet_bang(x->f_outlet);
    }
}

static void hoa_out_float(t_hoa_out *x, float f)
{
    if(x->f_outlet)
    {
        outlet_float(x->f_outlet, f);
    }
}

static void hoa_out_symbol(t_hoa_out *x, t_symbol* s)
{
    if(x->f_outlet)
    {
        outlet_symbol(x->f_outlet, s);
    }
}

static void hoa_out_list(t_hoa_out *x, t_symbol* s, int argc, t_atom* argv)
{
    if(x->f_outlet)
    {
        outlet_list(x->f_outlet, s, argc, argv);
    }
}

static void hoa_out_anything(t_hoa_out *x, t_symbol* s, int argc, t_atom* argv)
{
    if(x->f_outlet)
    {
        outlet_anything(x->f_outlet, s, argc, argv);
    }
}

extern void setup_hoa0x2eout(void)
{
    t_class* c = class_new(gensym("hoa.out"), (t_newmethod)hoa_out_new, (t_method)NULL,
                           (size_t)sizeof(t_hoa_out), CLASS_DEFAULT, A_GIMME, 0);
    
    if(c)
    {
        class_sethelpsymbol((t_class *)c, gensym("help/hoa.io"));
        class_addmethod(c, (t_method)hoa_out_bang,       gensym("bang"),     A_CANT,  0);
        class_addmethod(c, (t_method)hoa_out_float,      gensym("float"),    A_FLOAT, 0);
        class_addmethod(c, (t_method)hoa_out_symbol,     gensym("symbol"),   A_SYMBOL,0);
        class_addmethod(c, (t_method)hoa_out_list,       gensym("list"),     A_GIMME, 0);
        class_addmethod(c, (t_method)hoa_out_anything,   gensym("anything"), A_GIMME, 0);
    }
    
    hoa_out_class = c;
    hoa_sym_extra = gensym("extra");
}

