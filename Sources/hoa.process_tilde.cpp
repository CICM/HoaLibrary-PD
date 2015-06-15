/*
// Copyright (c) 2012-2015 Eliott Paris, Julien Colafrancesco, Thomas Le Meur & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.library.hpp"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

class ProcessInstance
{
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
    
    void getThisProcess(t_canvas* canvas)
    {
        for(t_gobj *y = canvas->gl_list; y; y = y->g_next)
        {
            if(eobj_getclassname(y) == hoa_sym_canvas)
            {
                getThisProcess((t_canvas *)y);
            }
            else if(eobj_getclassname(y) == hoa_sym_hoa_thisprocess)
            {
                m_thisprocesses.push_back((t_hoa_thisprocess*)y);
            }
        }
    }
    
    void getIos(t_canvas* canvas)
    {
        for(t_gobj *y = canvas->gl_list; y; y = y->g_next)
        {
            if(eobj_getclassname(y) == hoa_sym_canvas)
            {
                getIos((t_canvas *)y);
            }
            else if(eobj_getclassname(y) == hoa_sym_hoa_thisprocess)
            {
                m_thisprocesses.push_back((t_hoa_thisprocess*)y);
            }
            else if(eobj_getclassname(y) == hoa_sym_hoa_in)
            {
                t_hoa_in* inlet = (t_hoa_in *)y;
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
                t_hoa_out_tilde* outlet_sig = (t_hoa_out_tilde *)y;
                if(outlet_sig->f_extra)
                    m_outs_extra_sig.push_back(outlet_sig);
                else
                    m_outs_sig.push_back(outlet_sig);
            }
        }
    }
    
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
    
public:
    ProcessInstance(t_canvas* parent,
                    t_symbol* name,
                    t_symbol* domain,
                    t_symbol* dimension,
                    long ac1, long ac2, long ac3,
                    long argc,
                    t_atom* argv,
                    long nattr,
                    t_atom* attrs)
    {
        m_canvas = NULL;
        t_atom av[6];
        atom_setlong(av, 1);
        atom_setlong(av+1, 1);
        atom_setsym(av+2, name);
        atom_setfloat(av+3, ac1);
        atom_setfloat(av+4, ac2);
        atom_setfloat(av+5, ac3);
        pd_typedmess((t_pd *)parent, hoa_sym_obj, domain == hoa_sym_harmonics ? 6 : 5, av);
        for(t_gobj* z = parent->gl_list; z; z = z->g_next)
        {
            if(eobj_getclassname(z) == hoa_sym_canvas)
            {
                t_binbuf* b = ((t_canvas *)z)->gl_obj.te_binbuf;
                if(b)
                {
                    int natom      = binbuf_getnatom(b);
                    t_atom* vec  = binbuf_getvec(b);
                    if(natom > (domain == hoa_sym_harmonics ? 3 : 2) && vec)
                    {
                        if(atom_getsym(vec) == name &&
                           atom_getlong(vec+1) == ac1 &&
                           atom_getlong(vec+2) == ac2 &&
                           (domain == hoa_sym_planewaves || atom_getlong(vec+3) == ac3))
                        {
                            m_canvas = (t_canvas *)z;
                        }
                    }
                }
            }
        }
        if(m_canvas)
        {
            getThisProcess(m_canvas);
            for(size_t i = 0; i < m_thisprocesses.size(); i++)
            {
                atom_setsym(m_thisprocesses[i]->f_hoa_mode, dimension);
                atom_setsym(m_thisprocesses[i]->f_hoa_mode+1, domain);
                atom_setfloat(m_thisprocesses[i]->f_hoa_args, ac1);
                atom_setfloat(m_thisprocesses[i]->f_hoa_args+1, ac2);
                atom_setfloat(m_thisprocesses[i]->f_hoa_args+2, ac3);
                thisprocess_init(m_thisprocesses[i], argc, argv, nattr, attrs);
            }
            canvas_loadbang(m_canvas);
            getIos(m_canvas);
        }
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
    }
    
    inline void show() const noexcept
    {
        if(m_canvas)
            canvas_vis(m_canvas, 1);
    }
    
    inline void sendBang() const noexcept
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_bang((t_pd *)m_ins[i]);
        }
    }
    
    inline void sendBang(ulong extra) const noexcept
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(ulong(m_ins_extra[i]->f_extra) == extra)
            {
                pd_bang((t_pd *)m_ins_extra[i]);
            }
        }
    }
    
    inline void sendFloat(const float f) const noexcept
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_float((t_pd *)m_ins[i], f);
        }
    }
    
    inline void sendFloat(ulong extra, const float f) const noexcept
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(ulong(m_ins_extra[i]->f_extra) == extra)
            {
                pd_float((t_pd *)m_ins_extra[i], f);
            }
        }
    }
    
    inline void sendSymbol(t_symbol* s) const noexcept
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_symbol((t_pd *)m_ins[i], s);
        }
    }
    
    inline void sendSymbol(ulong extra, t_symbol* s) const noexcept
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(ulong(m_ins_extra[i]->f_extra) == extra)
            {
                pd_symbol((t_pd *)m_ins_extra[i], s);
            }
        }
    }
    
    inline void sendList(t_symbol* s, int argc, t_atom* argv) const noexcept
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_list((t_pd *)m_ins[i], s, argc, argv);
        }
    }
    
    inline void sendList(ulong extra, t_symbol* s, int argc, t_atom* argv) const noexcept
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(ulong(m_ins_extra[i]->f_extra) == extra)
            {
                pd_list((t_pd *)m_ins_extra[i], s, argc, argv);
            }
        }
    }
    
    inline void sendAnything(t_symbol* s, int argc, t_atom* argv) const noexcept
    {
        for(ulong i = 0; i < m_ins.size(); i++)
        {
            pd_typedmess((t_pd *)m_ins[i], s, argc, argv);
        }
    }
    
    inline void sendAnything(ulong extra, t_symbol* s, int argc, t_atom* argv) const noexcept
    {
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(ulong(m_ins_extra[i]->f_extra) == extra)
            {
                pd_typedmess((t_pd *)m_ins_extra[i], s, argc, argv);
            }
        }
    }
    
    inline bool hasNormalInputs() const
    {
        return !m_ins.empty();
    }
    
    inline bool hasNormalSignalInputs() const
    {
        return !m_ins_sig.empty();
    }
    
    inline ulong getMaximumInputExtraIndex() const
    {
        ulong n = 0ul;
        for(ulong i = 0; i < m_ins_extra.size(); i++)
        {
            if(ulong(m_ins_extra[i]->f_extra) > n)
                n = ulong(m_ins_extra[i]->f_extra);
        }
        return n;
    }
    
    inline ulong getMaximumSignalInputExtraIndex() const
    {
        ulong n = 0ul;
        for(ulong i = 0; i < m_ins_extra_sig.size(); i++)
        {
            if(ulong(m_ins_extra_sig[i]->f_extra) > n)
                n = ulong(m_ins_extra_sig[i]->f_extra);
        }
        return n;
    }
    
    inline bool hasNormalOutputs() const
    {
        return !m_outs.empty();
    }
    
    inline bool hasNormalSignalOutputs() const
    {
        return !m_outs_sig.empty();
    }
    
    inline ulong getMaximumOutputExtraIndex() const
    {
        ulong n = 0ul;
        for(ulong i = 0; i < m_outs_extra.size(); i++)
        {
            if(ulong(m_outs_extra[i]->f_extra) > n)
                n = ulong(m_outs_extra[i]->f_extra);
        }
        return n;
    }
    
    inline ulong getMaximumSignalOutputExtraIndex() const
    {
        ulong n = 0ul;
        for(ulong i = 0; i < m_outs_extra_sig.size(); i++)
        {
            if(ulong(m_outs_extra_sig[i]->f_extra) > n)
                n = ulong(m_outs_extra_sig[i]->f_extra);
        }
        return n;
    }
    
    inline void setNomalOutlet(t_outlet* outlet)
    {
        for(ulong i = 0; i < m_outs.size(); i++)
        {
            m_outs[i]->f_outlet = outlet;
        }
    }
    
    inline void setExtraOutlet(t_outlet* outlet, ulong index)
    {
        for(ulong i = 0; i < m_outs_extra.size(); i++)
        {
            if(ulong(m_outs_extra[i]->f_extra) == index)
            {
                m_outs_extra[i]->f_outlet = outlet;
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
            for(size_t i = 0; i < m_ins_sig.size(); i++)
            {
                m_ins_sig[i]->f_signal = in;
            }
        }
        for(size_t i = 0; i < m_ins_extra_sig.size(); i++)
        {
            if(ulong(m_ins_extra_sig[i]->f_extra) > ixtra.size() || !ixtra[size_t(m_ins_extra_sig[i]->f_extra-1)])
            {
                bug("process don't have input signal extra %i.", m_ins_extra_sig[i]->f_extra);
                return true;
            }
            m_ins_extra_sig[i]->f_signal = ixtra[size_t(m_ins_extra_sig[i]->f_extra-1)];
        }
        if(hasNormalSignalOutputs())
        {
            if(!out)
            {
                bug("process don't have output signal.");
                return true;
            }
            for(size_t i = 0; i < m_outs_sig.size(); i++)
            {
                m_outs_sig[i]->f_signal = out;
            }
        }
        for(size_t i = 0; i < m_outs_extra_sig.size(); i++)
        {
            if(ulong(m_outs_extra_sig[i]->f_extra) > oxtra.size() || !oxtra[size_t(m_outs_extra_sig[i]->f_extra-1)])
            {
                bug("process don't have output signal extra %i.", m_outs_extra_sig[i]->f_extra);
                return true;
            }
            m_outs_extra_sig[i]->f_signal = oxtra[size_t(m_outs_extra_sig[i]->f_extra-1)];
        }
        return false;
    }
};

typedef struct _hoa_process
{
    t_edspobj               f_obj;
    t_canvas*               f_global;
    t_object*               f_switch;
    t_symbol*               f_domain;
    t_symbol*               f_dimension;
    vector<ProcessInstance*>f_instances;
    long                    f_target;
    vector<t_sample*>       f_outlets_signals;
    bool                    f_have_ins;
    
    static const long target_all  = -1;
    
} t_hoa_process;

static t_eclass *hoa_process_class;

static void hoa_process_perform(t_hoa_process *x, t_object *dsp, float **inps, long ni, float **outs, long nouts, long sampleframe, long f,void *up)
{
    pd_bang((t_pd *)x->f_switch);
    for(int i = 0; i < nouts; i++)
    {
        memcpy(outs[i], x->f_outlets_signals[size_t(i)], size_t(sampleframe) * sizeof(t_sample));
        memset(x->f_outlets_signals[size_t(i)], 0, size_t(sampleframe) * sizeof(t_sample));
    }
}


static void hoa_process_dsp(t_hoa_process *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    if(!x->f_global && !x->f_switch)
    {
        pd_error(x, "process~ not initialized can't compile DSP chain.");
        return;
    }
    vector<t_sample*> ins;
    vector<t_sample*> ixtra;
    vector<t_sample*> outs;
    vector<t_sample*> oxtra;
    bool have_sig_ins   = false;
    bool have_sig_outs  = false;
    ulong max_sig_ins_extra     = 0ul;
    ulong max_sig_outs_extra    = 0ul;
    
    for(ulong i = 0; i < x->f_outlets_signals.size(); i++)
    {
        memset(x->f_outlets_signals[i], 0, HOA_MAXBLKSIZE * sizeof(t_sample));
    }
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
            ins.push_back(eobj_getsignalinput(x, long(i)));
        }
        for(ulong i = 0; i < max_sig_ins_extra; i++)
        {
            ixtra.push_back(eobj_getsignalinput(x, long(i+x->f_instances.size())));
        }
    }
    else
    {
        for(ulong i = 0; i < x->f_instances.size(); i++)
        {
            ins.push_back(NULL);
        }
        for(ulong i = 0; i < max_sig_ins_extra; i++)
        {
            ixtra.push_back(eobj_getsignalinput(x, long(i)));
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
        for(ulong i = 0; i < x->f_instances.size(); i++)
        {
            outs.push_back(NULL);
        }
        for(ulong i = 0; i < max_sig_outs_extra; i++)
        {
            oxtra.push_back(x->f_outlets_signals[i]);
        }
    }
    for(ulong i = 0; i < x->f_instances.size(); i++)
    {
        if(!x->f_instances[i] || x->f_instances[i]->prepareDsp(ins[i], ixtra, outs[i], oxtra))
        {
            pd_error(x, "hoa.process~ : Error while compiling the dsp chain.");
            return;
        }
    }
    mess0((t_pd *)x->f_global, gensym("dsp"));
    object_method(dsp, gensym("dsp_add"), x, (method)hoa_process_perform, 0, NULL);
}

static void hoa_process_click(t_hoa_process *x)
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

static void hoa_process_bang(t_hoa_process *x)
{
    ulong index = ulong(eobj_getproxy(x));
    if(x->f_have_ins && ulong(index) < x->f_instances.size())
    {
        x->f_instances[index]->sendBang();
    }
    else
    {
        ulong extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendBang(extra);
            }
        }
        else
        {
            x->f_instances[ulong(x->f_target)]->sendBang(extra);
        }
    }
}

static void hoa_process_float(t_hoa_process *x, float f)
{
    ulong index = ulong(eobj_getproxy(x));
    if(x->f_have_ins && ulong(index) < x->f_instances.size())
    {
        x->f_instances[index]->sendFloat(f);
    }
    else
    {
        ulong extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendFloat(extra, f);
            }
        }
        else
        {
            x->f_instances[ulong(x->f_target)]->sendFloat(extra, f);
        }
    }
}

static void hoa_process_symbol(t_hoa_process *x, t_symbol* s)
{
    ulong index = ulong(eobj_getproxy(x));
    if(x->f_have_ins && ulong(index) < x->f_instances.size())
    {
        x->f_instances[index]->sendSymbol(s);
    }
    else
    {
        ulong extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendSymbol(extra, s);
            }
        }
        else
        {
            x->f_instances[ulong(x->f_target)]->sendSymbol(extra, s);
        }
    }
}

static void hoa_process_list(t_hoa_process *x, t_symbol* s, int argc, t_atom* argv)
{
    ulong index = ulong(eobj_getproxy(x));
    if(x->f_have_ins && ulong(index) < x->f_instances.size())
    {
        x->f_instances[index]->sendList(s, argc, argv);
    }
    else
    {
        ulong extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendList(extra, s, argc, argv);
            }
        }
        else
        {
            x->f_instances[ulong(x->f_target)]->sendList(extra, s, argc, argv);
        }
    }
}

static void hoa_process_anything(t_hoa_process *x, t_symbol* s, int argc, t_atom* argv)
{
    ulong index = ulong(eobj_getproxy(x));
    if(x->f_have_ins && ulong(index) < x->f_instances.size())
    {
        x->f_instances[index]->sendAnything(s, argc, argv);
    }
    else
    {
        ulong extra = x->f_have_ins ? index - x->f_instances.size() + 1 : index + 1;
        if(x->f_target == _hoa_process::target_all)
        {
            for(ulong i = 0; i < x->f_instances.size(); i++)
            {
                x->f_instances[i]->sendAnything(extra, s, argc, argv);
            }
        }
        else
        {
            x->f_instances[ulong(x->f_target)]->sendAnything(extra, s, argc, argv);
        }
    }
}

static void hoa_process_free(t_hoa_process *x)
{
    int state = canvas_suspend_dsp();
    for(ulong i = 0 ; i < x->f_outlets_signals.size(); i++)
    {
        if(x->f_outlets_signals[i])
        {
            delete [] x->f_outlets_signals[i];
        }
    }
    x->f_outlets_signals.clear();
    for(ulong i = 0; i < x->f_instances.size(); i++)
    {
        if(x->f_instances[i])
        {
           delete x->f_instances[i];
        }
    }
    if(x->f_global && x->f_switch)
    {
        //canvas_free(x->f_global);
    }
    x->f_instances.clear();
    eobj_dspfree(x);
    canvas_resume_dsp(state);
}

static void *hoa_process_new(t_symbol *s, int argc, t_atom *argv)
{
    if(argc < 2 || atom_gettype(argv) != A_LONG || atom_gettype(argv+1) != A_SYM)
    {
        error("%s needs at least 2 arguments : 1 integer for the order of decomposition or number of planewaves, 1 symbol for the patch.", s->s_name);
        return NULL;
    }
    t_hoa_process *x = (t_hoa_process *)eobj_new(hoa_process_class);
    if(x)
    {
        x->f_global = NULL;
        x->f_switch = NULL;
        x->f_target = _hoa_process::target_all;
        t_pd *was = s__X.s_thing;
        x->f_global = canvas_new(NULL, gensym(""), 0, NULL);
        s__X.s_thing = was;
        if(x->f_global)
        {
            canvas_vis(x->f_global, 0);
            t_atom av[3];
            atom_setlong(av, 10); atom_setlong(av+1, 10);atom_setsym(av+2, gensym("switch~"));
            pd_typedmess((t_pd *)x->f_global, gensym("obj"), 3, av);
            if(x->f_global->gl_list->g_pd->c_name == gensym("block~"))
            {
                x->f_switch = (t_object *)x->f_global->gl_list;
            }
            if(x->f_switch)
            {
                long    narg = pd_clip_min(atoms_get_attributes_offset(argc - 3, argv + 3), 0);
                t_atom* args = argv + 3;
                long    natr = pd_clip_min(argc - narg - 3, 0);
                t_atom* atrs = argv + 3 + narg;
                if((s == hoa_sym_hoa_2d_process || s == hoa_sym_hoa_process) && atom_getsym(argv+2) != hoa_sym_planewaves)
                {
                    x->f_domain     = hoa_sym_harmonics;
                    x->f_dimension  = hoa_sym_2d;
                    ulong order  = pd_clip_minmax(atom_getlong(argv), 1, 63);
                    x->f_instances.resize(Harmonic<Hoa2d, t_sample>::getNumberOfHarmonics(order));
                    
                    for(ulong i = 0; i < x->f_instances.size(); i++)
                    {
                        x->f_instances[i] = new (std::nothrow) ProcessInstance(x->f_global,
                                                                               atom_getsym(argv+1),
                                                                               hoa_sym_harmonics,
                                                                               hoa_sym_2d,
                                                                               long(order),
                                                                               long(Harmonic<Hoa2d, t_sample>::getDegree(i)),
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
                else if(s == hoa_sym_hoa_3d_process && atom_getsym(argv+2) != hoa_sym_planewaves)
                {
                    x->f_domain    = hoa_sym_harmonics;
                    x->f_dimension = hoa_sym_3d;
                    ulong order  = pd_clip_minmax(atom_getlong(argv), 1, 10);
                    x->f_instances.resize(Harmonic<Hoa3d, t_sample>::getNumberOfHarmonics(order));
                    
                    for(ulong i = 0; i < x->f_instances.size(); i++)
                    {
                        x->f_instances[i] = new (std::nothrow) ProcessInstance(x->f_global,
                                                                               atom_getsym(argv+1),
                                                                               hoa_sym_harmonics,
                                                                               hoa_sym_3d,
                                                                               long(order),
                                                                               long(Harmonic<Hoa3d, t_sample>::getDegree(i)),
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
                    x->f_instances.resize(argument);
                    
                    for(ulong i = 0; i < x->f_instances.size(); i++)
                    {
                        x->f_instances[i] = new (std::nothrow) ProcessInstance(x->f_global,
                                                                               atom_getsym(argv+1),
                                                                               hoa_sym_planewaves,
                                                                               hoa_sym_2d,
                                                                               long(argument),
                                                                               long(i+1),
                                                                               long(i+1),
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
                    x->f_instances.resize(argument);
                    
                    for(ulong i = 0; i < x->f_instances.size(); i++)
                    {
                        x->f_instances[i] = new (std::nothrow) ProcessInstance(x->f_global,
                                                                               atom_getsym(argv+1),
                                                                               hoa_sym_planewaves,
                                                                               hoa_sym_3d,
                                                                               long(argument),
                                                                               long(i+1),
                                                                               long(i+1),
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
                    if(x->f_instances[i])
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
                }
                
                eobj_dspsetup(x,
                              long(have_sig_ins * x->f_instances.size() + max_sig_ins_extra),
                              long(have_sig_outs * x->f_instances.size() + max_sig_outs_extra));
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
                        if(x->f_instances[i])
                        {
                            x->f_instances[i]->setNomalOutlet(outlet_new((t_object *)x, &s_anything));
                        }
                    }
                }
                
                for(ulong i = 0; i < max_ctl_outs_extra; i++)
                {
                    t_outlet* outlet = outlet_new((t_object *)x, &s_anything);
                    for(ulong j = 0; j < x->f_instances.size(); j++)
                    {
                        if(x->f_instances[j])
                        {
                            x->f_instances[j]->setExtraOutlet(outlet, i+1);
                        }
                    }
                }
                x->f_have_ins = have_ctl_ins || have_sig_ins;
            }
        }
        else
        {
            x->f_global = NULL;
            x->f_switch = NULL;
        }
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
    
    eclass_addmethod(c, (method)hoa_process_dsp,        "dsp",      A_CANT, 0);
    eclass_addmethod(c, (method)hoa_process_click,      "click",    A_NULL, 0);
    eclass_addmethod(c, (method)hoa_process_open,       "open",     A_GIMME, 0);
    eclass_addmethod(c, (method)hoa_process_target,     "target",   A_GIMME, 0);

    eclass_addmethod(c, (method)hoa_process_bang,       "bang",     A_CANT,  0);
    eclass_addmethod(c, (method)hoa_process_float,      "float",    A_FLOAT, 0);
    eclass_addmethod(c, (method)hoa_process_symbol,     "symbol",   A_SYMBOL,0);
    eclass_addmethod(c, (method)hoa_process_list,       "list",     A_GIMME, 0);
    eclass_addmethod(c, (method)hoa_process_anything,   "anything", A_GIMME, 0);

    
    hoa_process_class = c;
}

