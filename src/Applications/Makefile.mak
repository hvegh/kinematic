APPS := Process Acquire NtripAc12

all: $(addprefix $(BINDIR), $(APPS))

$(BINDIR)% : %.cpp $(BINDIR)kinematic.a
	$(CXX) $^ $(CPPFLAGS) $(LDFLAGS) $(CPPOOPT) $(LDOPT) -o $@

$(BINDIR)kinematic.a :
	(cd $(PROJECT_ROOT)/Library; make)


