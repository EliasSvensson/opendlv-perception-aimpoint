# opendlv-perception-aimpoint
Detects cones and sends out a single aimpoint for the kiwi car.

docker run --rm -ti --init --net=host --ipc=host -v /tmp:/tmp -e DISPLAY=$DISPLAY perception --cid=111 --name=img.argb --width=1280 --height=720 --verbose --Bl0=115 --Bl1=100 --Bl2=30 --Bh0=145 --Bh1=255 --Bh2=200 --Yl0=10 --Yl1=40 --Yl2=46 --Yh0=34 --Yh1=255 --Yh2=200 --crop_up=430 --crop_down=570 --crop_up_min_cone_area=600 --crop_down_min_cone_area=1000 --max_unmatch_y=30 --clockwise=0
