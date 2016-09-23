/*
// Copyright (c) 2012-2015 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "hoa.pd.h"
#include <Hoa.hpp>

typedef struct _hoa_2d_decoder
{
    t_hoa_processor  f_obj;
    float            f_f;
    hoa::Decoder<hoa::Hoa2d, t_sample>* f_processor;
    t_sample*        f_signals;
    t_symbol*        f_mode;
} t_hoa_2d_decoder;

static t_class *hoa_2d_decoder_class;
static t_symbol* hoa_sym_regular;
static t_symbol* hoa_sym_irregular;
static t_symbol* hoa_sym_binaural;

static void *hoa_2d_decoder_new(t_float f)
{
    t_hoa_2d_decoder *x = (t_hoa_2d_decoder *)pd_new(hoa_2d_decoder_class);
    if(x)
    {
        const size_t order = (size_t)(f) < 1 ? 1 : (size_t)(f);
        x->f_processor = new hoa::Encoder<hoa::Hoa2d, t_sample>::Basic(order);
        x->f_signals   = new t_sample[x->f_processor->getNumberOfHarmonics() * 81092];
        hoa_processor_init(x, x->f_processor->getNumberOfHarmonics(), x->f_processor->getNumberOfPlanewaves());
    }
    return x;
}

static void hoa_2d_decoder_free(t_hoa_2d_decoder *x)
{
    hoa_processor_clear(x);
    delete x->f_processor;
    delete [] x->f_signals;
}

static void hoa_2d_decoder_perform(t_hoa_2d_decoder *x, size_t sampleframes,
                                   size_t nins, t_sample **ins,
                                   size_t nouts, t_sample **outs)
{
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_processor->setAzimuth(ins[1][i]);
        x->f_processor->process(ins[0]+i, x->f_signals + nouts * i);
    }
    for(long i = 0; i < nouts; i++)
    {
        hoa::Signal<t_sample>::copy(sampleframes, x->f_signals+i, nouts, outs[i], 1);
    }
}

static void hoa_2d_decoder_dsp(t_hoa_2d_decoder *x, t_signal **sp)
{
    hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_2d_decoder_perform, sp);
}

extern "C" void setup_hoa0x2e2d0x2edecoder_tilde(void)
{
    t_class *c = class_new(gensym("hoa.2d.decoder~"), (t_newmethod)hoa_2d_decoder_new, (t_method)hoa_2d_decoder_free,
                           (size_t)sizeof(t_hoa_2d_decoder), CLASS_DEFAULT, A_FLOAT, 0);
    if(c)
    {
        CLASS_MAINSIGNALIN(c, t_hoa_2d_decoder, f_f);
        class_addmethod(c, (t_method)hoa_2d_decoder_dsp, gensym("dsp"), A_CANT, 0);
        class_addcreator((t_newmethod)hoa_2d_decoder_new, gensym("hoa.decoder~"), A_FLOAT, 0);
        class_sethelpsymbol(c, gensym("helps/hoa.2d.decoder~"));
    }
    hoa_2d_decoder_class = c;
}








typedef struct _hoa_decoder
{
    t_edspobj                   f_obj;
    Decoder<Hoa2d, t_sample>*   f_decoder;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
    t_symbol*                   f_mode;
    void*                       f_attrs;
} t_hoa_decoder;

static t_eclass *hoa_decoder_class;

typedef struct _hoa_decoder_3d
{
    t_edspobj                   f_obj;
    Decoder<Hoa3d, t_sample>*   f_decoder;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
    t_symbol*                   f_mode;
    void*                       f_attrs;
} t_hoa_decoder_3d;

static t_eclass *hoa_decoder_3d_class;

static void *hoa_decoder_new(t_symbol *s, int argc, t_atom *argv)
{
    ulong order = 1;
    ulong arg   = 4;
    t_symbol* mode = gensym("regular");
    t_hoa_decoder *x = (t_hoa_decoder *)eobj_new(hoa_decoder_class);
    t_binbuf *d      = binbuf_via_atoms(argc,argv);

    if(x && d)
	{
        if(argc && argv && atom_gettype(argv) == A_LONG)
        {
            order = ulong(pd_clip_minmax(atom_getlong(argv), 1, 63));
        }
        if(argc > 1 && argv+1 && atom_gettype(argv+1) == A_SYM)
            mode = atom_getsym(argv+1);

        if(mode == gensym("irregular"))
        {
            arg = order * 2 + 1;
            if(argc > 2 && argv+2 && atom_gettype(argv+2) == A_LONG)
            {
                arg = ulong(pd_clip_min(atom_getlong(argv+2), 1));
            }
            x->f_decoder = new Decoder<Hoa2d, t_sample>::Irregular(order, arg);
            x->f_mode = mode;
        }
        else if(mode == gensym("binaural"))
        {
            x->f_decoder = new Decoder<Hoa2d, t_sample>::Binaural(order);
            x->f_mode = mode;
        }
        else
        {
            arg = order * 2 + 1;
            if(argc > 2 && argv+2 && atom_gettype(argv+2) == A_LONG)
            {
                arg = pd_clip_min(atom_getlong(argv+2), 1);
            }
            x->f_decoder = new Decoder<Hoa2d, t_sample>::Regular(order, arg);
            x->f_mode = gensym("regular");
        }

        eobj_dspsetup(x, long(x->f_decoder->getNumberOfHarmonics()), long(x->f_decoder->getNumberOfPlanewaves()));
        x->f_ins = Signal<t_sample>::alloc(x->f_decoder->getNumberOfHarmonics() * HOA_MAXBLKSIZE);
        x->f_outs= Signal<t_sample>::alloc(x->f_decoder->getNumberOfPlanewaves() * HOA_MAXBLKSIZE);
        ebox_attrprocess_viabinbuf(x, d);

        return x;
	}

    return NULL;
}

static void hoa_decoder_perform_regular(t_hoa_decoder *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, ulong(numins));
    }
	for(long i = 0; i < sampleframes; i++)
    {
        (static_cast<Decoder<Hoa2d, t_sample>::Regular*>(x->f_decoder))->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_decoder_perform_irregular(t_hoa_decoder *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_ins+i, ulong(numins));
    }
    for(long i = 0; i < sampleframes; i++)
    {
       (static_cast<Decoder<Hoa2d, t_sample>::Irregular*>(x->f_decoder))->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_decoder_perform_binaural(t_hoa_decoder *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    (static_cast<Decoder<Hoa2d, t_sample>::Binaural*>(x->f_decoder))->processBlock((const t_sample**)ins, outs);
}

static void hoa_decoder_dsp(t_hoa_decoder *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    x->f_decoder->computeRendering(ulong(maxvectorsize));
    if(x->f_mode == gensym("binaural"))
    {
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_perform_binaural, 0, NULL);
    }
    else if(x->f_mode == gensym("irregular"))
    {
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_perform_irregular, 0, NULL);
    }
    else
    {
        object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_perform_regular, 0, NULL);
    }
}

static t_pd_err hoa_decoder_angles_set(t_hoa_decoder *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        int dspState = canvas_suspend_dsp();
        for(long i = 0; i < argc && i < (long)x->f_decoder->getNumberOfPlanewaves(); i++)
        {
            if(atom_gettype(argv+i) == A_FLOAT)
                x->f_decoder->setPlanewaveAzimuth(ulong(i), atom_getfloat(argv+i) / 360. * HOA_2PI);
        }
        canvas_resume_dsp(dspState);
    }

    return 0;
}

static t_pd_err hoa_decoder_angles_get(t_hoa_decoder *x, void *attr, int* argc, t_atom **argv)
{
    *argc = long(x->f_decoder->getNumberOfPlanewaves());
    *argv = (t_atom *)malloc(size_t(*argc) * sizeof(t_atom));
    if(*argc && *argv)
    {
        for(int i = 0; i < *argc; i++)
        {
             atom_setfloat(*argv+i, x->f_decoder->getPlanewaveAzimuth(ulong(i)) * 360. / HOA_2PI);
        }
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
    return 0;
}

static t_pd_err hoa_decoder_offset_set(t_hoa_decoder *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv && atom_gettype(argv) == A_FLOAT)
    {
        int dspState = canvas_suspend_dsp();
        x->f_decoder->setPlanewavesRotation(0., 0., atom_getfloat(argv) / 360. * HOA_2PI);
        canvas_resume_dsp(dspState);
    }
    return 0;
}

static t_pd_err hoa_decoder_offset_get(t_hoa_decoder *x, void *attr, int* argc, t_atom **argv)
{
    *argc = 1;
    *argv = (t_atom *)malloc(size_t(*argc) * sizeof(t_atom));
    if(*argc && *argv)
    {
        atom_setfloat(*argv, x->f_decoder->getPlanewavesRotationZ() * 360. / HOA_2PI);
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
    return 0;
}

static t_pd_err hoa_decoder_crop_set(t_hoa_decoder *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv && atom_gettype(argv) == A_FLOAT)
    {
        if(x->f_mode == gensym("binaural"))
        {
            int dspState = canvas_suspend_dsp();
            (static_cast<Decoder<Hoa2d, t_sample>::Binaural*>(x->f_decoder))->setCropSize(atom_getfloat(argv));
            canvas_resume_dsp(dspState);
        }
    }
    return 0;
}

static t_pd_err hoa_decoder_crop_get(t_hoa_decoder *x, void *attr, int* argc, t_atom **argv)
{
    *argc = 1;
    *argv = (t_atom *)malloc(size_t(*argc) * sizeof(t_atom));
    if(*argc && *argv)
    {
        if(x->f_mode == gensym("binaural"))
        {
            atom_setfloat(*argv, (static_cast<Decoder<Hoa2d, t_sample>::Binaural*>(x->f_decoder))->getCropSize());
        }
        else
        {
            atom_setfloat(*argv, 0);
        }
    }
    else
    {
        *argc = 0;
        *argv = NULL;
    }
    return 0;
}

static void hoa_decoder_free(t_hoa_decoder *x)
{
    eobj_dspfree(x);
	delete x->f_decoder;
    Signal<t_sample>::free(x->f_ins);
    Signal<t_sample>::free(x->f_outs);
}

extern "C" void setup_hoa0x2e2d0x2edecoder_tilde(void)
{
    t_eclass* c;

    c = eclass_new("hoa.2d.decoder~", (method)hoa_decoder_new, (method)hoa_decoder_free, (short)sizeof(t_hoa_decoder), 0L, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_decoder_new, gensym("hoa.decoder~"), A_GIMME, 0);
    eclass_dspinit(c);
    eclass_addmethod(c, (method)hoa_decoder_dsp,           "dsp",          A_CANT,  0);

    CLASS_ATTR_DOUBLE_VARSIZE	(c, "angles",0, t_hoa_decoder, f_attrs, f_attrs, HOA_MAX_PLANEWAVES);
    CLASS_ATTR_ACCESSORS		(c, "angles", hoa_decoder_angles_get, hoa_decoder_angles_set);
    CLASS_ATTR_CATEGORY			(c, "angles", 0, "Planewaves");
    CLASS_ATTR_LABEL			(c, "angles", 0, "Angles of Loudspeakers");
    CLASS_ATTR_SAVE             (c, "angles", 0);

    CLASS_ATTR_DOUBLE           (c, "offset", 0, t_hoa_decoder, f_attrs);
    CLASS_ATTR_ACCESSORS		(c, "offset", hoa_decoder_offset_get, hoa_decoder_offset_set);
    CLASS_ATTR_CATEGORY			(c, "offset", 0, "Planewaves");
    CLASS_ATTR_LABEL            (c, "offset", 0, "Offset of Channels");
    CLASS_ATTR_SAVE             (c, "offset", 0);
    
    CLASS_ATTR_LONG             (c, "crop", 0, t_hoa_decoder, f_attrs);
    CLASS_ATTR_ACCESSORS		(c, "crop", hoa_decoder_crop_get, hoa_decoder_crop_set);
    CLASS_ATTR_CATEGORY			(c, "crop", 0, "Planewaves");
    CLASS_ATTR_LABEL            (c, "crop", 0, "Crop of the Responses");
    CLASS_ATTR_SAVE             (c, "crop", 0);

    eclass_register(CLASS_OBJ, c);
    hoa_decoder_class = c;
}


