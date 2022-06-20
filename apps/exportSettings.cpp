#include <stdio.h>
#include <stdlib.h> // system
#include <ctime>    // time, time_t, localtime, strftime
#include "settings.h"
#include "mainwin.h"

#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Output.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Radio_Round_Button.H>
#include <FL/Fl_Multiline_Output.H>

// TODO seriously consider some sort of settings container!

extern long int _timelapseStep;
extern long int _timelapseCount;
extern int _timelapseW;
extern int _timelapseH;
extern bool _timelapsePNG;

extern int previewX;
extern int previewY;
extern int previewW;
extern int previewH;

extern int _captureW;
extern int _captureH;
extern bool _capturePNG;
extern const char *_captureFolder;

extern double _zoom;
extern double _panH;
extern double _panV;

extern const char *getExposureString();
extern const char *getMeteringString();
extern void calculateTimelapse(MainWin *);
extern MainWin* _window; // TODO hack hack

// dialog box to pick amongst subsets: timelapse, capture, zoom, ... ; pick file to export to
// add comment for header
// TODO don't write setting if unchanged by user? OPTION (i.e. don't write defaults)

#define TIMELAPSE_SET 0x2
#define CAPTURE_SET   0x4
#define ZOOM_SET      0x8
#define PREVIEW_SET   0x10
#define ADVANCED_SET  0x20

Fl_Double_Window *exportDlg;
Fl_Check_Button  *previewChk;
Fl_Check_Button  *zoomChk;
Fl_Check_Button  *stillChk;
Fl_Radio_Round_Button  *timelapseChk;
Fl_Radio_Round_Button  *captureChk;
Fl_Input *commentInp;
Fl_Box *outFile;

static void writeDouble(FILE *f, const char *str, double val)
{
    fprintf(f, "%s=%g\n", str, val);
}

static void writeBool(FILE *f, const char *str, bool val)
{
    fprintf(f, "%s=%s\n", str, val ? "true" : "false");
}

static void writeInt(FILE *f, const char *str, long int val)
{
    fprintf(f, "%s=%ld\n", str, val);
}

static void writeString(FILE *f, const char *str, const char *val, bool delimit=false)
{
    if (delimit)
        fprintf(f, "%s='%s'\n", str, val);
    else
        fprintf(f, "%s=%s\n", str, val);
}

static bool writeConfigFile(const char *filename, int options, const char *comment)
{
    
    FILE *f = fl_fopen(filename, "w");
    if (!f)
    {
        // TODO error creating output file
        return false;
    }

    fprintf(f, "# libcamera-app config file\n");
    {
        char buffer[1024];
        time_t rawtime;
        struct tm* timeinfo;
        time(&rawtime);
        timeinfo=localtime(&rawtime);
        strftime(buffer,sizeof(buffer), "# generated by libcam_fltk %b-%d-%Y %H:%M:%S", timeinfo);
        fprintf(f, "%s\n", buffer);
    }
    if (comment)
    {
        fprintf(f, "# %s\n", comment);
    }
    fprintf(f, "\n");
    
    // Basic settings
    fprintf(f, "# basic settings\n");
    writeDouble(f, "analoggain", _analogGain);
    writeDouble(f, "brightness", _bright);
    writeDouble(f, "contrast", _contrast);
    writeDouble(f, "ev", _evComp);
    writeString(f, "exposure", getExposureString());
    writeBool  (f, "hflip", _hflip);
    writeString(f, "metering", getMeteringString());
    writeDouble(f, "saturation", _saturate);
    writeDouble(f, "sharpness", _sharp);
    writeBool  (f, "vflip", _vflip);

    // preview settings
    if (options & PREVIEW_SET)
    {
        fprintf(f, "\n");
        fprintf(f, "# preview settings\n");
        writeBool(f, "fullscreen", false);
        writeBool(f, "nopreview", !_previewOn);
        
        // preview X,Y,W,H
        fprintf(f, "preview=%d,%d,%d,%d\n",previewX,previewY,previewW,previewH);        
    }
    
    // timelapse settings
    if (options & TIMELAPSE_SET)
    {
        fprintf(f, "\n");
        fprintf(f, "# timelapse settings\n");
        // TODO do these need to be calculated? or pulled from prefs?
        calculateTimelapse(_window);
        writeInt(f, "timelapse", _timelapseStep);
        writeInt(f, "timeout", _timelapseCount * _timelapseStep);
        writeString(f, "encoding", _timelapsePNG ? "png" : "jpg");
        writeInt(f, "height", _timelapseH);
        writeBool(f, "timestamp", true);
        writeInt(f, "width", _timelapseW);
    }

    // capture settings
    if (options & CAPTURE_SET)
    {
        fprintf(f, "\n");
        fprintf(f, "# capture settings\n");

        writeString(f, "encoding", _capturePNG ? "png" : "jpg");
        writeInt(f, "height", _captureH);
        
        // TODO this doesn't work: libcamera-apps doesn't have the concept of an output _folder_
        // writeString(f, "output", _captureFolder, true);
        
        writeBool(f, "timestamp", true);
        writeInt(f, "width", _captureW);
    }
    
    // zoom settings
    if (options & ZOOM_SET)
    {
        fprintf(f, "\n");
        fprintf(f, "# zoom setting\n");
        fprintf(f, "roi=%g,%g,%g,%g\n", _panH, _panV, _zoom, _zoom);
    }
    
#if 0
    // advanced
    awb
    awbgains
    
#endif
    
    fclose(f);
    return true;
}

static void cbExport(Fl_Widget *, void *)
{
    // TODO verify destination set
    //const char *outfile = outFile->value();
    const char *outfile = outFile->label();

#if 0 // TODO pending update to FLTK    
    if (fl_make_path_for_file(outfile) == 0)
    {
        fl_alert("Unable to create necessary directory");
        return;
    }
#endif

    // determine which settings to write based on checkboxes
    int settings = 0;
    settings |= (previewChk->value() != 0) ? PREVIEW_SET : 0;
    settings |= (zoomChk->value() != 0) ? ZOOM_SET : 0;
    if (stillChk->value() != 0)
    {
        settings |= (timelapseChk->value() != 0) ? TIMELAPSE_SET : 0;
        settings |= (captureChk->value() != 0) ? CAPTURE_SET : 0;
    }
    
    const char *start = outFile->label(); // outFile->value()
    bool res = writeConfigFile(start, settings, commentInp->value());
    if (!res)
    {
        fl_alert("Failed to write config file");
    }
    else
    {
        switch( fl_choice("Successfully wrote config file", "View", "OK", nullptr))
        {
            case 1:
                return;
            case 0:
            {
                char buff[1024];
                sprintf(buff, "xdg-open '%s'", outfile);
                system(buff);
                return;
            }
        }
    }
}

static void cbClose(Fl_Widget *, void *)
{
    exportDlg->hide();
}

static void outFilePick(Fl_Widget *, void *)
{
    const char *start = outFile->label(); // outFile->value()
    Fl_File_Chooser* choose = new Fl_File_Chooser(start, "Text files (*.txt)",
                                                  Fl_File_Chooser::CREATE,
                                                  "Specify a file to export to");
    choose->preview(false); // force preview off

    // TODO common code below
    choose->show();

    // deactivate Fl::grab(), because it is incompatible with modal windows
    Fl_Window* g = Fl::grab();
    if (g) Fl::grab(nullptr);

    while (choose->shown())
        Fl::wait();

    if (g) // regrab the previous popup menu, if there was one
        Fl::grab(g);
    // TODO end common code

    
    if (!choose->count())  // no selection
        return;

    char *loaddir = (char*)choose->value();

    // update input field with picked folder
    outFile->label(loaddir);
    //outFile->value(loaddir);   
}

Fl_Double_Window *make_export()
{
    // TODO libcamera-still vs libcamera-vid settings?
    
    auto panel = new Fl_Double_Window(400, 400, "Export options");

    int Y = 15;
    
    {
    auto o = new Fl_Multiline_Output(10, Y, 375, 60);
    o->value("Using the current settings, creates a config\nfile which may be used with the libcamera\napps via the \"-c\" command line option.");
    o->box(FL_NO_BOX);
    }
    
    Y += 65;
    previewChk = new Fl_Check_Button(10, Y, 150, 25, "Preview Settings");
    previewChk->value(0);

    Y += 30;
    
    zoomChk = new Fl_Check_Button(10, Y, 150, 25, "Zoom Settings");
    zoomChk->value(0);

    Y += 30;

    stillChk = new Fl_Check_Button(10, Y, 150, 25, "libcamera-still Settings");
    stillChk->value(0);

    // TODO consider enabling/disabling timelapse/capture radios based on 'stillChk'
    
    Y += 30;    
    
    // Timelapse and capture are mutually exclusive
    timelapseChk = new Fl_Radio_Round_Button(30, Y, 150, 25, "Timelapse Settings");
    timelapseChk->value(0);

    Y += 30;

    captureChk = new Fl_Radio_Round_Button(30, Y, 150, 25, "Capture Settings");
    captureChk->value(1);

    Y += 30;

    // comment
    commentInp= new Fl_Input(10, Y+25, 350, 25, "Header comment:");
    commentInp->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    commentInp->tooltip("Provide text to include in the file header");

    Y += 60;
    
    // output file
    Fl_Box *outflabel = new Fl_Box(10, Y, 100, 25);
    outflabel->label("Output file:");
    outflabel->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
    //outFile->align(Fl_Align(FL_ALIGN_TOP_LEFT));
    outFile = new Fl_Box(10, Y+30, 300, 25);
    outFile->label("/home/pi/Documents/config.txt");
    outFile->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));

    Fl_Button* btn = new Fl_Button(110, Y, 50, 25, "Pick");
    btn->callback(outFilePick);
    
    Y += 60;
    {
    Fl_Button *o = new Fl_Button(50, Y+20, 83, 25, "Export");
    o->callback(cbExport);
    }
    {
    Fl_Button *o = new Fl_Button(250, Y+20, 83, 25, "Close");
    o->callback(cbClose);
    }
        
    panel->end();
    
    return panel;
}

void do_export_settings()
{
    exportDlg = make_export();
    exportDlg->set_modal();
    
    // TODO common code    
    exportDlg->show();

    // deactivate Fl::grab(), because it is incompatible with modal windows
    Fl_Window* g = Fl::grab();
    if (g) Fl::grab(nullptr);

    while (exportDlg->shown())
        Fl::wait();

    if (g) // regrab the previous popup menu, if there was one
        Fl::grab(g);
    // TODO end common code
    
    delete exportDlg;
    exportDlg = nullptr;
}
