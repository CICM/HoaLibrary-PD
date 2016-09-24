/*
 // Copyright (c) 2012-2016 Eliott Paris & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../hoa.pd.h"

static t_class*     hoa_in_class;
static t_symbol*    hoa_sym_extra;

static void *hoa_in_new(t_symbol *s, int argc, t_atom *argv)
{
    t_hoa_in *x = (t_hoa_in *)pd_new(hoa_in_class);
    if(x)
    {
        x->f_extra = 0;
        outlet_new((t_object *)x, NULL);
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

static void hoa_in_bang(t_hoa_in *x)
{
    outlet_bang(x->f_obj.ob_outlet);
}

static void hoa_in_float(t_hoa_in *x, float f)
{
    outlet_float(x->f_obj.ob_outlet, f);
}

static void hoa_in_symbol(t_hoa_in *x, t_symbol* s)
{
    outlet_symbol(x->f_obj.ob_outlet, s);
}

static void hoa_in_list(t_hoa_in *x, t_symbol* s, int argc, t_atom* argv)
{
    outlet_list(x->f_obj.ob_outlet, s, argc, argv);
}

static void hoa_in_anything(t_hoa_in *x, t_symbol* s, int argc, t_atom* argv)
{
    outlet_anything(x->f_obj.ob_outlet, s, argc, argv);
}

extern void setup_hoa0x2ein(void)
{
    t_class* c = class_new(gensym("hoa.in"), (t_newmethod)hoa_in_new, (t_method)NULL,
                           (size_t)sizeof(t_hoa_in), CLASS_NOINLET, A_GIMME, 0);

    if(c)
    {
        class_sethelpsymbol((t_class *)c, gensym("help/hoa.io"));
        class_addmethod(c, (t_method)hoa_in_bang,       gensym("bang"),     A_CANT,  0);
        class_addmethod(c, (t_method)hoa_in_float,      gensym("float"),    A_FLOAT, 0);
        class_addmethod(c, (t_method)hoa_in_symbol,     gensym("symbol"),   A_SYMBOL,0);
        class_addmethod(c, (t_method)hoa_in_list,       gensym("list"),     A_GIMME, 0);
        class_addmethod(c, (t_method)hoa_in_anything,   gensym("anything"), A_GIMME, 0);
    }

    hoa_in_class = c;
    hoa_sym_extra = gensym("extra");
}

