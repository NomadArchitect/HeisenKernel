M:=\
    $(DIR_SRC)/modules/arch/x86/smp/smp.ko \

N:=\
    $(DIR_SRC)/modules/arch/x86/smp/smp.ko.map \

O:=\
    $(DIR_SRC)/modules/arch/x86/smp/main.o \
    $(DIR_SRC)/modules/arch/x86/smp/init.o \

L:=\
    $(DIR_LIB)/fudge/fudge.a \

include $(DIR_MK)/kmod.mk
