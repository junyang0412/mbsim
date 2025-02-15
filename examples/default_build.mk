# This is the default Makefile for MBSim examples.
# It builds a "main" executable from all sources found under $(SRCDIR).
# SRCDIR must be set externally.
# PACKAGES must also be set externally to a list of all pkg-config names required by the example.
# Moreover the LDFLAGS, CPPFLAGS and CXXFLAGS are honored if set externally.

# Enable VPATH builds with sources at SRCDIR
VPATH=$(SRCDIR)

MAKEFILE_LIST_DEPS:=$(MAKEFILE_LIST)
# use a sources all *.cc file under SRCDIR
SOURCES:=$(shell (cd $(SRCDIR); find -name "*.cc"))
# object and dependency files (derived from SOURCES)
OBJECTS=$(SOURCES:.cc=.o)
DEPFILES=$(SOURCES:.cc=.o.d)

# enable C++11
CXXFLAGS += -std=c++17 -D_USE_MATH_DEFINES

# platform specific settings
ifeq ($(PLATFORM),Windows)
  SHEXT=.dll
  EXEEXT=.exe
  PIC=
  LDFLAGSRPATH=
  WIN=1
else
  SHEXT=.so
  EXEEXT=
  PIC=-fpic
  LDFLAGSRPATH=-Wl,--disable-new-dtags
  WIN=0
endif

# default target
all: main$(EXEEXT)

# FMI export target
fmiexport: mbsimfmi_model$(SHEXT)

# mingw -Wl,-Map generates some dependencies named rtr0*.o which needs to be removed

# link main executable with pkg-config options from PACKAGES
main$(EXEEXT): $(OBJECTS) $(MAKEFILE_LIST_DEPS) | main$(EXEEXT).d
	$(CXX) -Wl,-Map=$@.linkmap -o $@ $(OBJECTS) $(LDFLAGS) $(shell pkg-config --libs $(PACKAGES)) $(shell pkg-config --libs-only-L $(PACKAGES) | sed 's/-L/-Wl,-rpath,/g')
	@sed -rne "/^LOAD /s/^LOAD (.*)$$/ \1 \\\/p" $@.linkmap | grep -Ev rtr[0-9]+\.o > $@.d2 || true
	@test $(WIN) -eq 0 && (echo "$@: \\" > $@.d && cat $@.d2 >> $@.d && rm -f $@.linkmap $@.d2) || (rm -f $@.d2)

# FMI export target
mbsimfmi_model$(SHEXT): $(OBJECTS) $(MAKEFILE_LIST_DEPS) | mbsimfmi_model$(SHEXT).d
	$(CXX) -shared $(LDFLAGSRPATH) -Wl,-rpath,\$$ORIGIN,-Map=$@.linkmap -o $@ $(OBJECTS) $(LDFLAGS) $(shell pkg-config --libs $(PACKAGES))
	@sed -rne "/^LOAD /s/^LOAD (.*)$$/ \1 \\\/p" $@.linkmap | grep -Ev rtr[0-9]+\.o > $@.d2 || true
	@test $(WIN) -eq 0 && (echo "$@: \\" > $@.d && cat $@.d2 >> $@.d && rm -f $@.linkmap $@.d2) || (rm -f $@.d2)

# compile source with pkg-config options from PACKAGES (and generate dependency file)
%.o: %.cc $(MAKEFILE_LIST_DEPS) | %.o.d
	$(CXX) -MMD -MP -MF $@.d -c $(PIC) -o $@ $(CPPFLAGS) $(CXXFLAGS) $(shell pkg-config --cflags $(PACKAGES)) $<

# clean target: remove all generated files
clean:
	rm -f main$(EXEEXT) mbsimfmi_model$(SHEXT) $(OBJECTS) $(DEPFILES) main$(EXEEXT).d mbsimfmi_model$(SHEXT).d

# if the .d files do not exist create a empty one on the first run and touch the corresponding cc files to rebuild it
# which will regenerate the real .d file.
$(DEPFILES):
	@touch $@
	@touch -c $(@:.o.d=.cc)
main$(EXEEXT).d:
	@touch $@
	@touch -c $(OBJECTS)
mbsimfmi_model$(SHEXT).d:
	@touch $@
	@touch -c $(OBJECTS)
# include the generated make rules
-include $(DEPFILES)
-include main$(EXEEXT).d
-include mbsimfmi_model$(SHEXT).d
