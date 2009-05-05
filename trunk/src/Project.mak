
.SUFFIXES : .cpp .o .lib .exe .h .dll .a

# We are linking C++, not C
#  (take the normal link rule and change from $(CC) to $(CXX) )
LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)


# When linking executibles, use the Kinematic library. Won't work for building tools needed to create library.
LDLIBS += $(BINDIR)Kinematic.a


# objs := CompileTree <srcdir> <objdir> <includedirs>
CompileTree = $(foreach s,$(call FindDown,$1),$(call Compile,$s,$2,$3))

# obj := Compile <src> <objdir> <includedirs>
Compile = $(call CreateDir,$(call CreateRule,$(call ObjectName,$2,$1),$1,$$(COMPILE)))

define COMPILE
        $(COMPILE$(suffix $<)) $(OUTPUT_OPTION) $<
endef

# targets := CreateRule <targets> <dependencies> <body>
CreateRule = $(eval $1:$2;$3)$1

# Get the object file name from the source name
#   name = ObjectName <object dir> <source name>
ObjectName = $1$(basename $2).o

# Headers
HeaderTree = $(foreach h,$(call FindDown,$(1)%.h),$(call CreateHeader,$h,$2))

CreateHeader = $(call CreateDir,$(call CreateRule,$2$(notdir $1),$1,cp $1 $2$(notdir $1)))


# Directories
# <file name> = CreateDir <file name>
#  WORKS, BUT GENERATES EXCESSIVE COMMANDS. NEED TO FIX
CreateDir = $(eval $1 : | $(dir $1))$(call eval,$(dir $1) :: ; @ mkdir -p $(dir $1))$1


# Where to put generated files
OBJDIR := $(PROJECT_ROOT)Obj/
BINDIR := $(PROJECT_ROOT)Bin/
INCDIR := $(PROJECT_ROOT)Include/

# 
CPPFLAGS += -I$(INCDIR) 
CXXFLAGS += -g
LDFLAGS += -lsetupapi
