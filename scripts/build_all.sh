#!/bin/bash

set -x

/snap/clion/151/bin/cmake/linux/bin/cmake --build /home/pavel/reph/cmake-build-debug --target monitor -- -j 3
/snap/clion/151/bin/cmake/linux/bin/cmake --build /home/pavel/reph/cmake-build-debug --target client -- -j 3
/snap/clion/151/bin/cmake/linux/bin/cmake --build /home/pavel/reph/cmake-build-debug --target osd -- -j 3