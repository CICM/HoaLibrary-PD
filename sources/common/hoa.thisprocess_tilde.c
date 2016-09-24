/*
 // Copyright (c) 2012-2016 Eliott Paris & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../hoa.pd.h"

static t_class *hoa_thisprocess_class;
static t_symbol* hoa_sym_done;

static size_t hoa_get_attributes_offset(int argc, t_atom *argv)
{
    size_t i;
    char   a;
    for(i = 0; i < argc; ++i)
    {
        if(argv[i].a_type == A_SYMBOL)
        {
            a  = atom_getsymbol(argv+i)->s_name[0];
            if(a == '@' || a == '-')
            {
                return i;
            }
        }
    }
    return (size_t)argc;
}

static char hoa_get_atoms_get_attribtues_keys(int argc, t_atom *argv, size_t* nattrs, t_hoa_attr** attrs)
{
    size_t i, j;
    char   a;
    char   key_char[MAXPDSTRING];
    t_hoa_attr* temp;
    for(i = 0, *nattrs = 0; i < argc; ++i)
    {
        if(argv[i].a_type == A_SYMBOL)
        {
            a  = atom_getsymbol(argv+i)->s_name[0];
            if(a == '@' || a == '-')
            {
                if(*nattrs && *attrs)
                {
                    temp = (t_hoa_attr *)resizebytes(*attrs, (*nattrs) * sizeof(t_hoa_attr), ((*nattrs) + 1) * sizeof(t_hoa_attr));
                    if(temp)
                    {
                        (*nattrs)++;
                        *attrs = temp;
                    }
                }
                else
                {
                    
                }
            }
        }
    }
    return 0;
}

static void *hoa_thisprocess_new(t_symbol *s, int argc, t_atom *argv)
{
    int i;
    t_hoa_thisprocess *x = (t_hoa_thisprocess *)pd_new(hoa_thisprocess_class);
    if(x)
    {
        // arg1 arg2 -size 100 -delay 1000 -feedback 0.5 -rarefaction 0
        x->f_argc = hoa_get_attributes_offset(argc, argv);
        x->f_args = (t_atom *)getbytes(x->f_argc * sizeof(t_atom));
        for(i = 0; i < x->f_argc; i++)
        {
            x->f_args[i] = argv[i];
        }
        
        x->f_out_hoa_args = outlet_new((t_object *)x, &s_list);
        x->f_out_hoa_mode = outlet_new((t_object *)x, NULL);
        x->f_out_args     = outlet_new((t_object *)x, NULL);
        x->f_out_attrs    = outlet_new((t_object *)x, NULL);

        x->f_time = clock_getsystime();
    }

    return x;
}

static void hoa_thisprocess_bang(t_hoa_thisprocess *x)
{
    for(int i = 0; i < x->f_nattrs; i++)
    {
        outlet_anything(x->f_out_attrs, x->f_attrs[i].name, (int)(x->f_attrs[i].size), x->f_attrs[i].values);
    }

    if(x->f_argc && x->f_args)
    {
        outlet_list(x->f_out_args, &s_list, (int)x->f_argc, x->f_args);
    }
    outlet_list(x->f_out_hoa_mode,  &s_list, 2, x->f_hoa_mode);
    outlet_list(x->f_out_hoa_args, &s_list, 3, x->f_hoa_args);
    outlet_symbol(x->f_out_attrs, hoa_sym_done);
}

static void hoa_thisprocess_click(t_hoa_thisprocess *x)
{
    if(clock_gettimesince(x->f_time) < 250.)
        hoa_thisprocess_bang(x);
    x->f_time = clock_getsystime();
}

static void hoa_thisprocess_free(t_hoa_thisprocess *x)
{
    int i;
    if(x->f_argc && x->f_args)
    {
        freebytes(x->f_args, x->f_argc * sizeof(t_atom));
    }
    if(x->f_nattrs)
    {
        
    }
}

extern void setup_hoa0x2ethisprocess_tilde(void)
{
    t_class* c = class_new(gensym("hoa.thisprocess~"), (t_newmethod)hoa_thisprocess_new, (t_method)hoa_thisprocess_free,
                           (size_t)sizeof(t_hoa_thisprocess), CLASS_DEFAULT, A_GIMME, 0);

    if(c)
    {
        class_addmethod(c, (t_method)hoa_thisprocess_bang,  gensym("bang"),     A_NULL, 0);
        class_addmethod(c, (t_method)hoa_thisprocess_click, gensym("click"),    A_CANT, 0);
    }

    hoa_thisprocess_class = c;
    hoa_sym_done    = gensym("done");
}


