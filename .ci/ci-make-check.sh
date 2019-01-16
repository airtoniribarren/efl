#!/bin/sh

. .ci/travis.sh
if [ "$1" = "release-ready" ] ; then
  exit 0
fi

if [ "$1" = "mingw" ] ; then
  exit 0
fi

if [ "$1" = "coverity" ] ; then
  exit 0
fi

#T7151
if [ "$1" = "options-enabled" ] || [ "$1" = "options-disabled" ] ; then
  exit 0
fi

NUM_TRIES=5

travis_fold check "ninja test"
if [ "$DISTRO" != "" ] ; then
  for tries in $(seq 1 ${NUM_TRIES}); do
    if [ "$1" = "asan" ]; then
      (docker exec --env EINA_LOG_BACKTRACE="0" --env EIO_MONITOR_POLL=1 --env ASAN_OPTIONS=detect_leaks=0:abort_on_error=0 $(cat $HOME/cid) meson test -t 120 -C build --wrapper dbus-launch) && break
    else
      (docker exec --env EINA_LOG_BACKTRACE="0" --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) meson test -t 120 -C build --wrapper dbus-launch ) && break
    fi
    docker exec --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) cat build/meson-logs/testlog-dbus-launch.txt
    if [ $tries != ${NUM_TRIES} ] ; then echo "tests failed, trying again!" ; fi
      false
  done
  if [ "$1" = "exactness" ] ; then
#docker exec  --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) ninja -C build install
    docker exec  --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) sh -c 'cd /exactness-elm-data; git pull'
    docker exec  --env EIO_MONITOR_POLL=1 $(cat $HOME/cid) sh -c 'cd /exactness-elm-data; git checkout origin/devs/stefan/init-shots-docker-travis-ci -b docker'
    docker exec  --env EIO_MONITOR_POLL=1 --env LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/usr/local/lib:/usr/local/lib64  $(cat $HOME/cid) EINA_LOG_LEVELS_GLOB=eina_*:0,ecore*:0,efreet*:0,eldbus:0,elementary:0 exactness -j 20 -b /exactness-elm-data/default-profile -p /exactness-elm-data/default-profile/tests.txt
  fi
fi
ret=$?
travis_endfold check

exit $ret
