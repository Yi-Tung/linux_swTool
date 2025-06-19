function compile_main_c() {
  local main_c=$(find main_c -name '*.c')
  local exec_item=""

  for item in $main_c
  do
    export main_c=$item
    make
    if [ $? -ne 0 ]
    then
      return -1
    else
      exec_item=${item%.*}
      if [ -f $exec_item ] && [ -d $target_dir ]
      then
        mv $exec_item $target_dir/.
      fi
    fi
  done

  return 0
}


export target_dir='output'

mode='release'
main_c_count=""
exec_count=""


if [[ ! "$1" = "" ]]
then
  mode="$1"
fi

if [ ! -d "$target_dir" ]
then
  mkdir $target_dir
fi

export mode="$mode"

case "$mode" in
  'debug')
    export log_switch=1
    compile_main_c
    ;;
  'release')
    compile_main_c
    rm $(find . -name '*.o') >/dev/null 2>&1
    ;;
  *)
    echo "$0: invalid option '$1'"
    exit
    ;;
esac

main_c_count=$(find main_c -maxdepth 1 -type f -a -name '*.c' | wc -l)
exec_count=$(find output -maxdepth 1 -type f | wc -l)

if [[ "$main_c_count" = "$exec_count" ]]
then
  echo "======Build successful======"
else
  echo "======Build failed======"
fi
