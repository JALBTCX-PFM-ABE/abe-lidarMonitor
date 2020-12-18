
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



/*  lidarMonitor class definitions.  */

#ifndef __SLAS_HPP__
#define __SLAS_HPP__

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>


typedef struct
{
  double                      x;
  double                      y;
  float                       z;
  uint16_t                    intensity;
  uint8_t                     return_number;
  uint8_t                     number_of_returns;
  uint8_t                     scanner_channel;
  uint8_t                     scan_direction_flag;
  uint8_t                     edge_of_flightline;
  uint8_t                     classification;                  //!<  Modifiable in slas_update_point_data.
  uint8_t                     user_data;                       //!<  Modifiable in slas_update_point_data.
  int16_t                     scan_angle;
  uint16_t                    point_source_id;
  double                      gps_time;
  uint16_t                    red;                             //!<  Modifiable in slas_update_point_data.
  uint16_t                    green;                           //!<  Modifiable in slas_update_point_data.
  uint16_t                    blue;                            //!<  Modifiable in slas_update_point_data.
  uint16_t                    NIR;                             //!<  Modifiable in slas_update_point_data.
  uint8_t                     wavepacket_descriptor_index;
  uint64_t                    byte_offset_to_waveform_data;
  uint32_t                    waveform_packet_size;
  float                       return_point_waveform_location;
  float                       Xt;
  float                       Yt;
  float                       Zt;
  uint8_t                     withheld;                        //!<  Modifiable in slas_update_point_data.
  uint8_t                     keypoint;                        //!<  Modifiable in slas_update_point_data.
  uint8_t                     synthetic;                       //!<  Modifiable in slas_update_point_data.
  uint8_t                     overlap;                         //!<  Modifiable in slas_update_point_data.
} SLAS_POINT_DATA;


typedef struct
{
  int32_t                     index;
  uint8_t                     bits_per_sample;
  uint8_t                     compression_type;
  uint32_t                    number_of_samples;
  uint32_t                    temporal_spacing;
  double                      digitizer_gain;
  double                      digitizer_offset;
} SLAS_WAVEFORM_PACKET_DESCRIPTOR;


int32_t slas_read_point_data (FILE *fp, uint64_t recnum, LASheader *lasheader, uint8_t swap, SLAS_POINT_DATA *record);
int32_t slas_read_waveform_data (FILE *fp, LASheader *lasheader, SLAS_POINT_DATA *record, SLAS_WAVEFORM_PACKET_DESCRIPTOR *wf_packet_desc, uint32_t *wave);
int32_t slas_update_point_data (FILE *fp, uint64_t recnum, LASheader *lasheader, uint8_t swap, SLAS_POINT_DATA *record);


#endif
