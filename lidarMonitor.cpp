
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



#include "lidarMonitor.hpp"
#include "lidarMonitorHelp.hpp"


double settings_version = 1.00;


lidarMonitor::lidarMonitor (int32_t *argc, char **argv, QWidget * parent):
  QMainWindow (parent, 0)
{
  extern char     *optarg;
  
  strcpy (progname, argv[0]);
  lock_track = NVFalse;


  QResource::registerResource ("/icons.rcc");


  //  Have to set the focus policy or keypress events don't work properly at first in Focus Follows Mouse mode

  setFocusPolicy (Qt::WheelFocus);
  setFocus (Qt::ActiveWindowFocusReason);


  //  Set the main icon

  setWindowIcon (QIcon (":/icons/charts_monitor.png"));


  kill_switch = ANCILLARY_FORCE_EXIT;

  int32_t option_index = 0;
  while (NVTrue) 
    {
      static struct option long_options[] = {{"shared_memory_key", required_argument, 0, 0},
                                             {"kill_switch", required_argument, 0, 0},
                                             {0, no_argument, 0, 0}};

      char c = (char) getopt_long (*argc, argv, "o", long_options, &option_index);
      if (c == -1) break;

      switch (c) 
        {
        case 0:

          switch (option_index)
            {
            case 0:
              sscanf (optarg, "%d", &key);
              break;


            case 1:
              sscanf (optarg, "%d", &kill_switch);
              break;

            default:
              char tmp;
              sscanf (optarg, "%1c", &tmp);
              ac[option_index] = (uint32_t) tmp;
              break;
            }
          break;
        }
    }


  envin ();


  // Set the application font

  QApplication::setFont (font);


  setWindowTitle (QString (program_version));


  /******************************************* IMPORTANT NOTE ABOUT SHARED MEMORY **************************************** \

      This is a little note about the use of shared memory within the Area-Based Editor (ABE) programs.  If you read
      the Qt documentation (or anyone else's documentation) about the use of shared memory they will say "Dear [insert
      name of omnipotent being of your choice here], whatever you do, always lock shared memory when you use it!".
      The reason they say this is that access to shared memory is not atomic.  That is, reading shared memory and then
      writing to it is not a single operation.  An example of why this might be important - two programs are running,
      the first checks a value in shared memory, sees that it is a zero.  The second program checks the same location
      and sees that it is a zero.  These two programs have different actions they must perform depending on the value
      of that particular location in shared memory.  Now the first program writes a one to that location which was
      supposed to tell the second program to do something but the second program thinks it's a zero.  The second program
      doesn't do what it's supposed to do and it writes a two to that location.  The two will tell the first program 
      to do something.  Obviously this could be a problem.  In real life, this almost never occurs.  Also, if you write
      your program properly you can make sure this doesn't happen.  In ABE we almost never lock shared memory because
      something much worse than two programs getting out of sync can occur.  If we start a program and it locks shared
      memory and then dies, all the other programs will be locked up.  When you look through the ABE code you'll see
      that we very rarely lock shared memory, and then only for very short periods of time.  This is by design.

  \******************************************* IMPORTANT NOTE ABOUT SHARED MEMORY ****************************************/


  //  Get the shared memory area.  If it doesn't exist, quit.  It should have already been created 
  //  by pfmView or fileEdit3D.

  QString skey;
  skey.sprintf ("%d_abe", key);

  abeShare = new QSharedMemory (skey);

  if (!abeShare->attach (QSharedMemory::ReadWrite))
    {
      fprintf (stderr, "%s %s %s %d - abeShare - %s\n", progname, __FILE__, __FUNCTION__, __LINE__, strerror (errno));
      exit (-1);
    }

  abe_share = (ABE_SHARE *) abeShare->data ();


  //  Open all the PFM structures that are being viewed in pfmEdit/pfmView

  num_pfms = abe_share->pfm_count;

  for (int32_t pfm = 0 ; pfm < num_pfms ; pfm++)
    {
      open_args[pfm] = abe_share->open_args[pfm];

      if ((pfm_handle[pfm] = open_existing_pfm_file (&open_args[pfm])) < 0)
	{
	  QMessageBox::warning (this, tr ("Open PFM Structure"),
				tr ("The file %1 is not a PFM handle or list file or there was an error reading the file.\nThe error message returned was:\n\n%2").arg 
                                (QDir::toNativeSeparators (QString (abe_share->open_args[pfm].list_path))).arg (pfm_error_str (pfm_error)));
	}
    }


  //  Set the window size and location from the defaults

  this->resize (window_width, window_height);
  this->move (window_x, window_y);


  //  Set the GPS start time (00:00:00 on 6 January 1980) in POSIX form.

  time_t gps_tv_sec;
  long gps_tv_nsec;
  inv_cvtime (80, 6, 0, 0, 0.0, &gps_tv_sec, &gps_tv_nsec);
  gps_start_time = (double) gps_tv_sec;


  QFrame *frame = new QFrame (this, 0);
  
  setCentralWidget (frame);

  QVBoxLayout *vBox = new QVBoxLayout ();
  vBox->setSpacing(0);
  frame->setLayout (vBox);

  listBox = new QTextEdit (this);
  listBox->setReadOnly (true);


  vBox->addWidget (listBox);


  //  Setup the file menu.

  QAction *fileQuitAction = new QAction (tr ("&Quit"), this);


  fileQuitAction->setStatusTip (tr ("Exit from application"));
  connect (fileQuitAction, SIGNAL (triggered ()), this, SLOT (slotQuit ()));

  QAction *filePrefsAction= new QAction (tr ("&Preferences"), this);
  filePrefsAction->setStatusTip (tr ("Set Preferences"));
  connect (filePrefsAction, SIGNAL (triggered ()), this, SLOT (slotPrefs ()));


  QMenu *fileMenu = menuBar ()->addMenu (tr ("&File"));
  fileMenu->addAction (filePrefsAction);
  fileMenu->addSeparator ();
  fileMenu->addAction (fileQuitAction);


  //  Setup the help menu.  I like leaving the About Qt in since this is
  //  a really nice package - and it's open source.

  QAction *aboutAct = new QAction (tr ("&About"), this);
  aboutAct->setShortcut (tr ("Ctrl+A", "About shortcut key"));
  aboutAct->setStatusTip (tr ("Information about lidarMonitor"));
  connect (aboutAct, SIGNAL (triggered ()), this, SLOT (about ()));

  QAction *aboutQtAct = new QAction (tr ("About&Qt"), this);
  aboutQtAct->setStatusTip (tr ("Information about Qt"));
  connect (aboutQtAct, SIGNAL (triggered ()), this, SLOT (aboutQt ()));

  QAction *whatThisAct = QWhatsThis::createAction (this);

  QMenu *helpMenu = menuBar ()->addMenu (tr ("&Help"));
  helpMenu->addAction (aboutAct);
  helpMenu->addSeparator ();
  helpMenu->addAction (aboutQtAct);
  helpMenu->addAction (whatThisAct);


  //  Setup the tracking timer

  QTimer *track = new QTimer (this);
  connect (track, SIGNAL (timeout ()), this, SLOT (trackCursor ()));
  track->start (10);
}



lidarMonitor::~lidarMonitor ()
{
}


void lidarMonitor::closeEvent (QCloseEvent * event __attribute__ ((unused)))
{
  slotQuit ();
}

  

void 
lidarMonitor::slotResize (QResizeEvent *e __attribute__ ((unused)))
{
  force_redraw = NVTrue;
}



void
lidarMonitor::slotHelp ()
{
  QWhatsThis::enterWhatsThisMode ();
}



//  Timer - timeout signal.  Very much like an X workproc.

void
lidarMonitor::trackCursor ()
{
  int32_t                 year, day, mday, month, hour, minute, type, system_num, cpf_handle = -1, csf_handle = -1, channel = -1, retnum = -1;
  float                   second;
  static ABE_SHARE        l_share;
  FILE                    *cfp = NULL, *las_fp = NULL;
  HOF_HEADER_T            hof_header;
  TOF_HEADER_T            tof_header;
  CZMIL_CPF_Header        cpf_header;
  CZMIL_CSF_Header        csf_header;
  HYDRO_OUTPUT_T          hof_record;
  TOPO_OUTPUT_T           tof_record;
  CZMIL_CPF_Data          cpf_record;
  CZMIL_CSF_Data          csf_record;
  LASheader               lasheader;
  LASreadOpener           lasreadopener;
  LASreader               *lasreader;
  QString                 string, string2;
  char                    ltstring[30], lnstring[30], hem;
  double                  deg, min, sec;
  double                  las_time_offset = 0;
  int64_t                 las_timestamp, tv_sec;
  uint8_t                 csf_present = NVTrue, endian = NVFalse;


  //  We want to exit if we have locked the tracker to update our saved waveforms (in slotPlotWaves).

  if (lock_track) return;


  //  Since this is always a child process of something we want to exit if we see the CHILD_PROCESS_FORCE_EXIT key.
  //  We also want to exit on the ANCILLARY_FORCE_EXIT key (from pfmEdit) or if our own personal kill signal
  //  has been placed in abe_share->key.

  if (abe_share->key == CHILD_PROCESS_FORCE_EXIT || abe_share->key == ANCILLARY_FORCE_EXIT ||
      abe_share->key == kill_switch) slotQuit ();


  //  We can force an update using the same key as waveMonitor since they're looking at the same sahred memory area.

  if (abe_share->modcode == WAVEMONITOR_FORCE_REDRAW) force_redraw = NVTrue; 


  //  Check for HOF, TOF, CZMIL, LAS, no memory lock, record change, force_redraw.

  if (((abe_share->mwShare.multiType[0] == PFM_SHOALS_1K_DATA || abe_share->mwShare.multiType[0] == PFM_SHOALS_TOF_DATA ||
        abe_share->mwShare.multiType[0] == PFM_CHARTS_HOF_DATA || abe_share->mwShare.multiType[0] == PFM_CZMIL_DATA ||
        abe_share->mwShare.multiType[0] == PFM_LAS_DATA) && abe_share->key < INT32_MAX && l_share.nearest_point != abe_share->nearest_point) ||
      force_redraw)
    {
      lock_track = NVTrue;


      force_redraw = NVFalse;


      abeShare->lock ();
      l_share = *abe_share;
      abeShare->unlock ();


      //  Open the CHARTS file and read the data.

      if (strstr (l_share.nearest_filename, ".hof"))
        {
          cfp = open_hof_file (l_share.nearest_filename);

          if (cfp == NULL)
            {
              string = tr ("Error opening %1 :%2").arg (QDir::toNativeSeparators (QString (l_share.nearest_filename))).arg (strerror (errno));
              statusBar ()->showMessage (string);

              lock_track = NVFalse;
              return;
            }
          type = PFM_CHARTS_HOF_DATA;
        }
      else if (strstr (l_share.nearest_filename, ".tof"))
        {
          cfp = open_tof_file (l_share.nearest_filename);


          if (cfp == NULL)
            {
              string = tr ("Error opening %1 : %2").arg (QDir::toNativeSeparators (QString (l_share.nearest_filename))).arg (strerror (errno));
              statusBar ()->showMessage (string);
              
              lock_track = NVFalse;
              return;
            }
          type = PFM_SHOALS_TOF_DATA;
        }
      else if (strstr (l_share.nearest_filename, ".las"))
        {
          endian = big_endian ();
          strcpy (las_reflectance_name, "");


          //  Open the LAS file with LASlib and read the header.

          lasreadopener.set_file_name (l_share.nearest_filename);
          lasreader = lasreadopener.open ();
          if (!lasreader)
            {
              string = tr ("Error opening %1 : %2").arg (QDir::toNativeSeparators (QString (l_share.nearest_filename))).arg (strerror (errno));
              statusBar ()->showMessage (string);
              
              lock_track = NVFalse;
              return;
            }


          lasheader = lasreader->header;


          if (lasheader.version_major != 1)
            {
              lasreader->close ();
              string = tr ("\nLAS major version %1 incorrect, file %2 : %3 %4 %5\n\n").arg (lasheader.version_major).arg
                (l_share.nearest_filename).arg (__FILE__).arg (__FUNCTION__).arg (__LINE__);
              statusBar ()->showMessage (string);
              
              lock_track = NVFalse;
              return;
            }


          if (lasheader.version_minor > 4)
            {
              lasreader->close ();
              string = tr ("\nLAS minor version %1 incorrect, file %2 : %3 %4 %5\n\n").arg (lasheader.version_minor).arg
                (l_share.nearest_filename).arg (__FILE__).arg (__FUNCTION__).arg (__LINE__);
              statusBar ()->showMessage (string);
              
              lock_track = NVFalse;
              return;
            }


          //  Is this LAS 1.4?  If so, look for our extra bytes VLR for reflectance.

          if (lasheader.version_minor == 4)
            {
              //  Check for variable length records.  This is based on how it is done in Martin Isenburg's lasinfo.  If you want to
              //  see how it works you should go check that out.  It's in the LAStools distribution.

              for (int32_t k = 0 ; k < (int32_t) lasheader.number_of_variable_length_records ; k++)
                {
                  //  A record ID of 4 means it is an "extra bytes" VLR.

                  if (lasheader.vlrs[k].record_id == 4)
                    {
                      for (int j = 0; j < lasheader.vlrs[k].record_length_after_header; j += 192)
                        {
                          if (lasheader.vlrs[k].data[j+2])
                            {
                              char desc[128];
                              strcpy (desc, (char *) (lasheader.vlrs[k].data + j + 160));
                              if (!strncmp (desc, "Radiometric calibration output", 30))
                                {
                                  strcpy (las_reflectance_name, (char *) (lasheader.vlrs[k].data + j + 4));
                                }
                            }
                        }
                    }
                }
            }


          //  Now close it since all we really wanted was the header.

          lasreader->close ();


          if ((las_fp = fopen64 (l_share.nearest_filename, "rb")) == NULL)
            {
              string = tr ("Error opening %1 : %2").arg (QDir::toNativeSeparators (QString (l_share.nearest_filename))).arg (strerror (errno));
              statusBar ()->showMessage (string);

              lock_track = NVFalse;
              return;
            }

          type = PFM_LAS_DATA;
        }
      else
        {
          cpf_handle = czmil_open_cpf_file (l_share.nearest_filename, &cpf_header, CZMIL_READONLY);

          if (cpf_handle != CZMIL_SUCCESS)
            {
              string = tr ("Error opening %1 : %2").arg (QDir::toNativeSeparators (QString (l_share.nearest_filename))).arg (czmil_strerror ());
              statusBar ()->showMessage (string);

              lock_track = NVFalse;
              return;
            }


          csf_present = NVTrue;
          QString csfFile = l_share.nearest_filename;
          csfFile.replace (".cpf", ".csf");
          char csf_file[1024];
          strcpy (csf_file, csfFile.toLatin1 ());

          csf_handle = czmil_open_csf_file (csf_file, &csf_header, CZMIL_READONLY);

          if (csf_handle != CZMIL_SUCCESS) csf_present = NVFalse;

          type = PFM_CZMIL_DATA;
        }


      //  Erase the data

      listBox->clear ();


      switch (type)
        {
        case PFM_SHOALS_1K_DATA:
        case PFM_CHARTS_HOF_DATA:
          hof_read_header (cfp, &hof_header);

          system_num = hof_header.text.ab_system_number - 1;

          hof_read_record (cfp, l_share.mwShare.multiRecord[0], &hof_record);


          //  Populate the textEdit box.

          string = tr ("<b>Record number: </b>%1<br>").arg (l_share.mwShare.multiRecord[0]);
          listBox->insertHtml (string);

          string = tr ("<b>Timestamp : </b>%1<br>").arg (hof_record.timestamp);
          listBox->insertHtml (string);

          charts_cvtime (hof_record.timestamp, &year, &day, &hour, &minute, &second);
          charts_jday2mday (year, day, &month, &mday);
          month++;
          string = tr ("<b>Date/Time : </b>%1-%2-%3 (%4) %5:%6:%L7<br>").arg (year + 1900).arg (month, 2, 10, QChar ('0')).arg (mday, 2, 10, QChar ('0')).arg
            (day, 3, 10, QChar ('0')).arg (hour, 2, 10, QChar ('0')).arg (minute, 2, 10, QChar ('0')).arg (second, 5, 'f', 2, QChar ('0'));
          listBox->insertHtml (string);

          string = tr ("<b>System number : </b>%1<br>").arg (system_num);
          listBox->insertHtml (string);

          string = tr ("<b>HAPS version : </b>%1<br>").arg (hof_record.haps_version);
          listBox->insertHtml (string);

          string = tr ("<b>Position confidence : </b>%1<br>").arg (hof_record.position_conf);
          listBox->insertHtml (string);

          string = tr ("<b>Status : </b>0x%1<br>").arg ((int32_t) hof_record.status, 0, 16);
          listBox->insertHtml (string);

          string = tr ("<b>Suggested delete/keep/swap : </b>%1<br>").arg (hof_record.suggested_dks);
          listBox->insertHtml (string);

          string = tr ("<b>Suspect status : </b>0x%1<br>").arg ((int32_t) hof_record.suspect_status, 0, 16);
          listBox->insertHtml (string);

          string = tr ("<b>Tide status : </b>0x%1<br>").arg ((int32_t) (hof_record.tide_status & 0x3), 0, 16);
          listBox->insertHtml (string);

          strcpy (ltstring, fixpos (hof_record.latitude, &deg, &min, &sec, &hem, POS_LAT, pos_format));
          string = tr ("<b>Latitude : </b>%1<br>").arg (ltstring);
          listBox->insertHtml (string);

          strcpy (lnstring, fixpos (hof_record.longitude, &deg, &min, &sec, &hem, POS_LON, pos_format));
          string = tr ("<b>Longitude : </b>%1<br>").arg (lnstring);
          listBox->insertHtml (string);

          strcpy (ltstring, fixpos (hof_record.sec_latitude, &deg, &min, &sec, &hem, POS_LAT, pos_format));
          string = tr ("<b>Secondary latitude : </b>%1<br>").arg (hof_record.sec_latitude);
          listBox->insertHtml (string);

          strcpy (lnstring, fixpos (hof_record.sec_longitude, &deg, &min, &sec, &hem, POS_LON, pos_format));
          string = tr ("<b>Secondary longitude : </b>%1<br>").arg (hof_record.sec_longitude);
          listBox->insertHtml (string);

          string = tr ("<b>Correct depth : </b>%1<br>").arg (hof_record.correct_depth);
          listBox->insertHtml (string);

          string = tr ("<b>Correct secondary depth : </b>%1<br>").arg (hof_record.correct_sec_depth);
          listBox->insertHtml (string);

          string = tr ("<b>Abbreviated depth confidence (ABDC) : </b>%1<br>").arg (hof_record.abdc);
          listBox->insertHtml (string);

          string = tr ("<b>Secondary abbreviated depth confidence : </b>%1<br>").arg (hof_record.sec_abdc);
          listBox->insertHtml (string);

          if (hof_record.data_type)
            {
              listBox->insertHtml (tr ("<b>Data type : </b>KGPS<br>"));
            }
          else
            {
              listBox->insertHtml (tr ("<b>Data type : </b>DGPS<br>"));
            }

          string = tr ("<b>Tide corrected depth : </b>%1<br>").arg (hof_record.tide_cor_depth);
          listBox->insertHtml (string);

          string = tr ("<b>Reported depth : </b>%1<br>").arg (hof_record.reported_depth);
          listBox->insertHtml (string);

          string = tr ("<b>Result depth : </b>%1<br>").arg (hof_record.result_depth);
          listBox->insertHtml (string);

          string = tr ("<b>Secondary depth : </b>%1<br>").arg (hof_record.sec_depth);
          listBox->insertHtml (string);

          string = tr ("<b>Wave height : </b>%1<br>").arg (hof_record.wave_height);
          listBox->insertHtml (string);

          string = tr ("<b>Elevation : </b>%1<br>").arg (hof_record.elevation);
          listBox->insertHtml (string);

          string = tr ("<b>Topo : </b>%1<br>").arg (hof_record.topo);
          listBox->insertHtml (string);

          string = tr ("<b>Altitude : </b>%1<br>").arg (hof_record.altitude);
          listBox->insertHtml (string);

          string = tr ("<b>KGPS elevation : </b>%1<br>").arg (hof_record.kgps_elevation);
          listBox->insertHtml (string);

          string = tr ("<b>KGPS result elevation : </b>%1<br>").arg (hof_record.kgps_res_elev);
          listBox->insertHtml (string);

          string = tr ("<b>KGPS secondary elevation : </b>%1<br>").arg (hof_record.kgps_sec_elev);
          listBox->insertHtml (string);

          string = tr ("<b>KGPS topo : </b>%1<br>").arg (hof_record.kgps_topo);
          listBox->insertHtml (string);

          string = tr ("<b>KGPS datum : </b>%1<br>").arg (hof_record.kgps_datum);
          listBox->insertHtml (string);

          string = tr ("<b>KGPS water level : </b>%1<br>").arg (hof_record.kgps_water_level);
          listBox->insertHtml (string);

          string = tr ("<b>Bottom confidence : </b>%1<br>").arg (hof_record.bot_conf);
          listBox->insertHtml (string);

          string = tr ("<b>Secondary bottom confidence : </b>%1<br>").arg (hof_record.sec_bot_conf);
          listBox->insertHtml (string);

          string = tr ("<b>Nadir angle : </b>%1<br>").arg (hof_record.nadir_angle);
          listBox->insertHtml (string);

          string = tr ("<b>Scanner azimuth : </b>%1<br>").arg (hof_record.scanner_azimuth);
          listBox->insertHtml (string);

          string = tr ("<b>Surface figure of merit (FOM) apd : </b>%1<br>").arg (hof_record.sfc_fom_apd);
          listBox->insertHtml (string);

          string = tr ("<b>Surface figure of merit (FOM) ir : </b>%1<br>").arg (hof_record.sfc_fom_ir);
          listBox->insertHtml (string);

          string = tr ("<b>Surface figure of merit (FOM) raman : </b>%1<br>").arg (hof_record.sfc_fom_ram);
          listBox->insertHtml (string);

          string = tr ("<b>Depth confidence : </b>%1<br>").arg (hof_record.depth_conf);
          listBox->insertHtml (string);

          string = tr ("<b>Secondary depth confidence : </b>%1<br>").arg (hof_record.sec_depth_conf);
          listBox->insertHtml (string);

          string = tr ("<b>Warnings : </b>%1<br>").arg (hof_record.warnings);
          listBox->insertHtml (string);

          string = tr ("<b>Warnings2 : </b>%1<br>").arg (hof_record.warnings2);
          listBox->insertHtml (string);

          string = tr ("<b>Warnings3 : </b>%1<br>").arg (hof_record.warnings3);
          listBox->insertHtml (string);

          string = tr ("<b>Calc_bfom_thresh_times10[0] : </b>%1<br>").arg (hof_record.calc_bfom_thresh_times10[0]);
          listBox->insertHtml (string);

          string = tr ("<b>Calc_bfom_thresh_times10[1] : </b>%1<br>").arg (hof_record.calc_bfom_thresh_times10[1]);
          listBox->insertHtml (string);

          string = tr ("<b>Calc_bot_run_required[0] : </b>%1<br>").arg (hof_record.calc_bot_run_required[0]);
          listBox->insertHtml (string);

          string = tr ("<b>Calc_bot_run_required[1] : </b>%1<br>").arg (hof_record.calc_bot_run_required[1]);
          listBox->insertHtml (string);

          string = tr ("<b>Bot_bin_first : </b>%1<br>").arg (hof_record.bot_bin_first);
          listBox->insertHtml (string);

          string = tr ("<b>Bot_bin_second : </b>%1<br>").arg (hof_record.bot_bin_second);
          listBox->insertHtml (string);

          string = tr ("<b>Bot_bin_used_pmt : </b>%1<br>").arg (hof_record.bot_bin_used_pmt);
          listBox->insertHtml (string);

          string = tr ("<b>Sec_bot_bin_used_pmt : </b>%1<br>").arg (hof_record.sec_bot_bin_used_pmt);
          listBox->insertHtml (string);

          string = tr ("<b>Bot_bin_used_apd : </b>%1<br>").arg (hof_record.bot_bin_used_apd);
          listBox->insertHtml (string);

          string = tr ("<b>Sec_bot_bin_used_apd : </b>%1<br>").arg (hof_record.sec_bot_bin_used_apd);
          listBox->insertHtml (string);

          string = tr ("<b>Bot_channel : </b>%1<br>").arg (hof_record.bot_channel);
          listBox->insertHtml (string);

          string = tr ("<b>Sec_bot_chan : </b>%1<br>").arg (hof_record.sec_bot_chan);
          listBox->insertHtml (string);

          string = tr ("<b>Sfc_bin_apd : </b>%1<br>").arg (hof_record.sfc_bin_apd);
          listBox->insertHtml (string);

          string = tr ("<b>Sfc_bin_ir : </b>%1<br>").arg (hof_record.sfc_bin_ir);
          listBox->insertHtml (string);

          string = tr ("<b>Sfc_bin_ram : </b>%1<br>").arg (hof_record.sfc_bin_ram);
          listBox->insertHtml (string);

          string = tr ("<b>Sfc_channel_used : </b>%1<br>").arg (hof_record.sfc_channel_used);
          listBox->insertHtml (string);

          string = tr ("<b>Ab_dep_conf : </b>%1<br>").arg (hof_record.ab_dep_conf);
          listBox->insertHtml (string);

          string = tr ("<b>Sec_ab_dep_conf : </b>%1<br>").arg (hof_record.sec_ab_dep_conf);
          listBox->insertHtml (string);

          string = tr ("<b>KGPS_abd_conf : </b>%1<br>").arg (hof_record.kgps_abd_conf);
          listBox->insertHtml (string);

          string = tr ("<b>KGPS_sec_abd_conf : </b>%1<br>").arg (hof_record.kgps_sec_abd_conf);
          listBox->insertHtml (string);

          fclose (cfp);
          break;


        case PFM_SHOALS_TOF_DATA:
          tof_read_header (cfp, &tof_header);

          system_num = tof_header.text.ab_system_number - 1;

          tof_read_record (cfp, l_share.mwShare.multiRecord[0], &tof_record);


          //  Populate the textEdit box.

          string = tr ("<b>Record number: </b>%1<br>").arg (l_share.mwShare.multiRecord[0]);
          listBox->insertHtml (string);

          string = tr ("<b>Timestamp : </b>%1<br>").arg (tof_record.timestamp);
          listBox->insertHtml (string);

          charts_cvtime (tof_record.timestamp, &year, &day, &hour, &minute, &second);
          charts_jday2mday (year, day, &month, &mday);
          month++;
          string = tr ("<b>Date/Time : </b>%1-%2-%3 (%4) %5:%6:%L7<br>").arg (year + 1900).arg (month, 2, 10, QChar ('0')).arg (mday, 2, 10, QChar ('0')).arg
            (day, 3, 10, QChar ('0')).arg (hour, 2, 10, QChar ('0')).arg (minute, 2, 10, QChar ('0')).arg (second, 5, 'f', 2, QChar ('0'));
          listBox->insertHtml (string);

          string = tr ("<b>System number : </b>%1<br>").arg (system_num);
          listBox->insertHtml (string);

          strcpy (ltstring, fixpos (tof_record.latitude_first, &deg, &min, &sec, &hem, POS_LAT, pos_format));
          string = tr ("<b>Latitude first : </b>%1<br>").arg (tof_record.latitude_first);
          listBox->insertHtml (string);

          strcpy (lnstring, fixpos (tof_record.longitude_first, &deg, &min, &sec, &hem, POS_LON, pos_format));
          string = tr ("<b>Longitude first : </b>%1<br>").arg (tof_record.longitude_first);
          listBox->insertHtml (string);

          strcpy (ltstring, fixpos (tof_record.latitude_last, &deg, &min, &sec, &hem, POS_LAT, pos_format));
          string = tr ("<b>Latitude last : </b>%1<br>").arg (tof_record.latitude_last);
          listBox->insertHtml (string);

          strcpy (lnstring, fixpos (tof_record.longitude_last, &deg, &min, &sec, &hem, POS_LON, pos_format));
          string = tr ("<b>Longitude last : </b>%1<br>").arg (tof_record.longitude_last);
          listBox->insertHtml (string);

          string = tr ("<b>Elevation first : </b>%1<br>").arg (tof_record.elevation_first);
          listBox->insertHtml (string);

          string = tr ("<b>Elevation last : </b>%1<br>").arg (tof_record.elevation_last);
          listBox->insertHtml (string);

          string = tr ("<b>Scanner azimuth : </b>%1<br>").arg (tof_record.scanner_azimuth);
          listBox->insertHtml (string);

          string = tr ("<b>Nadir angle : </b>%1<br>").arg (tof_record.nadir_angle);
          listBox->insertHtml (string);

          string = tr ("<b>Confidence first : </b>%1<br>").arg (tof_record.conf_first);
          listBox->insertHtml (string);

          string = tr ("<b>Confidence last : </b>%1<br>").arg (tof_record.conf_last);
          listBox->insertHtml (string);

          string = tr ("<b>Intensity first : </b>%1<br>").arg (tof_record.intensity_first);
          listBox->insertHtml (string);

          string = tr ("<b>Intensity last : </b>%1<br>").arg (tof_record.intensity_last);
          listBox->insertHtml (string);

          string = tr ("<b>Classification status : </b>0x%1<br>").arg ((int32_t) tof_record.classification_status, 0, 16);
          listBox->insertHtml (string);

          string = tr ("<b>TBD 1 : </b>0x%1<br>").arg ((int32_t) tof_record.tbd_1, 0, 16);
          listBox->insertHtml (string);

          string = tr ("<b>Position conf : </b>%1<br>").arg (tof_record.pos_conf);
          listBox->insertHtml (string);

          string = tr ("<b>TBD 2 : </b>0x%1<br>").arg ((int32_t) tof_record.tbd_2, 0, 16);
          listBox->insertHtml (string);

          string = tr ("<b>Result elevation first : </b>%1<br>").arg (tof_record.result_elevation_first);
          listBox->insertHtml (string);

          string = tr ("<b>Result elevation last : </b>%1<br>").arg (tof_record.result_elevation_last);
          listBox->insertHtml (string);

          string = tr ("<b>Altitude : </b>%1<br>").arg (tof_record.altitude);
          listBox->insertHtml (string);

          string = tr ("<b>TBD float : </b>%1<br>").arg (tof_record.tbdfloat);
          listBox->insertHtml (string);

          fclose (cfp);
          break;


        case PFM_CZMIL_DATA:
          czmil_read_cpf_record (cpf_handle, l_share.mwShare.multiRecord[0], &cpf_record);
          if (csf_present) czmil_read_csf_record (csf_handle, l_share.mwShare.multiRecord[0], &csf_record);


          /*  The subrecord number for CZMIL consists of the channel (0-8) times 100 plus the return number (0-31).  */

          channel = l_share.mwShare.multiSubrecord[0] / 100;
          retnum = l_share.mwShare.multiSubrecord[0] % 100;


          //  Populate the textEdit box.

          string = tr ("<b>Record number: </b>%1<br>").arg (l_share.mwShare.multiRecord[0]);
          listBox->insertHtml (string);

          string = tr ("<b>Timestamp : </b>%1<br>").arg (cpf_record.timestamp);
          listBox->insertHtml (string);

          czmil_cvtime (cpf_record.timestamp, &year, &day, &hour, &minute, &second);
          jday2mday (year, day, &month, &mday);
          month++;
          string = tr ("<b>Date/Time : </b>%1-%2-%3 (%4) %5:%6:%L7<br>").arg (year + 1900).arg (month, 2, 10, QChar ('0')).arg (mday, 2, 10, QChar ('0')).arg
            (day, 3, 10, QChar ('0')).arg (hour, 2, 10, QChar ('0')).arg (minute, 2, 10, QChar ('0')).arg (second, 5, 'f', 2, QChar ('0'));
          listBox->insertHtml (string);

          string = tr ("<b>Off nadir angle : </b>%1<br>").arg (cpf_record.off_nadir_angle);
          listBox->insertHtml (string);
          strcpy (ltstring, fixpos (cpf_record.reference_latitude, &deg, &min, &sec, &hem, POS_LAT, pos_format));
          string = tr ("<b>Reference latitude : </b>%1<br>").arg (ltstring);
          listBox->insertHtml (string);
          strcpy (lnstring, fixpos (cpf_record.reference_longitude, &deg, &min, &sec, &hem, POS_LON, pos_format));
          string = tr ("<b>Reference longitude : </b>%1<br>").arg (lnstring);
          listBox->insertHtml (string);
          string = tr ("<b>Water level : </b>%1<br>").arg (cpf_record.water_level);
          listBox->insertHtml (string);
          string = tr ("<b>Local vertical datum offset : </b>%1<br>").arg (cpf_record.local_vertical_datum_offset);
          listBox->insertHtml (string);
          string = tr ("<b>Kd : </b>%L1<br>").arg (cpf_record.kd, 0, 'f', 4);
          listBox->insertHtml (string);
          string = tr ("<b>Laser energy : </b>%L1<br>").arg (cpf_record.laser_energy, 0, 'f', 4);
          listBox->insertHtml (string);
          string = tr ("<b>T0 interest point : </b>%L1<br>").arg (cpf_record.t0_interest_point, 0, 'f', 1);
          listBox->insertHtml (string);

          string = tr ("<b>Channel[%1] Return[%2]</b><br>").arg (channel).arg (retnum);
          listBox->insertHtml (string);

          strcpy (ltstring, fixpos (cpf_record.channel[channel][retnum].latitude, &deg, &min, &sec, &hem, POS_LAT, pos_format));
          string = tr ("<b>Latitude : </b>%1<br>").arg (ltstring);
          listBox->insertHtml (string);
          strcpy (lnstring, fixpos (cpf_record.channel[channel][retnum].longitude, &deg, &min, &sec, &hem, POS_LON, pos_format));
          string = tr ("<b>Longitude : </b>%1<br>").arg (lnstring);
          listBox->insertHtml (string);
          string = tr ("<b>Elevation : </b>%1<br>").arg (cpf_record.channel[channel][retnum].elevation);
          listBox->insertHtml (string);
          string = tr ("<b>Interest point : </b>%L1<br>").arg (cpf_record.channel[channel][retnum].interest_point, 0, 'f', 1);
          listBox->insertHtml (string);


          //  Translate the interest point flag (used to be IP rank).

          if (cpf_record.optech_classification[channel] == CZMIL_OPTECH_CLASS_HYBRID)
            {
              if (cpf_record.channel[channel][retnum].ip_rank)
                {
                  string = tr ("<b>Interest point flag : </b>Hybrid land (%1)<br>").arg (cpf_record.channel[channel][retnum].ip_rank);
                  listBox->insertHtml (string);
                }
              else
                {
                  string = tr ("<b>Interest point flag : </b>Hybrid water (%1)<br>").arg (cpf_record.channel[channel][retnum].ip_rank);
                  listBox->insertHtml (string);
                }
            }
          else
            {
              if (cpf_record.channel[channel][retnum].ip_rank)
                {
                  if (cpf_record.optech_classification[channel] < CZMIL_OPTECH_CLASS_HYBRID)
                    {
                      string = tr ("<b>Interest point flag : </b>Land (%1)<br>").arg (cpf_record.channel[channel][retnum].ip_rank);
                      listBox->insertHtml (string);
                    }
                  else
                    {
                      string = tr ("<b>Interest point flag : </b>Water (%1)<br>").arg (cpf_record.channel[channel][retnum].ip_rank);
                      listBox->insertHtml (string);
                    }
                }
              else
                {
                  string = tr ("<b>Interest point flag : </b>Water surface (%1)<br>").arg (cpf_record.channel[channel][retnum].ip_rank);
                  listBox->insertHtml (string);
                }
            }

          string = tr ("<b>Reflectance : </b>%1<br>").arg (cpf_record.channel[channel][retnum].reflectance);
          listBox->insertHtml (string);
          string = tr ("<b>Horizontal uncertainty : </b>%1<br>").arg (cpf_record.channel[channel][retnum].horizontal_uncertainty);
          listBox->insertHtml (string);
          string = tr ("<b>Vertical uncertainty : </b>%1<br>").arg (cpf_record.channel[channel][retnum].vertical_uncertainty);
          listBox->insertHtml (string);
          string = tr ("<b>Return status : </b>0x%1<br>").arg ((int32_t) cpf_record.channel[channel][retnum].status, 4, 16, QChar ('0'));
          listBox->insertHtml (string);

          char status_string[1024];

          czmil_get_cpf_return_status_string (cpf_record.channel[channel][retnum].status, status_string);
          listBox->insertHtml (status_string);
          listBox->insertHtml ("<br>");

          string = tr ("<b>Classification : </b>%1<br>").arg (cpf_record.channel[channel][retnum].classification);
          listBox->insertHtml (string);

          string = tr ("<b>Probability of detection : </b>%1<br>").arg (cpf_record.channel[channel][retnum].probability);
          listBox->insertHtml (string);
          string = tr ("<b>Filter reason : </b>%1<br>").arg (cpf_record.channel[channel][retnum].filter_reason);
          listBox->insertHtml (string);
          czmil_get_cpf_filter_reason_string (cpf_record.channel[channel][retnum].filter_reason, status_string);
          listBox->insertHtml (status_string);
          listBox->insertHtml ("<br>");

          string = tr ("<b>D_index : </b>%1<br>").arg (cpf_record.channel[channel][retnum].d_index);
          listBox->insertHtml (string);

          string = tr ("<b>User data : </b>%1<br>").arg (cpf_record.user_data);
          listBox->insertHtml (string);

          string = tr ("<b>Processing mode : </b>%1<br>").arg (cpf_record.optech_classification[channel]);
          listBox->insertHtml (string);

          string = tr ("<b>D_index_cube : </b>%1<br>").arg (cpf_record.d_index_cube);
          listBox->insertHtml (string);
          string = tr ("&nbsp;&nbsp;&nbsp;&nbsp;<b>Cube detection prob. : </b>%L1\%<br>").arg (czmil_compute_pd_cube (&cpf_record, false_alarm), 0, 'f', 8);
          listBox->insertHtml (string);
          string = tr ("&nbsp;&nbsp;&nbsp;&nbsp;<b>False alarm prob. : </b>%L1<br>").arg (0.01 / pow (10.0, (double) false_alarm), 0, 'f', 9);
          listBox->insertHtml (string);


          //  Check to see if this is a shallow channel.  If it is, dump the bare earth data as well.

          if (channel >= CZMIL_SHALLOW_CHANNEL_1 && channel <= CZMIL_SHALLOW_CHANNEL_7)
            {
              strcpy (ltstring, fixpos (cpf_record.bare_earth_latitude[channel], &deg, &min, &sec, &hem, POS_LAT, pos_format));
              string = tr ("<b>Bare earth latitude : </b>%1<br>").arg (ltstring);
              listBox->insertHtml (string);
              strcpy (ltstring, fixpos (cpf_record.bare_earth_longitude[channel], &deg, &min, &sec, &hem, POS_LON, pos_format));
              string = tr ("<b>Bare earth longitude : </b>%1<br>").arg (lnstring);
              listBox->insertHtml (string);
              string = tr ("<b>Bare earth elevation : </b>%1<br>").arg (cpf_record.bare_earth_elevation[channel]);
              listBox->insertHtml (string);
            }


          //  Dump the CSF data.
          if (csf_present)
            {
              listBox->insertHtml ("<br><b>CSF Data</b><br>");
              string = tr ("<b>Scan angle : </b>%1<br>").arg (csf_record.scan_angle);
              listBox->insertHtml (string);
              strcpy (ltstring, fixpos (csf_record.latitude, &deg, &min, &sec, &hem, POS_LAT, pos_format));
              string = tr ("<b>Platform latitude : </b>%1<br>").arg (ltstring);
              listBox->insertHtml (string);
              strcpy (lnstring, fixpos (csf_record.longitude, &deg, &min, &sec, &hem, POS_LON, pos_format));
              string = tr ("<b>Platform longitude : </b>%1<br>").arg (lnstring);
              listBox->insertHtml (string);
              string = tr ("<b>Platform altitude : </b>%1<br>").arg (csf_record.altitude);
              listBox->insertHtml (string);
              string = tr ("<b>Roll : </b>%1<br>").arg (csf_record.roll);
              listBox->insertHtml (string);
              string = tr ("<b>Pitch : </b>%1<br>").arg (csf_record.pitch);
              listBox->insertHtml (string);
              string = tr ("<b>Heading : </b>%1<br>").arg (csf_record.heading);
              listBox->insertHtml (string);
              for (int32_t cs = 0 ; cs < 9 ; cs++)
                {
                  string = tr ("<b>Channel[%1] range : </b>%2<br>").arg (cs).arg (csf_record.range[cs]);
                  listBox->insertHtml (string);
                  string = tr ("<b>Channel[%1] range in water : </b>%2<br>").arg (cs).arg (csf_record.range_in_water[cs]);
                  listBox->insertHtml (string);
                  string = tr ("<b>Channel[%1] intensity : </b>%2<br>").arg (cs).arg (csf_record.intensity[cs]);
                  listBox->insertHtml (string);
                  string = tr ("<b>Channel[%1] intensity in water : </b>%2<br>").arg (cs).arg (csf_record.intensity_in_water[cs]);
                  listBox->insertHtml (string);
                }

              czmil_close_csf_file (csf_handle);
            }
          else
            {
              listBox->insertHtml ("<br><b>No CSF File</b><br>");
            }

          czmil_close_cpf_file (cpf_handle);

          break;


        case PFM_LAS_DATA:

          slas_read_point_data (las_fp, l_share.mwShare.multiRecord[0] - 1, &lasheader, endian, &slas);

          fclose (las_fp);


          //  Populate the textEdit box.

          string = tr ("<b>Record number: </b>%1<br>").arg (l_share.mwShare.multiRecord[0]);
          listBox->insertHtml (string);

          if (lasheader.point_data_format != 2)
            {
              if (lasheader.global_encoding & 0x01)
                {
		  //  Note, GPS time is ahead of UTC time by some number of leap seconds depending on the date of the survey.
		  //  The leap seconds that are relevant for CHARTS and/or CZMIL data are as follows
		  //
		  //  December 31 2005 23:59:59 - 1136073599 - 14 seconds ahead
		  //  December 31 2008 23:59:59 - 1230767999 - 15 seconds ahead
		  //  June 30 2012 23:59:59     - 1341100799 - 16 seconds ahead
		  //  June 30 2015 23:59:59     - 1435708799 - 17 seconds ahead

		  las_time_offset = 1000000000.0 - 13.0;

		  tv_sec = slas.gps_time + gps_start_time + las_time_offset;
		  if (tv_sec > 1136073599) las_time_offset -= 1.0;
		  if (tv_sec > 1230767999) las_time_offset -= 1.0;
		  if (tv_sec > 1341100799) las_time_offset -= 1.0;
		  if (tv_sec > 1435708799) las_time_offset -= 1.0;

                  las_timestamp = (slas.gps_time + gps_start_time + las_time_offset) * 1000000.0;

                  charts_cvtime (las_timestamp, &year, &day, &hour, &minute, &second);
                  charts_jday2mday (year, day, &month, &mday);
                  month++;
                  string = tr ("<b>Date/Time : </b>%1-%2-%3 (%4) %5:%6:%L7<br>").arg (year + 1900).arg (month, 2, 10, QChar ('0')).arg (mday, 2, 10, QChar ('0')).arg
                    (day, 3, 10, QChar ('0')).arg (hour, 2, 10, QChar ('0')).arg (minute, 2, 10, QChar ('0')).arg (second, 5, 'f', 2, QChar ('0'));
                  listBox->insertHtml (string);
                }
              else
                {
                  string = tr ("<b>GPS seconds of the week : </b>%L1<br>").arg (slas.gps_time, 0, 'f', 6);
                  listBox->insertHtml (string);
                }
            }


          string = tr ("<b>X : </b>%1<br>").arg (slas.x);
          listBox->insertHtml (string);

          string = tr ("<b>Y : </b>%1<br>").arg (slas.y);
          listBox->insertHtml (string);

          string = tr ("<b>Elevation : </b>%1<br>").arg (slas.z);
          listBox->insertHtml (string);

          string = tr ("<b>Intensity : </b>%1<br>").arg (slas.intensity);
          listBox->insertHtml (string);

          string = tr ("<b>Return number : </b>%1<br>").arg (slas.return_number);
          listBox->insertHtml (string);

          string = tr ("<b>Number of returns : </b>%1<br>").arg (slas.number_of_returns);
          listBox->insertHtml (string);

          string = tr ("<b>Synthetic : </b>%1<br>").arg (slas.synthetic);
          listBox->insertHtml (string);

          string = tr ("<b>Keypoint : </b>%1<br>").arg (slas.keypoint);
          listBox->insertHtml (string);

          string = tr ("<b>Withheld : </b>%1<br>").arg (slas.withheld);
          listBox->insertHtml (string);

          string = tr ("<b>Overlap : </b>%1<br>").arg (slas.overlap);
          listBox->insertHtml (string);

          if (lasheader.point_data_format > 5)
            {
              string = tr ("<b>Scanner channel : </b>%1<br>").arg (slas.scanner_channel);
              listBox->insertHtml (string);
            }

          string = tr ("<b>Scan direction flag : </b>%1<br>").arg (slas.scan_direction_flag);
          listBox->insertHtml (string);

          string = tr ("<b>Flightline edge flag : </b>%1<br>").arg (slas.edge_of_flightline);
          listBox->insertHtml (string);

          string = tr ("<b>Classification : </b>%1<br>").arg (slas.classification);
          listBox->insertHtml (string);

          string = tr ("<b>User data : </b>%1<br>").arg (slas.user_data);
          listBox->insertHtml (string);

          string = tr ("<b>Scan angle : </b>%1<br>").arg (slas.scan_angle);
          listBox->insertHtml (string);

          string = tr ("<b>Point source ID : </b>%1<br>").arg (slas.point_source_id);
          listBox->insertHtml (string);

          if (lasheader.point_data_format == 2 || lasheader.point_data_format == 3 || lasheader.point_data_format == 5 ||
              lasheader.point_data_format == 7 || lasheader.point_data_format == 8 || lasheader.point_data_format == 10)
            {
              string = tr ("<b>Red : </b>%1<br>").arg (slas.red);
              listBox->insertHtml (string);

              string = tr ("<b>Green : </b>%1<br>").arg (slas.green);
              listBox->insertHtml (string);

              string = tr ("<b>Blue : </b>%1<br>").arg (slas.blue);
              listBox->insertHtml (string);
            }

          if (lasheader.point_data_format == 8 || lasheader.point_data_format == 10)
            {
              string = tr ("<b>NIR : </b>%1<br>").arg (slas.NIR);
              listBox->insertHtml (string);
            }

          if (lasheader.point_data_format == 4 || lasheader.point_data_format == 5 || lasheader.point_data_format == 9 ||
              lasheader.point_data_format == 10)
            {
              string = tr ("<b>Waveform descriptor index : </b>%1<br>").arg (slas.wavepacket_descriptor_index);
              listBox->insertHtml (string);

              string = tr ("<b>Byte offset to waveform data : </b>%1<br>").arg (slas.byte_offset_to_waveform_data);
              listBox->insertHtml (string);

              string = tr ("<b>Waveform packet size : </b>%1<br>").arg (slas.waveform_packet_size);
              listBox->insertHtml (string);

              string = tr ("<b>Return point waveform location : </b>%1<br>").arg (slas.return_point_waveform_location);
              listBox->insertHtml (string);

              string = tr ("<b>X(t) : </b>%1<br>").arg (slas.Xt);
              listBox->insertHtml (string);

              string = tr ("<b>Y(t) : </b>%1<br>").arg (slas.Yt);
              listBox->insertHtml (string);

              string = tr ("<b>Z(t) : </b>%1<br>").arg (slas.Zt);
              listBox->insertHtml (string);
            }

          if (strlen (las_reflectance_name) > 2 && (lasheader.point_data_format == 6 || lasheader.point_data_format == 7))
            {
              if (slas.reflectance)
                {
                  string = tr ("<b>%1 : </b>%2<br>").arg (las_reflectance_name).arg (((double) slas.reflectance * 0.01), 5, 'f', 2,
                                                                                     QChar ('0'));
                }
              else
                {
                  string = tr ("<b>%1 : </b>0.00<br>").arg (las_reflectance_name);
                }
              listBox->insertHtml (string);
            }

          break;
        }

      lock_track = NVFalse;
    }
}



//  A bunch of slots.

void 
lidarMonitor::slotQuit ()
{
  //  Let the parent program know that we have died from something other than the kill switch from the parent.

  if (abe_share->key != kill_switch) abe_share->killed = kill_switch;


  envout ();


  //  Let go of the shared memory.

  abeShare->detach ();


  exit (0);
}



void
lidarMonitor::slotPrefs ()
{
  prefsD = new QDialog (this, (Qt::WindowFlags) Qt::WA_DeleteOnClose);
  prefsD->setWindowTitle (tr ("lidarMonitor Preferences"));
  prefsD->setModal (true);

  QVBoxLayout *vbox = new QVBoxLayout (prefsD);
  vbox->setMargin (5);
  vbox->setSpacing (5);

  QGroupBox *fbox = new QGroupBox (tr ("Position Format"), prefsD);
  fbox->setWhatsThis (bGrpText);

  QRadioButton *hdms = new QRadioButton (tr ("Hemisphere Degrees Minutes Seconds.decimal"));
  QRadioButton *hdm_ = new QRadioButton (tr ("Hemisphere Degrees Minutes.decimal"));
  QRadioButton *hd__ = new QRadioButton (tr ("Hemisphere Degrees.decimal"));
  QRadioButton *sdms = new QRadioButton (tr ("+/-Degrees Minutes Seconds.decimal"));
  QRadioButton *sdm_ = new QRadioButton (tr ("+/-Degrees Minutes.decimal"));
  QRadioButton *sd__ = new QRadioButton (tr ("+/-Degrees.decimal"));

  bGrp = new QButtonGroup (prefsD);
  bGrp->setExclusive (true);
  connect (bGrp, SIGNAL (buttonClicked (int)), this, SLOT (slotPosClicked (int)));

  bGrp->addButton (hdms, 0);
  bGrp->addButton (hdm_, 1);
  bGrp->addButton (hd__, 2);
  bGrp->addButton (sdms, 3);
  bGrp->addButton (sdm_, 4);
  bGrp->addButton (sd__, 5);

  QHBoxLayout *fboxSplit = new QHBoxLayout;
  QVBoxLayout *fboxLeft = new QVBoxLayout;
  QVBoxLayout *fboxRight = new QVBoxLayout;
  fboxSplit->addLayout (fboxLeft);
  fboxSplit->addLayout (fboxRight);
  fboxLeft->addWidget (hdms);
  fboxLeft->addWidget (hdm_);
  fboxLeft->addWidget (hd__);
  fboxRight->addWidget (sdms);
  fboxRight->addWidget (sdm_);
  fboxRight->addWidget (sd__);
  fbox->setLayout (fboxSplit);

  vbox->addWidget (fbox, 1);

  bGrp->button (pos_format)->setChecked (true);


  QGroupBox *abox = new QGroupBox (tr ("Optech False Alarm Probability"), prefsD);
  QHBoxLayout *aboxLayout = new QHBoxLayout;
  abox->setLayout (aboxLayout);
  abox->setWhatsThis (falseAlarmText);

  falseAlarm = new QComboBox (abox);
  falseAlarm->setToolTip (tr ("Select the Optech cube detection false alarm probability"));
  falseAlarm->setWhatsThis (falseAlarmText);
  falseAlarm->setEditable (false);
  falseAlarm->addItem (QString ("%L1").arg (0.01, 0, 'f', 2));
  falseAlarm->addItem (QString ("%L1").arg (0.001, 0, 'f', 3));
  falseAlarm->addItem (QString ("%L1").arg (0.0001, 0, 'f', 4));
  falseAlarm->addItem (QString ("%L1").arg (0.00001, 0, 'f', 5));
  falseAlarm->addItem (QString ("%L1").arg (0.000001, 0, 'f', 6));
  falseAlarm->addItem (QString ("%L1").arg (0.0000001, 0, 'f', 7));
  falseAlarm->addItem (QString ("%L1").arg (0.00000001, 0, 'f', 8));
  falseAlarm->setCurrentIndex (false_alarm);
  connect (falseAlarm, SIGNAL (currentIndexChanged (int)), this, SLOT (slotFalseAlarmChanged (int)));
  aboxLayout->addWidget (falseAlarm);


  vbox->addWidget (abox);


  QHBoxLayout *actions = new QHBoxLayout (0);
  vbox->addLayout (actions);

  QPushButton *bHelp = new QPushButton (prefsD);
  bHelp->setIcon (QIcon (":/icons/contextHelp.png"));
  bHelp->setToolTip (tr ("Enter What's This mode for help"));
  connect (bHelp, SIGNAL (clicked ()), this, SLOT (slotHelp ()));
  actions->addWidget (bHelp);

  actions->addStretch (10);

  bRestoreDefaults = new QPushButton (tr ("Restore Defaults"), this);
  bRestoreDefaults->setToolTip (tr ("Restore all preferences to the default state"));
  bRestoreDefaults->setWhatsThis (restoreDefaultsText);
  connect (bRestoreDefaults, SIGNAL (clicked ()), this, SLOT (slotRestoreDefaults ()));
  actions->addWidget (bRestoreDefaults);

  QPushButton *closeButton = new QPushButton (tr ("Close"), prefsD);
  closeButton->setToolTip (tr ("Close the preferences dialog"));
  connect (closeButton, SIGNAL (clicked ()), this, SLOT (slotClosePrefs ()));
  actions->addWidget (closeButton);


  //  Put the dialog in the middle of the screen.

  prefsD->move (x () + width () / 2 - prefsD->width () / 2, y () + height () / 2 - prefsD->height () / 2);

  prefsD->show ();
}



void
lidarMonitor::slotPosClicked (int id)
{
  pos_format = id;
}


void 
lidarMonitor::slotFalseAlarmChanged (int index)
{
  false_alarm = index;
}



void
lidarMonitor::slotClosePrefs ()
{
  prefsD->close ();
}



void
lidarMonitor::slotRestoreDefaults ()
{
  pos_format = POS_HDMS;
  false_alarm = 3;
  window_width = 324;
  window_height = 600;
  window_x = 0;
  window_y = 0;


  //  Get the proper version based on the ABE_CZMIL environment variable.

  strcpy (program_version, VERSION);

  if (getenv ("ABE_CZMIL") != NULL)
    {
      if (strstr (getenv ("ABE_CZMIL"), "CZMIL"))
        {
          QString qv = QString (VERSION);
          QString str = qv.section (' ', 4, 6);

          str.prepend ("CME Software - Attribute List ");

          strcpy (program_version, str.toLatin1 ());
        }
      else
        {
          strcpy (program_version, VERSION);
        }
    }

  force_redraw = NVTrue;
}



void
lidarMonitor::about ()
{
  QMessageBox::about (this, program_version,
                      "lidarMonitor - CHARTS, CZMIL, LAS monitor."
                      "\n\nAuthor : Jan C. Depner (area.based.editor@gmail.com)");
}


void
lidarMonitor::aboutQt ()
{
  QMessageBox::aboutQt (this, program_version);
}



//  Get the users defaults.

void
lidarMonitor::envin ()
{
  //  We need to get the font from the global settings.

#ifdef NVWIN3X
  QString ini_file2 = QString (getenv ("USERPROFILE")) + "/ABE.config/" + "globalABE.ini";
#else
  QString ini_file2 = QString (getenv ("HOME")) + "/ABE.config/" + "globalABE.ini";
#endif

  font = QApplication::font ();

  QSettings settings2 (ini_file2, QSettings::IniFormat);
  settings2.beginGroup ("globalABE");


  QString defaultFont = font.toString ();
  QString fontString = settings2.value (QString ("ABE map GUI font"), defaultFont).toString ();
  font.fromString (fontString);


  settings2.endGroup ();


  double saved_version = 1.01;


  // Set Defaults so the if keys don't exist the parms are defined

  slotRestoreDefaults ();
  force_redraw = NVFalse;


  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/lidarMonitor.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/lidarMonitor.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("lidarMonitor");

  saved_version = settings.value (QString ("settings version"), saved_version).toDouble ();


  //  If the settings version has changed we need to leave the values at the new defaults since they may have changed.

  if (settings_version != saved_version) return;


  pos_format = settings.value (QString ("position form"), pos_format).toInt ();

  false_alarm = settings.value (QString ("false alarm array index"), false_alarm).toInt ();

  window_width = settings.value (QString ("width"), window_width).toInt ();

  window_height = settings.value (QString ("height"), window_height).toInt ();

  window_x = settings.value (QString ("window x"), window_x).toInt ();

  window_y = settings.value (QString ("window y"), window_y).toInt ();


  this->restoreState (settings.value (QString ("main window state")).toByteArray (), NINT (settings_version * 100.0));

  settings.endGroup ();
}




//  Save the users defaults.

void
lidarMonitor::envout ()
{
  //  Use frame geometry to get the absolute x and y.

  QRect tmp = this->frameGeometry ();

  window_x = tmp.x ();
  window_y = tmp.y ();


  //  Use geometry to get the width and height.

  tmp = this->geometry ();
  window_width = tmp.width ();
  window_height = tmp.height ();


  //  Get the INI file name

#ifdef NVWIN3X
  QString ini_file = QString (getenv ("USERPROFILE")) + "/ABE.config/lidarMonitor.ini";
#else
  QString ini_file = QString (getenv ("HOME")) + "/ABE.config/lidarMonitor.ini";
#endif

  QSettings settings (ini_file, QSettings::IniFormat);
  settings.beginGroup ("lidarMonitor");


  settings.setValue (QString ("settings version"), settings_version);

  settings.setValue (QString ("position form"), pos_format);

  settings.setValue (QString ("false alarm array index"), false_alarm);

  settings.setValue (QString ("width"), window_width);

  settings.setValue (QString ("height"), window_height);

  settings.setValue (QString ("window x"), window_x);

  settings.setValue (QString ("window y"), window_y);


  settings.setValue (QString ("main window state"), this->saveState (NINT (settings_version * 100.0)));

  settings.endGroup ();
}
