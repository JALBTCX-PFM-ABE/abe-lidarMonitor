
/*********************************************************************************************

    This is public domain software that was developed by or for the U.S. Naval Oceanographic
    Office and/or the U.S. Army Corps of Engineers.

    This is a work of the U.S. Government. In accordance with 17 USC 105, copyright protection
    is not available for any work of the U.S. Government.

    Neither the United States Government, nor any employees of the United States Government,
    nor the author, makes any warranty, express or implied, without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, or assumes any liability or
    responsibility for the accuracy, completeness, or usefulness of any information,
    apparatus, product, or process disclosed, or represents that its use would not infringe
    privately-owned rights. Reference herein to any specific commercial products, process,
    or service by trade name, trademark, manufacturer, or otherwise, does not necessarily
    constitute or imply its endorsement, recommendation, or favoring by the United States
    Government. The views and opinions of authors expressed herein do not necessarily state
    or reflect those of the United States Government, and shall not be used for advertising
    or product endorsement purposes.
*********************************************************************************************/


/****************************************  IMPORTANT NOTE  **********************************

    Comments in this file that start with / * ! or / / ! are being used by Doxygen to
    document the software.  Dashes in these comment blocks are used to create bullet lists.
    The lack of blank lines after a block of dash preceeded comments means that the next
    block of dash preceeded comments is a new, indented bullet list.  I've tried to keep the
    Doxygen formatting to a minimum but there are some other items (like <br> and <pre>)
    that need to be left alone.  If you see a comment that starts with / * ! or / / ! and
    there is something that looks a bit weird it is probably due to some arcane Doxygen
    syntax.  Be very careful modifying blocks of Doxygen comments.

*****************************************  IMPORTANT NOTE  **********************************/




#ifndef VERSION

#define     VERSION     "PFM Software - lidarMonitor V3.19 - 01/16/20"

#endif

/*

    Version 1.0
    Jan C. Depner
    11/25/08

    First working version.


    Version 1.01
    Jan C. Depner
    11/28/08

    Added kill_switch option.


    Version 1.02
    Jan C. Depner
    04/13/09

    Use NINT instead of typecasting to NV_INT32 when saving Qt window state.  Integer truncation was inconsistent on Windows.


    Version 1.03
    Jan C. Depner
    06/15/09

    Added support for PFM_CHARTS_HOF_DATA and PFM_WLF_DATA.


    Version 1.04
    Jan C. Depner
    09/16/09

    Set killed flag in abe_share when program exits other than from kill switch from parent.


    Version 1.05
    Jan C. Depner
    04/29/10

    Fixed posfix and fixpos calls to use numeric constants instead of strings for type.


    Version 1.06
    Jan C. Depner
    06/25/10

    Added support for PFM_HAWKEYE_DATA.


    Version 2.00
    Jan C. Depner
    09/20/10

    Changed name to lidarMonitor from chartsMonitor since we're now supporting more data types than
    just CHARTS.


    Version 2.01
    Jan C. Depner
    01/06/11

    Correct problem with the way I was instantiating the main widget.  This caused an intermittent error on Windows7/XP.


    Version 2.02
    Jan C. Depner
    02/25/11

    Switched to using PFM_HAWKEYE_HYDRO_DATA and PFM_HAWKEYE_TOPO_DATA to replace the deprecated PFM_HAWKEYE_DATA.


    Version 2.10
    Jan C. Depner
    07/21/11

    Added DRAFT CZMIL data support.


    Version 2.11
    Jan C. Depner
    11/30/11

    Converted .xpm icons to .png icons.


    Version 2.12
    Jan C. Depner
    06/20/12

    Removed support for CZMIL until the formats are completely defined.


    Version 2.13
    Jan C. Depner
    07/16/12

    - Added support for the preliminary CZMIL CPF format data.


    Version 2.14
    Jan C. Depner (PFM Software)
    06/05/13

    - Fixed CZMIL timestamp print bug.


    Version 2.15
    Jan C. Depner (PFM Software)
    06/19/13

    - Really fixed CZMIL timestamp print bug.  Since I was using Qt's sprintf function it
      doesn't understand %I64d and does understand %lld.


    Version 2.16
    Jan C. Depner (PFM Software)
    12/09/13

    Switched to using .ini file in $HOME (Linux) or $USERPROFILE (Windows) in the ABE.config directory.  Now
    the applications qsettings will not end up in unknown places like ~/.config/navo.navy.mil/blah_blah_blah on
    Linux or, in the registry (shudder) on Windows.


    Version 2.17
    Jan C. Depner (PFM Software)
    02/03/14

    Added record number to text.


    Version 2.18
    Jan C. Depner (PFM Software)
    02/05/14

    Added probabilty of detection, validity reason, and processing mode to CPF text.


    Version 2.19
    Jan C. Depner (PFM Software)
    03/17/14

    Removed WLF support.  Top o' the mornin' to ye!


    Version 2.20
    Jan C. Depner (PFM Software)
    06/25/14

    Added display of associated CSF data for CZMIL.


    Version 2.21
    Jan C. Depner (PFM Software)
    07/01/14

    - Replaced all of the old, borrowed icons with new, public domain icons.  Mostly from the Tango set
      but a few from flavour-extended and 32pxmania.


    Version 2.30
    Jan C. Depner (PFM Software)
    07/13/14

    - Added support for LAS files.  Only geographic or projected UTM using NAD83 zones 3-23 or WGS86
      zones 1-60 north or south.


    Version 2.31
    Jan C. Depner (PFM Software)
    07/17/14

    - No longer uses liblas.  Now uses libslas (my own API for LAS).


    Version 2.32
    Jan C. Depner (PFM Software)
    07/23/14

    - Switched from using the old NV_INT64 and NV_U_INT32 type definitions to the C99 standard stdint.h and
      inttypes.h sized data types (e.g. int64_t and uint32_t).


    Version 2.33
    Jan C. Depner (PFM Software)
    09/04/14

    Support for new CZMIL v2 fields.


    Version 2.34
    Jan C. Depner (PFM Software)
    09/16/14

    Ignores missing CSF file.


    Version 2.35
    Jan C. Depner (PFM Software)
    10/21/14

    More CZMIL v2 changes (specifically, removal of validity_reason).


    Version 2.36
    Jan C. Depner (PFM Software)
    02/11/15

    - Got rid of the compile time CZMIL naming BS.  It now checks the ABE_CZMIL environment variable at run time.


    Version 2.37
    Jan C. Depner (PFM Software)
    02/16/15

    - To give better feedback to shelling programs in the case of errors I've added the program name to all
      output to stderr.


    Version 3.00
    Jan C. Depner (PFM Software)
    03/13/15

    - Removed LAS support since we're switching to rapidlasso GmbH LASlib and LASzip.  Neither of those packages
      support random reads.


    Version 3.01
    Jan C. Depner (PFM Software)
    03/14/15

    - Since AHAB Hawkeye has switched to LAS format I have removed support for the old Hawkeye I binary format.


    Version 3.02
    Jan C. Depner (PFM Software)
    03/20/15

    - Added LAS support back in.  All I had to do was update the record reading part of libslas
      to handle 1.3 and 1.4.


    Version 3.03
    Jan C. Depner (PFM Software)
    03/27/15

    - Added slas_update_point_data to slas.cpp.


    Version 3.04
    Jan C. Depner (PFM Software)
    04/09/15

    - Modified slas.cpp to use extended_number_of_point_records for LAS v1.4 files.


    Version 3.05
    Jan C. Depner (PFM Software)
    07/03/15

    - Finally straightened out the GPS time/leap second problem (I hope).


    Version 3.06
    Jan C. Depner (PFM Software)
    07/09/15

    - Now handles Riegl LAS files that don't have GeographicTypeGeoKey and ProjectedCSTypeGeoKey defined in
      the GeoKeyDirectoryTag (34735) required VLR.


    Version 3.07
    Jan C. Depner (PFM Software)
    02/11/16

    - Added CZMIL CSF range value (don't know how I skipped that the first time).


    Version 3.08
    Jan C. Depner (PFM Software)
    05/02/16

    - Now that we're accepting user input WKT for LAS files in pfmLoader we can't very easily check for the coordinate
      system in here.  I've removed all of the Proj.4 stuff.  Now it just prints that original X and Y regardless of
      units or projection.


    Version 3.09
    Jan C. Depner (PFM Software)
    07/03/16

    - Changed description of ip_rank to "Non-water surface flag" to reflect changes made in CZMIL API v3.0.


    Version 3.10
    Jan C. Depner (PFM Software)
    08/04/16

    - Added "user data", "d_index", and "d_index_cube" to output to reflect changes made in CZMIL API v3.0.


    Version 3.11
    Jan C. Depner (PFM Software)
    08/24/16

    - Added Optech probability of cube detection and probability of a false alarm to the D_index_cube
      line.
    - Added ability to set probability of false alarm to Preferences.


    Version 3.12
    Jan C. Depner (PFM Software)
    08/27/16

    - Now uses the same font as all other ABE GUI apps.  Font can only be changed in pfmView Preferences.


    Version 3.13
    Jan C. Depner (PFM Software)
    08/29/16

    - Cleaned up the D_index_cube output.


    Version 3.14
    Jan C. Depner (PFM Software)
    04/19/17

    - For CZMIL data, interprets the ip_rank properly for land/water/hybrid processed returns.
    - For CZMIL data, outputs a better description for the ip_rank value (now called "Interest point flag").


    Version 3.15
    Jan C. Depner (PFM Software)
    06/23/17

    -  Removed redundant functions from slas.cpp that are available in the nvutility library.


    Version 3.16
    Jan C. Depner (PFM Software)
    09/26/17

    - A bunch of changes to support doing translations in the future.  There is a generic
      lidarMonitor_xx.ts file that can be run through Qt's "linguist" to translate to another language.


    Version 3.17
    Jan C. Depner (PFM Software)
    07/09/18

    - Fixed format bug with waveform byte offset.



    Version 3.18
    Jan C. Depner (PFM Software)
    09/12/19

    - Added display of (pseudo-)reflectance "extra bytes" field in LASv1.4.


    Version 3.19
    Jan C. Depner (PFM Software)
    01/16/20

    - Multiplied (pseudo-)reflectance "extra bytes" field in LASv1.4 by 0.01.

*/
