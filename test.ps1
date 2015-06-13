$MAPNIK_SDK = "$pwd\mapnik-sdk"
$ENV:PATH = "$MAPNIK_SDK\lib" + ";" + $ENV:Path
$ENV:PATH = "$MAPNIK_SDK\bin" + ";" + $ENV:Path
$ENV:ICU_DATA = "$MAPNIK_SDK\share\icu"
$ENV:PROJ_LIB = "$MAPNIK_SDK\share\proj"
$ENV:GDAL_DATA = "$MAPNIK_SDK\share\gdal"
.\test\test.exe
