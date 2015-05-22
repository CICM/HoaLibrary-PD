/*
// Copyright (c) 2012-2015 Eliott Paris, Julien Colafrancesco, Thomas Le Meur & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.library.h"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

extern "C"
{
#include "../ThirdParty/CicmWrapper/Sources/ecommon/d_ugen.h"
}

#ifdef PD_EXTENDED
extern t_canvas *canvas_list;
#endif

static t_canvas *sys_getcanvaslist()
{
#ifdef PD_EXTENDED
    return canvas_list;
#else
    return pd_this->pd_canvaslist;
#endif
}

static void sys_setcanvaslist(t_canvas* canvas)
{
#ifdef PD_EXTENDED
    canvas_list = canvas;
#else
    pd_this->pd_canvaslist = canvas;
#endif
}

static void canvas_removefromlist(t_canvas *x)
{
    t_canvas *z;
    if (x == sys_getcanvaslist())
    {
        sys_setcanvaslist(x->gl_next);
    }
    else
    {
        for (z = sys_getcanvaslist(); z->gl_next != x; z = z->gl_next)
            ;
        z->gl_next = x->gl_next;
    }
}

typedef struct _hoa_in
{
    t_eobj  f_obj;
    int     f_extra;
} t_hoa_in;

typedef struct _hoa_out
{
    t_eobj      f_obj;
    t_outlet *f_outlet;
    int         f_extra;
} t_hoa_out;

typedef struct _hoa_in_tilde
{
    t_edspobj   f_obj;
    t_sample*   f_signal;
    int         f_extra;
} t_hoa_in_tilde;

typedef struct _hoa_out_tilde
{
    t_edspobj   f_obj;
    t_sample*   f_signal;
    int         f_extra;
} t_hoa_out_tilde;

typedef struct _hoa_thisprocess
{
    t_eobj      j_box;
    char        f_nit;

    t_outlet*   f_out_hoa_args;
    t_outlet*   f_out_hoa_mode;
	t_outlet*   f_out_args;
    t_outlet*   f_out_attrs;
    t_outlet*   f_out_mute;

    t_atom      f_hoa_args[3];
    t_atom      f_hoa_mode[2];

    t_atom*     f_args;
    long        f_argc;

    long        f_n_attrs;
    t_symbol**  f_attr_name;
    t_atom*     f_attr_vals[EPD_MAX_SIGS];
    long        f_attr_size[EPD_MAX_SIGS];
    double      f_time;
} t_hoa_thisprocess;

class ProcessInstance
{
    t_dspcontext*               m_context;
    t_canvas*                   m_canvas;
    vector<t_hoa_thisprocess*>  m_thisprocesses;
    vector<t_hoa_in*>           m_ins;
    vector<t_hoa_in*>           m_ins_extra;
    vector<t_hoa_in_tilde*>     m_ins_sig;
    vector<t_hoa_in_tilde*>     m_ins_extra_sig;
    vector<t_hoa_out*>          m_outs;
    vector<t_hoa_out*>          m_outs_extra;
    vector<t_hoa_out_tilde*>    m_outs_sig;
    vector<t_hoa_out_tilde*>    m_outs_extra_sig;
    
private:
    
    void getObjects(t_canvas* canvas)
    {
        for(t_gobj *y = canvas->gl_list; y; y = y->g_next)
        {
            if(eobj_getclassname(y) == hoa_sym_canvas)
            {
                getObjects((t_canvas *)y);
            }
            else if(eobj_getclassname(y) == hoa_sym_hoa_thisprocess)
            {
                m_thisprocesses.push_back((t_hoa_thisprocess*)y);
            }
            else if(eobj_getclassname(y) == hoa_sym_hoa_in)
            {
                t_hoa_in *inlet = (t_hoa_in *)y;
                if(inlet->f_extra)
                    m_ins_extra.push_back(inlet);
                else
                    m_ins.push_back(inlet);
            }
            else if(eobj_getclassname(y) == hoa_sym_hoa_out)
            {
                t_hoa_out* outlet = (t_hoa_out *)y;
                if(outlet->f_extra)
                    m_outs_extra.push_back(outlet);
                else
                    m_outs.push_back(outlet);
            }
            else if(eobj_getclassname(y) == hoa_sym_hoa_in_tilde)
            {
                t_hoa_in_tilde* inlet_sig = (t_hoa_in_tilde *)y;
                if(inlet_sig->f_extra)
                    m_ins_extra_sig.push_back(inlet_sig);
                else
                    m_ins_sig.push_back(inlet_sig);
            }
            else if(eobj_getclassname(y) == hoa_sym_hoa_out_tilde)
            {
                t_hoa_out_tilde * outlet_sig = (t_hoa_out_tilde *)y;
                if(outlet_sig->f_extra)
                    m_outs_extra_sig.push_back(outlet_sig);
                else
                    m_outs_sig.push_back(outlet_sig);
            }
        }
    }
    
    static void thisprocess_init(t_hoa_thisprocess* thisprocess, long argc, t_atom* argv, long nattrs, t_atom* attrs)
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
                thisprocess->f_args = (t_atom *)malloc(argc * sizeof(t_atom));
            }
            memcpy(thisprocess->f_args, argv, argc * sizeof(t_atom));
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
    
public:
    ProcessInstance(t_canvas* parent,
                    t_symbol* name,
                    t_symbol* dir,
                    t_symbol* domain,
                    t_symbol* dimension,
                    long ac1, long ac2, long ac3,
                    long argc,
                    t_atom* argv,
                    long nattr,
                    t_atom* attrs)
    {
        m_context = dsp_context_new();
        t_pd *canvas = s__X.s_thing;
        s__X.s_thing = 0;
        t_atom av[3];
        atom_setfloat(av, ac1);
        atom_setfloat(av+1, ac2);
        atom_setfloat(av+2, ac3);
        canvas_setargs(domain == hoa_sym_planewaves ? 2 : 3, av);
        binbuf_evalfile(name, dir);
        while(((t_pd *)m_canvas != s__X.s_thing) && s__X.s_thing)
        {
            m_canvas = (t_canvas *)s__X.s_thing;
            vmess((t_pd *)m_canvas, gensym("pop"), "i", 1);
        }
        
        if(m_canvas)
        {
            getObjects(m_canvas);
            for(size_t i = 0; i < m_thisprocesses.size(); i++)
            {
                atom_setsym(m_thisprocesses[i]->f_hoa_mode, dimension);
                atom_setsym(m_thisprocesses[i]->f_hoa_mode+1, domain);
                atom_setfloat(m_thisprocesses[i]->f_hoa_args, ac1);
                atom_setfloat(m_thisprocesses[i]->f_hoa_args+1, ac2);
                atom_setfloat(m_thisprocesses[i]->f_hoa_args+2, ac3);
                thisprocess_init(m_thisprocesses[i], argc, argv, nattr, attrs);
            }
            m_canvas->gl_owner = parent;
            canvas_removefromlist(m_canvas);
            canvas_loadbang(m_canvas);
            canvas_vis(m_canvas, 0);
        }
        s__X.s_thing = canvas;
    }
    
    ~ProcessInstance()
    {
        m_thisprocesses.clear();
        m_ins.clear();
        m_ins_extra.clear();
        m_ins_sig.clear();
        m_ins_extra_sig.clear();
        m_outs.clear();
        m_outs_extra.clear();
        m_outs_sig.clear();
        m_outs_extra_sig.clear();
        if(m_canvas)
        {
            canvas_free(m_canvas);
        }
        dsp_context_free(m_context);
    }
    
    void show()
    {
        if(m_canvas)
            canvas_vis(m_canvas, 1);
    }
    
    void sendBang()
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_bang((t_pd *)m_ins[i]);
        }
    }
    
    void sendBang(ulong extra)
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(m_ins_extra[i]->f_extra == extra)
            {
                pd_bang((t_pd *)m_ins_extra[i]);
            }
        }
    }
    
    void sendFloat(const float f)
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_float((t_pd *)m_ins[i], f);
        }
    }
    
    void sendFloat(ulong extra, const float f)
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(m_ins_extra[i]->f_extra == extra)
            {
                pd_float((t_pd *)m_ins_extra[i], f);
            }
        }
    }
    
    void sendSymbol(t_symbol* s)
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_symbol((t_pd *)m_ins[i], s);
        }
    }
    
    void sendSymbol(ulong extra, t_symbol* s)
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(m_ins_extra[i]->f_extra == extra)
            {
                pd_symbol((t_pd *)m_ins_extra[i], s);
            }
        }
    }
    
    void sendList(t_symbol* s, int argc, t_atom* argv)
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_list((t_pd *)m_ins[i], s, argc, argv);
        }
    }
    
    void sendList(ulong extra, t_symbol* s, int argc, t_atom* argv)
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(m_ins_extra[i]->f_extra == extra)
            {
                pd_list((t_pd *)m_ins_extra[i], s, argc, argv);
            }
        }
    }
    
    void sendAnything(t_symbol* s, int argc, t_atom* argv)
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_typedmess((t_pd *)m_ins[i], s, argc, argv);
        }
    }
    
    void sendAnything(ulong extra, t_symbol* s, int argc, t_atom* argv)
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(m_ins_extra[i]->f_extra == extra)
            {
                pd_typedmess((t_pd *)m_ins_extra[i], s, argc, argv);
            }
        }
    }
    
    bool hasNormalInputs() const
    {
        return !m_ins.empty();
    }
    
    bool hasNormalSignalInputs() const
    {
        return !m_ins_sig.empty();
    }
    
    ulong getMaximumInputExtraIndex() const
    {
        ulong n = 0ul;
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(m_ins_extra[i]->f_extra > n)
                n = m_ins_extra[i]->f_extra;
        }
        return n;
    }
    
    ulong getMaximumSignalInputExtraIndex() const
    {
        ulong n = 0ul;
        for(ulong i = 0; i < m_ins_extra_sig.size(); i++)
        {
            if(m_ins_extra_sig[i]->f_extra > n)
                n = m_ins_extra_sig[i]->f_extra;
        }
        return n;
    }
    
    bool hasNormalOutputs() const
    {
        return !m_outs.empty();
    }
    
    bool hasNormalSignalOutputs() const
    {
        return !m_outs_sig.empty();
    }
    
    ulong getMaximumOutputExtraIndex() const
    {
        ulong n = 0ul;
        for(ulong i = 0; i < m_outs_extra.size(); i++)
        {
            if(m_outs_extra[i]->f_extra > n)
                n = m_outs_extra[i]->f_extra;
        }
        return n;
    }
    
    ulong getMaximumSignalOutputExtraIndex() const
    {
        ulong n = 0ul;
        for(ulong i = 0; i < m_outs_extra_sig.size(); i++)
        {
            if(m_outs_extra_sig[i]->f_extra > n)
                n = m_outs_extra_sig[i]->f_extra;
        }
        return n;
    }
    
    void setNomalOutlet(t_outlet* outlet)
    {
        for(ulong i = 0; i < m_outs.size(); i++)
        {
            m_outs[i]->f_outlet = outlet;
        }
    }
    
    void setExtraOutlet(t_outlet* outlet, int index)
    {
        for(ulong i = 0; i < m_outs_extra.size(); i++)
        {
            if(m_outs_extra[i]->f_extra == index)
            {
                m_outs_extra[i]->f_outlet = outlet;
            }
        }
    }
    
    bool prepareDsp(t_sample* in, vector<t_sample*>& ixtra, t_sample* out, vector<t_sample*>& oxtra)
    {
        dsp_context_removecanvas(m_context);
        if(hasNormalSignalInputs())
        {
            if(!in) return true;
            for(size_t i = 0; i < m_ins_sig.size(); i++)
            {
                m_ins_sig[i]->f_signal = in;
            }
        }
        for(size_t i = 0; i < m_ins_extra_sig.size(); i++)
        {
            size_t index = m_ins_extra_sig[i]->f_extra-1;
            if(index >= ixtra.size()) return true;
            m_ins_extra_sig[i]->f_signal = ixtra[m_ins_extra_sig[i]->f_extra-1];
        }
        if(hasNormalSignalOutputs())
        {
            if(!out) return true;
            for(size_t i = 0; i < m_outs_sig.size(); i++)
            {
                m_outs_sig[i]->f_signal = out;
            }
        }
        for(size_t i = 0; i < m_outs_extra_sig.size(); i++)
        {
            size_t index = m_outs_extra_sig[i]->f_extra-1;
            if(index >= oxtra.size()) return true;
            m_outs_extra_sig[i]->f_signal = oxtra[m_outs_extra_sig[i]->f_extra-1];
        }
        
        dsp_context_addcanvas(m_context, m_canvas);
        dsp_context_compile(m_context);
        return false;
    }
};

typedef struct _hoa_process
{
    t_edspobj               f_obj;
    t_symbol*               f_domain;
    t_symbol*               f_dimension;
    vector<ProcessInstance*>f_instances;
    long                    f_target;
    vector<t_sample*>       f_outlets_signals;
    bool                    f_have_ins;
    
    static const long target_all  = -1;
    
} t_hoa_process;

static t_eclass *hoa_process_class;

void hoa_process_perform(t_hoa_process *x, t_object *dsp, float **inps, long ni, float **outs, long nouts, long sampleframe, long f,void *up)
{
    for(int i = 0; i < nouts; i++)
    {
        memcpy(outs[i], x->f_outlets_signals[i], sampleframe * sizeof(float));
        memset(x->f_outlets_signals[i], 0, sampleframe * sizeof(float));
    }
}


void hoa_process_dsp(t_hoa_process *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
#ifndef _LINUX
    signal_cleanup();
#endif
    vector<t_sample*> ins;
    vector<t_sample*> ixtra;
    vector<t_sample*> outs;
    vector<t_sample*> oxtra;
    bool have_sig_ins   = false;
    bool have_sig_outs  = false;
    ulong max_sig_ins_extra     = 0ul;
    ulong max_sig_outs_extra    = 0ul;
    
    for(ulong i = 0; i < x->f_instances.size(); i++)
    {
        have_sig_ins = max(have_sig_ins, x->f_instances[i]->hasNormalSignalInputs());
        have_sig_outs = max(have_sig_outs, x->f_instances[i]->hasNormalSignalOutputs());
        max_sig_ins_extra   = max(max_sig_ins_extra, x->f_instances[i]->getMaximumSignalInputExtraIndex());
        max_sig_outs_extra  = max(max_sig_outs_extra, x->f_instances[i]->getMaximumSignalOutputExtraIndex());
    }
    if(have_sig_ins)
    {
        for(ulong i = 0; i < x->f_instances.size(); i++)
        {
            ins.push_back(eobj_getsignalinput(x, i));
        }
        for(ulong i = 0; i < max_sig_ins_extra; i++)
        {
            ixtra.push_back(eobj_getsignalinput(x, i+x->f_instances.size()));
        }
    }
    else
    {
        for(ulong i = 0; i < max_sig_ins_extra; i++)
        {
            ixtra.push_back(eobj_getsignalinput(x, i));
        }
    }
    if(have_sig_outs)
    {
        for(ulong i = 0; i < x->f_instances.size(); i++)
        {
            outs.push_back(x->f_outlets_signals[i]);
        }
        for(ulong i = 0; i < max_sig_outs_extra; i++)
        {
            oxtra.push_back(x->f_outlets_signals[i+x->f_instances.size()]);
        }
    }
    else
    {
        for(ulong i = 0; i < max_sig_outs_extra; i++)
        {
            oxtra.push_back(x->f_outlets_signals[i]);
        }
    }
    
    for(size_t i = 0; i < x->f_instances.size(); i++)
    {
        if(x->f_instances[i]->prepareDsp(ins[i], ixtra, outs[i], oxtra))
        {
            pd_error(x, "hoa.process~ : Error while compiling the dsp chain.");
            return;
        }
    }
    object_method(dsp, gensym("dsp_add"), x, (method)hoa_process_perform, 0, NULL);
}

void hoa_process_click(t_hoa_process *x)
{
    if(!x->f_instances.empty())
        x->f_instances[0]->show();
}

static void hoa_process_open(t_hoa_process *x, t_symbol* s, int argc, t_atom* argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_SYM && atom_getsym(argv) == hoa_sym_all)
        {
            for(size_t i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->show();
            }
        }
        else if(x->f_dimension == hoa_sym_2d && x->f_domain == hoa_sym_harmonics)
        {
            if(argc > 1 && atom_gettype(argv) == A_FLOAT && atom_gettype(argv+1) == A_FLOAT)
            {
                x->f_instances[ulong(pd_clip_minmax(Harmonic<Hoa2d, t_sample>::getIndex(atom_getfloat(argv), atom_getfloat(argv+1)), 0, x->f_instances.size() - 1))]->show();
            }
            else if(atom_gettype(argv) == A_FLOAT)
            {
                 x->f_instances[ulong(pd_clip_minmax(Harmonic<Hoa2d, t_sample>::getIndex(abs(atom_getfloat(argv)), atom_getfloat(argv)), 0, x->f_instances.size() - 1))]->show();
            }
        }
        else if(x->f_dimension == hoa_sym_3d && x->f_domain == hoa_sym_harmonics)
        {
            if(argc > 1 && atom_gettype(argv) == A_FLOAT && atom_gettype(argv+1) == A_FLOAT)
            {
                x->f_instances[ulong(pd_clip_minmax(Harmonic<Hoa3d, t_sample>::getIndex(atom_getfloat(argv), atom_getfloat(argv+1)), 0, x->f_instances.size() - 1))]->show();
            }
        }
        else if(x->f_domain == hoa_sym_planewaves && atom_gettype(argv) == A_FLOAT)
        {
            x->f_instances[ulong(pd_clip_minmax(atom_getfloat(argv) - 1, 0, x->f_instances.size() - 1))]->show();
        }
    }
}

static void hoa_process_target(t_hoa_process *x, t_symbol* s, int argc, t_atom* argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_SYM && atom_getsym(argv) == hoa_sym_all)
        {
            x->f_target = _hoa_process::target_all;
        }
        else if(x->f_dimension == hoa_sym_2d && x->f_domain == hoa_sym_harmonics)
        {
            if(argc > 1 && atom_gettype(argv) == A_FLOAT && atom_gettype(argv+1) == A_FLOAT)
            {
                x->f_target = long(pd_clip_minmax(Harmonic<Hoa2d, t_sample>::getIndex(atom_getfloat(argv), atom_getfloat(argv+1)), 0, x->f_instances.size() - 1));
            }
            else if(atom_gettype(argv) == A_FLOAT)
            {
                x->f_target = long(pd_clip_minmax(Harmonic<Hoa2d, t_sample>::getIndex(abs(atom_getfloat(argv)), atom_getfloat(argv)), 0, x->f_instances.size() - 1));
            }
        }
        else if(x->f_dimension == hoa_sym_3d && x->f_domain == hoa_sym_harmonics)
        {
            if(argc > 1 && atom_gettype(argv) == A_FLOAT && atom_gettype(argv+1) == A_FLOAT)
            {
                x->f_target = long(pd_clip_minmax(Harmonic<Hoa3d, t_sample>::getIndex(atom_getfloat(argv), atom_getfloat(argv+1)), 0, x->f_instances.size() - 1));
            }
        }
        else if(x->f_domain == hoa_sym_planewaves && atom_gettype(argv) == A_FLOAT)
        {
            x->f_target = long(pd_clip_minmax(atom_getfloat(argv) - 1, 0, x->f_instances.size() - 1));
        }
    }
}

void hoa_process_bang(t_hoa_process *x)
{
    long index = eobj_getproxy(x);
    if(x->f_have_ins && index < x->f_instances.size())
    {
        x->f_instances[index]->sendBang();
    }
    else
    {
        long extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendBang(extra);
            }
        }
        else
        {
            x->f_instances[x->f_target]->sendBang(extra);
        }
    }
}

void hoa_process_float(t_hoa_process *x, float f)
{
    long index = eobj_getproxy(x);
    if(x->f_have_ins && index < x->f_instances.size())
    {
        x->f_instances[index]->sendFloat(f);
    }
    else
    {
        long extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendFloat(extra, f);
            }
        }
        else
        {
            x->f_instances[x->f_target]->sendFloat(extra, f);
        }
    }
}

void hoa_process_symbol(t_hoa_process *x, t_symbol* s)
{
    long index = eobj_getproxy(x);
    if(x->f_have_ins && index < x->f_instances.size())
    {
        x->f_instances[index]->sendSymbol(s);
    }
    else
    {
        long extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendSymbol(extra, s);
            }
        }
        else
        {
            x->f_instances[x->f_target]->sendSymbol(extra, s);
        }
    }
}

void hoa_process_list(t_hoa_process *x, t_symbol* s, int argc, t_atom* argv)
{
    long index = eobj_getproxy(x);
    if(x->f_have_ins && index < x->f_instances.size())
    {
        x->f_instances[index]->sendList(s, argc, argv);
    }
    else
    {
        long extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendList(extra, s, argc, argv);
            }
        }
        else
        {
            x->f_instances[x->f_target]->sendList(extra, s, argc, argv);
        }
    }
}

void hoa_process_anything(t_hoa_process *x, t_symbol* s, int argc, t_atom* argv)
{
    long index = eobj_getproxy(x);
    if(x->f_have_ins && index < x->f_instances.size())
    {
        x->f_instances[index]->sendAnything(s, argc, argv);
    }
    else
    {
        long extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendAnything(extra, s, argc, argv);
            }
        }
        else
        {
            x->f_instances[x->f_target]->sendAnything(extra, s, argc, argv);
        }
    }
}

static void hoa_process_free(t_hoa_process *x)
{
    int state = canvas_suspend_dsp();
    signal_cleanup();
    for(int i = 0 ; i < x->f_outlets_signals.size(); i++)
    {
        delete [] x->f_outlets_signals[i];
    }
    x->f_outlets_signals.clear();
    for(int i = 0; i < x->f_instances.size(); i++)
    {
        delete x->f_instances[i];
    }
    x->f_instances.clear();
    eobj_dspfree(x);
    canvas_fixlinesfor(eobj_getcanvas(x), (t_text *)x);
    canvas_resume_dsp(state);
}

static void *hoa_process_new(t_symbol *s, long argc, t_atom *argv)
{
    if(argc < 3 || atom_gettype(argv) != A_LONG || atom_gettype(argv+1) != A_SYM || atom_gettype(argv+2) != A_SYM)
    {
        error("%s needs at least 3 arguments : 1 integer for the order of decomposition or number of planewaves, 1 symbol for the patch and 1 symbol for the domain.", s->s_name);
        return NULL;
    }

    t_hoa_process *x = (t_hoa_process *)eobj_new(hoa_process_class);
    if(x)
    {
        char dirbuf[MAXPDSTRING], *nameptr;
        int state = canvas_suspend_dsp();
        x->f_target = _hoa_process::target_all;
        if(canvas_open(canvas_getcurrent(), atom_getsym(argv+1)->s_name, ".pd", dirbuf, &nameptr, MAXPDSTRING, 0) >= 0)
        {
            t_symbol* name = gensym(nameptr);
            t_symbol* dir  = gensym(dirbuf);
            long    narg = pd_clip_min(atoms_get_attributes_offset(argc - 3, argv + 3), 0);
            t_atom* args = argv + 3;
            long    natr = pd_clip_min(argc - narg - 3, 0);
            t_atom* atrs = argv + 3 + narg;
            
            if((s == hoa_sym_hoa_2d_process || s == hoa_sym_hoa_process) && atom_getsym(argv+2) == hoa_sym_harmonics)
            {
                x->f_domain     = hoa_sym_harmonics;
                x->f_dimension  = hoa_sym_2d;
                ulong order  = pd_clip_minmax(atom_getlong(argv), 1, 63);
                x->f_instances.resize(Harmonic<Hoa2d, t_sample>::getNumberOfHarmonics(order));
                
                for(ulong i = 0; i < x->f_instances.size(); i++)
                {
                    x->f_instances[i] = new (std::nothrow) ProcessInstance(eobj_getcanvas(x),
                                                            name,
                                                            dir,
                                                            hoa_sym_harmonics,
                                                            hoa_sym_2d,
                                                            order,
                                                            Harmonic<Hoa2d, t_sample>::getDegree(i),
                                                            Harmonic<Hoa2d, t_sample>::getOrder(i),
                                                            narg, args, natr, atrs);
                    if(!x->f_instances[i])
                    {
                        pd_error(x, "%s : Error while loading canvas.", s->s_name);
                        hoa_process_free(x);
                        return NULL;
                    }
                }
            }
            else if(s == hoa_sym_hoa_3d_process && atom_getsym(argv+2) == hoa_sym_harmonics)
            {
                x->f_domain    = hoa_sym_harmonics;
                x->f_dimension = hoa_sym_3d;
                ulong order  = pd_clip_minmax(atom_getlong(argv), 1, 10);
                x->f_instances.resize(Harmonic<Hoa3d, t_sample>::getNumberOfHarmonics(order));
                
                for(ulong i = 0; i < x->f_instances.size(); i++)
                {
                    x->f_instances[i] = new (std::nothrow) ProcessInstance(eobj_getcanvas(x),
                                                            name,
                                                            dir,
                                                            hoa_sym_harmonics,
                                                            hoa_sym_3d,
                                                            order,
                                                            Harmonic<Hoa3d, t_sample>::getDegree(i),
                                                            Harmonic<Hoa3d, t_sample>::getOrder(i),
                                                            narg, args, natr, atrs);
                    if(!x->f_instances[i])
                    {
                        pd_error(x, "%s : Error while loading canvas.", s->s_name);
                        hoa_process_free(x);
                        return NULL;
                    }
                }
            }
            else if((s == hoa_sym_hoa_2d_process || s == hoa_sym_hoa_process) && atom_getsym(argv+2) == hoa_sym_planewaves)
            {
                x->f_domain    = hoa_sym_planewaves;
                x->f_dimension = hoa_sym_2d;
                ulong argument  = pd_clip_minmax(atom_getlong(argv), 1, HOA_MAX_PLANEWAVES);
                
                for(ulong i = 0; i < x->f_instances.size(); i++)
                {
                    x->f_instances[i] = new (std::nothrow) ProcessInstance(eobj_getcanvas(x),
                                                            name,
                                                            dir,
                                                            hoa_sym_harmonics,
                                                            hoa_sym_2d,
                                                            argument,
                                                            i+1,
                                                            i+1,
                                                            narg, args, natr, atrs);
                    if(!x->f_instances[i])
                    {
                        pd_error(x, "%s : Error while loading canvas.", s->s_name);
                        hoa_process_free(x);
                        return NULL;
                    }
                }
            }
            else if(s == hoa_sym_hoa_3d_process && atom_getsym(argv+2) == hoa_sym_planewaves)
            {
                x->f_domain    = hoa_sym_planewaves;
                x->f_dimension = hoa_sym_3d;
                ulong argument = pd_clip_minmax(atom_getlong(argv), 1, HOA_MAX_PLANEWAVES);
                
                for(ulong i = 0; i < x->f_instances.size(); i++)
                {
                    x->f_instances[i] = new (std::nothrow) ProcessInstance(eobj_getcanvas(x),
                                                            name,
                                                            dir,
                                                            hoa_sym_harmonics,
                                                            hoa_sym_2d,
                                                            argument,
                                                            i+1,
                                                            i+1,
                                                            narg, args, natr, atrs);
                    if(!x->f_instances[i])
                    {
                        pd_error(x, "%s : Error while loading canvas.", s->s_name);
                        hoa_process_free(x);
                        return NULL;
                    }
                }
            }
            else
            {
                pd_error(x, "hoa.process~ : 3rd argument must \"harmonics\" or \"planewaves\".");
            }
        }
        else
        {
            pd_error(x, "hoa.process~ : error while loading canvas : %s.", atom_getsym(argv+1)->s_name);
        }
        
        bool have_ctl_ins   = false;
        bool have_sig_ins   = false;
        bool have_ctl_outs  = false;
        bool have_sig_outs  = false;
        ulong max_ctl_ins_extra     = 0ul;
        ulong max_sig_ins_extra     = 0ul;
        ulong max_ctl_outs_extra    = 0ul;
        ulong max_sig_outs_extra    = 0ul;
        
        for(ulong i = 0; i < x->f_instances.size(); i++)
        {
            have_ctl_ins = max(have_ctl_ins, x->f_instances[i]->hasNormalInputs());
            have_sig_ins = max(have_sig_ins, x->f_instances[i]->hasNormalSignalInputs());
            have_ctl_outs = max(have_ctl_outs, x->f_instances[i]->hasNormalOutputs());
            have_sig_outs = max(have_sig_outs, x->f_instances[i]->hasNormalSignalOutputs());
            max_ctl_ins_extra   = max(max_ctl_ins_extra, x->f_instances[i]->getMaximumInputExtraIndex());
            max_sig_ins_extra   = max(max_sig_ins_extra, x->f_instances[i]->getMaximumSignalInputExtraIndex());
            max_ctl_outs_extra  = max(max_ctl_outs_extra, x->f_instances[i]->getMaximumOutputExtraIndex());
            max_sig_outs_extra  = max(max_sig_outs_extra, x->f_instances[i]->getMaximumSignalOutputExtraIndex());
        }
        
        eobj_dspsetup(x,
                      have_sig_ins * x->f_instances.size() + max_sig_ins_extra,
                      have_sig_outs * x->f_instances.size() + max_sig_outs_extra);
        for(ulong i = 0; i < have_sig_outs * x->f_instances.size() + max_sig_outs_extra; i++)
        {
            x->f_outlets_signals.push_back(new t_sample[HOA_MAXBLKSIZE]);
        }
        
        if(have_ctl_ins && !have_sig_ins)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                eobj_proxynew(x);
            }
        }
        for(ulong i = max_sig_ins_extra; i < max_ctl_ins_extra; i++)
        {
            eobj_proxynew(x);
        }
        
        if(have_ctl_outs)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->setNomalOutlet(outlet_new((t_object *)x, &s_anything));
            }
        }
        
        for(ulong i = 0; i < max_ctl_outs_extra; i++)
        {
            t_outlet* outlet = outlet_new((t_object *)x, &s_anything);
            for(ulong j = 0; j < x->f_instances.size(); j++)
            {
                x->f_instances[i]->setExtraOutlet(outlet, i+1);
            }
        }
        x->f_have_ins = have_ctl_ins || have_sig_ins;
        canvas_resume_dsp(state);
    }

    return x;
}

extern "C" void setup_hoa0x2eprocess_tilde(void)
{
    t_eclass* c;
    c = eclass_new("hoa.process~", (method)hoa_process_new, (method)hoa_process_free, (short)sizeof(t_hoa_process), CLASS_NOINLET, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_process_new, gensym("hoa.2d.process~"), A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_process_new, gensym("hoa.3d.process~"), A_GIMME, 0);

    eclass_dspinit(c);
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_process_dsp,        "dsp",      A_CANT, 0);
    eclass_addmethod(c, (method)hoa_process_click,      "click",    A_NULL, 0);
    eclass_addmethod(c, (method)hoa_process_open,       "open",     A_GIMME, 0);
    eclass_addmethod(c, (method)hoa_process_target,     "target",   A_GIMME, 0);

    eclass_addmethod(c, (method)hoa_process_bang,       "bang",     A_CANT,  0);
    eclass_addmethod(c, (method)hoa_process_float,      "float",    A_FLOAT, 0);
    eclass_addmethod(c, (method)hoa_process_symbol,     "symbol",   A_SYMBOL,0);
    eclass_addmethod(c, (method)hoa_process_list,       "list",     A_GIMME, 0);
    eclass_addmethod(c, (method)hoa_process_anything,   "anything", A_GIMME, 0);

    eclass_register(CLASS_OBJ, c);
    hoa_process_class = c;
}

