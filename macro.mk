
CFLAG ?=

define add_define_string
  CFLAG += -D${1}=\"${2}\"
endef

define add_define_int
  CFLAG += -D${1}=${2}
endef

define add_include_paths
  CFLAG += $(foreach dir,${1},-I$(dir))
endef
