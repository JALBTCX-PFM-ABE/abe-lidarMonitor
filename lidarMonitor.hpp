
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

#ifndef __LIDARMONITOR_H__
#define __LIDARMONITOR_H__

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <getopt.h>
#include <cmath>

#include "nvutility.h"
#include "pfm.h"
#include "fixpos.h"
#include "pfm_extras.h"
#include "nvmap.hpp"
#include "FileHydroOutput.h"
#include "FileTopoOutput.h"
#include "czmil.h"
#include <lasreader.hpp>
#include "ABE.h"
#include "slas.hpp"
#include "version.hpp"

#ifdef NVWIN3X
    #include "windows_getuid.h"
#endif

#include <QtCore>
#include <QtGui>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif


#define GCS_NAD83 4269
#define GCS_WGS_84 4326


//   WGS84 / UTM northern hemisphere: 326zz where zz is UTM zone number
//   WGS84 / UTM southern hemisphere: 327zz where zz is UTM zone number
//   US State Plane (NAD83): 269xx

#define PCS_NAD83_UTM_zone_3N 26903
#define PCS_NAD83_UTM_zone_23N 26923
#define PCS_WGS84_UTM_zone_1N 32601
#define PCS_WGS84_UTM_zone_60N 32660
#define PCS_WGS84_UTM_zone_1S 32701
#define PCS_WGS84_UTM_zone_60S 32760


class lidarMonitor:public QMainWindow
{
  Q_OBJECT 


public:

  lidarMonitor (int32_t *argc = 0, char **argv = 0, QWidget *parent = 0);
  ~lidarMonitor ();


protected:

  int32_t             key, pfm_handle[MAX_ABE_PFMS], num_pfms, las_reserved, false_alarm;

  char                program_version[256], progname[256];

  uint32_t            kill_switch;

  double              gps_start_time;  //  For LAS data using GPS time instead of GPS seconds of the week.

  SLAS_POINT_DATA     slas;

  QSharedMemory       *abeShare;

  ABE_SHARE           *abe_share;

  PFM_OPEN_ARGS       open_args[MAX_ABE_PFMS];        

  QTextEdit           *listBox;

  int32_t             window_width, window_height, window_x, window_y;

  QPushButton         *bRestoreDefaults; 

  uint8_t             force_redraw, lock_track;

  uint32_t            ac[1];

  int32_t             pos_format;

  QButtonGroup        *bGrp;

  QDialog             *prefsD;

  QToolButton         *bQuit, *bPrefs;

  QComboBox           *falseAlarm;

  QFont               font;                       //  Font used for all ABE GUI applications


  void envin ();
  void envout ();

  void closeEvent (QCloseEvent *);


protected slots:

  void slotResize (QResizeEvent *e);
  
  void trackCursor ();

  void slotHelp ();

  void slotQuit ();

  void slotPrefs ();
  void slotPosClicked (int id);
  void slotFalseAlarmChanged (int index);
  void slotClosePrefs ();

  void slotRestoreDefaults ();
  
  void about ();
  void aboutQt ();


 private:

};

#endif
