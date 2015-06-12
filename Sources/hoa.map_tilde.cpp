/*
 // Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../hoa.library.hpp"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct _hoa_map_tilde
{
    t_edspobj                       f_obj;
    Encoder<Hoa2d, t_sample>::Multi*f_map;
    PolarLines<Hoa2d, t_sample>*    f_lines;
    t_sample*                       f_sig_ins;
    t_sample*                       f_sig_outs;
    t_sample*                       f_lines_vector;
    float                           f_ramp;
    int                             f_mode;
} t_hoa_map_tilde;

static t_eclass *hoa_map_tilde_class;

typedef struct _hoa_map_3d_tilde
{
    t_edspobj                       f_obj;
    Encoder<Hoa3d, t_sample>::Multi*f_map;
    PolarLines<Hoa3d, t_sample>*    f_lines;
    t_sample*                       f_sig_ins;
    t_sample*                       f_sig_outs;
    t_sample*                       f_lines_vector;
    float                           f_ramp;
    int                             f_mode;
} t_hoa_map_3d_tilde;

static t_eclass *hoa_map_3d_tilde_class;

static void *hoa_map_tilde_new(t_symbol *s, long argc, t_atom *argv)
{
    ulong order = 1;
    ulong numberOfSources = 1;
    t_hoa_map_tilde *x  = (t_hoa_map_tilde *)eobj_new(hoa_map_tilde_class);
    t_binbuf *d         = binbuf_via_atoms(argc,argv);

	if(x && d)
	{
		if(atom_gettype(argv) == A_LONG)
			order = ulong(pd_clip_min(atom_getlong(argv), 1));
        if(argc > 1 && atom_gettype(argv+1) == A_LONG)
            numberOfSources = ulong(pd_clip_minmax(atom_getlong(argv+1), 1, 255));

        if(argc > 2 && atom_gettype(argv+2) == A_SYM)
        {
            if(atom_getsym(argv+2) == hoa_sym_car || atom_getsym(argv+2) == hoa_sym_cartesian)
                x->f_mode = 1;
            else
                x->f_mode = 0;
        }
        else
            x->f_mode = 0;

        x->f_ramp       = 100;
		x->f_map        = new Encoder<Hoa2d, t_sample>::Multi(order, numberOfSources);
		x->f_lines      = new PolarLines<Hoa2d, t_sample>(x->f_map->getNumberOfSources());
        x->f_lines->setRamp(0.1 * sys_getsr());

        for(ulong i = 0; i < x->f_map->getNumberOfSources(); i++)
        {
            x->f_lines->setRadiusDirect(i, 1);
            x->f_lines->setAzimuthDirect(i, 0.);
        }

		if(x->f_map->getNumberOfSources() == 1)
            eobj_dspsetup(x, 3, long(x->f_map->getNumberOfHarmonics()));
        else
            eobj_dspsetup(x, long(x->f_map->getNumberOfSources()), long(x->f_map->getNumberOfHarmonics()));

        if(x->f_map->getNumberOfSources() == 1)
            x->f_sig_ins    = new t_sample[3 * HOA_MAXBLKSIZE];
        else
            x->f_sig_ins    = new t_sample[x->f_map->getNumberOfSources() * HOA_MAXBLKSIZE];

        x->f_sig_outs       = new t_sample[x->f_map->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        x->f_lines_vector   = new t_sample[x->f_map->getNumberOfSources() * 2];

        ebox_attrprocess_viabinbuf(x, d);

        return x;
	}

	return NULL;
}

static void hoa_map_tilde_float(t_hoa_map_tilde *x, float f)
{
    if(x->f_map->getNumberOfSources() == 1)
    {
		if(x->f_mode == 0)
		{
			if(eobj_getproxy((t_object *)x) == 1)
			{
				x->f_lines->setRadius(0, pd_clip_min(f, 0.));
			}
			else if(eobj_getproxy((t_object *)x) == 2)
			{
				x->f_lines->setAzimuth(0, f);
			}
		}
		else if(x->f_mode == 1)
		{
			if(eobj_getproxy((t_object *)x) == 1)
			{
                float ord = Math<float>::ordinate(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0));
				x->f_lines->setRadius(0, Math<float>::radius(f, ord));
                x->f_lines->setAzimuth(0, Math<float>::azimuth(f, ord));
			}
			else if(eobj_getproxy((t_object *)x) == 2)
			{
				float abs = Math<float>::abscissa(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0));
                x->f_lines->setRadius(0, Math<float>::radius(abs, f));
				x->f_lines->setAzimuth(0, Math<float>::azimuth(abs, f));
			}
		}
    }
}

static void hoa_map_tilde_list(t_hoa_map_tilde *x, t_symbol* s, long argc, t_atom* argv)
{
    if(argc > 2 && argv && atom_gettype(argv) == A_LONG && atom_gettype(argv+1) == A_SYM)
    {
        int index = atom_getlong(argv);
        if(index < 1 || (ulong)index > x->f_map->getNumberOfSources())
            return;

        if(argc > 3 && (atom_getsym(argv+1) == hoa_sym_polar || atom_getsym(argv+1) == hoa_sym_pol))
        {
            x->f_lines->setRadius(ulong(index-1), atom_getfloat(argv+2));
            x->f_lines->setAzimuth(ulong(index-1), atom_getfloat(argv+3));
        }
        else if(argc > 3 && (atom_getsym(argv+1) == hoa_sym_cartesian || atom_getsym(argv+1) == hoa_sym_car))
        {
            x->f_lines->setRadius(ulong(index-1), Math<float>::radius(atom_getfloat(argv+2), atom_getfloat(argv+3)));
            x->f_lines->setAzimuth(ulong(index-1), Math<float>::azimuth(atom_getfloat(argv+2), atom_getfloat(argv+3)));
        }
        else if(argc > 2 && atom_getsym(argv+1) == hoa_sym_mute)
        {
            x->f_map->setMute(ulong(index-1), atom_getlong(argv+2));
        }
    }
}

static t_pd_err hoa_map_tilde_ramp_set(t_hoa_map_tilde *x, t_object *attr, long argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_LONG || atom_gettype(argv) == A_FLOAT)
        {
            x->f_ramp = pd_clip_min(atom_getfloat(argv), 0);
            x->f_lines->setRamp(x->f_ramp / 1000. * sys_getsr());
        }
    }
    return 0;
}

static void hoa_map_tilde_perform_multisources(t_hoa_map_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
	ulong nsources = x->f_map->getNumberOfSources();
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_sig_ins+i, ulong(numins));
    }
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_lines->process(x->f_lines_vector);
		for(ulong j = 0; j < nsources; j++)
			x->f_map->setRadius(j, x->f_lines_vector[j]);
        for(ulong j = 0; j < nsources; j++)
			x->f_map->setAzimuth(j, x->f_lines_vector[j + nsources]);

        x->f_map->process(x->f_sig_ins + numins * i, x->f_sig_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_tilde_perform(t_hoa_map_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < sampleframes; i++)
    {
		x->f_lines->process(x->f_lines_vector);
		x->f_map->setRadius(0, x->f_lines_vector[0]);
		x->f_map->setAzimuth(0, x->f_lines_vector[1]);
        x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_tilde_perform_in1(t_hoa_map_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setRadius(0, ins[1][i]);
            x->f_map->setAzimuth(0, x->f_lines_vector[1]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setAzimuth(0, Math<float>::azimuth(ins[1][i], x->f_lines_vector[1]));
            x->f_map->setRadius(0, Math<float>::radius(ins[1][i], x->f_lines_vector[1]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_tilde_perform_in2(t_hoa_map_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setRadius(0, x->f_lines_vector[0]);
            x->f_map->setAzimuth(0, ins[2][i]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setAzimuth(0, Math<float>::azimuth(x->f_lines_vector[0], ins[2][i]));
            x->f_map->setRadius(0, Math<float>::radius(x->f_lines_vector[0], ins[2][i]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_tilde_perform_in1_in2(t_hoa_map_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_map->setRadius(0, ins[1][i]);
            x->f_map->setAzimuth(0, ins[2][i]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_map->setAzimuth(0, Math<float>::azimuth(ins[1][i], ins[2][i]));
            x->f_map->setRadius(0, Math<float>::radius(ins[1][i], ins[2][i]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);

        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_tilde_dsp(t_hoa_map_tilde *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    x->f_lines->setRamp(x->f_ramp / 1000. * samplerate);
    if(x->f_map->getNumberOfSources() == 1)
    {
        if(count[1] && count[2])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_tilde_perform_in1_in2, 0, NULL);
        else if(count[1] && !count[2])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_tilde_perform_in1, 0, NULL);
        else if(!count[1] && count[2])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_tilde_perform_in2, 0, NULL);
        else if(!count[1] && !count[2])
            object_method(dsp, gensym("dsp_add"), x, (method)hoa_map_tilde_perform, 0, NULL);
    }
    else
    {
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_map_tilde_perform_multisources, 0, NULL);
    }
}

static void hoa_map_tilde_free(t_hoa_map_tilde *x)
{
	eobj_dspfree(x);
	delete x->f_lines;
	delete x->f_map;
    delete [] x->f_sig_ins;
    delete [] x->f_sig_outs;
	delete [] x->f_lines_vector;
}

extern "C" void setup_hoa0x2e2d0x2emap_tilde(void)
{
    t_eclass* c;

    c = eclass_new("hoa.2d.map~", (method)hoa_map_tilde_new, (method)hoa_map_tilde_free, (short)sizeof(t_hoa_map_tilde), 0L, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_map_tilde_new, gensym("hoa.map~"), A_GIMME, 0);

    eclass_dspinit(c);
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_map_tilde_dsp,          "dsp",      A_CANT, 0);
    eclass_addmethod(c, (method)hoa_map_tilde_list,         "list",     A_GIMME, 0);
    eclass_addmethod(c, (method)hoa_map_tilde_float,        "float",    A_FLOAT, 0);

    CLASS_ATTR_FLOAT            (c, "ramp", 0, t_hoa_map_tilde, f_ramp);
    CLASS_ATTR_CATEGORY			(c, "ramp", 0, "Behavior");
    CLASS_ATTR_LABEL			(c, "ramp", 0, "Ramp Time (ms)");
    CLASS_ATTR_ORDER			(c, "ramp", 0, "2");
    CLASS_ATTR_ACCESSORS		(c, "ramp", NULL, hoa_map_tilde_ramp_set);
    CLASS_ATTR_SAVE				(c, "ramp", 1);

    eclass_register(CLASS_OBJ, c);
    hoa_map_tilde_class = c;
}

static void *hoa_map_3d_tilde_new(t_symbol *s, long argc, t_atom *argv)
{
    ulong order = 1;
    ulong numberOfSources = 1;
    t_hoa_map_3d_tilde *x = (t_hoa_map_3d_tilde *)eobj_new(hoa_map_3d_tilde_class);
    t_binbuf *d           = binbuf_via_atoms(argc,argv);
    if(x && d)
    {
        if(atom_gettype(argv) == A_LONG)
            order = ulong(pd_clip_min(atom_getlong(argv), 1));
        if(argc > 1 && atom_gettype(argv+1) == A_LONG)
            numberOfSources = ulong(pd_clip_minmax(atom_getlong(argv+1), 1, 255));

        if(argc > 2 && atom_gettype(argv+2) == A_SYM)
        {
            if(atom_getsym(argv+2) == gensym("car") || atom_getsym(argv+2) == gensym("cartesian"))
                x->f_mode = 1;
            else
                x->f_mode = 0;
        }
        else
            x->f_mode = 0;

        x->f_ramp       = 100;
        x->f_map        = new Encoder<Hoa3d, t_sample>::Multi(order, numberOfSources);
        x->f_lines      = new PolarLines<Hoa3d, t_sample>(x->f_map->getNumberOfSources());
        x->f_lines->setRamp(0.1 * sys_getsr());

        for(ulong i = 0; i < x->f_map->getNumberOfSources(); i++)
        {
            x->f_lines->setRadiusDirect(i, 1);
            x->f_lines->setAzimuthDirect(i, 0.);
            x->f_lines->setElevationDirect(i, 0.);
        }

        if(x->f_map->getNumberOfSources() == 1)
            eobj_dspsetup(x, 4, long(x->f_map->getNumberOfHarmonics()));
        else
            eobj_dspsetup(x, long(x->f_map->getNumberOfSources()), long(x->f_map->getNumberOfHarmonics()));

        if(x->f_map->getNumberOfSources() == 1)
            x->f_sig_ins    = new t_sample[4 * HOA_MAXBLKSIZE];
        else
            x->f_sig_ins    = new t_sample[x->f_map->getNumberOfSources() * HOA_MAXBLKSIZE];

        x->f_sig_outs       = new t_sample[x->f_map->getNumberOfHarmonics() * HOA_MAXBLKSIZE];
        x->f_lines_vector   = new t_sample[x->f_map->getNumberOfSources() * 3];

        ebox_attrprocess_viabinbuf(x, d);

        return x;
    }

    return NULL;
}

static void hoa_map_3d_tilde_float(t_hoa_map_3d_tilde *x, float f)
{
    if(x->f_map->getNumberOfSources() == 1)
    {
        if(x->f_mode == 0)
        {
            if(eobj_getproxy((t_object *)x) == 1)
            {
                x->f_lines->setRadius(0, pd_clip_min(f, 0.));
            }
            else if(eobj_getproxy((t_object *)x) == 2)
            {
                x->f_lines->setAzimuth(0, f);
            }
            else if(eobj_getproxy((t_object *)x) == 3)
            {
                x->f_lines->setElevation(0, f);
            }
        }
        else if(x->f_mode == 1)
        {
            if(eobj_getproxy((t_object *)x) == 1)
            {
                float abs = f;
                float ord = Math<float>::ordinate(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0), x->f_lines->getElevation(0));
                float hei = Math<float>::height(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0), x->f_lines->getElevation(0));
                x->f_lines->setRadius(0, Math<float>::radius(abs, ord, hei));
                x->f_lines->setAzimuth(0, Math<float>::azimuth(abs, ord, hei));
                x->f_lines->setElevation(0, Math<float>::elevation(abs, ord, hei));
            }
            else if(eobj_getproxy((t_object *)x) == 2)
            {
                float abs = Math<float>::abscissa(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0), x->f_lines->getElevation(0));
                float ord = f;
                float hei = Math<float>::height(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0), x->f_lines->getElevation(0));
                x->f_lines->setRadius(0, Math<float>::radius(abs, ord, hei));
                x->f_lines->setAzimuth(0, Math<float>::azimuth(abs, ord, hei));
                x->f_lines->setElevation(0, Math<float>::elevation(abs, ord, hei));
            }
            else if(eobj_getproxy((t_object *)x) == 3)
            {
                float abs = Math<float>::abscissa(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0), x->f_lines->getElevation(0));
                float ord = Math<float>::ordinate(x->f_lines->getRadius(0), x->f_lines->getAzimuth(0), x->f_lines->getElevation(0));
                float hei = f;
                x->f_lines->setRadius(0, Math<float>::radius(abs, ord, hei));
                x->f_lines->setAzimuth(0, Math<float>::azimuth(abs, ord, hei));
                x->f_lines->setElevation(0, Math<float>::elevation(abs, ord, hei));
            }
        }
    }
}

static void hoa_map_3d_tilde_list(t_hoa_map_3d_tilde *x, t_symbol* s, long argc, t_atom* argv)
{
    if(argc > 2 && argv && atom_gettype(argv) == A_LONG && atom_gettype(argv+1) == A_SYM)
    {
        ulong index = ulong(atom_getlong(argv));
        if(index < 1 || (ulong)index > x->f_map->getNumberOfSources())
            return;

        if(argc > 4 && (atom_getsym(argv+1) == hoa_sym_polar || atom_getsym(argv+1) == hoa_sym_pol))
        {
            x->f_lines->setRadius(index-1, atom_getfloat(argv+2));
            x->f_lines->setAzimuth(index-1, atom_getfloat(argv+3));
            x->f_lines->setElevation(index-1, atom_getfloat(argv+4));
        }
        else if(argc > 4 && (atom_getsym(argv+1) == hoa_sym_cartesian || atom_getsym(argv+1) == hoa_sym_car))
        {
            x->f_lines->setRadius(index-1, Math<float>::radius(atom_getfloat(argv+2), atom_getfloat(argv+3), atom_getfloat(argv+4)));
            x->f_lines->setAzimuth(index-1, Math<float>::azimuth(atom_getfloat(argv+2), atom_getfloat(argv+3), atom_getfloat(argv+4)));
            x->f_lines->setElevation(index-1, Math<float>::elevation(atom_getfloat(argv+2), atom_getfloat(argv+3), atom_getfloat(argv+4)));
        }
        else if(argc > 2 && atom_getsym(argv+1) == hoa_sym_mute)
        {
            x->f_map->setMute(index-1, atom_getlong(argv+2));
        }
    }
}

static t_pd_err hoa_map_3d_tilde_ramp_set(t_hoa_map_3d_tilde *x, t_object *attr, long argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_LONG || atom_gettype(argv) == A_FLOAT)
        {
            x->f_ramp = pd_clip_min(atom_getfloat(argv), 0);
            x->f_lines->setRamp(x->f_ramp / 1000. * sys_getsr());
        }
    }

    return 0;
}

static void hoa_map_3d_tilde_perform_in1_in2_in3(t_hoa_map_3d_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_map->setRadius(0, ins[1][i]);
            x->f_map->setAzimuth(0, ins[2][i]);
            x->f_map->setElevation(0, ins[3][i]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_map->setAzimuth(0, Math<t_sample>::azimuth(ins[1][i], ins[2][i], ins[3][i]));
            x->f_map->setRadius(0, Math<t_sample>::radius(ins[1][i], ins[2][i], ins[3][i]));
            x->f_map->setElevation(0, Math<t_sample>::elevation(ins[1][i], ins[2][i], ins[3][i]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_3d_tilde_perform_in1_in2(t_hoa_map_3d_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setRadius(0, ins[1][i]);
            x->f_map->setAzimuth(0, ins[2][i]);
            x->f_map->setElevation(0, x->f_lines_vector[2]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setAzimuth(0, Math<t_sample>::azimuth(ins[1][i], ins[2][i], x->f_lines_vector[2]));
            x->f_map->setRadius(0, Math<t_sample>::radius(ins[1][i], ins[2][i], x->f_lines_vector[2]));
            x->f_map->setElevation(0, Math<t_sample>::elevation(ins[1][i], ins[2][i], x->f_lines_vector[2]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_3d_tilde_perform_in1_in3(t_hoa_map_3d_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setRadius(0, ins[1][i]);
            x->f_map->setAzimuth(0, x->f_lines_vector[1]);
            x->f_map->setElevation(0, ins[3][i]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setAzimuth(0, Math<t_sample>::azimuth(ins[1][i], x->f_lines_vector[1], ins[3][i]));
            x->f_map->setRadius(0, Math<t_sample>::radius(ins[1][i], x->f_lines_vector[1], ins[3][i]));
            x->f_map->setElevation(0, Math<t_sample>::elevation(ins[1][i], x->f_lines_vector[1], ins[3][i]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

/*
static void hoa_map_3d_tilde_perform_in2_in3(t_hoa_map_3d_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setRadius(0, x->f_lines_vector[0]);
            x->f_map->setAzimuth(0, ins[2][i]);
            x->f_map->setElevation(0, ins[3][i]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setAzimuth(0, Math<t_sample>::azimuth(x->f_lines_vector[0], ins[2][i], ins[3][i]));
            x->f_map->setRadius(0, Math<t_sample>::radius(x->f_lines_vector[0], ins[2][i], ins[3][i]));
            x->f_map->setElevation(0, Math<t_sample>::elevation(x->f_lines_vector[0], ins[2][i], ins[3][i]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}
 */

static void hoa_map_3d_tilde_perform_in1(t_hoa_map_3d_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setRadius(0, ins[1][i]);
            x->f_map->setAzimuth(0, x->f_lines_vector[1]);
            x->f_map->setElevation(0, x->f_lines_vector[2]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setAzimuth(0, Math<t_sample>::azimuth(ins[1][i], x->f_lines_vector[1], x->f_lines_vector[2]));
            x->f_map->setRadius(0, Math<t_sample>::radius(ins[1][i], x->f_lines_vector[1], x->f_lines_vector[2]));
            x->f_map->setElevation(0, Math<t_sample>::elevation(ins[1][i], x->f_lines_vector[1], x->f_lines_vector[2]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_3d_tilde_perform_in2(t_hoa_map_3d_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setRadius(0, x->f_lines_vector[0]);
            x->f_map->setAzimuth(0, ins[2][i]);
            x->f_map->setElevation(0, x->f_lines_vector[2]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setAzimuth(0, Math<t_sample>::azimuth(x->f_lines_vector[0], ins[2][i], x->f_lines_vector[2]));
            x->f_map->setRadius(0, Math<t_sample>::radius(x->f_lines_vector[0], ins[2][i], x->f_lines_vector[2]));
            x->f_map->setElevation(0, Math<t_sample>::elevation(x->f_lines_vector[0], ins[2][i], x->f_lines_vector[2]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_3d_tilde_perform_in3(t_hoa_map_3d_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    if(!x->f_mode)
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setRadius(0, x->f_lines_vector[0]);
            x->f_map->setAzimuth(0, x->f_lines_vector[1]);
            x->f_map->setElevation(0, ins[3][i]);
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    else
    {
        for(long i = 0; i < sampleframes; i++)
        {
            x->f_lines->process(x->f_lines_vector);
            x->f_map->setAzimuth(0, Math<t_sample>::azimuth(x->f_lines_vector[0], x->f_lines_vector[1], ins[3][i]));
            x->f_map->setRadius(0, Math<t_sample>::radius(x->f_lines_vector[0], x->f_lines_vector[1], ins[3][i]));
            x->f_map->setElevation(0, Math<t_sample>::elevation(x->f_lines_vector[0], x->f_lines_vector[1], ins[3][i]));
            x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
        }
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_3d_tilde_perform(t_hoa_map_3d_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_lines->process(x->f_lines_vector);
        x->f_map->setRadius(0, x->f_lines_vector[0]);
        x->f_map->setAzimuth(0, x->f_lines_vector[1]);
        x->f_map->setElevation(0, x->f_lines_vector[2]);
        x->f_map->process(&ins[0][i], x->f_sig_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_3d_tilde_perform_multisources(t_hoa_map_3d_tilde *x, t_object *dsp64, t_sample **ins, long numins, t_sample **outs, long numouts, long sampleframes, long flags, void *userparam)
{
    ulong nsources = x->f_map->getNumberOfSources();
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_sig_ins+i, ulong(numins));
    }
    for(long i = 0; i < sampleframes; i++)
    {
        x->f_lines->process(x->f_lines_vector);
        for(ulong j = 0; j < nsources; j++)
            x->f_map->setRadius(j, x->f_lines_vector[j]);
        for(ulong j = 0; j < nsources; j++)
            x->f_map->setAzimuth(j, x->f_lines_vector[j + nsources]);
        for(ulong j = 0; j < nsources; j++)
            x->f_map->setElevation(j, x->f_lines_vector[j + nsources * 2]);

        x->f_map->process(x->f_sig_ins + numins * i, x->f_sig_outs + numouts * i);
    }
    for(long i = 0; i < numouts; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), x->f_sig_outs+i, ulong(numouts), outs[i], 1);
    }
}

static void hoa_map_3d_tilde_dsp(t_hoa_map_3d_tilde *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    x->f_lines->setRamp(x->f_ramp / 1000. * samplerate);

    if(x->f_map->getNumberOfSources() == 1)
    {
        if(count[1] && count[2] && count[3])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_3d_tilde_perform_in1_in2_in3, 0, NULL);
        else if(count[1] && count[2] && !count[3])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_3d_tilde_perform_in1_in2, 0, NULL);
        else if(count[1] && !count[2] && count[3])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_3d_tilde_perform_in1_in3, 0, NULL);
        else if(!count[1] && count[2] && count[3])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_3d_tilde_perform_in1_in3, 0, NULL);
        else if(count[1] && !count[2])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_3d_tilde_perform_in1, 0, NULL);
        else if(!count[1] && count[2])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_3d_tilde_perform_in2, 0, NULL);
        else if(!count[1] && count[2])
            object_method(dsp, gensym("dsp_add64"), x, (method)hoa_map_3d_tilde_perform_in3, 0, NULL);
        else if(!count[1] && !count[2] && !count[3])
            object_method(dsp, gensym("dsp_add"), x, (method)hoa_map_3d_tilde_perform, 0, NULL);
    }
    else
    {
        object_method(dsp, gensym("dsp_add"), x, (method)hoa_map_3d_tilde_perform_multisources, 0, NULL);
    }
}

static void hoa_map_3d_tilde_free(t_hoa_map_3d_tilde *x)
{
    eobj_dspfree(x);
    delete x->f_lines;
    delete x->f_map;
    delete [] x->f_sig_ins;
    delete [] x->f_sig_outs;
    delete [] x->f_lines_vector;
}

extern "C" void setup_hoa0x2e3d0x2emap_tilde(void)
{
    t_eclass* c;

    c = eclass_new("hoa.3d.map~", (method)hoa_map_3d_tilde_new, (method)hoa_map_3d_tilde_free, (short)sizeof(t_hoa_map_3d_tilde), CLASS_NOINLET, A_GIMME, 0);

    eclass_dspinit(c);
    hoa_initclass(c);
    eclass_addmethod(c, (method)hoa_map_3d_tilde_dsp,          "dsp",      A_CANT, 0);
    eclass_addmethod(c, (method)hoa_map_3d_tilde_list,         "list",     A_GIMME, 0);
    eclass_addmethod(c, (method)hoa_map_3d_tilde_float,        "float",    A_FLOAT, 0);

    CLASS_ATTR_DOUBLE           (c, "ramp", 0, t_hoa_map_3d_tilde, f_ramp);
    CLASS_ATTR_CATEGORY			(c, "ramp", 0, "Behavior");
    CLASS_ATTR_LABEL			(c, "ramp", 0, "Ramp Time (ms)");
    CLASS_ATTR_ORDER			(c, "ramp", 0, "2");
    CLASS_ATTR_ACCESSORS		(c, "ramp", NULL, hoa_map_3d_tilde_ramp_set);
    CLASS_ATTR_SAVE				(c, "ramp", 1);

    eclass_register(CLASS_OBJ, c);
    hoa_map_3d_tilde_class = c;
}
