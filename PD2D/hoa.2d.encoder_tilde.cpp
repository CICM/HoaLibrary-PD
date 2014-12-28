/*
 // Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "Hoa2D.pd.h"

typedef struct _hoa_encoder_tilde
{
    t_edspobj                               f_obj;
    unique_ptr<EncoderMulti<Hoa2d, t_float>>f_encoder;
    unique_ptr<PolarLines<t_float>>         f_lines;
    t_float*                                f_sig_ins;
    t_float*                                f_sig_outs;
    t_float*                                f_lines_vector;
	char                                    f_mode;
    float                                   f_ramp;
} t_hoa_encoder_tilde;

t_eclass *hoa_encoder_tilde_class;

void *hoa_encoder_tilde_new(t_symbol *s, long argc, t_atom *argv);
void hoa_encoder_tilde_free(t_hoa_encoder_tilde *x);
void hoa_encoder_tilde_float(t_hoa_encoder_tilde *x, float f);
void hoa_encoder_tilde_list(t_hoa_encoder_tilde *x, t_symbol *s, long argc, t_atom *argv);

void hoa_encoder_tilde_dsp(t_hoa_encoder_tilde *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags);

void hoa_encoder_tilde_perform_multisources(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam);
void hoa_encoder_tilde_perform(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam);
void hoa_encoder_tilde_perform_in1(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam);
void hoa_encoder_tilde_perform_in2(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam);
void hoa_encoder_tilde_perform_in1_in2(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam);

t_pd_err ramp_set(t_hoa_encoder_tilde *x, t_object *attr, long argc, t_atom *argv);

t_hoa_err hoa_getinfos(t_hoa_encoder_tilde* x, t_hoa_boxinfos* boxinfos);

extern "C" void setup_hoa0x2e2d0x2eencoder_tilde(void)
{
    t_eclass* c;
    
    c = eclass_new("hoa.2d.encoder~", (method)hoa_encoder_tilde_new, (method)hoa_encoder_tilde_free, (short)sizeof(t_hoa_encoder_tilde), CLASS_NOINLET, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_encoder_tilde_new, gensym("hoa.encoder~"), A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_encoder_tilde_new, gensym("hoa.map~"), A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_encoder_tilde_new, gensym("hoa.2d.map~"), A_GIMME, 0);
    
	eclass_dspinit(c);
    hoa_initclass(c, (method)hoa_getinfos);
	eclass_addmethod(c, (method)hoa_encoder_tilde_dsp,          "dsp",      A_CANT, 0);
    eclass_addmethod(c, (method)hoa_encoder_tilde_list,         "list",     A_GIMME, 0);
    eclass_addmethod(c, (method)hoa_encoder_tilde_float,        "float",    A_FLOAT, 0);
    
    CLASS_ATTR_DOUBLE           (c, "ramp", 0, t_hoa_encoder_tilde, f_ramp);
	CLASS_ATTR_CATEGORY			(c, "ramp", 0, "Behavior");
	CLASS_ATTR_LABEL			(c, "ramp", 0, "Ramp Time (ms)");
	CLASS_ATTR_ORDER			(c, "ramp", 0, "2");
	CLASS_ATTR_ACCESSORS		(c, "ramp", NULL, ramp_set);
	CLASS_ATTR_SAVE				(c, "ramp", 0);
    
    eclass_register(CLASS_OBJ, c);
    hoa_encoder_tilde_class = c;
}

void *hoa_encoder_tilde_new(t_symbol *s, long argc, t_atom *argv)
{
    int	order           = 1;
    int numberOfSources = 1;
    t_hoa_encoder_tilde *x = (t_hoa_encoder_tilde *)eobj_new(hoa_encoder_tilde_class);
    t_binbuf *d            = binbuf_via_atoms(argc,argv);
	if(x && d)
	{
        x->f_mode = 0;
		if(argc && atom_gettype(argv) == A_LONG)
        {
			order = max(atom_getlong(argv), (long)1);
        }
        if(argc > 1 && atom_gettype(argv+1) == A_LONG)
        {
            numberOfSources = clip(atom_getlong(argv+1), (long)1, (long)255);
        }
        if(argc > 2 && atom_gettype(argv+2) == A_SYM)
        {
            if(atom_getsym(argv+2) == gensym("car") || atom_getsym(argv+2) == gensym("cartesian"))
                x->f_mode = 1;
            else
                x->f_mode = 0;
        }
        
        x->f_ramp = 100;
        x->f_encoder    = unique_ptr<EncoderMulti<Hoa2d, t_float>>(new EncoderMulti<Hoa2d, t_float>(order, numberOfSources));
        x->f_lines      = unique_ptr<PolarLines<t_float>>(new PolarLines<t_float>(x->f_encoder->getNumberOfSources()));
        x->f_lines->setRamp(0.1 * sys_getsr());
		
        for(ulong i = 0; i < x->f_encoder->getNumberOfSources(); i++)
        {
            x->f_lines->setRadiusDirect(i, 1);
            x->f_lines->setAzimuthDirect(i, 0.);
        }
        
		if(x->f_encoder->getNumberOfSources() == 1)
        {
            eobj_dspsetup(x, 3, x->f_encoder->getNumberOfHarmonics());
            x->f_sig_ins    = new t_float[3 * HOA_MAX_BLOCKSIZE];
        }
        else
        {
            eobj_dspsetup(x, x->f_encoder->getNumberOfSources(), x->f_encoder->getNumberOfHarmonics());
            x->f_sig_ins    = new t_float[x->f_encoder->getNumberOfSources() * HOA_MAX_BLOCKSIZE];
        }
        x->f_sig_outs       = new t_float[x->f_encoder->getNumberOfHarmonics() * HOA_MAX_BLOCKSIZE];
        x->f_lines_vector   = new float[x->f_encoder->getNumberOfSources() * 2];

        ebox_attrprocess_viabinbuf(x, d);
	}

	return (x);
}


void hoa_encoder_tilde_free(t_hoa_encoder_tilde *x)
{
    eobj_dspfree(x);
    delete [] x->f_sig_ins;
    delete [] x->f_sig_outs;
    delete [] x->f_lines_vector;
}

t_hoa_err hoa_getinfos(t_hoa_encoder_tilde* x, t_hoa_boxinfos* boxinfos)
{
	boxinfos->object_type = HOA_OBJECT_2D;
	
	if(x->f_encoder->getNumberOfSources() == 1)
		boxinfos->autoconnect_inputs = 1;
	else
		boxinfos->autoconnect_inputs = x->f_encoder->getNumberOfSources();
	
	boxinfos->autoconnect_outputs = x->f_encoder->getNumberOfHarmonics();
	boxinfos->autoconnect_inputs_type = HOA_CONNECT_TYPE_STANDARD;
	boxinfos->autoconnect_outputs_type = HOA_CONNECT_TYPE_AMBISONICS;
	return HOA_ERR_NONE;
}

void hoa_encoder_tilde_float(t_hoa_encoder_tilde *x, float f)
{
    if(x->f_encoder->getNumberOfSources() == 1)
    {
		if(x->f_mode == 0)
		{
			if(eobj_getproxy((t_object *)x) == 1)
			{
				x->f_lines->setRadiusDirect(0, max(f, 0.f));
                x->f_encoder->setRadius(0, max(f, 0.f));
			}
			else if(eobj_getproxy((t_object *)x) == 2)
			{
				x->f_lines->setAzimuthDirect(0, f);
                x->f_encoder->setAzimuth(0, f);
			}
		}
		else if(x->f_mode == 1)
		{
			if(eobj_getproxy((t_object *)x) == 1)
			{
                float ord = ordinate(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0));
				x->f_lines->setRadiusDirect(0, radius(f, ord));
                x->f_lines->setAzimuthDirect(0, azimuth(f, ord));
                x->f_encoder->setRadius(0, radius(f, ord));
                x->f_encoder->setAzimuth(0, azimuth(f, ord));
			}
			else if(eobj_getproxy((t_object *)x) == 2)
			{
				float abs = abscissa(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0));
                x->f_lines->setRadiusDirect(0, radius(abs, f));
				x->f_lines->setAzimuthDirect(0, azimuth(abs, f));
                x->f_encoder->setRadius(0, radius(abs, f));
                x->f_encoder->setAzimuth(0, azimuth(abs, f));
			}
		}
    }
}

void hoa_encoder_tilde_list(t_hoa_encoder_tilde *x, t_symbol* s, long argc, t_atom* argv)
{
    if(argc > 2 && argv && atom_gettype(argv) == A_LONG && atom_gettype(argv+1) == A_SYM)
    {
        int index = atom_getlong(argv);
        if(index < 1 || index > x->f_encoder->getNumberOfSources())
            return;
        
        if(argc > 3 && (atom_getsym(argv+1) == hoa_sym_polar || atom_getsym(argv+1) == hoa_sym_pol))
        {
            x->f_lines->setRadius(index-1, atom_getfloat(argv+2));
            x->f_lines->setAzimuth(index-1, atom_getfloat(argv+3));
        }
        else if(argc > 3 && (atom_getsym(argv+1) == hoa_sym_cartesian || atom_getsym(argv+1) == hoa_sym_car))
        {
            x->f_lines->setRadius(index-1, radius(atom_getfloat(argv+2), atom_getfloat(argv+3)));
            x->f_lines->setAzimuth(index-1, azimuth(atom_getfloat(argv+2), atom_getfloat(argv+3)));
        }
        else if(argc > 2 && atom_getsym(argv+1) == hoa_sym_mute)
        {
            x->f_encoder->setMute(index-1, atom_getlong(argv+2));
        }
    }
}

t_pd_err ramp_set(t_hoa_encoder_tilde *x, t_object *attr, long argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_FLOAT)
        {
            x->f_ramp = max(atom_getfloat(argv), 1.f);
            x->f_lines->setRamp(x->f_ramp / 1000. * sys_getsr());
        }
    }
    return 0;
}

void hoa_encoder_tilde_dsp(t_hoa_encoder_tilde *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    x->f_lines->setRamp(x->f_ramp / 1000. * samplerate);
    if(x->f_encoder->getNumberOfSources() == 1)
    {
		if(count[1] && count[2])
            object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_tilde_perform_in1_in2, 0, NULL);
        else if(!count[1] && !count[2])
            object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_tilde_perform, 0, NULL);
        else if(count[1])
            object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_tilde_perform_in1, 0, NULL);
        else
            object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_tilde_perform_in2, 0, NULL);
    }
    else
    {
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_encoder_tilde_perform_multisources, 0, NULL);
    }
}

void hoa_encoder_tilde_perform_multisources(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	long nsources = x->f_encoder->getNumberOfSources();
    for(long i = 0; i < numins; i++)
    {
        cblas_scopy(sampleframes, ins[i], 1, x->f_sig_ins+i, numins);
    }
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_lines->process(x->f_lines_vector);
		for(long j = 0; j < nsources; j++)
        {
			x->f_encoder->setRadius(j, (*x->f_lines_vector+j));
        }
        for(long j = 0; j < nsources; j++)
        {
			x->f_encoder->setAzimuth(j, (*x->f_lines_vector+j+nsources));
        }
        x->f_encoder->process(x->f_sig_ins + numins * i, x->f_sig_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_sig_outs+i, numouts, outs[i], 1);
    }
}

void hoa_encoder_tilde_perform(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < sampleframes; i++)
    {
		x->f_lines->process(x->f_lines_vector);
		x->f_encoder->setRadius(0, x->f_lines_vector[0]);
		x->f_encoder->setAzimuth(0, x->f_lines_vector[1]);
        x->f_encoder->process(ins[0]+i, x->f_sig_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_sig_outs+i, numouts, outs[i], 1);
    }
}

void hoa_encoder_tilde_perform_in1(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(int i = 0; i < sampleframes; i++)
    {
        x->f_encoder->setRadius(0, ins[1][i]);
        x->f_encoder->process(&ins[0][i], x->f_sig_outs + numouts * i);
    }
    for(int i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_sig_outs+i, numouts, outs[i], 1);
    }
}

void hoa_encoder_tilde_perform_in2(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(int i = 0; i < sampleframes; i++)
    {
        x->f_encoder->setAzimuth(0, ins[2][i]);
        x->f_encoder->process(&ins[0][i], x->f_sig_outs + numouts * i);
    }
    for(int i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_sig_outs+i, numouts, outs[i], 1);
    }
}

void hoa_encoder_tilde_perform_in1_in2(t_hoa_encoder_tilde *x, t_object *dsp64, float **ins, long numins, float **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_encoder->setRadius(0, ins[1][i]);
            x->f_encoder->setAzimuth(0, ins[2][i]);
            x->f_encoder->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_encoder->setAzimuth(0, azimuth(ins[1][i], ins[2][i]));
            x->f_encoder->setRadius(0, radius(ins[1][i], ins[2][i]));
            x->f_encoder->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        cblas_scopy(sampleframes, x->f_sig_outs+i, numouts, outs[i], 1);
    }
}

