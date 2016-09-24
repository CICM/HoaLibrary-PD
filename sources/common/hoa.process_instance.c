/*
// Copyright (c) 2012-2016 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.pd.h"
#include <g_canvas.h>
#include <m_imp.h>

static t_symbol* hoa_sym_canvas;
static t_symbol* hoa_sym_obj;

static t_symbol* hoa_sym_hoa_thisprocess;
static t_symbol* hoa_sym_hoa_in;
static t_symbol* hoa_sym_hoa_out;
static t_symbol* hoa_sym_hoa_in_tilde;
static t_symbol* hoa_sym_hoa_out_tilde;

static t_symbol* hoa_sym_harmonics;
static t_symbol* hoa_sym_planewaves;

typedef struct _hoa_process_instance
{
    t_canvas*           f_canvas;
    t_hoa_thisprocess*  f_thisprocesses;
    t_hoa_in*           f_ins;
    t_hoa_in*           f_ins_extra;
    t_hoa_io_tilde*     f_ins_sig;
    t_hoa_io_tilde*     f_ins_extra_sig;
    t_hoa_out*          f_outs;
    t_hoa_out*          f_outs_extra;
    t_hoa_io_tilde*     f_outs_sig;
    t_hoa_io_tilde*     f_outs_extra_sig;
} t_hoa_process_instance;

extern void hoa_process_instance_setup()
{
    hoa_sym_canvas          = gensym("canvas");
    hoa_sym_obj             = gensym("obj");
    
    hoa_sym_hoa_thisprocess = gensym("hoa.thisprocess~");
    hoa_sym_hoa_in          = gensym("hoa.in");
    hoa_sym_hoa_out         = gensym("hoa.out");
    hoa_sym_hoa_in_tilde    = gensym("hoa.in~");
    hoa_sym_hoa_out_tilde   = gensym("hoa.out~");
    
    hoa_sym_harmonics       = gensym("harmonics");
    hoa_sym_planewaves      = gensym("planewaves");
}

static void hoa_process_instance_get_thisprocesses(t_hoa_process_instance* x, t_canvas* cnv)
{
    t_gobj *y;
    t_symbol const* name;
    for(y = cnv->gl_list; y; y = y->g_next)
    {
        name = y->g_pd->c_name;
        if(name == hoa_sym_canvas)
        {
            hoa_process_instance_get_thisprocesses(x, (t_canvas *)y);
        }
        else if(name == hoa_sym_hoa_thisprocess)
        {
            ((t_hoa_thisprocess*)y)->f_next = x->f_thisprocesses;
            x->f_thisprocesses = (t_hoa_thisprocess*)y;
        }
    }
}

static void hoa_process_instance_get_hoas(t_hoa_process_instance* x, t_canvas* cnv)
{
    t_gobj *y;
    t_symbol const* name;
    for(y = cnv->gl_list; y; y = y->g_next)
    {
        name = y->g_pd->c_name;
        if(name == hoa_sym_canvas)
        {
            hoa_process_instance_get_hoas(x, (t_canvas *)y);
        }
        else if(name == hoa_sym_hoa_thisprocess)
        {
            ((t_hoa_thisprocess*)y)->f_next = x->f_thisprocesses;
            x->f_thisprocesses = (t_hoa_thisprocess*)y;
        }
        else if(name == hoa_sym_hoa_in)
        {
            t_hoa_in* inlet = (t_hoa_in *)y;
            if(inlet->f_extra)
            {
                inlet->f_next = x->f_ins_extra;
                x->f_ins_extra = inlet;
            }
            else
            {
                inlet->f_next = x->f_ins;
                x->f_ins = inlet;
            }
        }
        else if(name == hoa_sym_hoa_out)
        {
            t_hoa_out* outlet = (t_hoa_out *)y;
            if(outlet->f_extra)
            {
                outlet->f_next = x->f_outs_extra;
                x->f_outs_extra = outlet;
            }
            else
            {
                outlet->f_next = x->f_outs;
                x->f_outs = outlet;
            }
        }
        else if(name == hoa_sym_hoa_in_tilde)
        {
            t_hoa_io_tilde* inlet_sig = (t_hoa_io_tilde *)y;
            if(inlet_sig->f_extra)
            {
                inlet_sig->f_next = x->f_ins_extra_sig;
                x->f_ins_extra_sig = inlet_sig;
            }
            else
            {
                inlet_sig->f_next = x->f_ins_sig;
                x->f_ins_sig = inlet_sig;
            }
        }
        else if(name == hoa_sym_hoa_out_tilde)
        {
            t_hoa_io_tilde* outlet_sig = (t_hoa_io_tilde *)y;
            if(outlet_sig->f_extra)
            {
                outlet_sig->f_next = x->f_outs_extra_sig;
                x->f_outs_extra_sig = outlet_sig;
            }
            else
            {
                outlet_sig->f_next = x->f_outs_sig;
                x->f_outs_sig = outlet_sig;
            }
        }
    }
}

/*
static void thisprocess_init(t_hoa_thisprocess* thisprocess, int argc, t_atom* argv, int nattrs, t_atom* attrs)
{
    if(thisprocess)
    {
        if(argc > 0 && argv)
        {
            if(thisprocess->f_argc < argc)
            {
                if(!thisprocess->f_argc && !thisprocess->f_args)
                {
                    free(thisprocess->f_args);
                }
                thisprocess->f_argc = argc;
                thisprocess->f_args = (t_atom *)malloc((size_t)argc * sizeof(t_atom));
            }
            memcpy(thisprocess->f_args, argv, (size_t)argc * sizeof(t_atom));
        }
        if(nattrs && attrs)
        {
            for(int i = 0; i < thisprocess->f_n_attrs; i++)
            {
                if(atoms_has_attribute(nattrs, attrs, thisprocess->f_attr_name[i]))
                {
                    free(thisprocess->f_attr_vals[i]);
                    thisprocess->f_attr_size[i] = 0;
                    atoms_get_attribute(nattrs, attrs, thisprocess->f_attr_name[i],
                                        &thisprocess->f_attr_size[i],
                                        &thisprocess->f_attr_vals[i]);
                }
            }
        }
        thisprocess->f_nit = 1;
    }
}
 */

extern void hoa_process_instance_init(t_hoa_process_instance* x, t_canvas* parent,
                t_symbol* name, t_symbol* domain, t_symbol* dimension,
                long ac1, long ac2, long ac3,
                long argc, t_atom* argv,
                long nattr, t_atom* attrs)
{
    t_gobj* z;
    t_atom av[6];
    t_binbuf* b;
    
    int natom;
    t_atom* vec;
    
    SETFLOAT(av, 1);
    SETFLOAT(av+1, 1);
    SETSYMBOL(av+2, name);
    SETFLOAT(av+3, ac1);
    SETFLOAT(av+4, ac2);
    SETFLOAT(av+5, ac3);
    
    x->f_canvas = NULL;
    pd_typedmess((t_pd *)parent, hoa_sym_obj, domain == hoa_sym_harmonics ? 6 : 5, av);
    
    for(z = parent->gl_list; z; z = z->g_next)
    {
        if(z->g_pd->c_name == hoa_sym_canvas)
        {
            b = ((t_canvas *)z)->gl_obj.te_binbuf;
            if(b)
            {
                natom   = binbuf_getnatom(b);
                vec     = binbuf_getvec(b);
                if(natom > (domain == hoa_sym_harmonics ? 3 : 2) && vec)
                {
                    if(atom_getsymbol(vec) == name && atom_getfloat(vec+1) == (t_float)ac1 && atom_getfloat(vec+2) == (t_float)ac2 && (domain == hoa_sym_planewaves || atom_getfloat(vec+3) == (t_float)ac3))
                    {
                        x->f_canvas = (t_canvas *)z;
                    }
                }
            }
        }
    }
    
    if(x->f_canvas)
    {
        hoa_process_instance_get_thisprocesses(x, x->f_canvas);
        
        for(size_t i = 0; i < f_thisprocesses.size(); i++)
        {
            atom_setsym(f_thisprocesses[i]->f_hoa_mode, dimension);
            atom_setsym(f_thisprocesses[i]->f_hoa_mode+1, domain);
            atom_setfloat(f_thisprocesses[i]->f_hoa_args, ac1);
            atom_setfloat(f_thisprocesses[i]->f_hoa_args+1, ac2);
            atom_setfloat(f_thisprocesses[i]->f_hoa_args+2, ac3);
            thisprocess_init(f_thisprocesses[i], argc, argv, nattr, attrs);
        }
        
        canvas_loadbang(x->f_canvas);
        hoa_process_instance_get_hoas(x, x->f_canvas);
    }
}






inline void show() const noexcept
{
    if(f_canvas)
        canvas_vis(f_canvas, 1);
}

inline void sendBang() const noexcept
{
    for(ulong i = 0; i < f_ins.size(); i++)
    {
        pd_bang((t_pd *)f_ins[i]);
    }
}

inline void sendBang(ulong extra) const noexcept
{
    for(ulong i = 0; i < f_ins_extra.size(); i++)
    {
        if(ulong(f_ins_extra[i]->f_extra) == extra)
        {
            pd_bang((t_pd *)f_ins_extra[i]);
        }
    }
}

inline void sendFloat(const float f) const noexcept
{
    for(ulong i = 0; i < f_ins.size(); i++)
    {
        pd_float((t_pd *)f_ins[i], f);
    }
}

inline void sendFloat(ulong extra, const float f) const noexcept
{
    for(ulong i = 0; i < f_ins_extra.size(); i++)
    {
        if(ulong(f_ins_extra[i]->f_extra) == extra)
        {
            pd_float((t_pd *)f_ins_extra[i], f);
        }
    }
}

inline void sendSymbol(t_symbol* s) const noexcept
{
    for(ulong i = 0; i < f_ins.size(); i++)
    {
        pd_symbol((t_pd *)f_ins[i], s);
    }
}

inline void sendSymbol(ulong extra, t_symbol* s) const noexcept
{
    for(ulong i = 0; i < f_ins_extra.size(); i++)
    {
        if(ulong(f_ins_extra[i]->f_extra) == extra)
        {
            pd_symbol((t_pd *)f_ins_extra[i], s);
        }
    }
}

inline void sendList(t_symbol* s, int argc, t_atom* argv) const noexcept
{
    for(ulong i = 0; i < f_ins.size(); i++)
    {
        pd_list((t_pd *)f_ins[i], s, argc, argv);
    }
}

inline void sendList(ulong extra, t_symbol* s, int argc, t_atom* argv) const noexcept
{
    for(ulong i = 0; i < f_ins_extra.size(); i++)
    {
        if(ulong(f_ins_extra[i]->f_extra) == extra)
        {
            pd_list((t_pd *)f_ins_extra[i], s, argc, argv);
        }
    }
}

inline void sendAnything(t_symbol* s, int argc, t_atom* argv) const noexcept
{
    for(ulong i = 0; i < f_ins.size(); i++)
    {
        pd_typedmess((t_pd *)f_ins[i], s, argc, argv);
    }
}

inline void sendAnything(ulong extra, t_symbol* s, int argc, t_atom* argv) const noexcept
{
    for(ulong i = 0; i < f_ins_extra.size(); i++)
    {
        if(ulong(f_ins_extra[i]->f_extra) == extra)
        {
            pd_typedmess((t_pd *)f_ins_extra[i], s, argc, argv);
        }
    }
}

inline bool hasNormalInputs() const
{
    return !f_ins.empty();
}

inline bool hasNormalSignalInputs() const
{
    return !f_ins_sig.empty();
}

inline ulong getMaximumInputExtraIndex() const
{
    ulong n = 0ul;
    for(ulong i = 0; i < f_ins_extra.size(); i++)
    {
        if(ulong(f_ins_extra[i]->f_extra) > n)
            n = ulong(f_ins_extra[i]->f_extra);
    }
    return n;
}

inline ulong getMaximumSignalInputExtraIndex() const
{
    ulong n = 0ul;
    for(ulong i = 0; i < f_ins_extra_sig.size(); i++)
    {
        if(ulong(f_ins_extra_sig[i]->f_extra) > n)
            n = ulong(f_ins_extra_sig[i]->f_extra);
    }
    return n;
}

inline bool hasNormalOutputs() const
{
    return !f_outs.empty();
}

inline bool hasNormalSignalOutputs() const
{
    return !f_outs_sig.empty();
}

inline ulong getMaximumOutputExtraIndex() const
{
    ulong n = 0ul;
    for(ulong i = 0; i < f_outs_extra.size(); i++)
    {
        if(ulong(f_outs_extra[i]->f_extra) > n)
            n = ulong(f_outs_extra[i]->f_extra);
    }
    return n;
}

inline ulong getMaximumSignalOutputExtraIndex() const
{
    ulong n = 0ul;
    for(ulong i = 0; i < f_outs_extra_sig.size(); i++)
    {
        if(ulong(f_outs_extra_sig[i]->f_extra) > n)
            n = ulong(f_outs_extra_sig[i]->f_extra);
    }
    return n;
}

inline void setNomalOutlet(t_outlet* outlet)
{
    for(ulong i = 0; i < f_outs.size(); i++)
    {
        f_outs[i]->f_outlet = outlet;
    }
}

inline void setExtraOutlet(t_outlet* outlet, ulong index)
{
    for(ulong i = 0; i < f_outs_extra.size(); i++)
    {
        if(ulong(f_outs_extra[i]->f_extra) == index)
        {
            f_outs_extra[i]->f_outlet = outlet;
        }
    }
}

bool prepareDsp(t_sample* in, vector<t_sample*>& ixtra, t_sample* out, vector<t_sample*>& oxtra)
{
    if(hasNormalSignalInputs())
    {
        if(!in)
        {
            bug("process don't have input signal.");
            return true;
        }
        for(size_t i = 0; i < f_ins_sig.size(); i++)
        {
            f_ins_sig[i]->f_signal = in;
        }
    }
    for(size_t i = 0; i < f_ins_extra_sig.size(); i++)
    {
        if(ulong(f_ins_extra_sig[i]->f_extra) > ixtra.size() || !ixtra[size_t(f_ins_extra_sig[i]->f_extra-1)])
        {
            bug("process don't have input signal extra %i.", f_ins_extra_sig[i]->f_extra);
            return true;
        }
        f_ins_extra_sig[i]->f_signal = ixtra[size_t(f_ins_extra_sig[i]->f_extra-1)];
    }
    if(hasNormalSignalOutputs())
    {
        if(!out)
        {
            bug("process don't have output signal.");
            return true;
        }
        for(size_t i = 0; i < f_outs_sig.size(); i++)
        {
            f_outs_sig[i]->f_signal = out;
        }
    }
    for(size_t i = 0; i < f_outs_extra_sig.size(); i++)
    {
        if(ulong(f_outs_extra_sig[i]->f_extra) > oxtra.size() || !oxtra[size_t(f_outs_extra_sig[i]->f_extra-1)])
        {
            bug("process don't have output signal extra %i.", f_outs_extra_sig[i]->f_extra);
            return true;
        }
        f_outs_extra_sig[i]->f_signal = oxtra[size_t(f_outs_extra_sig[i]->f_extra-1)];
    }
    return false;
}







