AUTOMAKE_OPTIONS = foreign
noinst_LTLIBRARIES = libHoaPd.la

libHoaPd_la_CXXFLAGS = @PD_CPPFLAGS@ -O2 -fPIC -fpermissive -std=c++11 -pedantic
libHoaPd_la_LDFLAGS = -module -avoid-version -shared -shrext .@EXTENSION@  @PD_LDFLAGS@ -fPIC
libHoaPd_la_LIBADD = 

libHoaPd_la_SOURCES = \
hoa.decoder_tilde.cpp \
hoa.encoder_tilde.cpp \
hoa.io.cpp \
hoa.map_gui.cpp\
hoa.map_tilde.cpp \
hoa.meter_gui_tilde.cpp \
hoa.optim_tilde.cpp \
hoa.process_tilde.cpp \
hoa.projector_tilde.cpp \
hoa.recomposer_tilde.cpp \
hoa.rotate_tilde.cpp \
hoa.scope_gui_tilde.cpp \
hoa.space_gui.cpp \
hoa.tools.cpp \
hoa.wider_tilde.cpp

