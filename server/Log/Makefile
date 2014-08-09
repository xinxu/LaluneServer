CC = g++
AR = ar

USERHEADERS := -iquote./ -iquote../ -iquote../include/ -iquote../protobuf/ -iquote../../server/include/
OTHERHEADERS := -I/usr/local/include/boost/
DEFINES := -D_STATIC_NETLIB_ -D_NOT_LINK_NETLIB_LIB_

CXXFLAGS := -std=c++0x

DEBUG := NO

ifeq (YES, $(DEBUG))
	DEFINES += -D_DEBUG
	CXXFLAGS += -g
	prog := libLog_d.a
	cfgName := Debug
else
	CXXFLAGS += -g -O3
	prog := libLog.a
	cfgName := Release
endif

CXXFLAGS += $(USERHEADERS) $(DEFINES)
MMCXXFLAGS := $(CXXFLAGS)
CXXFLAGS += $(OTHERHEADERS)

srcs := $(wildcard *.cpp) ../protobuf/log.pb.cc

todep = $(cfgName)/$(basename $(notdir $(1))).d
toobj = $(cfgName)/$(basename $(notdir $(1))).o

objs := $(foreach s,$(srcs),$(call toobj,$(s)))
deps := $(foreach s,$(srcs),$(call todep,$(s)))

#must recursively call make, because include instruction is static.
default : dep
	make target --no-print-directory

target : $(prog)

$(prog) : $(objs)	
	$(AR) -r $(prog) $?

-include $(deps)

.PHONY: dep cleanall clean rebuild cleandep mkdirs target default

cleandep :
	-rm -f ./$(cfgName)/*.d

mkdirs :
	-mkdir $(cfgName)
	
dep : cleandep mkdirs
	$(foreach s,$(srcs),echo -n $(cfgName)/ > $(call todep,$(s)); $(CC) $(MMCXXFLAGS) -MM $(s) >> $(call todep,$(s)); \
                                echo -e "\t"$(CC) $(CXXFLAGS) -c $(s) -o $(call toobj,$(s)) >> $(call todep,$(s)); )

clean :
	@echo Clean intermediate files..
	-rm -f ./$(cfgName)/*.o
	-rm -f $(prog)


cleanall : cleandep clean

rebuild : cleanall default

