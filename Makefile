
## initial variable ##
target ?= exe
target_dir ?= output

src_c := swTool.c swLog.c
src_o := swTool.o swLog.o

GCC := cc
CFLAG := -O2 -Werror

## include external files
include macro.mk

# add some defined variables to c code ##
ifneq ($(mode),)
  $(eval $(call add_define_string,mode,$(mode)))
endif

ifneq ($(log_switch),)
  $(eval $(call add_define_int,log_switch,$(log_switch)))
endif

## compile source code ##
$(target): $(src_o)
	$(GCC) -o $(target) $(src_o) 

## compile .c code file to .o file ##
%.o: %.c
	$(GCC) $(CFLAG) -c $<


## set some methods ##
.PHONY: clean

clean:
	@-rm -f $(src_o)
	@-rm -f $(target) a.out
	@-rm -rf $(target_dir)
