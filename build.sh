
export target='exe'
export target_dir='output'

mode='release'

if [[ ! "$1" = "" ]]
then
  mode="$1"
fi

export mode="$mode"

case "$mode" in
  'debug')
    export log_switch=1
    make
    ;;
  'release')
    make
    rm $(find . -name '*.o') >/dev/null 2>&1
    ;;
  *)
    echo "$0: invalid option '$1'"
    ;;
esac



if [ ! -d "$target_dir" ]
then
  mkdir $target_dir
fi

if [ -f "$target" ]
then
  mv $target $target_dir/.
  echo "======Build successful======"
else
  echo "======Build failed======"
fi
