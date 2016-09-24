/*
// Copyright (c) 2012-2015 Pierre Guillot, Eliott Paris & Thomas Le Meur CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

extern "C"
{
#include "../hoa.pd.h"
}
#include <Hoa.hpp>

typedef struct _hoa_2d_decoder
{
    t_hoa_processor  f_obj;
    float            f_f;
    hoa::Decoder<hoa::Hoa2d, t_sample>* f_processor;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
} t_hoa_2d_decoder;

static t_class *hoa_2d_decoder_class;
static t_symbol* hoa_sym_regular;
static t_symbol* hoa_sym_irregular;
static t_symbol* hoa_sym_binaural;

static t_symbol* hoa_sym_tcrop;
static t_symbol* hoa_sym_acrop;

static t_symbol* hoa_sym_toffset;
static t_symbol* hoa_sym_aoffset;

static t_symbol* hoa_sym_tangles;
static t_symbol* hoa_sym_aangles;

static void *hoa_2d_decoder_new(t_symbol* s, int argc, t_atom* argv)
{
    t_hoa_2d_decoder *x = (t_hoa_2d_decoder *)pd_new(hoa_2d_decoder_class);
    if(x)
    {
        const size_t order   = hoa_processor_clip_order(x, (size_t)atom_getfloatarg(0, argc, argv));
        const t_symbol* mode = atom_getsymbolarg(1, argc, argv);
        if(mode == hoa_sym_regular)
        {
            size_t nplws = atom_getfloatarg(2, argc, argv);
            if(nplws < order * 2 + 1)
            {
                pd_error(x, "hoa.2d.decoder~: bad argument : number of planewaves must be at least 2 * order + 1.");
                nplws = order * 2 + 1;
            }
            x->f_processor = new hoa::DecoderRegular<hoa::Hoa2d, t_sample>(order, nplws);
            for(int i = 3; i < argc; ++i)
            {
                if(argv[i].a_type == A_SYMBOL)
                {
                    if(atom_getsymbol(argv+i) == hoa_sym_toffset || atom_getsymbol(argv+i) == hoa_sym_aoffset)
                    {
                        if(i+1 < argc && argv[i+1].a_type == A_FLOAT)
                        {
                            x->f_processor->setPlanewavesRotation(0, 0,atom_getfloat(argv+i+1) / 360. * HOA_2PI);
                        }
                        else
                        {
                            pd_error(x, "hoa.2d.decoder~: -offset bad argument for the attribute.");
                        }
                    }
                    break;
                }
            }
            x->f_processor->setPlanewavesRotation(0., 0., atom_getfloatarg(3, argc, argv) / 360. * HOA_2PI);
        }
        else if(mode == hoa_sym_irregular)
        {
            size_t nplws = atom_getfloatarg(2, argc, argv);
            if(nplws < 1)
            {
                pd_error(x, "hoa.2d.decoder~: bad argument : number of planewaves must be at least 1.");
                nplws = 1;
            }
            x->f_processor = new hoa::DecoderIrregular<hoa::Hoa2d, t_sample>(order, nplws);
            for(int i = 3; i < argc; ++i)
            {
                if(argv[i].a_type == A_SYMBOL)
                {
                    if(atom_getsymbol(argv+i) == hoa_sym_toffset || atom_getsymbol(argv+i) == hoa_sym_aoffset)
                    {
                        if(i+1 < argc && argv[i+1].a_type == A_FLOAT)
                        {
                            x->f_processor->setPlanewavesRotation(0, 0,atom_getfloat(argv+i+1));
                        }
                        else
                        {
                            pd_error(x, "hoa.2d.decoder~: -offset bad argument for the attribute.");
                        }
                    }
                    break;
                }
            }
            
            for(int i = 3; i < argc; ++i)
            {
                if(argv[i].a_type == A_SYMBOL)
                {
                    if(atom_getsymbol(argv+i) == hoa_sym_tangles || atom_getsymbol(argv+i) == hoa_sym_aangles)
                    {
                        for(int j = i+1, k = 0; j < argc && k < (int)nplws; ++i, ++j)
                        {
                            if(argv[j].a_type == A_FLOAT)
                            {
                                x->f_processor->setPlanewaveAzimuth(size_t(k), atom_getfloat(argv+j) / 360. * HOA_2PI);
                            }
                            else
                            {
                                pd_error(x, "hoa.2d.decoder~: -angles bad argument for the attribute.");
                            }
                        }
                    }
                    break;
                }
            }
        }
        else
        {
            x->f_processor = new hoa::DecoderBinaural<hoa::Hoa2d, t_sample>(order);
            if(mode != hoa_sym_binaural)
            {
                pd_error(x, "hoa.2d.decoder~: bad argument : mode must be regular, irregular or binaural.");
            }
            for(int i = 2; i < argc; ++i)
            {
                if(argv[i].a_type == A_SYMBOL)
                {
                    if(atom_getsymbol(argv+i) == hoa_sym_tcrop || atom_getsymbol(argv+i) == hoa_sym_acrop)
                    {
                        if(i+1 < argc && argv[i+1].a_type == A_FLOAT)
                        {
                            reinterpret_cast<hoa::DecoderBinaural<hoa::Hoa2d, t_sample> *>(x->f_processor)->setCropSize(atom_getfloat(argv+i+1));
                        }
                        else
                        {
                            pd_error(x, "hoa.2d.decoder~: -crop bad argument for the attribute.");
                        }
                    }
                    break;
                }
            }
        }
        
        
        x->f_ins   = new t_sample[x->f_processor->getNumberOfHarmonics() * 81092];
        x->f_outs  = new t_sample[x->f_processor->getNumberOfPlanewaves() * 81092];
        hoa_processor_init(x, x->f_processor->getNumberOfHarmonics(), x->f_processor->getNumberOfPlanewaves());
    }
    return x;
}

static void hoa_2d_decoder_free(t_hoa_2d_decoder *x)
{
    hoa_processor_clear(x);
    delete x->f_processor;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

static void hoa_2d_decoder_perform_regular(t_hoa_2d_decoder *x, size_t sampleframes,
                                        size_t nins, t_sample **ins,
                                        size_t nouts, t_sample **outs)
{
    for(long i = 0; i < nins; i++)
    {
        hoa::Signal<t_sample>::copy(size_t(sampleframes), ins[i], 1, x->f_ins+i, size_t(nins));
    }
    for(long i = 0; i < sampleframes; i++)
    {
        (static_cast<hoa::DecoderRegular<hoa::Hoa2d, t_sample>*>(x->f_processor))->process(x->f_ins + nins * i, x->f_outs + nouts * i);
    }
    for(long i = 0; i < nouts; i++)
    {
       hoa:: Signal<t_sample>::copy(size_t(sampleframes), x->f_outs+i, size_t(nouts), outs[i], 1);
    }
}

static void hoa_2d_decoder_perform_irregular(t_hoa_2d_decoder *x, size_t sampleframes,
                                          size_t nins, t_sample **ins,
                                          size_t nouts, t_sample **outs)
{
    for(long i = 0; i < nins; i++)
    {
        hoa::Signal<t_sample>::copy(size_t(sampleframes), ins[i], 1, x->f_ins+i, size_t(nins));
    }
    for(long i = 0; i < sampleframes; i++)
    {
        (static_cast<hoa::DecoderIrregular<hoa::Hoa2d, t_sample>*>(x->f_processor))->process(x->f_ins + nins * i, x->f_outs + nouts * i);
    }
    for(long i = 0; i < nouts; i++)
    {
        hoa::Signal<t_sample>::copy(size_t(sampleframes), x->f_outs+i, size_t(nouts), outs[i], 1);
    }
}

static void hoa_2d_decoder_perform_binaural(t_hoa_2d_decoder *x, size_t sampleframes,
                                         size_t nins, t_sample **ins,
                                         size_t nouts, t_sample **outs)
{
    (static_cast<hoa::DecoderBinaural<hoa::Hoa2d, t_sample>*>(x->f_processor))->processBlock((const t_sample**)ins, outs);
}



static void hoa_2d_decoder_dsp(t_hoa_2d_decoder *x, t_signal **sp)
{
    x->f_processor->computeRendering((size_t)sp[0]->s_n);
    if(x->f_processor->getMode() == hoa::Decoder<hoa::Hoa2d, t_sample>::BinauralMode)
    {
        hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_2d_decoder_perform_binaural, sp);
    }
    else if(x->f_processor->getMode() == hoa::Decoder<hoa::Hoa2d, t_sample>::IrregularMode)
    {
        hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_2d_decoder_perform_irregular, sp);
    }
    else
    {
        hoa_processor_prepare(x, (t_hoa_processor_perfm)hoa_2d_decoder_perform_regular, sp);
    }
}

extern "C" void setup_hoa0x2e2d0x2edecoder_tilde(void)
{
    t_class *c = class_new(gensym("hoa.2d.decoder~"), (t_newmethod)hoa_2d_decoder_new, (t_method)hoa_2d_decoder_free,
                           (size_t)sizeof(t_hoa_2d_decoder), CLASS_DEFAULT, A_GIMME, 0);
    if(c)
    {
        CLASS_MAINSIGNALIN(c, t_hoa_2d_decoder, f_f);
        class_addmethod(c, (t_method)hoa_2d_decoder_dsp, gensym("dsp"), A_CANT, 0);
        class_addcreator((t_newmethod)hoa_2d_decoder_new, gensym("hoa.decoder~"), A_GIMME, 0);
        class_sethelpsymbol(c, gensym("helps/hoa.2d.decoder~"));
    }
    hoa_2d_decoder_class = c;
    
    hoa_sym_regular     = gensym("regular");
    hoa_sym_irregular   = gensym("irregular");
    hoa_sym_binaural    = gensym("binaural");
    
    hoa_sym_tcrop       = gensym("-crop");
    hoa_sym_acrop       = gensym("@crop");
    
    hoa_sym_toffset     = gensym("-offset");
    hoa_sym_aoffset     = gensym("@offset");
    
    hoa_sym_tangles     = gensym("-angles");
    hoa_sym_aangles     = gensym("@angles");
}




