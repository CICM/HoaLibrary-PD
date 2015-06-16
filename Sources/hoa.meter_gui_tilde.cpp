/*
 // Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
 // For information on usage and redistribution, and for a DISCLAIMER OF ALL
 // WARRANTIES, see the file, "LICENSE.txt," in this distribution.
 */

#include "../hoa.library.hpp"
#include "../ThirdParty/HoaLibrary/Sources/Hoa.hpp"
using namespace hoa;

typedef struct  _hoa_meter
{
	t_edspbox               f_box;
    Meter<Hoa2d, t_sample>* f_meter;
    Vector<Hoa2d, t_sample>*f_vector;
    t_sample*               f_signals;
    t_sample                f_vector_coords[4];
    long                    f_ramp;
	int                     f_startclock;
	long                    f_interval;

    t_symbol*               f_vector_type;
    t_symbol*               f_clockwise;

    t_rgba                  f_color_bg;
    t_rgba                  f_color_bd;
	t_rgba                  f_color_cold_signal;
	t_rgba                  f_color_tepid_signal;
	t_rgba                  f_color_warm_signal;
	t_rgba                  f_color_hot_signal;
	t_rgba                  f_color_over_signal;
	t_rgba                  f_color_energy_vector;
	t_rgba                  f_color_velocity_vector;

    double                  f_radius;
    double                  f_center;
	double                  f_radius_center;

	t_clock*                f_clock;
    void*                   f_attrs;

} t_hoa_meter;

static t_eclass *hoa_meter_class;

static void hoa_meter_getdrawparams(t_hoa_meter *x, t_object *patcherview, t_edrawparams *params)
{
    params->d_boxfillcolor = x->f_color_bg;
    params->d_bordercolor = x->f_color_bd;
    params->d_borderthickness = 1;
    params->d_cornersize = 8;
}


static void hoa_meter_oksize(t_hoa_meter *x, t_rect *newrect)
{
    newrect->width = pd_clip_min(newrect->width, 20.);
    newrect->height = pd_clip_min(newrect->height, 20.);
}

static t_pd_err channels_get(t_hoa_meter *x, void *attr, int* argc, t_atom **argv)
{
    argc[0] = 1;
    argv[0] = (t_atom *)malloc(sizeof(t_atom));
    if(argv[0] && argc[0])
    {
        atom_setfloat(argv[0], x->f_meter->getNumberOfPlanewaves());
    }
    else
    {
        argc[0] = 0;
        argv[0] = NULL;
    }
    return 0;
}

static t_pd_err channels_set(t_hoa_meter *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_FLOAT)
        {
            ulong d = pd_clip_minmax(atom_getfloat(argv), 1, HOA_MAX_PLANEWAVES);
            if(d != x->f_meter->getNumberOfPlanewaves())
            {
                int dspState = canvas_suspend_dsp();
                delete x->f_meter;
                x->f_meter = new Meter<Hoa2d, t_sample>(d);
                delete x->f_vector;
                x->f_vector = new Vector<Hoa2d, t_sample>(d);

                x->f_meter->computeRendering();
                x->f_vector->computeRendering();
                eobj_resize_inputs((t_ebox *)x, long(x->f_meter->getNumberOfPlanewaves()));

                ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
                ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
                ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
                ebox_redraw((t_ebox *)x);
                canvas_resume_dsp(dspState);
            }
        }
    }
    return 0;
}

static t_pd_err angles_get(t_hoa_meter *x, void *attr, int* argc, t_atom **argv)
{
    argc[0] = long(x->f_meter->getNumberOfPlanewaves());
    argv[0] = (t_atom *)malloc(sizeof(t_atom) * x->f_meter->getNumberOfPlanewaves());
    if(argv[0] && argc[0])
    {
        for(ulong i = 0; i < x->f_meter->getNumberOfPlanewaves(); i++)
        {
            atom_setfloat(argv[0]+i, x->f_meter->getPlanewaveAzimuth(i, false) / HOA_2PI * 360.);
        }
    }
    else
    {
        argc[0] = 0;
        argv[0] = NULL;
    }
    return 0;
}

static t_pd_err angles_set(t_hoa_meter *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        for(long i = 0; i < argc && (ulong)i < x->f_meter->getNumberOfPlanewaves(); i++)
        {
            if(atom_gettype(argv+i) == A_FLOAT)
            {
                x->f_meter->setPlanewaveAzimuth(ulong(i), atom_getfloat(argv+i) / 360.f * HOA_2PI);
                x->f_vector->setPlanewaveAzimuth(ulong(i), atom_getfloat(argv+i) / 360.f * HOA_2PI);
            }
        }

        x->f_meter->computeRendering();
        x->f_vector->computeRendering();

        ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
        ebox_redraw((t_ebox *)x);
    }
    return 0;
}

static t_pd_err offset_get(t_hoa_meter *x, void *attr, int* argc, t_atom **argv)
{
    argc[0] = 1;
    argv[0] = (t_atom *)malloc(sizeof(t_atom));
    if(argv[0] && argc[0])
    {
        atom_setfloat(argv[0], x->f_meter->getPlanewavesRotationZ() / HOA_2PI * 360.);
    }
    else
    {
        argc[0] = 0;
        argv[0] = NULL;
    }
    return 0;
}

static t_pd_err offset_set(t_hoa_meter *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv && atom_gettype(argv) == A_FLOAT)
    {
        x->f_vector->setPlanewavesRotation(0., 0., atom_getfloat(argv) / 360 * HOA_2PI);
        x->f_meter->setPlanewavesRotation(0., 0., atom_getfloat(argv) / 360 * HOA_2PI);
        x->f_vector->computeRendering();
        x->f_meter->computeRendering();

        ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
        ebox_redraw((t_ebox *)x);
    }
    return 0;
}

static t_pd_err vectors_set(t_hoa_meter *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_SYM)
        {
            if(atom_getsym(argv) == hoa_sym_energy)
                x->f_vector_type = hoa_sym_energy;
            else if(atom_getsym(argv) == hoa_sym_velocity)
                x->f_vector_type = hoa_sym_velocity;
            else if(atom_getsym(argv) == hoa_sym_both)
                x->f_vector_type = hoa_sym_both;
            else
                x->f_vector_type = hoa_sym_none;
        }
        else if(atom_gettype(argv) == A_FLOAT)
        {
            if(atom_getlong(argv) == 1)
                x->f_vector_type = hoa_sym_energy;
            else if(atom_getlong(argv) == 2)
                x->f_vector_type = hoa_sym_velocity;
            else if(atom_getlong(argv) == 3)
                x->f_vector_type = hoa_sym_both;
            else
                x->f_vector_type = hoa_sym_none;
        }
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
        ebox_redraw((t_ebox *)x);
    }
    return 0;
}


static t_pd_err rotation_set(t_hoa_meter *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_SYM)
        {
            if(atom_getsym(argv) == hoa_sym_clockwise)
                x->f_clockwise = hoa_sym_clockwise;
            else
                x->f_clockwise = hoa_sym_anticlock;
        }
        else if(atom_gettype(argv) == A_FLOAT)
        {
            if(atom_getlong(argv) == 1)
                x->f_clockwise = hoa_sym_clockwise;
            else
                x->f_clockwise = hoa_sym_anticlock;
        }

        ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
        ebox_redraw((t_ebox *)x);
    }
    return 0;
}

static void hoa_meter_perform(t_hoa_meter *x, t_object *dsp, t_sample **ins, long numins, t_sample **outs, long no, long sampleframes, long f,void *up)
{
    for(long i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_signals+i, ulong(numins));
    }
    for(x->f_ramp = 0; x->f_ramp < sampleframes; x->f_ramp++)
    {
        x->f_meter->process(x->f_signals + numins * x->f_ramp);
    }
    if(x->f_startclock)
    {
        x->f_startclock = 0;
        clock_delay(x->f_clock,0);
    }
}


static void hoa_meter_tick(t_hoa_meter *x)
{
    if(x->f_vector_type == hoa_sym_both)
        x->f_vector->process(x->f_signals, x->f_vector_coords);
    else if(x->f_vector_type == hoa_sym_velocity)
        x->f_vector->processVelocity(x->f_signals, x->f_vector_coords);
    else if(x->f_vector_type == hoa_sym_energy)
        x->f_vector->processEnergy(x->f_signals, x->f_vector_coords + 2);

    x->f_meter->tick(ulong(1000 / x->f_interval));
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
    ebox_redraw((t_ebox *)x);

    if (canvas_dspstate)
        clock_delay(x->f_clock, x->f_interval);
}

static void hoa_meter_dsp(t_hoa_meter *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    x->f_meter->setVectorSize(ulong(maxvectorsize));
    object_method(dsp, gensym("dsp_add"), x, (method)hoa_meter_perform, 0, NULL);
    x->f_startclock = 1;
}

static void draw_background(t_hoa_meter *x,  t_object *view, t_rect *rect)
{
    float coso, sino, angle, x1, y1, x2, y2;
    t_rgba black, white;
    t_matrix transform;
    t_elayer *g = ebox_start_layer((t_ebox *)x, hoa_sym_background_layer, rect->width, rect->height);

    black = rgba_addContrast(x->f_color_bg, -HOA_CONTRAST_DARKER);
    white = rgba_addContrast(x->f_color_bg, HOA_CONTRAST_LIGHTER);

    if (g)
    {
        egraphics_matrix_init(&transform, 1, 0, 0, -1, rect->width * .5, rect->width * .5);
        egraphics_set_matrix(g, &transform);
        egraphics_rotate(g, -HOA_PI2);

        egraphics_set_line_width(g, 1.);

        egraphics_set_color_rgba(g, &white);
        egraphics_set_line_width(g, 1.f);
        egraphics_circle(g, 1, 1, x->f_radius);
        egraphics_stroke(g);
        egraphics_circle(g, 1, 1, x->f_radius_center);
        egraphics_stroke(g);

        egraphics_set_color_rgba(g, &black);
        egraphics_set_line_width(g, 1.f);
        egraphics_circle(g, 0.f, 0.f, x->f_radius);
        egraphics_stroke(g);
        egraphics_circle(g, 0.f, 0.f, x->f_radius_center);
        egraphics_stroke(g);

        if(x->f_meter->getNumberOfPlanewaves() != 1)
        {
            for(ulong i = 0; i < x->f_meter->getNumberOfPlanewaves(); i++)
            {
                angle = x->f_meter->getPlanewaveAzimuthMapped(i) - x->f_meter->getPlanewaveWidth(i) * 0.5f;
                if(x->f_clockwise == hoa_sym_clockwise)
                    angle = -angle;

                egraphics_set_line_width(g, 1.f);

                coso = cosf(angle);
                sino = sinf(angle);
                x1 = x->f_radius_center * coso;
                y1 = x->f_radius_center * sino;
                x2 = x->f_radius * coso;
                y2 = x->f_radius * sino;
                if(angle >= HOA_PI4 && angle <= HOA_PI + HOA_PI4)
                {
                    egraphics_move_to(g, x1 - 0.5, y1 - 0.5);
                    egraphics_line_to(g, x2 - 0.5, y2 - 0.5);
                }
                else
                {
                    egraphics_move_to(g, x1 + 0.5, y1 + 0.5);
                    egraphics_line_to(g, x2 + 0.5, y2 + 0.5);
                }

                egraphics_set_color_rgba(g, &white);
                egraphics_stroke(g);

                egraphics_move_to(g, x1, y1);
                egraphics_line_to(g, x2, y2);

                egraphics_set_color_rgba(g, &black);
                egraphics_stroke(g);
            }
        }
        ebox_end_layer((t_ebox*)x, hoa_sym_background_layer);
    }
    ebox_paint_layer((t_ebox *)x, hoa_sym_background_layer, 0., 0.);
}

static void draw_leds(t_hoa_meter *x, t_object *view, t_rect *rect)
{
    float angle, width;
    const float height = 0.49 * rect->width / 17.;
    const float lwidth = height - pd_clip_min(360. / rect->width, 2.);
    t_matrix transform;
    t_elayer *g = ebox_start_layer((t_ebox *)x,  hoa_sym_leds_layer, rect->width, rect->height);

    if(g)
    {
        t_rgba black = rgba_addContrast(x->f_color_bg, -HOA_CONTRAST_DARKER);
        egraphics_matrix_init(&transform, 1, 0, 0, -1, rect->width * .5, rect->width * .5);
        egraphics_set_matrix(g, &transform);
        egraphics_rotate(g, HOA_PI2);

        for(ulong i = 0; i < x->f_meter->getNumberOfPlanewaves(); i++)
        {
            width = x->f_meter->getPlanewaveWidth(i) * 0.5f;
            angle = x->f_meter->getPlanewaveAzimuthMapped(i);
            if(x->f_clockwise == hoa_sym_clockwise)
                egraphics_rotate(g, -angle);
            else
                egraphics_rotate(g, angle);

            float j = 12.f, dB = -39.f;
            while(j > 0)
            {
                float radius    = (j + 4.) * height;
                if(x->f_meter->getPlanewaveEnergy(i) > dB)
                {
                    if(j > 9)
                        egraphics_set_color_rgba(g, &x->f_color_cold_signal);
                    else if(j > 6)
                        egraphics_set_color_rgba(g, &x->f_color_tepid_signal);
                    else if(j > 3)
                        egraphics_set_color_rgba(g, &x->f_color_warm_signal);
                    else
                        egraphics_set_color_rgba(g, &x->f_color_hot_signal);

                    egraphics_set_line_width(g, lwidth);
                    egraphics_arc(g, 0., 0., radius,  width, 3.f * width);
                    egraphics_stroke(g);
                }
                else if(j)
                {
                    egraphics_set_color_rgba(g, &black);
                    egraphics_set_line_width(g, lwidth);
                    egraphics_arc(g, 0., 0., radius,  width, 3.f * width);
                    egraphics_stroke(g);
                }
                j--; dB += 3.;
            }
            if(x->f_meter->getPlanewaveOverLed(i))
            {
                egraphics_set_color_rgba(g, &x->f_color_over_signal);
                egraphics_set_line_width(g, lwidth);
                egraphics_arc(g, 0., 0., 4. * height,  width, 3.f * width);
                egraphics_stroke(g);
            }
            else
            {
                egraphics_set_color_rgba(g, &black);
                egraphics_set_line_width(g, lwidth);
                egraphics_arc(g, 0., 0., 4. * height,  width, 3.f * width);
                egraphics_stroke(g);
            }

            if(x->f_clockwise == hoa_sym_clockwise)
                egraphics_rotate(g, angle);
            else
                egraphics_rotate(g, -angle);
        }
        ebox_end_layer((t_ebox*)x,  hoa_sym_leds_layer);
    }
    ebox_paint_layer((t_ebox *)x, hoa_sym_leds_layer, 0., 0.);
}

static void draw_vectors(t_hoa_meter *x, t_object *view, t_rect *rect)
{
    double x1, y1, size;
    t_matrix transform;
    t_elayer *g = ebox_start_layer((t_ebox *)x,  hoa_sym_vector_layer, rect->width, rect->height);

    if(g)
    {
        egraphics_matrix_init(&transform, 1, 0, 0, -1, rect->width / 2., rect->width / 2.);
        egraphics_set_matrix(g, &transform);
        size = 1. / 64. * rect->width;

        if(x->f_vector_type == hoa_sym_both || x->f_vector_type == hoa_sym_energy)
        {
            egraphics_set_color_rgba(g, &x->f_color_energy_vector);
            if(x->f_clockwise == hoa_sym_anticlock)
            {
                x1 = x->f_vector_coords[2] * x->f_radius_center * 0.85;
                y1 = x->f_vector_coords[3] * x->f_radius_center * 0.85;
            }
            else
            {
                double rad = Math<float>::radius(x->f_vector_coords[2], x->f_vector_coords[3]) * x->f_radius_center * 0.85;
                double ang = -Math<float>::azimuth(x->f_vector_coords[2], x->f_vector_coords[3]);
                x1 = Math<float>::abscissa(rad, ang);
                y1 = Math<float>::ordinate(rad, ang);
            }
            egraphics_circle(g, x1, y1, size);
            egraphics_fill(g);
        }
        if(x->f_vector_type == hoa_sym_both || x->f_vector_type == hoa_sym_velocity)
        {
            egraphics_set_color_rgba(g, &x->f_color_velocity_vector);
            if(x->f_clockwise == hoa_sym_anticlock)
            {
                x1 = x->f_vector_coords[0] * x->f_radius_center * 0.85;
                y1 = x->f_vector_coords[1] * x->f_radius_center * 0.85;
            }
            else
            {
                double rad = Math<float>::radius(x->f_vector_coords[0], x->f_vector_coords[1]) * x->f_radius_center * 0.85;
                double ang = -Math<float>::azimuth(x->f_vector_coords[0], x->f_vector_coords[1]);
                x1 = Math<float>::abscissa(rad, ang);
                y1 = Math<float>::ordinate(rad, ang);
            }
            egraphics_circle(g, x1, y1, size);
            egraphics_fill(g);
        }

        ebox_end_layer((t_ebox*)x,  hoa_sym_vector_layer);
    }
    ebox_paint_layer((t_ebox *)x, hoa_sym_vector_layer, 0., 0.);
}

static void hoa_meter_paint(t_hoa_meter *x, t_object *view)
{
    t_rect rect;
    ebox_get_rect_for_view((t_ebox *)x, &rect);

    x->f_center = rect.width * .5;
    x->f_radius = x->f_center * 0.95;
    x->f_radius_center = x->f_radius / 5.;

    draw_leds(x, view, &rect);
    draw_vectors(x, view, &rect);
    draw_background(x, view, &rect);
}

static void hoa_meter_free(t_hoa_meter *x)
{
    ebox_free((t_ebox *)x);
    clock_free(x->f_clock);
    delete x->f_meter;
    delete x->f_vector;
    delete [] x->f_signals;
}

static void *hoa_meter_new(t_symbol *s, int argc, t_atom *argv)
{
    long flags;
    t_hoa_meter *x  = (t_hoa_meter *)eobj_new(hoa_meter_class);
    t_binbuf *d     = binbuf_via_atoms(argc, argv);

    if(x && d)
    {
        x->f_ramp = 0;
        x->f_meter  = new Meter<Hoa2d, t_sample>(4);
        x->f_vector = new Vector<Hoa2d, t_sample>(4);
        x->f_signals = new t_sample[HOA_MAX_PLANEWAVES * HOA_MAXBLKSIZE];
        x->f_meter->computeRendering();
        x->f_vector->computeRendering();

        x->f_clock = clock_new(x,(t_method)hoa_meter_tick);
        x->f_startclock = 0;
        eobj_dspsetup((t_ebox *)x, long(x->f_meter->getNumberOfPlanewaves()), 0);

        flags = 0
        | EBOX_GROWLINK
        | EBOX_IGNORELOCKCLICK
        ;

        ebox_new((t_ebox *)x, flags);
        ebox_attrprocess_viabinbuf(x, d);

        ebox_ready((t_ebox *)x);

        return x;
    }

    return NULL;
}

extern "C" void setup_hoa0x2e2d0x2emeter_tilde(void)
{
    t_eclass *c;

    c = eclass_new("hoa.2d.meter~", (method)hoa_meter_new, (method)hoa_meter_free, (short)sizeof(t_hoa_meter), CLASS_NOINLET, A_GIMME, 0);
    class_addcreator((t_newmethod)hoa_meter_new, gensym("hoa.meter~"), A_GIMME, 0);

    eclass_guiinit(c, 0);
    eclass_dspinit(c);

    eclass_addmethod(c, (method) hoa_meter_dsp,             "dsp",           A_CANT, 0);
    eclass_addmethod(c, (method) hoa_meter_paint,           "paint",		 A_CANT, 0);
    eclass_addmethod(c, (method) hoa_meter_getdrawparams,   "getdrawparams", A_CANT, 0);
    eclass_addmethod(c, (method) hoa_meter_oksize,          "oksize",        A_CANT, 0);

    CLASS_ATTR_INVISIBLE            (c, "fontname", 1);
    CLASS_ATTR_INVISIBLE            (c, "fontweight", 1);
    CLASS_ATTR_INVISIBLE            (c, "fontslant", 1);
    CLASS_ATTR_INVISIBLE            (c, "fontsize", 1);
    CLASS_ATTR_INVISIBLE            (c, "send", 1);
    CLASS_ATTR_DEFAULT              (c, "size", 0, "225. 225.");

    CLASS_ATTR_LONG                 (c, "channels", 0 , t_hoa_meter, f_attrs);
    CLASS_ATTR_ACCESSORS            (c, "channels", channels_get, channels_set);
    CLASS_ATTR_ORDER                (c, "channels", 0, "1");
    CLASS_ATTR_LABEL                (c, "channels", 0, "Number of Channels");
    CLASS_ATTR_SAVE                 (c, "channels", 1);
    CLASS_ATTR_DEFAULT              (c, "channels", 0, "8");
    CLASS_ATTR_STYLE                (c, "channels", 1, "number");

    CLASS_ATTR_FLOAT_VARSIZE        (c, "angles", 0, t_hoa_meter, f_attrs, f_attrs, HOA_MAX_PLANEWAVES);
    CLASS_ATTR_ACCESSORS            (c, "angles", angles_get, angles_set);
    CLASS_ATTR_ORDER                (c, "angles", 0, "2");
    CLASS_ATTR_LABEL                (c, "angles", 0, "Angles of Channels");
    CLASS_ATTR_SAVE                 (c, "angles", 1);
    CLASS_ATTR_DEFAULT              (c, "angles", 0, "0 45 90 135 180 225 270 315");

    CLASS_ATTR_FLOAT                (c, "offset", 0, t_hoa_meter, f_attrs);
    CLASS_ATTR_ACCESSORS            (c, "offset", offset_get, offset_set);
    CLASS_ATTR_ORDER                (c, "offset", 0, "3");
    CLASS_ATTR_LABEL                (c, "offset", 0, "Offset of Channels");
    CLASS_ATTR_DEFAULT              (c, "offset", 0, "0");
    CLASS_ATTR_SAVE                 (c, "offset", 1);
    CLASS_ATTR_STYLE                (c, "offset", 1, "number");

    CLASS_ATTR_SYMBOL               (c, "rotation", 0, t_hoa_meter, f_clockwise);
    CLASS_ATTR_ACCESSORS            (c, "rotation", NULL, rotation_set);
    CLASS_ATTR_ORDER                (c, "rotation", 0, "4");
    CLASS_ATTR_LABEL                (c, "rotation", 0, "Rotation of Channels");
    CLASS_ATTR_SAVE                 (c, "rotation", 1);
    CLASS_ATTR_STYLE                (c, "rotation", 1, "menu");
    CLASS_ATTR_ITEMS                (c, "rotation", 1, "anti-clockwise clockwise");

    CLASS_ATTR_SYMBOL               (c, "vectors", 0, t_hoa_meter, f_vector_type);
    CLASS_ATTR_ACCESSORS            (c, "vectors", NULL, vectors_set);
    CLASS_ATTR_ORDER                (c, "vectors", 0, "2");
    CLASS_ATTR_LABEL                (c, "vectors", 0, "Vectors");
    CLASS_ATTR_DEFAULT              (c, "vectors", 0, "Energy");
    CLASS_ATTR_SAVE                 (c, "vectors", 1);
    CLASS_ATTR_STYLE                (c, "vectors", 1, "menu");
    CLASS_ATTR_ITEMS                (c, "vectors", 1, "none energy velocity both");

    CLASS_ATTR_LONG                 (c, "interval", 0, t_hoa_meter, f_interval);
    CLASS_ATTR_ORDER                (c, "interval", 0, "5");
    CLASS_ATTR_LABEL                (c, "interval", 0, "Refresh Interval (in ms)");
    CLASS_ATTR_FILTER_MIN           (c, "interval", 20);
    CLASS_ATTR_DEFAULT              (c, "interval", 0, "50");
    CLASS_ATTR_SAVE                 (c, "interval", 1);
    CLASS_ATTR_STYLE                (c, "interval", 1, "number");

    CLASS_ATTR_RGBA					(c, "bgcolor", 0, t_hoa_meter, f_color_bg);
    CLASS_ATTR_CATEGORY				(c, "bgcolor", 0, "Color");
    CLASS_ATTR_STYLE				(c, "bgcolor", 0, "rgba");
    CLASS_ATTR_LABEL				(c, "bgcolor", 0, "Background Color");
    CLASS_ATTR_DEFAULT_SAVE_PAINT	(c, "bgcolor", 0, "0.76 0.76 0.76 1.");
    CLASS_ATTR_STYLE                (c, "bgcolor", 1, "color");

    CLASS_ATTR_RGBA					(c, "bdcolor", 0, t_hoa_meter, f_color_bd);
    CLASS_ATTR_CATEGORY				(c, "bdcolor", 0, "Color");
    CLASS_ATTR_STYLE                (c, "bdcolor", 0, "rgba");
    CLASS_ATTR_LABEL				(c, "bdcolor", 0, "Border Color");
    CLASS_ATTR_DEFAULT_SAVE_PAINT	(c, "bdcolor", 0, "0.7 0.7 0.7 1.");
    CLASS_ATTR_STYLE                (c, "bdcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "coldcolor", 0, t_hoa_meter, f_color_cold_signal);
    CLASS_ATTR_LABEL                (c, "coldcolor", 0, "Cold Signal Color");
    CLASS_ATTR_ORDER                (c, "coldcolor", 0, "4");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "coldcolor", 0, "0. 0.6 0. 0.8");
    CLASS_ATTR_STYLE                (c, "coldcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "tepidcolor", 0, t_hoa_meter, f_color_tepid_signal);
    CLASS_ATTR_LABEL                (c, "tepidcolor", 0, "Tepid Signal Color");
    CLASS_ATTR_ORDER                (c, "tepidcolor", 0, "5");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "tepidcolor", 0, "0.6 0.73 0. 0.8");
    CLASS_ATTR_STYLE                (c, "tepidcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "warmcolor", 0, t_hoa_meter, f_color_warm_signal);
    CLASS_ATTR_LABEL                (c, "warmcolor", 0, "Warm Signal Color");
    CLASS_ATTR_ORDER                (c, "warmcolor", 0, "6");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "warmcolor", 0, ".85 .85 0. 0.8");
    CLASS_ATTR_STYLE                (c, "warmcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "hotcolor", 0, t_hoa_meter, f_color_hot_signal);
    CLASS_ATTR_LABEL                (c, "hotcolor", 0, "Hot Signal Color");
    CLASS_ATTR_ORDER                (c, "hotcolor", 0, "7");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "hotcolor", 0, "1. 0.6 0. 0.8");
    CLASS_ATTR_STYLE                (c, "hotcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "overcolor", 0, t_hoa_meter, f_color_over_signal);
    CLASS_ATTR_LABEL                (c, "overcolor", 0, "Overload Signal Color");
    CLASS_ATTR_ORDER                (c, "overcolor", 0, "8");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "overcolor", 0, "1. 0. 0. 0.8");
    CLASS_ATTR_STYLE                (c, "overcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "energycolor", 0, t_hoa_meter, f_color_energy_vector);
    CLASS_ATTR_LABEL                (c, "energycolor", 0, "Energy Vector Color");
    CLASS_ATTR_ORDER                (c, "energycolor", 0, "9");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "energycolor", 0, "0. 0. 1. 0.8");
    CLASS_ATTR_STYLE                (c, "energycolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "velocitycolor", 0, t_hoa_meter, f_color_velocity_vector);
    CLASS_ATTR_LABEL                (c, "velocitycolor", 0, "Velocity Vector Color");
    CLASS_ATTR_ORDER                (c, "velocitycolor", 0, "9");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "velocitycolor", 0, "1. 0. 0. 0.8");
    CLASS_ATTR_STYLE                (c, "velocitycolor", 1, "color");

    eclass_register(CLASS_BOX, c);
    hoa_meter_class = c;
}

typedef struct  _hoa_meter_3d
{
    t_edspbox               f_box;
    Meter<Hoa3d, t_sample>* f_meter;
    Vector<Hoa3d, t_sample>*f_vector;
    t_sample*               f_signals;
    t_sample                f_vector_coords[6];
    long                    f_ramp;
    int                     f_startclock;
    long                    f_interval;

    t_symbol*               f_vector_type;
    t_symbol*               f_clockwise;
    t_symbol*               f_view;

    t_rgba                  f_color_bg;
    t_rgba                  f_color_bd;
    t_rgba                  f_color_cold_signal;
    t_rgba                  f_color_tepid_signal;
    t_rgba                  f_color_warm_signal;
    t_rgba                  f_color_hot_signal;
    t_rgba                  f_color_over_signal;
    t_rgba                  f_color_energy_vector;
    t_rgba                  f_color_velocity_vector;

    double                  f_radius;
    double                  f_center;
    double                  f_radius_center;

    t_clock*                f_clock;
    void*                   f_attrs;

} t_hoa_meter_3d;

static t_eclass *hoa_meter_3d_class;

static void hoa_meter_3d_getdrawparams(t_hoa_meter_3d *x, t_object *patcherview, t_edrawparams *params)
{
    params->d_boxfillcolor = x->f_color_bg;
    params->d_bordercolor = x->f_color_bd;
    params->d_borderthickness = 1;
    params->d_cornersize = 8;
}

static void hoa_meter_3d_oksize(t_hoa_meter_3d *x, t_rect *newrect)
{
    newrect->width = pd_clip_min(newrect->width, 20.);
    newrect->height = pd_clip_min(newrect->height, 20.);
    double delta1 = newrect->width - x->f_box.b_rect_last.width;
    double delta2 = newrect->height - x->f_box.b_rect_last.height;

    if(x->f_view == hoa_sym_topnextbottom)
    {
        if(fabs(delta1) < fabs(delta2))
            newrect->width = newrect->height * 2;
        else
            newrect->height = newrect->width * 0.5;
    }
    else if(x->f_view == hoa_sym_toponbottom)
    {
        if(fabs(delta1) < fabs(delta2))
            newrect->width = newrect->height * 0.5;
        else
            newrect->height = newrect->width * 2;
    }
    else
    {
        if(fabs(delta1) < fabs(delta2))
            newrect->width = newrect->height;
        else
            newrect->height = newrect->width;
    }
    x->f_box.b_rect_last = *newrect;
}

static t_pd_err channels_3d_get(t_hoa_meter_3d *x, void *attr, int* argc, t_atom **argv)
{
    argc[0] = 1;
    argv[0] = (t_atom *)malloc(sizeof(t_atom));
    if(argv[0] && argc[0])
    {
        atom_setfloat(argv[0], x->f_meter->getNumberOfPlanewaves());
    }
    else
    {
        argc[0] = 0;
        argv[0] = NULL;
    }
    return 0;
}

static t_pd_err channels_3d_set(t_hoa_meter_3d *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_FLOAT)
        {
            ulong d = pd_clip_minmax(atom_getfloat(argv), 4, HOA_MAX_PLANEWAVES);
            if(d != x->f_meter->getNumberOfPlanewaves())
            {
                int dspState = canvas_suspend_dsp();
                delete x->f_meter;
                x->f_meter = new Meter<Hoa3d, t_sample>(d);
                delete x->f_vector;
                x->f_vector = new Vector<Hoa3d, t_sample>(d);

                x->f_meter->computeRendering();
                x->f_vector->computeRendering();
                eobj_resize_inputs((t_ebox *)x, long(x->f_meter->getNumberOfPlanewaves()));

                ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
                ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
                ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
                ebox_redraw((t_ebox *)x);
                canvas_resume_dsp(dspState);
            }
        }
    }
    return 0;
}

static t_pd_err angles_3d_get(t_hoa_meter_3d *x, void *attr, int* argc, t_atom **argv)
{
    argc[0] = long(x->f_meter->getNumberOfPlanewaves()) * 2;
    argv[0] = (t_atom *)malloc(sizeof(t_atom) * x->f_meter->getNumberOfPlanewaves() * 2);
    if(argv[0] && argc[0])
    {
        for(ulong i = 0; i < x->f_meter->getNumberOfPlanewaves(); i++)
        {
            atom_setfloat(argv[0]+i*2, round(x->f_meter->getPlanewaveAzimuth(i, false) / HOA_2PI * 3600.) / 10.);
            atom_setfloat(argv[0]+i*2+1, round(x->f_meter->getPlanewaveElevation(i, false) / HOA_2PI * 3600.) / 10.);
        }
    }
    else
    {
        argc[0] = 0;
        argv[0] = NULL;
    }
    return 0;
}

static t_pd_err angles_3d_set(t_hoa_meter_3d *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        for(long i = 0; (ulong)i < x->f_meter->getNumberOfPlanewaves() * 2 && i < argc; i++)
        {
            if(atom_gettype(argv+i) == A_FLOAT)
            {
                if(i%2)
                {
                    x->f_meter->setPlanewaveElevation(ulong((i-1)/2), atom_getfloat(argv+i) / 360.f * HOA_2PI);
                    x->f_vector->setPlanewaveElevation(ulong((i-1)/2), atom_getfloat(argv+i) / 360.f * HOA_2PI);
                }
                else
                {
                    x->f_meter->setPlanewaveAzimuth(ulong(i/2), atom_getfloat(argv+i) / 360.f * HOA_2PI);
                    x->f_vector->setPlanewaveAzimuth(ulong(i/2), atom_getfloat(argv+i) / 360.f * HOA_2PI);
                }
            }
        }

        x->f_meter->computeRendering();
        x->f_vector->computeRendering();

        ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
        ebox_redraw((t_ebox *)x);
    }

    return 0;
}

static t_pd_err offset_3d_get(t_hoa_meter_3d *x, void *attr, int* argc, t_atom **argv)
{
    argc[0] = 3;
    argv[0] = (t_atom *)malloc(3 * sizeof(t_atom));
    if(argv[0] && argc[0])
    {
        atom_setfloat(argv[0], round(x->f_meter->getPlanewavesRotationX() / HOA_2PI * 3600.) / 10.);
        atom_setfloat(argv[0]+1, round(x->f_meter->getPlanewavesRotationY() / HOA_2PI * 3600.) / 10.);
        atom_setfloat(argv[0]+2, round(x->f_meter->getPlanewavesRotationZ() / HOA_2PI * 3600.) / 10.);
    }
    else
    {
        argc[0] = 0;
        argv[0] = NULL;
    }
    return 0;
}

static t_pd_err offset_3d_set(t_hoa_meter_3d *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        double ax, ay, az;
        if(atom_gettype(argv) == A_FLOAT)
            ax = atom_getfloat(argv) / 360. * HOA_2PI;
        else
            ax = x->f_meter->getPlanewavesRotationX();

        if(argc > 1 && atom_gettype(argv+1) == A_FLOAT)
            ay = atom_getfloat(argv+1) / 360. * HOA_2PI;
        else
            ay = x->f_meter->getPlanewavesRotationY();

        if(argc > 2 &&  atom_gettype(argv+2) == A_FLOAT)
            az = atom_getfloat(argv+2) / 360. * HOA_2PI;
        else
            az = x->f_meter->getPlanewavesRotationZ();

        x->f_meter->setPlanewavesRotation(ax, ay, az);
        x->f_vector->setPlanewavesRotation(ax, ay, az);

        x->f_meter->computeRendering();
        x->f_vector->computeRendering();

        ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
        ebox_redraw((t_ebox *)x);

    }

    return 0;
}

static t_pd_err vectors_3d_set(t_hoa_meter_3d *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_SYM)
        {
            if(atom_getsym(argv) == hoa_sym_energy)
                x->f_vector_type = hoa_sym_energy;
            else if(atom_getsym(argv) == hoa_sym_velocity)
                x->f_vector_type = hoa_sym_velocity;
            else if(atom_getsym(argv) == hoa_sym_both)
                x->f_vector_type = hoa_sym_both;
            else
                x->f_vector_type = hoa_sym_none;
        }
        else if(atom_gettype(argv) == A_FLOAT)
        {
            if(atom_getlong(argv) == 1)
                x->f_vector_type = hoa_sym_energy;
            else if(atom_getlong(argv) == 2)
                x->f_vector_type = hoa_sym_velocity;
            else if(atom_getlong(argv) == 3)
                x->f_vector_type = hoa_sym_both;
            else
                x->f_vector_type = hoa_sym_none;
        }
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
        ebox_redraw((t_ebox *)x);
    }
    return 0;
}

static t_pd_err rotation_3d_set(t_hoa_meter_3d *x, void *attr, int argc, t_atom *argv)
{
    if(argc && argv)
    {
        if(atom_gettype(argv) == A_SYM)
        {
            if(atom_getsym(argv) == hoa_sym_clockwise)
                x->f_clockwise = hoa_sym_clockwise;
            else
                x->f_clockwise = hoa_sym_anticlock;
        }
        else if(atom_gettype(argv) == A_FLOAT)
        {
            if(atom_getlong(argv) == 1)
                x->f_clockwise = hoa_sym_clockwise;
            else
                x->f_clockwise = hoa_sym_anticlock;
        }

        ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
        ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
        ebox_redraw((t_ebox *)x);
    }
    return 0;
}

static t_pd_err view_3d_set(t_hoa_meter_3d *x, void *attr, int argc, t_atom *argv)
{
    t_symbol* view = x->f_view;
    if(argc && argv)
    {

        if(atom_gettype(argv) == A_SYM)
        {
            if(atom_getsym(argv) == hoa_sym_bottom)
                view = hoa_sym_bottom;
            else if(atom_getsym(argv) == hoa_sym_topnextbottom)
                view = hoa_sym_topnextbottom;
            else if(atom_getsym(argv) == hoa_sym_toponbottom)
                view = hoa_sym_toponbottom;
            else
                view = hoa_sym_top;
        }
        else if(atom_gettype(argv) == A_FLOAT)
        {
            if(atom_getlong(argv) == 1)
                view = hoa_sym_bottom;
            else if(atom_getlong(argv) == 2)
                view = hoa_sym_topnextbottom;
            else if(atom_getlong(argv) == 3)
                view = hoa_sym_toponbottom;
            else
                view = hoa_sym_top;
        }

        if(view != x->f_view)
        {
            x->f_view = view;
            x->f_box.b_rect_last = x->f_box.b_rect;
            object_attr_setvalueof((t_object *)x, gensym("size"), 0, NULL);
        }
    }
    return 0;
}

static void hoa_meter_3d_perform(t_hoa_meter_3d *x, t_object *dsp, float **ins, long numins, float **outs, long no, long sampleframes, long f,void *up)
{
    for(int i = 0; i < numins; i++)
    {
        Signal<t_sample>::copy(ulong(sampleframes), ins[i], 1, x->f_signals+i, ulong(numins));
    }
    for(x->f_ramp = 0; x->f_ramp < sampleframes; x->f_ramp++)
    {
        x->f_meter->process(x->f_signals + numins * x->f_ramp);
    }
    if(x->f_startclock)
    {
        x->f_startclock = 0;
        clock_delay(x->f_clock,0);
    }

}

static void hoa_meter_3d_dsp(t_hoa_meter_3d *x, t_object *dsp, short *count, double samplerate, long maxvectorsize, long flags)
{
    x->f_meter->setVectorSize(ulong(maxvectorsize));
    object_method(dsp, gensym("dsp_add"), x, (method)hoa_meter_3d_perform, 0, NULL);
    x->f_startclock = 1;
}

static void hoa_meter_3d_tick(t_hoa_meter_3d *x)
{
    if(x->f_vector_type == hoa_sym_both)
        x->f_vector->process(x->f_signals, x->f_vector_coords);
    else if(x->f_vector_type == hoa_sym_velocity)
        x->f_vector->processVelocity(x->f_signals, x->f_vector_coords);
    else if(x->f_vector_type == hoa_sym_energy)
        x->f_vector->processEnergy(x->f_signals, x->f_vector_coords + 3);

    x->f_meter->tick(ulong((1000.f / (float)x->f_interval)));
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
    ebox_redraw((t_ebox *)x);

    if (canvas_dspstate)
        clock_delay(x->f_clock, x->f_interval);
}

static void draw_3d_background(t_hoa_meter_3d *x,  t_object *view, t_rect *rect)
{
    t_elayer *g = ebox_start_layer((t_ebox *)x, hoa_sym_background_layer, rect->width, rect->height);
    if(g)
    {
        t_rgba black = rgba_addContrast(x->f_color_bg, -HOA_CONTRAST_DARKER);
        t_rgba white = rgba_addContrast(x->f_color_bg, HOA_CONTRAST_LIGHTER);
        if(x->f_view == hoa_sym_topnextbottom)
        {
            egraphics_set_color_rgba(g, &white);
            egraphics_set_line_width(g, 3.);
            egraphics_circle(g, x->f_center, x->f_center, x->f_radius);
            egraphics_stroke_preserve(g);
            egraphics_set_color_rgba(g, &black);
            egraphics_set_line_width(g, 1.);
            egraphics_stroke(g);

            egraphics_set_color_rgba(g, &white);
            egraphics_set_line_width(g, 3.);
            egraphics_line_fast(g, x->f_center * 2, 0, x->f_center * 2, x->f_center * 2);
            egraphics_set_color_rgba(g, &black);
            egraphics_set_line_width(g, 1.);
            egraphics_line_fast(g, x->f_center * 2, 0, x->f_center * 2, x->f_center * 2);

            egraphics_set_color_rgba(g, &white);
            egraphics_set_line_width(g, 3.);
            egraphics_circle(g, x->f_center * 3, x->f_center, x->f_radius);
            egraphics_stroke_preserve(g);
            egraphics_set_color_rgba(g, &black);
            egraphics_set_line_width(g, 1.);
            egraphics_stroke(g);
        }
        else if(x->f_view == hoa_sym_toponbottom)
        {
            egraphics_set_color_rgba(g, &white);
            egraphics_set_line_width(g, 3.);
            egraphics_circle(g, x->f_center, x->f_center, x->f_radius);
            egraphics_stroke_preserve(g);
            egraphics_set_color_rgba(g, &black);
            egraphics_set_line_width(g, 1.);
            egraphics_stroke(g);

            egraphics_set_color_rgba(g, &white);
            egraphics_set_line_width(g, 3.);
            egraphics_line_fast(g, 0, x->f_center * 2, x->f_center * 2, x->f_center * 2);
            egraphics_set_color_rgba(g, &black);
            egraphics_set_line_width(g, 1.);
            egraphics_line_fast(g, 0, x->f_center * 2, x->f_center * 2, x->f_center * 2);

            egraphics_set_color_rgba(g, &white);
            egraphics_set_line_width(g, 3.);
            egraphics_circle(g, x->f_center, x->f_center * 3, x->f_radius);
            egraphics_stroke_preserve(g);
            egraphics_set_color_rgba(g, &black);
            egraphics_set_line_width(g, 1.);
            egraphics_stroke(g);
        }
        else
        {
            egraphics_set_color_rgba(g, &white);
            egraphics_set_line_width(g, 3.);
            egraphics_circle(g, x->f_center, x->f_center, x->f_radius);
            egraphics_stroke_preserve(g);
            egraphics_set_color_rgba(g, &black);
            egraphics_set_line_width(g, 1.);
            egraphics_stroke(g);
        }
        ebox_end_layer((t_ebox*)x, hoa_sym_background_layer);
    }
    ebox_paint_layer((t_ebox *)x, hoa_sym_background_layer, 0., 0.);
}

static t_rgba* meter_3d_getcolor(t_hoa_meter_3d *x, const bool peak, const double db)
{
    if(peak)
    {
        return &x->f_color_over_signal;
    }
    else if(db >= -12.)
    {
        return &x->f_color_hot_signal;
    }
    else if(db >= -21)
    {
        return &x->f_color_warm_signal;
    }
    else if(db >= -30.)
    {
        return &x->f_color_tepid_signal;
    }
    else if(db >= -39.)
    {
        return &x->f_color_cold_signal;
    }
    else
    {
        return &x->f_color_bd;
    }
}

static void draw_3d_leds(t_hoa_meter_3d *x, t_object *view, t_rect *rect)
{
    t_matrix transform;
    t_elayer *g = ebox_start_layer((t_ebox *)x,  hoa_sym_leds_layer, rect->width, rect->height);
    if(g)
    {
        t_rgba black = rgba_addContrast(x->f_color_bg, -HOA_CONTRAST_DARKER);
        t_rgba white = rgba_addContrast(x->f_color_bg, HOA_CONTRAST_LIGHTER);
        egraphics_matrix_init(&transform, 1, 0, 0, -1, x->f_center, x->f_center);
        egraphics_set_matrix(g, &transform);

        const float width = x->f_radius - 2.;
        const bool top = (x->f_view == hoa_sym_bottom) ? false : true;

        for(ulong i = 0; i < x->f_meter->getNumberOfPlanewaves(); i++)
        {
            Meter<Hoa3d, t_sample>::Path const& path = x->f_meter->getPlanewavePath(i, top);
            if(path.size() > 2)
            {
                float angle1 = Math<double>::azimuth(path[0].x, path[0].y);
                float radius1= Math<double>::radius(path[0].x, path[0].y);
                egraphics_move_to(g, path[0].x * width, path[0].y * width);
                for(ulong j = 1; j < path.size(); j++)
                {
                    const float angle2 = Math<double>::azimuth(path[j].x, path[j].y);
                    const float radius2= Math<double>::radius(path[j].x, path[j].y);
                    const float extend = Math<double>::wrap_pi(angle2 - angle1);
                    if(fabs(extend) > HOA_EPSILON && fabs(radius2 - radius1) < HOA_EPSILON && fabs(fabs(extend) - HOA_PI) > 1e-3)
                    {
                        egraphics_arc_to(g, 0., 0., extend);
                    }
                    else
                    {
                        egraphics_line_to(g, path[j].x * width, path[j].y * width);
                    }
                    angle1 = angle2;
                    radius1 = radius2;
                }

                const float angle2 = Math<double>::azimuth(path[0].x, path[0].y);
                const float radius2 = Math<double>::radius(path[0].x, path[0].y);
                const float extend = Math<double>::wrap_pi(angle2 - angle1);
                if(fabs(extend) > HOA_EPSILON && fabs(radius2 - radius1) < HOA_EPSILON && fabs(fabs(extend) - HOA_PI) > 1e-3)
                {
                    egraphics_arc_to(g, 0., 0., extend);
                }
                else
                {
                    egraphics_line_to(g, path[0].x * width, path[0].y * width);
                }

                egraphics_set_color_rgba(g, meter_3d_getcolor(x, x->f_meter->getPlanewaveOverLed(i), x->f_meter->getPlanewaveEnergy(i)));
                egraphics_fill_preserve(g);
                egraphics_set_line_width(g, 1.);
                egraphics_set_color_rgba(g, &white);
                egraphics_stroke_preserve(g);
                egraphics_set_line_width(g, 2.);
                egraphics_set_color_rgba(g, &black);
                egraphics_stroke(g);
            }
        }

        if(x->f_view == hoa_sym_toponbottom || x->f_view == hoa_sym_topnextbottom)
        {
            if(x->f_view == hoa_sym_toponbottom)
            {
                egraphics_matrix_init(&transform, 1, 0, 0, -1, x->f_center, rect->width + x->f_center);
                egraphics_set_matrix(g, &transform);
            }
            else
            {
                egraphics_matrix_init(&transform, 1, 0, 0, -1, rect->height + x->f_center, x->f_center);
                egraphics_set_matrix(g, &transform);
            }

            for(ulong i = 0; i < x->f_meter->getNumberOfPlanewaves(); i++)
            {
                Meter<Hoa3d, t_sample>::Path const& path = x->f_meter->getPlanewavePath(i, false);
                if(path.size() > 2)
                {
                    float angle1 = Math<double>::azimuth(path[0].x, path[0].y);
                    float radius1= Math<double>::radius(path[0].x, path[0].y);
                    egraphics_move_to(g, path[0].x * width, path[0].y * width);
                    for(ulong j = 1; j < path.size(); j++)
                    {
                        const float angle2 = Math<double>::azimuth(path[j].x, path[j].y);
                        const float radius2= Math<double>::radius(path[j].x, path[j].y);
                        const float extend = Math<double>::wrap_pi(angle2 - angle1);
                        if(fabs(extend) > HOA_EPSILON && fabs(radius2 - radius1) < HOA_EPSILON && fabs(fabs(extend) - HOA_PI) > 1e-3)
                        {
                            egraphics_arc_to(g, 0., 0., extend);
                        }
                        else
                        {
                            egraphics_line_to(g, path[j].x * width, path[j].y * width);
                        }
                        angle1 = angle2;
                        radius1 = radius2;
                    }

                    const float angle2 = Math<double>::azimuth(path[0].x, path[0].y);
                    const float radius2 = Math<double>::radius(path[0].x, path[0].y);
                    const float extend = Math<double>::wrap_pi(angle2 - angle1);
                    if(fabs(extend) > HOA_EPSILON && fabs(radius2 - radius1) < HOA_EPSILON && fabs(fabs(extend) - HOA_PI) > 1e-3)
                    {
                        egraphics_arc_to(g, 0., 0., extend);
                    }
                    else
                    {
                        egraphics_line_to(g, path[0].x * width, path[0].y * width);
                    }

                    egraphics_set_color_rgba(g, meter_3d_getcolor(x, x->f_meter->getPlanewaveOverLed(i), x->f_meter->getPlanewaveEnergy(i)));
                    egraphics_fill_preserve(g);
                    egraphics_set_line_width(g, 1.);
                    egraphics_set_color_rgba(g, &white);
                    egraphics_stroke_preserve(g);
                    egraphics_set_line_width(g, 2.);
                    egraphics_set_color_rgba(g, &black);
                    egraphics_stroke(g);
                }
            }
        }

        ebox_end_layer((t_ebox*)x,  hoa_sym_leds_layer);
    }
    ebox_paint_layer((t_ebox *)x, hoa_sym_leds_layer, 0., 0.);
}

static void draw_3d_vectors(t_hoa_meter_3d *x, t_object *view, t_rect *rect)
{
    double x1, y1, size;
    t_matrix transform;
    t_elayer *g = ebox_start_layer((t_ebox *)x,  hoa_sym_vector_layer, rect->width, rect->height);
    t_rgba color;
    double distance;
    if(g)
    {
        egraphics_matrix_init(&transform, 1, 0, 0, -1, x->f_center, x->f_center);
        egraphics_set_matrix(g, &transform);
        size = x->f_center / 32.;

        if(x->f_vector_type == hoa_sym_both || x->f_vector_type == hoa_sym_energy)
        {
            double rad = Math<float>::radius(x->f_vector_coords[3], x->f_vector_coords[4], x->f_vector_coords[5]);
            distance = (fabs(rad) * 0.5 + 0.5);
            color = rgba_addContrast(x->f_color_energy_vector, -(1. - distance));
            egraphics_set_color_rgba(g, &color);
            if(x->f_clockwise == hoa_sym_anticlock)
            {
                x1 = x->f_vector_coords[3] * x->f_radius;
                y1 = x->f_vector_coords[4] * x->f_radius;
            }
            else
            {
                double ang = -Math<float>::azimuth(x->f_vector_coords[3], x->f_vector_coords[4], x->f_vector_coords[5]);
                x1 = Math<float>::abscissa(rad * x->f_radius, ang);
                y1 = Math<float>::ordinate(rad * x->f_radius, ang);
            }

            if((x->f_vector_coords[5] >= 0 && (x->f_view == hoa_sym_top || x->f_view == hoa_sym_toponbottom || x->f_view == hoa_sym_topnextbottom)) ||  (x->f_vector_coords[5] <= 0 && x->f_view == hoa_sym_bottom))
            {
                egraphics_circle(g, x1, y1, size * distance);
                egraphics_fill(g);
            }
            else if(x->f_vector_coords[5] <= 0 && x->f_view == hoa_sym_toponbottom)
            {
                egraphics_circle(g, x1, y1 - x->f_center * 2, size * distance);
                egraphics_fill(g);
            }
            else if(x->f_vector_coords[5] <= 0 && x->f_view == hoa_sym_topnextbottom)
            {
                egraphics_circle(g, x1 + x->f_center * 2, y1, size * distance);
                egraphics_fill(g);
            }
        }
        if(x->f_vector_type == hoa_sym_both || x->f_vector_type == hoa_sym_velocity)
        {
            double rad = Math<float>::radius(x->f_vector_coords[0], x->f_vector_coords[1], x->f_vector_coords[2]);
            distance = (fabs(rad) * 0.5 + 0.5);
            color = rgba_addContrast(x->f_color_velocity_vector, -(1. - distance));
            egraphics_set_color_rgba(g, &color);
            if(x->f_clockwise == hoa_sym_anticlock)
            {
                x1 = x->f_vector_coords[0] * x->f_radius;
                y1 = x->f_vector_coords[1] * x->f_radius;
            }
            else
            {
                double ang = -Math<float>::azimuth(x->f_vector_coords[0], x->f_vector_coords[1], x->f_vector_coords[2]);
                x1 = Math<float>::abscissa(rad * x->f_radius, ang);
                y1 = Math<float>::ordinate(rad * x->f_radius, ang);
            }

            if((x->f_vector_coords[2] >= 0 && (x->f_view == hoa_sym_top || x->f_view == hoa_sym_toponbottom || x->f_view == hoa_sym_topnextbottom)) ||  (x->f_vector_coords[2] <= 0 && x->f_view == hoa_sym_bottom))
            {
                egraphics_circle(g, x1, y1, size * distance);
                egraphics_fill(g);
            }
            else if(x->f_vector_coords[2] <= 0 && x->f_view == hoa_sym_toponbottom)
            {
                egraphics_circle(g, x1, y1 - x->f_center * 2, size * distance);
                egraphics_fill(g);
            }
            else if(x->f_vector_coords[2] <= 0 && x->f_view == hoa_sym_topnextbottom)
            {
                egraphics_circle(g, x1 + x->f_center * 2, y1, size * distance);
                egraphics_fill(g);
            }
        }

        ebox_end_layer((t_ebox*)x,  hoa_sym_vector_layer);
    }
    ebox_paint_layer((t_ebox *)x, hoa_sym_vector_layer, 0., 0.);
}

static void hoa_meter_3d_paint(t_hoa_meter_3d *x, t_object *view)
{
    t_rect rect;
    ebox_get_rect_for_view((t_ebox *)x, &rect);

    if(x->f_view == hoa_sym_topnextbottom)
        x->f_center = rect.width * 0.25;
    else
        x->f_center = rect.width * 0.5;
    x->f_radius = x->f_center * 0.95;

    draw_3d_leds(x, view, &rect);
    draw_3d_vectors(x, view, &rect);
    draw_3d_background(x, view, &rect);
}

static void hoa_meter_3d_free(t_hoa_meter_3d *x)
{
    ebox_free((t_ebox *)x);
    clock_free(x->f_clock);
    delete x->f_meter;
    delete x->f_vector;
    delete [] x->f_signals;
}

static void *hoa_meter_3d_new(t_symbol *s, int argc, t_atom *argv)
{
    long flags;
    t_hoa_meter_3d *x =  (t_hoa_meter_3d *)eobj_new(hoa_meter_3d_class);
    t_binbuf *d       = binbuf_via_atoms(argc, argv);

    if(x && d)
    {
        x->f_ramp = 0;
        x->f_meter  = new Meter<Hoa3d, t_sample>(4);
        x->f_vector = new Vector<Hoa3d, t_sample>(4);
        x->f_signals = new t_float[HOA_MAX_PLANEWAVES * HOA_MAXBLKSIZE];

        x->f_meter->computeRendering();
        x->f_vector->computeRendering();
        x->f_clock = clock_new(x,(t_method)hoa_meter_3d_tick);
        x->f_startclock = 0;
        eobj_dspsetup((t_ebox *)x, long(x->f_meter->getNumberOfPlanewaves()), 0);

        flags = 0
        | EBOX_GROWINDI
        | EBOX_IGNORELOCKCLICK
        ;
        ebox_new((t_ebox *)x, flags);

        t_atom *av;
        int    ac;

        binbuf_get_attribute(d, gensym("@size"), &ac, &av);
        if(ac && av)
        {
            x->f_box.b_rect.width = atom_getfloat(av);
            x->f_box.b_rect.height = atom_getfloat(av+1);
            x->f_box.b_rect_last = x->f_box.b_rect;
            free(av);
        }

        ebox_attrprocess_viabinbuf(x, d);
        ebox_ready((t_ebox *)x);

        return x;
    }


    return NULL;
}

static void hoa_meter_3d_beatles(t_hoa_meter_3d *x)
{
    int dspState = canvas_suspend_dsp();
    delete x->f_meter;
    x->f_meter = new Meter<Hoa3d, t_sample>(101);
    delete x->f_vector;
    x->f_vector = new Vector<Hoa3d, t_sample>(101);
    x->f_vector_type = hoa_sym_none;
    for(ulong i = 0; i < x->f_meter->getNumberOfPlanewaves(); i++)
    {
        x->f_meter->setPlanewaveAzimuth(i, (double)i / (double)x->f_meter->getNumberOfPlanewaves() * HOA_2PI - HOA_PI2);
        x->f_vector->setPlanewaveAzimuth(i, (double)i / (double)x->f_meter->getNumberOfPlanewaves() * HOA_2PI - HOA_PI2);
        x->f_meter->setPlanewaveElevation(i, (double)i / (double)(x->f_meter->getNumberOfPlanewaves() - 1) * HOA_PI);
        x->f_vector->setPlanewaveElevation(i, (double)i / (double)(x->f_meter->getNumberOfPlanewaves() - 1) * HOA_PI);
    }

    x->f_meter->computeRendering();
    x->f_vector->computeRendering();

    ebox_invalidate_layer((t_ebox *)x, hoa_sym_background_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_leds_layer);
    ebox_invalidate_layer((t_ebox *)x, hoa_sym_vector_layer);
    ebox_redraw((t_ebox *)x);

    eobj_resize_inputs((t_ebox *)x, (long)x->f_meter->getNumberOfPlanewaves());
    canvas_resume_dsp(dspState);
}

extern "C" void setup_hoa0x2e3d0x2emeter_tilde(void)
{
    t_eclass *c;

    c = eclass_new("hoa.3d.meter~", (method)hoa_meter_3d_new, (method)hoa_meter_3d_free, (short)sizeof(t_hoa_meter_3d), 0L, A_GIMME, 0);

    eclass_guiinit(c, 0);
    eclass_dspinit(c);

    eclass_addmethod(c, (method) hoa_meter_3d_dsp,             "dsp",           A_CANT, 0);
    eclass_addmethod(c, (method) hoa_meter_3d_paint,           "paint",         A_CANT, 0);
    eclass_addmethod(c, (method) hoa_meter_3d_getdrawparams,   "getdrawparams", A_CANT, 0);
    eclass_addmethod(c, (method) hoa_meter_3d_oksize,          "oksize",        A_CANT, 0);
    eclass_addmethod(c, (method) hoa_meter_3d_beatles,         "beatles",       A_NULL, 0);

    CLASS_ATTR_INVISIBLE            (c, "fontname", 1);
    CLASS_ATTR_INVISIBLE            (c, "fontweight", 1);
    CLASS_ATTR_INVISIBLE            (c, "fontslant", 1);
    CLASS_ATTR_INVISIBLE            (c, "fontsize", 1);
    CLASS_ATTR_INVISIBLE            (c, "send", 1);
    CLASS_ATTR_DEFAULT              (c, "size", 0, "225. 225.");

    CLASS_ATTR_LONG                 (c, "channels", 0 , t_hoa_meter_3d, f_attrs);
    CLASS_ATTR_ACCESSORS            (c, "channels", channels_3d_get, channels_3d_set);
    CLASS_ATTR_ORDER                (c, "channels", 0, "1");
    CLASS_ATTR_LABEL                (c, "channels", 0, "Number of Channels");
    CLASS_ATTR_SAVE                 (c, "channels", 1);
    CLASS_ATTR_DEFAULT              (c, "channels", 0, "8");
    CLASS_ATTR_STYLE                (c, "channels", 1, "number");

    CLASS_ATTR_FLOAT_VARSIZE        (c, "angles", 0, t_hoa_meter_3d, f_attrs, f_attrs, HOA_MAX_PLANEWAVES*2);
    CLASS_ATTR_ACCESSORS            (c, "angles", angles_3d_get, angles_3d_set);
    CLASS_ATTR_ORDER                (c, "angles", 0, "2");
    CLASS_ATTR_LABEL                (c, "angles", 0, "Angles of Channels");
    CLASS_ATTR_SAVE                 (c, "angles", 1);
    CLASS_ATTR_DEFAULT              (c, "angles", 0, "45 35.2644 135 35.2644 225 35.2644 315 35.2644 45 -35.2644 135 -35.2644 225 -35.2644 315 -35.2644");

    CLASS_ATTR_DOUBLE_ARRAY         (c, "offset", 0, t_hoa_meter_3d, f_attrs, 3);
    CLASS_ATTR_ACCESSORS            (c, "offset", offset_3d_get, offset_3d_set);
    CLASS_ATTR_ORDER                (c, "offset", 0, "3");
    CLASS_ATTR_LABEL                (c, "offset", 0, "Offset of Channels");
    CLASS_ATTR_DEFAULT              (c, "offset", 0, "0 0 0");
    CLASS_ATTR_SAVE                 (c, "offset", 1);
    CLASS_ATTR_STYLE                (c, "offset", 1, "number");

    CLASS_ATTR_SYMBOL               (c, "rotation", 0, t_hoa_meter_3d, f_clockwise);
    CLASS_ATTR_ACCESSORS            (c, "rotation", NULL, rotation_3d_set);
    CLASS_ATTR_ORDER                (c, "rotation", 0, "4");
    CLASS_ATTR_LABEL                (c, "rotation", 0, "Rotation of Channels");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "rotation", 0, "anti-clockwise");
    CLASS_ATTR_STYLE                (c, "rotation", 1, "menu");
    CLASS_ATTR_ITEMS                (c, "rotation", 1, "anti-clockwise clockwise");

    CLASS_ATTR_SYMBOL               (c, "view", 0 , t_hoa_meter_3d, f_view);
    CLASS_ATTR_ACCESSORS            (c, "view", NULL, view_3d_set);
    CLASS_ATTR_ORDER                (c, "view", 0, "5");
    CLASS_ATTR_LABEL                (c, "view", 0, "Number of Channels");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "view", 0, "top");
    CLASS_ATTR_STYLE                (c, "view", 1, "menu");
    CLASS_ATTR_ITEMS                (c, "view", 1, "top bottom top-bottom top/bottom");

    CLASS_ATTR_SYMBOL               (c, "vectors", 0, t_hoa_meter_3d, f_vector_type);
    CLASS_ATTR_ACCESSORS            (c, "vectors", NULL, vectors_3d_set);
    CLASS_ATTR_ORDER                (c, "vectors", 0, "2");
    CLASS_ATTR_LABEL                (c, "vectors", 0, "Vectors");
    CLASS_ATTR_DEFAULT              (c, "vectors", 0, "Energy");
    CLASS_ATTR_SAVE                 (c, "vectors", 1);
    CLASS_ATTR_STYLE                (c, "vectors", 1, "menu");
    CLASS_ATTR_ITEMS                (c, "vectors", 1, "none energy velocity both");

    CLASS_ATTR_LONG                 (c, "interval", 0, t_hoa_meter_3d, f_interval);
    CLASS_ATTR_ORDER                (c, "interval", 0, "5");
    CLASS_ATTR_LABEL                (c, "interval", 0, "Refresh Interval (in ms)");
    CLASS_ATTR_FILTER_MIN           (c, "interval", 20);
    CLASS_ATTR_DEFAULT              (c, "interval", 0, "50");
    CLASS_ATTR_SAVE                 (c, "interval", 1);
    CLASS_ATTR_STYLE                (c, "interval", 1, "number");

    CLASS_ATTR_RGBA					(c, "bgcolor", 0, t_hoa_meter_3d, f_color_bg);
    CLASS_ATTR_CATEGORY				(c, "bgcolor", 0, "Color");
    CLASS_ATTR_STYLE				(c, "bgcolor", 0, "rgba");
    CLASS_ATTR_LABEL				(c, "bgcolor", 0, "Background Color");
    CLASS_ATTR_DEFAULT_SAVE_PAINT	(c, "bgcolor", 0, "0.76 0.76 0.76 1.");
    CLASS_ATTR_STYLE                (c, "bgcolor", 1, "color");

    CLASS_ATTR_RGBA					(c, "bdcolor", 0, t_hoa_meter_3d, f_color_bd);
    CLASS_ATTR_CATEGORY				(c, "bdcolor", 0, "Color");
    CLASS_ATTR_STYLE                (c, "bdcolor", 0, "rgba");
    CLASS_ATTR_LABEL				(c, "bdcolor", 0, "Border Color");
    CLASS_ATTR_DEFAULT_SAVE_PAINT	(c, "bdcolor", 0, "0.7 0.7 0.7 1.");
    CLASS_ATTR_STYLE                (c, "bdcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "coldcolor", 0, t_hoa_meter_3d, f_color_cold_signal);
    CLASS_ATTR_LABEL                (c, "coldcolor", 0, "Cold Signal Color");
    CLASS_ATTR_ORDER                (c, "coldcolor", 0, "4");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "coldcolor", 0, "0. 0.6 0. 0.8");
    CLASS_ATTR_STYLE                (c, "coldcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "tepidcolor", 0, t_hoa_meter_3d, f_color_tepid_signal);
    CLASS_ATTR_LABEL                (c, "tepidcolor", 0, "Tepid Signal Color");
    CLASS_ATTR_ORDER                (c, "tepidcolor", 0, "5");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "tepidcolor", 0, "0.6 0.73 0. 0.8");
    CLASS_ATTR_STYLE                (c, "tepidcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "warmcolor", 0, t_hoa_meter_3d, f_color_warm_signal);
    CLASS_ATTR_LABEL                (c, "warmcolor", 0, "Warm Signal Color");
    CLASS_ATTR_ORDER                (c, "warmcolor", 0, "6");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "warmcolor", 0, ".85 .85 0. 0.8");
    CLASS_ATTR_STYLE                (c, "warmcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "hotcolor", 0, t_hoa_meter_3d, f_color_hot_signal);
    CLASS_ATTR_LABEL                (c, "hotcolor", 0, "Hot Signal Color");
    CLASS_ATTR_ORDER                (c, "hotcolor", 0, "7");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "hotcolor", 0, "1. 0.6 0. 0.8");
    CLASS_ATTR_STYLE                (c, "hotcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "overcolor", 0, t_hoa_meter_3d, f_color_over_signal);
    CLASS_ATTR_LABEL                (c, "overcolor", 0, "Overload Signal Color");
    CLASS_ATTR_ORDER                (c, "overcolor", 0, "8");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "overcolor", 0, "1. 0. 0. 0.8");
    CLASS_ATTR_STYLE                (c, "overcolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "energycolor", 0, t_hoa_meter_3d, f_color_energy_vector);
    CLASS_ATTR_LABEL                (c, "energycolor", 0, "Energy Vector Color");
    CLASS_ATTR_ORDER                (c, "energycolor", 0, "9");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "energycolor", 0, "0. 0. 1. 0.8");
    CLASS_ATTR_STYLE                (c, "energycolor", 1, "color");

    CLASS_ATTR_RGBA                 (c, "velocitycolor", 0, t_hoa_meter_3d, f_color_velocity_vector);
    CLASS_ATTR_LABEL                (c, "velocitycolor", 0, "Velocity Vector Color");
    CLASS_ATTR_ORDER                (c, "velocitycolor", 0, "9");
    CLASS_ATTR_DEFAULT_SAVE_PAINT   (c, "velocitycolor", 0, "1. 0. 0. 0.8");
    CLASS_ATTR_STYLE                (c, "velocitycolor", 1, "color");

    eclass_register(CLASS_BOX, c);
    hoa_meter_3d_class = c;
}

