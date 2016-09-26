/*
// Copyright (c) 2012-2016 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "../hoa.pd.h"
#include <g_canvas.h>
#include <m_imp.h>
#include <string.h>

typedef struct _hoa_process_master
{
    t_canvas*               canvas;
    t_object*               block;
    t_bangmethod            method;
    t_hoa_process_instance* instances;
    size_t                  ninstances;
} t_hoa_process_master;

typedef struct _hoa_process
{
    t_hoa_processor         f_obj;
    t_float                 f_f;
    size_t                  f_n;
    t_symbol*               f_domain;
    t_hoa_process_master    f_master;
} t_hoa_2d_process_tilde;

static t_class*     hoa_2d_process_tilde_class;
static t_symbol*    hoa_sym_switch;
static t_symbol*    hoa_sym_block;
static t_symbol*    hoa_sym_obj;
static t_symbol*    hoa_sym_harmonics;
static t_symbol*    hoa_sym_planewaves;
static t_symbol*    hoa_sym_2d;


static void hoa_2d_process_tilde_perform(t_hoa_2d_process_tilde *x, size_t sampleframes,
                                size_t nins, t_sample **ins,
                                size_t nouts, t_sample **outs)
{
    size_t i;
    x->f_master.method((t_pd *)x->f_master.block);
    for(i = 0; i < nouts; i++)
    {
        int todo;
        //memcpy(outs[i], x->f_outlets_signals[size_t(i)], size_t(sampleframe) * sizeof(t_sample));
        //memset(x->f_outlets_signals[size_t(i)], 0, size_t(sampleframe) * sizeof(t_sample));
    }
}

/*
static void hoa_2d_process_tilde_dsp(t_hoa_2d_process_tilde *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
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
    object_method(dsp, gensym("dsp_add"), x, (t_method)hoa_2d_process_tilde_perform, 0, NULL);
}

static void hoa_2d_process_tilde_click(t_hoa_2d_process_tilde *x)
{
    if(x->f_instances)
    {
        hoa_2d_process_tilde_instance_show(x->f_instances);
    }
}

static void hoa_2d_process_tilde_open(t_hoa_2d_process_tilde *x, t_symbol* s, int argc, t_atom* argv)
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

static void hoa_2d_process_tilde_target(t_hoa_2d_process_tilde *x, t_symbol* s, int argc, t_atom* argv)
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

static void hoa_2d_process_tilde_bang(t_hoa_2d_process_tilde *x)
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

static void hoa_2d_process_tilde_float(t_hoa_2d_process_tilde *x, float f)
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

static void hoa_2d_process_tilde_symbol(t_hoa_2d_process_tilde *x, t_symbol* s)
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

static void hoa_2d_process_tilde_list(t_hoa_2d_process_tilde *x, t_symbol* s, int argc, t_atom* argv)
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

static void hoa_2d_process_tilde_anything(t_hoa_2d_process_tilde *x, t_symbol* s, int argc, t_atom* argv)
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
 */

static char hoa_process_master_init(t_hoa_process_master* x)
{
    t_atom av[3];
    x->block        = (t_object *)NULL;
    x->method       = (t_bangmethod)NULL;
    x->instances    = (t_hoa_process_instance *)NULL;
    x->canvas       = canvas_new(NULL, gensym(""), 0, NULL);
    
    if(x->canvas)
    {
        pd_popsym((t_pd *)x->canvas);
        canvas_vis(x->canvas, 0);
        SETFLOAT(av, 10.f);
        SETFLOAT(av+1, 10.f);
        SETSYMBOL(av+2, hoa_sym_switch);
        
        pd_typedmess((t_pd *)x->canvas, hoa_sym_obj, 3, av);
        if(x->canvas->gl_list->g_pd->c_name == hoa_sym_block)
        {
            x->block = (t_object *)x->canvas->gl_list;
            x->method = x->block->te_g.g_pd->c_bangmethod;
            return x->method != NULL;
        }
    }
    return 0;
}

static void hoa_process_master_free(t_hoa_process_master* x)
{
    if(x->canvas)
    {
        canvas_free(x->canvas);
    }
    if(x->instances && x->ninstances)
    {
        freebytes(x->instances, x->ninstances * sizeof(t_hoa_process_instance));
        x->instances = NULL;
    }
}

static char hoa_2d_process_master_load_planewaves(t_hoa_process_master* x, size_t nplws, t_symbol* name, int argc, t_atom *argv)
{
    int ac;
    t_atom* av;
    size_t i;
    x->instances    = (t_hoa_process_instance *)getbytes(nplws * sizeof(t_hoa_process_instance));
    if(x->instances)
    {
        x->ninstances = nplws;
        ac = 4 + argc;
        av = (t_atom *)getbytes(ac * sizeof(t_atom));
        if(ac && av)
        {
            SETSYMBOL(av, hoa_sym_2d);
            SETSYMBOL(av+1, hoa_sym_planewaves);
            SETFLOAT(av+2, (float)nplws);
            memcpy(av+4, argv, argc * sizeof(t_atom));
            for(i = 0; i < x->ninstances; ++i)
            {
                SETFLOAT(av+3, (float)i);
                if(!hoa_process_instance_init(x->instances+i, x->canvas, name, ac, av))
                {
                    freebytes(av, ac * sizeof(t_atom));
                    freebytes(x->instances, nplws * sizeof(t_hoa_process_instance));
                    x->instances = NULL;
                    return 0;
                }
            }
            freebytes(av, ac * sizeof(t_atom));
            return 1;
        }
        freebytes(x->instances, nplws * sizeof(t_hoa_process_instance));
        x->instances = NULL;
        return 0;
    }
    return 0;
}

static char hoa_2d_process_master_load_harmonics(t_hoa_process_master* x, size_t order, t_symbol* name, int argc, t_atom *argv)
{
    int ac;
    t_atom* av;
    size_t i;
    x->instances    = (t_hoa_process_instance *)getbytes((order * 2 + 1) * sizeof(t_hoa_process_instance));
    if(x->instances)
    {
        x->ninstances = order * 2 + 1;
        ac = 5 + argc;
        av = (t_atom *)getbytes(ac * sizeof(t_atom));
        if(ac && av)
        {
            SETSYMBOL(av, hoa_sym_2d);
            SETSYMBOL(av+1, hoa_sym_harmonics);
            SETFLOAT(av+2, (float)order);
            memcpy(av+5, argv, argc * sizeof(t_atom));
            for(i = 0; i < x->ninstances; ++i)
            {
                SETFLOAT(av+3, (float)hoa_2d_get_degree(i));
                SETFLOAT(av+4, (float)hoa_2d_get_azimuthal_order(i));
                if(!hoa_process_instance_init(x->instances+i, x->canvas, name, ac, av))
                {
                    freebytes(av, ac * sizeof(t_atom));
                    freebytes(x->instances, order * 2 + 1 * sizeof(t_hoa_process_instance));
                    x->instances = NULL;
                    return 0;
                }
            }
            freebytes(av, ac * sizeof(t_atom));
            return 1;
        }
        freebytes(x->instances, (order * 2 + 1) * sizeof(t_hoa_process_instance));
        x->instances = NULL;
        return 0;
    }
    return 0;
}

static void hoa_2d_process_tilde_free(t_hoa_2d_process_tilde *x)
{
    hoa_process_master_free(&x->f_master);
}

static void *hoa_2d_process_tilde_new(t_symbol *s, int argc, t_atom *argv)
{
    size_t order, nplws;
    t_symbol *patch, *domain;
    
    t_canvas* current = canvas_getcurrent();
    t_hoa_2d_process_tilde *x = (t_hoa_2d_process_tilde *)pd_new(hoa_2d_process_tilde_class);
    if(x)
    {
        patch   = atom_getsymbolarg(1, argc, argv);
        domain  = atom_getsymbolarg(2, argc, argv);
        if(hoa_process_master_init(&x->f_master))
        {
            if(patch)
            {
                if(domain == hoa_sym_harmonics)
                {
                    order = hoa_processor_clip_order(x, (size_t)atom_getfloatarg(0, argc, argv));
                    if(!hoa_2d_process_master_load_harmonics(&x->f_master, order, patch, argc < 3 ? 0 : argc-3, argv+3))
                    {
                        pd_error(x, "hoa.2d.process~: can't load the patch %s.pd.", patch->s_name);
                    }
                    x->f_domain = hoa_sym_harmonics;
                }
                else
                {
                    nplws = hoa_processor_clip_nplanewaves(x, (size_t)atom_getfloatarg(0, argc, argv));
                    if(!hoa_2d_process_master_load_planewaves(&x->f_master, nplws, patch, argc < 3 ? 0 : argc-3, argv+3))
                    {
                        pd_error(x, "hoa.2d.process~: can't load the patch %s.pd.", patch->s_name);
                    }
                    if(domain != hoa_sym_planewaves)
                    {
                        pd_error(x, "hoa.2d.process~: bad argument, third argument must harmonics or planewaves.");
                    }
                    x->f_domain = hoa_sym_planewaves;
                }
            }
            else
            {
                pd_error(x, "hoa.2d.process~: bad argument, second argument must be a patch name.");
            }
        }
    }
    canvas_setcurrent(current);
    return x;
}


extern void setup_hoa0x2e2d0x2eprocess_tilde(void)
{
    t_class* c = class_new(gensym("hoa.2d.process~"), (t_newmethod)hoa_2d_process_tilde_new, (t_method)hoa_2d_process_tilde_free,
                           (size_t)sizeof(t_hoa_2d_process_tilde), CLASS_DEFAULT, A_GIMME, 0);
    
    class_addcreator((t_newmethod)hoa_2d_process_tilde_new, gensym("hoa.process~"), A_GIMME, 0);

    CLASS_MAINSIGNALIN(c, t_hoa_2d_process_tilde, f_f);
    /*
    class_addmethod(c, (t_method)hoa_2d_process_tilde_dsp,        "dsp",      A_CANT, 0);
    class_addmethod(c, (t_method)hoa_2d_process_tilde_click,      "click",    A_NULL, 0);
    class_addmethod(c, (t_method)hoa_2d_process_tilde_open,       "open",     A_GIMME, 0);
    class_addmethod(c, (t_method)hoa_2d_process_tilde_target,     "target",   A_GIMME, 0);

    class_addmethod(c, (t_method)hoa_2d_process_tilde_bang,       "bang",     A_CANT,  0);
    class_addmethod(c, (t_method)hoa_2d_process_tilde_float,      "float",    A_FLOAT, 0);
    class_addmethod(c, (t_method)hoa_2d_process_tilde_symbol,     "symbol",   A_SYMBOL,0);
    class_addmethod(c, (t_method)hoa_2d_process_tilde_list,       "list",     A_GIMME, 0);
    class_addmethod(c, (t_method)hoa_2d_process_tilde_anything,   "anything", A_GIMME, 0);
     */

    hoa_2d_process_tilde_class = c;
    
    hoa_sym_switch      = gensym("switch~");
    hoa_sym_block       = gensym("block~");
    hoa_sym_obj         = gensym("obj");
    hoa_sym_harmonics   = gensym("harmonics");
    hoa_sym_planewaves  = gensym("planewaves");
    hoa_sym_2d          = gensym("2d");

    hoa_process_instance_setup();
}

