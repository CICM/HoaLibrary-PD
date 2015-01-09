/*
// Copyright (c) 2012-2014 Eliott Paris, Julien Colafrancesco & Pierre Guillot, CICM, Universite Paris 8.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#ifndef DEF_HOA_PD
#define DEF_HOA_PD

extern "C"
{
#include "ThirdParty/CicmWrapper/Sources/cicm_wrapper.h"
#include "hoa.pd_commonsyms.h"
}

#define HOA_MAX_PLANEWAVES      EPD_MAX_SIGS
#define HOA_MAXBLKSIZE          8192
#define HOA_UI_BORDERTHICKNESS  1
#define HOA_UI_CORNERSIZE       8
#define HOA_CONTRAST_LIGHTER    0.06f
#define HOA_CONTRAST_DARKER     0.14f
#define HOA_DISPLAY_NPOINTS     180

static t_symbol* _sym_is_hoa	   = gensym("is_hoa");
static t_symbol* _sym_hoa_version  = gensym("(v2.1)");
static t_symbol* _sym_credit_line1 = gensym("by Julien Colafrancesco, Pierre Guillot & Eliott Paris");
static t_symbol* _sym_credit_line2 = gensym("Copyright \u00a9 2012 - 2015, CICM | Universite Paris 8");

typedef long t_hoa_err;		///< an integer value suitable to be returned as an hoa error code  @ingroup misc

/**
 Various errors definitions returned by Hoa functions
 @ingroup hoa_max
 */
typedef enum {
	HOA_ERR_ZERO			=					 0,
	HOA_ERR_NONE			=					-1,
	HOA_ERR_OUT_OF_MEMORY	=					-2,
	HOA_ERR_NOT_IMPLEMENTED =					-3,
} e_hoa_err_type;

/**
 The type of connection an Hoa object may have
 @ingroup hoa_max
 */
typedef enum {
	HOA_CONNECT_TYPE_STANDARD =		0,	///< a standard connection type
	HOA_CONNECT_TYPE_AMBISONICS =	1,	///< an ambisonic connection type
	HOA_CONNECT_TYPE_PLANEWAVES =	2,	///< a planewave connection type
} e_hoa_connect_type;

/**
 Is the Hoa object standard or a 2D, 3D decated one
 @ingroup hoa_max
 */
typedef enum {
	HOA_OBJECT_STANDARD =			0, ///< a standard object type
	HOA_OBJECT_2D		=			1, ///< a 2D dedicated object type
	HOA_OBJECT_3D		=			2, ///< a 3D dedicated object type
} e_hoa_object_type;

/**
 The data structure to describe an hoa box in max.
 @ingroup hoa_max
 */
typedef struct _hoa_boxinfos
{
	e_hoa_object_type object_type;					///< is the object a 3D dedicated one ?
	int autoconnect_inputs;							///< number of inputs that can be autoconnected
	int autoconnect_outputs;						///< number of outputs that can be autoconnected
	e_hoa_connect_type autoconnect_inputs_type;		///< the type of inputs (planewaves, ambisonic2D, ambisonic3D..)
	e_hoa_connect_type autoconnect_outputs_type;	///< the type of outputs (planewaves, ambisonic2D, ambisonic3D..)
} t_hoa_boxinfos;

/**
 Print to the Max console some credits for the HoaLibrary
 @ingroup hoa_max
 */
void hoa_print_credit();

long object_is_hoa(t_object* o);

void hoa_boxinfos_init(t_hoa_boxinfos* boxinfos);

int hoa_method_true(void *x);
t_hoa_err hoa_not_implemented_method();
t_hoa_err hoa_initclass(t_eclass* c, method hoabox_getinfos);

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


#endif
