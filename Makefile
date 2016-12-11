include gertrude.mk

BINFILES = gertrude

all: $(BINFILES)

%.a:
	$(MAKE) -C ${@D} ${@F}

$(BINFILES): $(PORTAUDIOLIBS) $(SNOWBOYDETECTLIBFILE) src/PortAudioRead.o src/PortAudioWrite.o

$(PORTAUDIOLIBS):
	@-./install_portaudio.sh

clean:
	-rm -f *.o *.a $(BINFILES)

depend:
	-$(CXX) -M $(CXXFLAGS) *.cc > .depend.mk

# Putting "-" so no error messages.
-include .depend.mk
