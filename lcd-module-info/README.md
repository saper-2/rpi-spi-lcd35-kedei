# KeDei's LCD Versions

So far there is a 9 versions of displays (at 2017-08-27). I'm pretty sure that versions are just for identifing the production series that KeDei launch. 
There are different lcd modules in each batch but subversions might not differ form each other so much (see v60,61,62,63 especially).

# Displays version: 6.1 , 6.2 , 6.3
For v6.2 is already known init seq. but the sequence don't work certainly with v6.3, so my guess is that the driver setup all registers, even those that doesn't exists in v6.2, but do exists in v6.3 . And the lcd controller just ignore the writes to register that don't exists in v6.2 ...


# Links
* TinDRM: https://github.com/notro/tinydrm/blob/master/README.md
* TinyDRM for v5.0 display: https://github.com/FREEWING-JP/tinydrm/tree/feature/kedei_35_v50
