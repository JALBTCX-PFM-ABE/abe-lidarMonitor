
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


#include <lasreader.hpp>
#include "slas.hpp"
#include "nvutility.hpp"

#include <QtCore>


static char out_str[2048];


/********************************************************************************************/
/*!

 - Function:    slas_read_point_data

 - Purpose:     Retrieve a LAS point data record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/20/15

 - Arguments:
                - fp             =    The file pointer
                - recnum         =    The record number of the LAS point data record to be
                                      retrieved (records start at 0)
                - lasheader      =    The LASheader retrieved from the LAS file
                - swap           =    Flag that indicates that the system is big endian and
                                      therefor we need to byte swap the records
                - record         =    The returned Simple LAS point data record

 - Returns:     int32_t          =    Negative number on error, 0 on success

*********************************************************************************************/

int32_t slas_read_point_data (FILE *fp, uint64_t recnum, LASheader *lasheader, uint8_t swap, SLAS_POINT_DATA *record)
{
  int32_t  x, y, z;
  int64_t  addr, pos;
  uint8_t  data[128], rets, cls;


  //  Check for record out of bounds.

  if (lasheader->version_minor < 4)
    {
      if (recnum >= (uint64_t) lasheader->number_of_point_records)
        {
          strcpy (out_str, QObject::tr ("Record number %L1 out of range :\nFunction: %2, Line: %3\n").arg (recnum).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
          fprintf (stderr, "%s", out_str);
          fflush (stderr);
          return (-1);
        }
    }
  else
    {
      if (recnum >= lasheader->extended_number_of_point_records)
        {
          strcpy (out_str, QObject::tr ("Record number %L1 out of range :\nFunction: %2, Line: %3\n").arg (recnum).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
          fprintf (stderr, "%s", out_str);
          fflush (stderr);
          return (-1);
        }
    }


  addr = (int64_t) lasheader->offset_to_point_data + (int64_t) lasheader->point_data_record_length * (int64_t) recnum;


  if (fseeko64 (fp, addr, SEEK_SET) < 0)
    {
      strcpy (out_str, QObject::tr ("Error on fseek :\n%1\nFunction: %2, Line: %3\n").arg (strerror (errno)).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
      fprintf (stderr, "%s", out_str);
      fflush (stderr);
      return (-2);
    }


  memset (record, 0, sizeof (SLAS_POINT_DATA));
  memset (data, 0, 128);


  //  Read the data buffer.

  if (!fread (data, lasheader->point_data_record_length, 1, fp))
    {
      strcpy (out_str, QObject::tr ("Error reading LAS record :\n%1\nFunction: %2, Line: %3\n").arg (strerror (errno)).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
      fprintf (stderr, "%s", out_str);
      fflush (stderr);
      return (-3);
    }


  //  Get the data out of the buffer.

  pos = 0;
  memcpy (&x, &data[pos], 4); pos += 4;
  memcpy (&y, &data[pos], 4); pos += 4;
  memcpy (&z, &data[pos], 4); pos += 4;
  memcpy (&record->intensity, &data[pos], 2); pos += 2;
  memcpy (&rets, &data[pos], 1); pos += 1;
  memcpy (&cls, &data[pos], 1); pos += 1;


  //  Check for point format ID above 5.

  if (lasheader->point_data_format > 5)
    {
      memcpy (&record->classification, &data[pos], 1); pos += 1;
      memcpy (&record->user_data, &data[pos], 1); pos += 1;
      memcpy (&record->scan_angle, &data[pos], 2); pos += 2;
    }
  else
    {
      memcpy (&record->scan_angle, &data[pos], 1); pos += 1;
      memcpy (&record->user_data, &data[pos], 1); pos += 1;
    }

  memcpy (&record->point_source_id, &data[pos], 2); pos += 2;


  switch (lasheader->point_data_format)
    {
    case 1:
    case 6:
      memcpy (&record->gps_time, &data[pos], 8); pos += 8;
      break;

    case 2:
      memcpy (&record->red, &data[pos], 2); pos += 2;
      memcpy (&record->green, &data[pos], 2); pos += 2;
      memcpy (&record->blue, &data[pos], 2); pos += 2;
      break;

    case 3:
    case 7:
      memcpy (&record->gps_time, &data[pos], 8); pos += 8;
      memcpy (&record->red, &data[pos], 2); pos += 2;
      memcpy (&record->green, &data[pos], 2); pos += 2;
      memcpy (&record->blue, &data[pos], 2); pos += 2;
      break;

    case 4:
    case 9:
      memcpy (&record->gps_time, &data[pos], 8); pos += 8;
      memcpy (&record->wavepacket_descriptor_index, &data[pos], 1); pos += 1;
      memcpy (&record->byte_offset_to_waveform_data, &data[pos], 8); pos += 8;
      memcpy (&record->waveform_packet_size, &data[pos], 4); pos += 4;
      memcpy (&record->return_point_waveform_location, &data[pos], 4); pos += 4;
      memcpy (&record->Xt, &data[pos], 4); pos += 4;
      memcpy (&record->Yt, &data[pos], 4); pos += 4;
      memcpy (&record->Zt, &data[pos], 4); pos += 4;
      break;

    case 5:
      memcpy (&record->gps_time, &data[pos], 8); pos += 8;
      memcpy (&record->red, &data[pos], 2); pos += 2;
      memcpy (&record->green, &data[pos], 2); pos += 2;
      memcpy (&record->blue, &data[pos], 2); pos += 2;
      memcpy (&record->wavepacket_descriptor_index, &data[pos], 1); pos += 1;
      memcpy (&record->byte_offset_to_waveform_data, &data[pos], 8); pos += 8;
      memcpy (&record->waveform_packet_size, &data[pos], 4); pos += 4;
      memcpy (&record->return_point_waveform_location, &data[pos], 4); pos += 4;
      memcpy (&record->Xt, &data[pos], 4); pos += 4;
      memcpy (&record->Yt, &data[pos], 4); pos += 4;
      memcpy (&record->Zt, &data[pos], 4); pos += 4;
      break;

    case 8:
      memcpy (&record->gps_time, &data[pos], 8); pos += 8;
      memcpy (&record->red, &data[pos], 2); pos += 2;
      memcpy (&record->green, &data[pos], 2); pos += 2;
      memcpy (&record->blue, &data[pos], 2); pos += 2;
      memcpy (&record->NIR, &data[pos], 2); pos += 2;
      break;

    case 10:
      memcpy (&record->gps_time, &data[pos], 8); pos += 8;
      memcpy (&record->red, &data[pos], 2); pos += 2;
      memcpy (&record->green, &data[pos], 2); pos += 2;
      memcpy (&record->blue, &data[pos], 2); pos += 2;
      memcpy (&record->NIR, &data[pos], 2); pos += 2;
      memcpy (&record->wavepacket_descriptor_index, &data[pos], 1); pos += 1;
      memcpy (&record->byte_offset_to_waveform_data, &data[pos], 8); pos += 8;
      memcpy (&record->waveform_packet_size, &data[pos], 4); pos += 4;
      memcpy (&record->return_point_waveform_location, &data[pos], 4); pos += 4;
      memcpy (&record->Xt, &data[pos], 4); pos += 4;
      memcpy (&record->Yt, &data[pos], 4); pos += 4;
      memcpy (&record->Zt, &data[pos], 4); pos += 4;
      break;
    }


  //  If we have to swap the fields of the record, do so.

  if (swap)
    {
      swap_int (&x);
      swap_int (&y);
      swap_int (&z);
      swap_short ((int16_t *) &record->intensity);
      swap_short ((int16_t *) &record->point_source_id);

      switch (lasheader->point_data_format)
        {
        case 1:
        case 6:
          swap_double (&record->gps_time);
          break;

        case 2:
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
          break;

        case 3:
        case 7:
          swap_double (&record->gps_time);
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
          break;

        case 4:
        case 9:
          swap_double (&record->gps_time);
          swap_double ((double *) &record->byte_offset_to_waveform_data);
          swap_int ((int32_t *) &record->waveform_packet_size);
          swap_float (&record->return_point_waveform_location);
          swap_float (&record->Xt);
          swap_float (&record->Yt);
          swap_float (&record->Zt);
          break;

        case 5:
          swap_double (&record->gps_time);
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
          swap_double ((double *) &record->byte_offset_to_waveform_data);
          swap_int ((int32_t *) &record->waveform_packet_size);
          swap_float (&record->return_point_waveform_location);
          swap_float (&record->Xt);
          swap_float (&record->Yt);
          swap_float (&record->Zt);
          break;

        case 8:
          swap_double (&record->gps_time);
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
          swap_short ((int16_t *) &record->NIR);
          break;

        case 10:
          swap_double (&record->gps_time);
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
          swap_short ((int16_t *) &record->NIR);
          swap_double ((double *) &record->byte_offset_to_waveform_data);
          swap_int ((int32_t *) &record->waveform_packet_size);
          swap_float (&record->return_point_waveform_location);
          swap_float (&record->Xt);
          swap_float (&record->Yt);
          swap_float (&record->Zt);
          break;
        }
    }


  //  Now put the rest of the data into the structure.

  record->x = ((double) x * lasheader->x_scale_factor) + lasheader->x_offset;
  record->y = ((double) y * lasheader->y_scale_factor) + lasheader->y_offset;
  record->z = (float) (((double) z * lasheader->z_scale_factor) + lasheader->z_offset);


  //  Check for point format ID above 5.

  if (lasheader->point_data_format > 5)
    {
      record->return_number = rets & 0x0f;
      record->number_of_returns = (rets & 0xf0) >> 4;
      record->scanner_channel = (cls & 0x30) >> 4;
      record->scan_direction_flag = (cls & 0x40) >> 6;
      record->edge_of_flightline = (cls & 0x80) >> 7;


      //  Just to make life easier we're breaking out the 4 bits of the classification flags.

      uint8_t new_cls = cls & 0x0f;
      record->overlap = (new_cls & 0x10) >> 4;
      record->synthetic = (new_cls & 0x20) >> 5;
      record->keypoint = (new_cls & 0x40) >> 6;
      record->withheld = (new_cls & 0x80) >> 7;
    }
  else
    {
      record->return_number = rets & 0x07;
      record->number_of_returns = (rets & 0x38) >> 3;
      record->scan_direction_flag = (rets & 0x40) >> 6;
      record->edge_of_flightline = (rets & 0x80) >> 7;
      record->classification = cls & 0x1f;


      //  Just to make life easier we're breaking out the 4 bits of the classification flags.

      record->synthetic = (cls & 0x20) >> 5;
      record->keypoint = (cls & 0x40) >> 6;
      record->withheld = (cls & 0x80) >> 7;
      record->overlap = 0;
    }


  return (0);
}



/********************************************************************************************/
/*!

 - Function:    slas_read_waveform_data

 - Purpose:     Retrieve a LAS waveform record.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/20/15

 - Arguments:
                - fp             =    The file pointer
                - lasheader      =    The LASheader retrieved from the LAS file
                - record         =    The Simple LAS point data record for which waveform data
                                      is to be retrieved
                - wf_packet_desc =    The array of 255 possible waveform packet descriptor
                                      records
                - wave           =    wf_packet_desc[record->wavepacket_descriptor_index].number_of_samples
                                      sized array of uint32_t variables (this is where we stuff the
                                      waveform data)

 - Returns:     int32_t          =    Negative number on error, 0 on success

*********************************************************************************************/

int32_t slas_read_waveform_data (FILE *fp, LASheader *lasheader, SLAS_POINT_DATA *record, SLAS_WAVEFORM_PACKET_DESCRIPTOR *wf_packet_desc, uint32_t *wave)
{
  int64_t  addr, pos;
  uint8_t  *wave_data;


  addr = lasheader->start_of_waveform_data_packet_record + record->byte_offset_to_waveform_data;


  if (fseeko64 (fp, addr, SEEK_SET) < 0)
    {
      strcpy (out_str, QObject::tr ("Error on fseek :\n%1\nFunction: %2, Line: %3\n").arg (strerror (errno)).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
      fprintf (stderr, "%s", out_str);
      fflush (stderr);
      return (-1);
    }


  wave_data = (uint8_t *) malloc (record->waveform_packet_size);


  //  Read the wave_data buffer.

  if (!fread (wave_data, record->waveform_packet_size, 1, fp))
    {
      strcpy (out_str, QObject::tr ("Error reading LAS waveform data :\n%1\nFunction: %2, Line: %3\n").arg (strerror (errno)).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
      fprintf (stderr, "%s", out_str);
      fflush (stderr);
      free (wave_data);
      return (-2);
    }


  int32_t ndx = record->wavepacket_descriptor_index;
  int32_t count = wf_packet_desc[ndx].number_of_samples;
  int32_t size = wf_packet_desc[ndx].bits_per_sample;

  pos = 0;
  for (int32_t i = 0 ; i < count ; i++)
    {
      wave[i] = bit_unpack (wave_data, pos, size); pos += size;
    }


  free (wave_data);


  return (0);
}



/********************************************************************************************/
/*!

 - Function:    slas_update_point_data

 - Purpose:     Updates the user modifiable fields of a LAS point data record without affecting
                the "non-modifiable" fields.

 - Author:      Jan C. Depner (area.based.editor@gmail.com)

 - Date:        03/20/15

 - Arguments:
                - fp             =    The file pointer
                - recnum         =    The record number of the LAS point data record to be written
		                      (records start at 0)
                - lasheader      =    The LASheader retrieved from the LAS file
                - swap           =    Flag that indicates that the system is big endian and
                                      therefor we need to byte swap the records
                - record         =    The SLAS_POINT_DATA structure to be updated

 - Returns:     int32_t          =    Negative number on error, 0 on success

*********************************************************************************************/

int32_t slas_update_point_data (FILE *fp, uint64_t recnum, LASheader *lasheader, uint8_t swap, SLAS_POINT_DATA *record)
{
  int32_t   pos;
  uint8_t   data[128], cls;
  int64_t   addr;


  //  Check for record out of bounds.

  if (lasheader->version_minor < 4)
    {
      if (recnum >= (uint64_t) lasheader->number_of_point_records)
        {
          strcpy (out_str, QObject::tr ("Record number %L1 out of range :\nFunction: %2, Line: %3\n").arg (recnum).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
          fprintf (stderr, "%s", out_str);
          fflush (stderr);
          return (-1);
        }
    }
  else
    {
      if (recnum >= lasheader->extended_number_of_point_records)
        {
          strcpy (out_str, QObject::tr ("Record number %L1 out of range :\nFunction: %2, Line: %3\n").arg (recnum).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
          fprintf (stderr, "%s", out_str);
          fflush (stderr);
          return (-1);
        }
    }


  //  Move to the beginning of the requested record in the file.

  addr = (int64_t) lasheader->offset_to_point_data + (int64_t) lasheader->point_data_record_length * (int64_t) recnum;


  if (fseeko64 (fp, addr, SEEK_SET) < 0)
    {
      strcpy (out_str, QObject::tr ("Error on fseek :\n%1\nFunction: %2, Line: %3\n").arg (strerror (errno)).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
      fprintf (stderr, "%s", out_str);
      fflush (stderr);
      return (-2);
    }


  memset (data, 0, 128);


  //  Read the data buffer.

  if (!fread (data, lasheader->point_data_record_length, 1, fp))
    {
      strcpy (out_str, QObject::tr ("Error reading LAS record :\n%1\nFunction: %2, Line: %3\n").arg (strerror (errno)).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
      fprintf (stderr, "%s", out_str);
      fflush (stderr);
      return (-3);
    }


  //  Extract only the "modifiable" fields.

  pos = 0;

  pos += 4;   //  X
  pos += 4;   //  Y
  pos += 4;   //  Z
  pos += 2;   //  Intensity
  pos += 1;   //  Formats 0 through 5 = Return Number, Number of Returns, Scan Direction Flag, Edge of Flightline
              //  Formats 6 through 10 = Return Number, Number of Returns


  //  We need to modify this byte in different ways for formats above and below 5.  For 6 through 10 we have to preserve
  //  the Scanner Channel, Scan Direction Flag, and the Edge of Flightline.  For 0 through 5 we will be replacing the
  //  classification part of the classification flags as well as the bit fields.

  memcpy (&cls, &data[pos], 1);


  //  Check for point format ID above 5.

  if (lasheader->point_data_format > 5)
    {
      //  Set the "synthetic" bit.

      if (record->synthetic)
        {
          cls |= 0x01;
        }
      else
        {
          cls &= 0xfe;
        }


      //  Set the "keypoint" bit.

      if (record->keypoint)
        {
          cls |= 0x02;
        }
      else
        {
          cls &= 0xfd;
        }


      //  Set the "withheld" bit.

      if (record->withheld)
        {
          cls |= 0x04;
        }
      else
        {
          cls &= 0xfb;
        }


      //  Set the "overlap" bit.

      if (record->overlap)
        {
          cls |= 0x08;
        }
      else
        {
          cls &= 0xf7;
        }
    }
  else
    {
      //  Set the classification value first and then add in the bit fields.

      if (record->classification > 31)
        {
          strcpy (out_str, QObject::tr ("Classification value %1 out of bounds :\nFunction: %2, Line: %3\n").arg (record->classification).arg
                  (__FUNCTION__).arg (__LINE__).toUtf8 ());
          fprintf (stderr, "%s", out_str);
          fflush (stderr);
          return (-4);
        }
      cls = record->classification;


      //  Set the "synthetic" bit.

      if (record->synthetic)
        {
          cls |= 0x20;
        }
      else
        {
          cls &= 0xdf;
        }


      //  Set the "key point" bit.

      if (record->keypoint)
        {
          cls |= 0x40;
        }
      else
        {
          cls &= 0xbf;
        }


      //  Set the "withheld" bit.

      if (record->withheld)
        {
          cls |= 0x80;
        }
      else
        {
          cls &= 0x7f;
        }
    }


  //  Replace the byte with the bit flags (and possibly classification) then increment the byte position.

  data[pos] = cls; pos += 1;


  //  Check for point format ID above 5.

  if (lasheader->point_data_format > 5)
    {
      data[pos] = record->classification; pos += 1;
      data[pos] = record->user_data; pos += 1;
      pos += 2;  //  Scan Angle
    }
  else
    {
      pos += 1;  //  Scan Angle
      data[pos] = record->user_data; pos += 1;
    }

  pos += 2;  //  Point Source ID


  switch (lasheader->point_data_format)
    {
    case 1:
    case 6:
      pos += 8;  //  GPS Time
      break;

    case 2:
      if (swap)
        {
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
        }
      memcpy (&data[pos], &record->red, 2); pos += 2;
      memcpy (&data[pos], &record->green, 2); pos += 2;
      memcpy (&data[pos], &record->blue, 2); pos += 2;
      break;

    case 3:
    case 7:
      pos += 8;  //  GPS Time
      if (swap)
        {
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
        }
      memcpy (&data[pos], &record->red, 2); pos += 2;
      memcpy (&data[pos], &record->green, 2); pos += 2;
      memcpy (&data[pos], &record->blue, 2); pos += 2;
      break;

    case 4:
    case 9:
      pos += 8;  //  GPS Time
      break;

    case 5:
      pos += 8;  //  GPS Time
      if (swap)
        {
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
        }
      memcpy (&data[pos], &record->red, 2); pos += 2;
      memcpy (&data[pos], &record->green, 2); pos += 2;
      memcpy (&data[pos], &record->blue, 2); pos += 2;
      break;

    case 8:
      pos += 8;  //  GPS Time
      if (swap)
        {
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
          swap_short ((int16_t *) &record->NIR);
        }
      memcpy (&data[pos], &record->red, 2); pos += 2;
      memcpy (&data[pos], &record->green, 2); pos += 2;
      memcpy (&data[pos], &record->blue, 2); pos += 2;
      memcpy (&data[pos], &record->NIR, 2); pos += 2;
      break;

    case 10:
      pos += 8;  //  GPS Time
      if (swap)
        {
          swap_short ((int16_t *) &record->red);
          swap_short ((int16_t *) &record->green);
          swap_short ((int16_t *) &record->blue);
          swap_short ((int16_t *) &record->NIR);
        }
      memcpy (&data[pos], &record->red, 2); pos += 2;
      memcpy (&data[pos], &record->green, 2); pos += 2;
      memcpy (&data[pos], &record->blue, 2); pos += 2;
      memcpy (&data[pos], &record->NIR, 2); pos += 2;
      break;
    }


  //  Go back to the beginning of the record.

  if (fseeko64 (fp, addr, SEEK_SET) < 0)
    {
      strcpy (out_str, QObject::tr ("Error on fseek :\n%1\nFunction: %2, Line: %3\n").arg (strerror (errno)).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
      fprintf (stderr, "%s", out_str);
      fflush (stderr);
      return (-5);
    }


  //  Write the record.

  if (!fwrite (data, lasheader->point_data_record_length, 1, fp))
    {
      strcpy (out_str, QObject::tr ("Error writing LAS record :\n%1\nFunction: %2, Line: %3\n").arg (strerror (errno)).arg (__FUNCTION__).arg (__LINE__).toUtf8 ());
      fprintf (stderr, "%s", out_str);
      fflush (stderr);
      return (-6);
    }


  return (0);
}
