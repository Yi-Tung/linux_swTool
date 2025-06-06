
## initial variables ##
target ?= exe
target_dir ?= output

main_c := swTool.c
modules_c := $(shell find modules -name '*.c')
modules_dir := $(shell find modules -type d)

src_c := $(main_c) $(modules_c)
src_o := $(patsubst %.c,%.o,$(src_c))

GCC := cc
CFLAG := -std=c11 -O2 -Werror $(foreach dir,$(modules_dir),-I$(dir))

## include external files ##
include macro.mk

## add some defined variables to c code ##
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
	$(GCC) $(CFLAG) -c $< -o $@


## set some methods ##
.PHONY: clean

clean:
	@-rm -f $(src_o)
	@-rm -f $(target) a.out
	@-rm -rf $(target_dir)
