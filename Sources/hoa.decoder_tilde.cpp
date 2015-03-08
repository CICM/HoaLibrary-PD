/*
// Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include "hoa.library.h"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_decoder
{
    t_edspobj                   f_obj;
    Decoder<Hoa2d, t_sample>*   f_decoder;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
    long                        f_number_of_channels;
    double                      f_angles_of_channels[HOA_MAX_PLANEWAVES];
    double                      f_offset;
    long                        f_pinna;
    
} t_hoa_decoder;

t_eclass *hoa_decoder_class;

typedef struct _hoa_decoder_3D
{
    t_edspobj                   f_obj;
    Decoder<Hoa3d, t_sample>*   f_decoder;
    t_sample*                   f_ins;
    t_sample*                   f_outs;
    long                        f_number_of_channels;
    double                      f_angles_of_channels[HOA_MAX_PLANEWAVES * 2];
    double                      f_offset;
    long                        f_pinna;
} t_hoa_decoder_3D;

t_eclass *hoa_decoder_3D_class;

extern void *hoa_decoder_new(t_symbol *s, long argc, t_atom *argv)
{
    int	order = 1;
    int number_of_channels = 4;
    t_hoa_decoder *x = (t_hoa_decoder *)eobj_new(hoa_decoder_class);
    t_binbuf *d      = binbuf_via_atoms(argc,argv);
	
    if(x && d)
	{
        if(argc && argv && atom_gettype(argv) == A_LONG)
            order = pd_clip_min(atom_getlong(argv), 1);
        if(argc > 2 && argv && atom_gettype(argv+2) == A_LONG)
            number_of_channels = pd_clip_min(atom_getlong(argv+2), 1);
        
        x->f_decoder = new Decoder<Hoa2d, t_sample>(order, number_of_channels);
        x->f_number_of_channels = x->f_decoder->getNumberOfPlanewaves();
    
        eobj_dspsetup(x, x->f_decoder->getNumberOfHarmonics(), x->f_decoder->getNumberOfPlanewaves());
        x->f_ins = new t_float[x->f_decoder->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        x->f_outs= new t_float[HOA_MAX_PLANEWAVES * HOA_MAXBLKSIZE];
    
        ebox_attrprocess_viabinbuf(x, d);
        
        return x;
	}
    
    return NULL;
}

extern void hoa_decoder_perform64(t_hoa_decoder *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < numins; i++)
    {
        cblas_scopy(sampleframes, ins[i], 1, x->f_ins+i, numins);
    }
	for(long i = 0; i < sampleframes; i++)
    {
        x->f_decoder->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_outs+i, numouts, outs[i], 1);
    }
}

extern void hoa_decoder_dsp(t_hoa_decoder *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_perform64, 0, NULL);
}

extern t_pd_err hoa_decoder_angles_set(t_hoa_decoder *x, void *attr, long argc, t_atom *argv)
{
    if(argc && argv)
    {
        int dspState = canvas_suspend_dsp();
        for(int i = 0; i < argc && i < x->f_decoder->getNumberOfPlanewaves(); i++)
        {
            if(atom_gettype(argv+i) == A_FLOAT)
                x->f_decoder->setPlanewaveAzimuth(i, atom_getfloat(argv+i) / 360. * HOA_2PI);
        }
        x->f_decoder->computeMatrix();
        canvas_resume_dsp(dspState);
    }
    
    return 0;
}

extern t_pd_err hoa_decoder_pinna_set(t_hoa_decoder *x, void *attr, long argc, t_atom *argv)
{
    if(argc && argv && atom_gettype(argv) == A_SYM)
	{
        ;
    }
    return 0;
}

extern t_pd_err hoa_decoder_offset_set(t_hoa_decoder *x, void *attr, long argc, t_atom *argv)
{
    if(argc && argv && atom_gettype(argv) == A_FLOAT)
    {
        int dspState = canvas_suspend_dsp();
        x->f_decoder->setPlanewavesRotation(atom_getfloat(argv) / 360. * HOA_2PI);
        x->f_decoder->computeMatrix();
        canvas_resume_dsp(dspState);
    }
    return 0;
}

extern void hoa_decoder_free(t_hoa_decoder *x)
{
    eobj_dspfree(x);
	delete x->f_decoder;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

extern "C" void setup_hoa0x2e2d0x2edecoder_tilde(void)
{
    t_eclass* c;
    
    c = eclass_new("hoa.2d.decoder~", (method)hoa_decoder_new, (method)hoa_decoder_free, (short)sizeof(t_hoa_decoder), 0L, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_decoder_new, gensym("hoa.decoder~"), A_GIMME, 0);
    
    eclass_dspinit(c);
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_decoder_dsp,           "dsp",          A_CANT,  0);
    
    CLASS_ATTR_DOUBLE_VARSIZE	(c, "angles",0, t_hoa_decoder, f_angles_of_channels, f_number_of_channels, HOA_MAX_PLANEWAVES);
    CLASS_ATTR_ACCESSORS		(c, "angles", NULL, hoa_decoder_angles_set);
    CLASS_ATTR_ORDER			(c, "angles", 0, "2");
    CLASS_ATTR_LABEL			(c, "angles", 0, "Angles of Loudspeakers");
    
    CLASS_ATTR_DOUBLE           (c, "offset", 0, t_hoa_decoder, f_offset);
    CLASS_ATTR_CATEGORY			(c, "offset", 0, "Planewaves");
    CLASS_ATTR_LABEL            (c, "offset", 0, "Offset of Channels");
    CLASS_ATTR_ACCESSORS		(c, "offset", NULL, hoa_decoder_offset_set);
    CLASS_ATTR_DEFAULT          (c, "offset", 0, "0");
    CLASS_ATTR_ORDER            (c, "offset", 0, "3");
    CLASS_ATTR_SAVE             (c, "offset", 0);
    
    CLASS_ATTR_LONG				(c, "pinna", 0 , t_hoa_decoder, f_pinna);
    CLASS_ATTR_ACCESSORS		(c, "pinna", NULL, hoa_decoder_pinna_set);
    CLASS_ATTR_LABEL			(c, "pinna", 0, "Pinna Size");
    
    eclass_register(CLASS_OBJ, c);
    hoa_decoder_class = c;
}

extern void *hoa_decoder_3D_new(t_symbol *s, long argc, t_atom *argv)
{
    int	order = 1;
    int number_of_channels = 4;
    t_hoa_decoder_3D *x = (t_hoa_decoder_3D *)eobj_new(hoa_decoder_3D_class);
    t_binbuf *d         = binbuf_via_atoms(argc,argv);
    
    if(x && d)
    {
        if(argc && argv && atom_gettype(argv) == A_LONG)
            order = pd_clip_min(atom_getlong(argv), 1);
        if(argc > 2 && argv && atom_gettype(argv+2) == A_LONG)
            number_of_channels = pd_clip_min(atom_getlong(argv+2), 1);
        
        x->f_decoder = new Decoder<Hoa3d, t_sample>(order, number_of_channels);
        x->f_number_of_channels = x->f_decoder->getNumberOfPlanewaves() * 2;
        
        eobj_dspsetup(x, x->f_decoder->getNumberOfHarmonics(), x->f_decoder->getNumberOfPlanewaves());
        x->f_ins = new t_float[x->f_decoder->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        x->f_outs= new t_float[HOA_MAX_PLANEWAVES * HOA_MAXBLKSIZE];
        
        ebox_attrprocess_viabinbuf(x, d);
        
        return x;
    }
    
    return NULL;
}

extern void hoa_decoder_3D_perform64(t_hoa_decoder_3D *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < numins; i++)
    {
        cblas_scopy(sampleframes, ins[i], 1, x->f_ins+i, numins);
    }
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_decoder->process(x->f_ins + numins * i, x->f_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_outs+i, numouts, outs[i], 1);
    }
    
}

extern void hoa_decoder_3D_dsp(t_hoa_decoder_3D *x, t_object *dsp64, short *count, double samplerate, long maxvectorsize, long flags)
{
    object_method(dsp64, gensym("dsp_add64"), x, (method)hoa_decoder_3D_perform64, 0, NULL);
}

extern t_pd_err hoa_decoder_3D_angles_set(t_hoa_decoder_3D *x, void *attr, long argc, t_atom *argv)
{
    if(argc && argv)
    {
        int dspState = canvas_suspend_dsp();
        for(long i = 0, j = 0; j < argc && i < x->f_decoder->getNumberOfPlanewaves() * 2; j++)
        {
            if(atom_gettype(argv+j) == A_FLOAT)
            {
                if(j%2)
                {
                    x->f_decoder->setPlanewaveElevation(i, atom_getfloat(argv+j) / 360. * HOA_2PI);
                    i++;
                }
                else
                {
                    x->f_decoder->setPlanewaveAzimuth(i, atom_getfloat(argv+j) / 360. * HOA_2PI);
                }
                
            }
        }
        x->f_decoder->computeMatrix();
        canvas_resume_dsp(dspState);
    }

    return 0;
}

extern t_pd_err hoa_decoder_3D_offset_set(t_hoa_decoder_3D *x, void *attr, long argc, t_atom *argv)
{
    if(argc && argv)
    {
        double ax, ay, az;
        int dspState = canvas_suspend_dsp();
        if(atom_gettype(argv) == A_FLOAT)
            ax = atom_getfloat(argv) / 360. * HOA_2PI;
        else
            ax = x->f_decoder->getPlanewavesRotationX();
        if(argc > 1 && atom_gettype(argv+1) == A_FLOAT)
            ay = atom_getfloat(argv+1) / 360. * HOA_2PI;
        else
            ay = x->f_decoder->getPlanewavesRotationX();
        if(argc > 2 &&  atom_gettype(argv+2) == A_FLOAT)
            az = atom_getfloat(argv+2) / 360. * HOA_2PI;
        else
            az = x->f_decoder->getPlanewavesRotationX();
        x->f_decoder->setPlanewavesRotation(ax, ay, az);
        x->f_decoder->computeMatrix();
        canvas_resume_dsp(dspState);
    }
    return 0;
}

extern t_pd_err hoa_decoder_3D_pinna_set(t_hoa_decoder_3D *x, void *attr, long argc, t_atom *argv)
{
    if(argc && argv && atom_gettype(argv) == A_SYM)
    {
        ;
    }
    return 0;
}

extern void hoa_decoder_3D_free(t_hoa_decoder_3D *x)
{
    eobj_dspfree(x);
    delete x->f_decoder;
    delete [] x->f_ins;
    delete [] x->f_outs;
}

extern "C" void setup_hoa0x2e3d0x2edecoder_tilde(void)
{
    t_eclass* c;
    
    c = eclass_new("hoa.3d.decoder~", (method)hoa_decoder_3D_new, (method)hoa_decoder_3D_free, (short)sizeof(t_hoa_decoder_3D), 0L, A_GIMME, 0);
    
    eclass_dspinit(c);
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_decoder_3D_dsp,           "dsp",          A_CANT,  0);
    
    CLASS_ATTR_DOUBLE_VARSIZE	(c, "angles",0, t_hoa_decoder_3D, f_angles_of_channels, f_number_of_channels, HOA_MAX_PLANEWAVES*2);
    CLASS_ATTR_ACCESSORS		(c, "angles", NULL, hoa_decoder_3D_angles_set);
    CLASS_ATTR_ORDER			(c, "angles", 0, "2");
    CLASS_ATTR_LABEL			(c, "angles", 0, "Angles of Loudspeakers");
    
    CLASS_ATTR_DOUBLE_ARRAY     (c, "offset", 0, t_hoa_decoder_3D, f_offset, 3);
    CLASS_ATTR_CATEGORY			(c, "offset", 0, "Planewaves");
    CLASS_ATTR_LABEL            (c, "offset", 0, "Offset of Channels");
    CLASS_ATTR_ACCESSORS		(c, "offset", NULL, hoa_decoder_3D_offset_set);
    CLASS_ATTR_DEFAULT          (c, "offset", 0, "0 0");
    CLASS_ATTR_ORDER            (c, "offset", 0, "3");
    CLASS_ATTR_SAVE             (c, "offset", 0);
    
    CLASS_ATTR_LONG				(c, "pinna", 0 , t_hoa_decoder_3D, f_pinna);
    CLASS_ATTR_ACCESSORS		(c, "pinna", NULL, hoa_decoder_3D_pinna_set);
    CLASS_ATTR_LABEL			(c, "pinna", 0, "Pinna Size");
    
    eclass_register(CLASS_OBJ, c);
    hoa_decoder_3D_class = c;
}

