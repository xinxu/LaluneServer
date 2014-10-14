CC = g++
AR = ar

USERHEADERS := -iquote../ -iquote../../../LaluneCommon/3rdparty/protobuf/src/ -iquote../ -iquote../../3rdparty/boost/ -iquote../../../LaluneCommon/include -iquote../../../LaluneCommon/protobuf -iquote../ServerCommonLib -iquote../protobuf -iquote../../../LaluneCommon/protobuf/protos/out -iquote../../3rdparty/rapidjson/
OTHERHEADERS := -I/usr/local/include/boost/
DEFINES := -D_STATIC_NETLIB_
LIBPATH := -L/usr/local/lib/
LIBS :=
USRLIBS := 

CXXFLAGS := -std=c++0x

DEBUG := NO

ifeq (YES, $(DEBUG))
	DEFINES += -D_DEBUG
	CXXFLAGS += -g
	USRLIBS += ../Log/libLog_d.a ../NetLib/libNetLib_d.a ../ServerCommonLib/libServerCommon_d.a
	prog := ControlServer_d
	cfgName := Debug
else
	CXXFLAGS += -g -O3
	USRLIBS += ../Log/libLog.a ../NetLib/libNetLib.a ../ServerCommonLib/libServerCommon.a
	prog := ControlServer
	cfgName := Release
endif

LIBS += -lboost_regex -lboost_date_time -lboost_thread -lboost_system -lboost_filesystem -lprotobuf -lpthread

CXXFLAGS += $(USERHEADERS) $(DEFINES)
MMCXXFLAGS := $(CXXFLAGS)
CXXFLAGS += $(OTHERHEADERS)

srcs := $(wildcard *.cpp) ../protobuf/commonlib/CommonLib.pb.cc ../protobuf/controlserver/ControlServer.pb.cc

todep = $(cfgName)/$(basename $(notdir $(1))).d
toobj = $(cfgName)/$(basename $(notdir $(1))).o

objs := $(foreach s,$(srcs),$(call toobj,$(s)))
deps := $(foreach s,$(srcs),$(call todep,$(s)))

#must recursively call make, because include instruction is static.
default : dep
	make target --no-print-directory

refproj :
	make -C ../NetLib/
	make -C ../Log/
	make -C ../ServerCommonLib/

all : refproj default

target : $(prog)

$(prog) : $(objs) $(USRLIBS)	
	$(CC) $(LIBPATH) $(objs) -o$(prog) $(USRLIBS) $(LIBS)

-include $(deps)

.PHONY: dep cleanall clean rebuild cleandep mkdirs target default refproj

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

rebuildall : cleanall all

