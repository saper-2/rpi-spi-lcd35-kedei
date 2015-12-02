# Version 2

This worked few hours... And I think, I made a short so one of 4094 gone bad and lcd stopped working (partially). But this worked for a moment fine :) . I'm going to make a board for fbtft spi interface/adapter and test lcd with it again.

Working movie: https://www.youtube.com/watch?v=wGVt_bYuE5g
Sorry it just a test of init, not actual working.

In photos, you can see that there is something working but not very much :/

## Scripts

Just run script ```sudo ./ili.sh``` and it will do everything :) , or just run to init lcd:
```
sudo ./ili-init.sh
sudo con2fbmap 2 1
```
Switch to tty2 (*2* - tty2 , *1* - framebuffer 1 )...

To unload fbtft module & restore tty2 to fb0:
```
sudo ./unili.sh
```
