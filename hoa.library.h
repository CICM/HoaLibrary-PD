/*
// Copyright (c) 2012-2015 Eliott Paris, Julien Colafrancesco, Thomas Le Meur & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#ifndef DEF_HOA_PD
#define DEF_HOA_PD

extern "C"
{
#include "ThirdParty/CicmWrapper/Sources/cicm_wrapper.h"
}

#define HOA_MAX_PLANEWAVES      EPD_MAX_SIGS
#define HOA_MAXBLKSIZE          8192
#define HOA_UI_BORDERTHICKNESS  1
#define HOA_UI_CORNERSIZE       8
#define HOA_CONTRAST_LIGHTER    0.06f
#define HOA_CONTRAST_DARKER     0.14f
#define HOA_DISPLAY_NPOINTS     180

extern void hoa_initclass(t_eclass* c);

extern "C" void setup_hoa0x2econnect(void);
extern "C" void setup_hoa0x2edac_tilde(void);
extern "C" void setup_hoa0x2ein(void);
extern "C" void setup_hoa0x2ein_tilde(void);
extern "C" void setup_hoa0x2eout(void);
extern "C" void setup_hoa0x2eout_tilde(void);
extern "C" void setup_hoa0x2epi(void);
extern "C" void setup_hoa0x2epi_tilde(void);
extern "C" void setup_hoa0x2eprocess_tilde(void);
extern "C" void setup_hoa0x2ethisprocess_tilde(void);
extern "C" void setup_hoa0x2emap(void);

extern "C" void setup_hoa0x2e2d0x2ewider_tilde(void);
extern "C" void setup_hoa0x2e2d0x2escope_tilde(void);
extern "C" void setup_hoa0x2e2d0x2espace(void);
extern "C" void setup_hoa0x2e2d0x2erotate_tilde(void);
extern "C" void setup_hoa0x2e2d0x2erecomposer_tilde(void);
extern "C" void setup_hoa0x2e2d0x2eprojector_tilde(void);
extern "C" void setup_hoa0x2e2d0x2eoptim_tilde(void);
extern "C" void setup_hoa0x2e2d0x2emeter_tilde(void);
extern "C" void setup_hoa0x2e2d0x2emap_tilde(void);
extern "C" void setup_hoa0x2e2d0x2eencoder_tilde(void);
extern "C" void setup_hoa0x2e2d0x2edecoder_tilde(void);

extern "C" void setup_hoa0x2e3d0x2ewider_tilde(void);
extern "C" void setup_hoa0x2e3d0x2edecoder_tilde(void);
extern "C" void setup_hoa0x2e3d0x2eencoder_tilde(void);
extern "C" void setup_hoa0x2e3d0x2eoptim_tilde(void);
extern "C" void setup_hoa0x2e3d0x2ewider_tilde(void);
extern "C" void setup_hoa0x2e3d0x2emap_tilde(void);
extern "C" void setup_hoa0x2e3d0x2emeter_tilde(void);
extern "C" void setup_hoa0x2e3d0x2escope_tilde(void);

static t_symbol* hoa_sym_none               = gensym("none");
static t_symbol* hoa_sym_energy             = gensym("energy");
static t_symbol* hoa_sym_velocity           = gensym("velocity");
static t_symbol* hoa_sym_both               = gensym("both");
static t_symbol* hoa_sym_clockwise          = gensym("clockwise");
static t_symbol* hoa_sym_anticlock          = gensym("anti-clockwise");
static t_symbol* hoa_sym_vector_layer       = gensym("vectors_layer");
static t_symbol* hoa_sym_leds_layer         = gensym("leds_layers");
static t_symbol* hoa_sym_top                = gensym("top");
static t_symbol* hoa_sym_bottom             = gensym("bottom");
static t_symbol* hoa_sym_toponbottom        = gensym("top/bottom");
static t_symbol* hoa_sym_topnextbottom      = gensym("top-bottom");

static t_symbol* hoa_sym_basic              = gensym("basic");
static t_symbol* hoa_sym_maxRe              = gensym("maxRe");
static t_symbol* hoa_sym_inPhase            = gensym("inPhase");

static t_symbol* hoa_sym_fixe               = gensym("fixe");
static t_symbol* hoa_sym_fisheye            = gensym("fisheye");
static t_symbol* hoa_sym_free               = gensym("free");

static t_symbol* hoa_sym_all                = gensym("all");
static t_symbol* hoa_sym_canvas             = gensym("canvas");
static t_symbol* hoa_sym_hoathisprocess     = gensym("hoa.thisprocess~");


static t_symbol* hoa_sym_order              = gensym("order");
static t_symbol* hoa_sym_offset             = gensym("offset");
static t_symbol* hoa_sym_nhcolor            = gensym("nhcolor");
static t_symbol* hoa_sym_phcolor            = gensym("phcolor");

// notify
static t_symbol* hoa_sym_null						= gensym("(null)");
static t_symbol* hoa_sym_nothing 					= gensym("");
static t_symbol* hoa_sym_notify 					= gensym("notify");
static t_symbol* hoa_sym_modified 					= gensym("modified");
static t_symbol* hoa_sym_attr_modified 				= gensym("attr_modified");

// Methods
static t_symbol* hoa_sym_set 						= gensym("set");
static t_symbol* hoa_sym_bang 						= gensym("bang");
static t_symbol* hoa_sym_float 						= gensym("float");
static t_symbol* hoa_sym_int 						= gensym("int");
static t_symbol* hoa_sym_list 						= gensym("list");
static t_symbol* hoa_sym_anything					= gensym("anything");
static t_symbol* hoa_sym_dynlet_begin 				= gensym("dynlet_begin");
static t_symbol* hoa_sym_dynlet_end 				= gensym("dynlet_end");
static t_symbol* hoa_sym_start						= gensym("start");
static t_symbol* hoa_sym_stop 						= gensym("stop");

// Trigo
static t_symbol* hoa_sym_cartesian 					= gensym("cartesian");
static t_symbol* hoa_sym_car 						= gensym("car");
static t_symbol* hoa_sym_polar 						= gensym("polar");
static t_symbol* hoa_sym_pol 						= gensym("pol");
static t_symbol* hoa_sym_mute 						= gensym("mute");
static t_symbol* hoa_sym_abscissa 					= gensym("abscissa");
static t_symbol* hoa_sym_ordinate 					= gensym("ordinate");
static t_symbol* hoa_sym_height 					= gensym("height");
static t_symbol* hoa_sym_radius 					= gensym("radius");
static t_symbol* hoa_sym_azimuth 					= gensym("azimuth");
static t_symbol* hoa_sym_elevation 					= gensym("elevation");
static t_symbol* hoa_sym_angle 						= gensym("angle");
static t_symbol* hoa_sym_angles						= gensym("angles");
static t_symbol* hoa_sym_directivities				= gensym("directivities");

// Paint
static t_symbol* hoa_sym_rect 						= gensym("rect");
static t_symbol* hoa_sym_bgcolor 					= gensym("bgcolor");
static t_symbol* hoa_sym_bdcolor 					= gensym("bdcolor");

// Map
static t_symbol* hoa_sym_source 					= gensym("source");
static t_symbol* hoa_sym_group 						= gensym("group");
static t_symbol* hoa_sym_slot 						= gensym("slot");
static t_symbol* hoa_sym_remove 					= gensym("remove");
static t_symbol* hoa_sym_description 				= gensym("description");
static t_symbol* hoa_sym_color 						= gensym("color");

// recomposer
static t_symbol* hoa_sym_channels					= gensym("channels");

// Layers
static t_symbol* hoa_sym_background_layer 			= gensym("background_layer");
static t_symbol* hoa_sym_sources_layer 				= gensym("sources_layer");
static t_symbol* hoa_sym_groups_layer 				= gensym("groups_layer");
static t_symbol* hoa_sym_rect_selection_layer 		= gensym("rect_selection_layer");
static t_symbol* hoa_sym_harmonics_layer	 		= gensym("harmonics_layer");
static t_symbol* hoa_sym_space_layer		 		= gensym("space_layer");
static t_symbol* hoa_sym_points_layer		 		= gensym("points_layer");
static t_symbol* hoa_sym_channels_layer				= gensym("channels_layer");
static t_symbol* hoa_sym_text_layer					= gensym("text_layer");
static t_symbol* hoa_sym_fisheye_layer				= gensym("fisheye_layer");
static t_symbol* hoa_sym_rectselection_layer		= gensym("rectselection_layer");


static t_symbol* hoa_sym_relpolar 					= gensym("relpolar");
static t_symbol* hoa_sym_relativepolar 				= gensym("relativepolar");
static t_symbol* hoa_sym_relradius 					= gensym("relradius");
static t_symbol* hoa_sym_relazimuth					= gensym("relazimuth");
static t_symbol* hoa_sym_relelevation				= gensym("relelevation");
static t_symbol* hoa_sym_relativeradius				= gensym("relativeradius");
static t_symbol* hoa_sym_relangle 					= gensym("relangle");
static t_symbol* hoa_sym_relativeangle				= gensym("relativeangle");

// Preset
static t_symbol* hoa_sym_store 						= gensym("store");
static t_symbol* hoa_sym_storeagain 				= gensym("storeagain");
static t_symbol* hoa_sym_storeempty 				= gensym("storeempty");
static t_symbol* hoa_sym_storeend 					= gensym("storeend");
static t_symbol* hoa_sym_storenext 					= gensym("storenext");
static t_symbol* hoa_sym_insert 					= gensym("insert");
static t_symbol* hoa_sym_delete 					= gensym("delete");
static t_symbol* hoa_sym_copy 						= gensym("copy");
static t_symbol* hoa_sym_renumber 					= gensym("renumber");
static t_symbol* hoa_sym_clear 						= gensym("clear");
static t_symbol* hoa_sym_recall 					= gensym("recall");
static t_symbol* hoa_sym_read 						= gensym("read");
static t_symbol* hoa_sym_write 						= gensym("write");
static t_symbol* hoa_sym_storesource 				= gensym("storesource");
static t_symbol* hoa_sym_storegroup 				= gensym("storegroup");
static t_symbol* hoa_sym_record 					= gensym("record");
static t_symbol* hoa_sym_limit 						= gensym("limit");
static t_symbol* hoa_sym_erase 						= gensym("erase");
static t_symbol* hoa_sym_erasepart 					= gensym("erasepart");

static t_symbol* hoa_sym_endeditbox 				= gensym("endeditbox");
static t_symbol* hoa_sym_text 						= gensym("text");
static t_symbol* hoa_sym_getname 					= gensym("getname");

static t_symbol* hoa_sym_zoom 						= gensym("zoom");
static t_symbol* hoa_sym_number 					= gensym("number");
static t_symbol* hoa_sym_index 						= gensym("index");


#endif
