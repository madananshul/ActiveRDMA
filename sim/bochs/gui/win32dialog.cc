/////////////////////////////////////////////////////////////////////////
// $Id: win32dialog.cc,v 1.89 2009/04/11 13:53:14 vruppert Exp $
/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2009  The Bochs Project
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

#include "win32dialog.h"

#if BX_USE_TEXTCONFIG && defined(WIN32) && (BX_WITH_WIN32 || BX_WITH_SDL)

#include "bochs.h"
#include "win32res.h"
#include "win32paramdlg.h"

const char log_choices[5][16] = {"ignore", "log", "ask user", "end simulation", "no change"};
#if BX_DEBUGGER
extern char *debug_cmd;
extern bx_bool debug_cmd_ready;
extern bx_bool vgaw_refresh;
#endif

char *backslashes(char *s)
{
  if (s != NULL) {
    while (*s != 0) {
       if (*s == '/') *s = '\\';
       s++;
    }
  }
  return s;
}

HWND GetBochsWindow()
{
  HWND hwnd;

  hwnd = FindWindow("Bochs for Windows", NULL);
  if (hwnd == NULL) {
    hwnd = GetForegroundWindow();
  }
  return hwnd;
}

BOOL CreateImage(HWND hDlg, int sectors, const char *filename)
{
  if (sectors < 1) {
    MessageBox(hDlg, "The disk size is invalid.", "Invalid size", MB_ICONERROR);
    return FALSE;
  }
  if (lstrlen(filename) < 1) {
    MessageBox(hDlg, "You must type a file name for the new disk image.", "Bad filename", MB_ICONERROR);
    return FALSE;
  }
  int ret = SIM->create_disk_image (filename, sectors, 0);
  if (ret == -1) {  // already exists
    int answer = MessageBox(hDlg, "File exists.  Do you want to overwrite it?",
                            "File exists", MB_YESNO);
    if (answer == IDYES)
      ret = SIM->create_disk_image (filename, sectors, 1);
    else
      return FALSE;
  }
  if (ret == -2) {
    MessageBox(hDlg, "I could not create the disk image. Check for permission problems or available disk space.", "Failed", MB_ICONERROR);
    return FALSE;
  }
  return TRUE;
}

int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
{
  char path[MAX_PATH];

  if (uMsg == BFFM_INITIALIZED) {
    GetCurrentDirectory(MAX_PATH, path);
    SendMessage(hwnd, BFFM_SETSELECTION, TRUE, (LPARAM)path);
  }
  return 0;
}

#ifndef BIF_NEWDIALOGSTYLE
#define BIF_NEWDIALOGSTYLE 0
#endif

int BrowseDir(const char *Title, char *result)
{
  BROWSEINFO browseInfo;
  LPITEMIDLIST ItemIDList;
  int r = -1;

  memset(&browseInfo,0,sizeof(BROWSEINFO));
  browseInfo.hwndOwner = GetBochsWindow();
  browseInfo.pszDisplayName = result;
  browseInfo.lpszTitle = (LPCSTR)Title;
  browseInfo.ulFlags = BIF_NEWDIALOGSTYLE | BIF_RETURNONLYFSDIRS;
  browseInfo.lpfn = BrowseCallbackProc;
  ItemIDList = SHBrowseForFolder(&browseInfo);
  if (ItemIDList != NULL) {
    *result = 0;
    if (SHGetPathFromIDList(ItemIDList, result)) {
      if (result[0]) r = 0;
    }
    // free memory used
    IMalloc * imalloc = 0;
    if (SUCCEEDED(SHGetMalloc(&imalloc))) {
      imalloc->Free(ItemIDList);
      imalloc->Release();
    }
  }
  return r;
}

static BOOL CALLBACK LogAskProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  BxEvent *event;
  int level;

  switch (msg) {
    case WM_INITDIALOG:
      event = (BxEvent*)lParam;
      level = event->u.logmsg.level;
      SetWindowText(hDlg, SIM->get_log_level_name(level));
      SetWindowText(GetDlgItem(hDlg, IDASKDEV), event->u.logmsg.prefix);
      SetWindowText(GetDlgItem(hDlg, IDASKMSG), event->u.logmsg.msg);
      SendMessage(GetDlgItem(hDlg, IDASKLIST), LB_ADDSTRING, 0, (LPARAM)"Continue");
      SendMessage(GetDlgItem(hDlg, IDASKLIST), LB_ADDSTRING, 0, (LPARAM)"Continue and don't ask again");
      SendMessage(GetDlgItem(hDlg, IDASKLIST), LB_ADDSTRING, 0, (LPARAM)"Kill simulation");
      SendMessage(GetDlgItem(hDlg, IDASKLIST), LB_ADDSTRING, 0, (LPARAM)"Abort (dump core)");
#if BX_DEBUGGER
      SendMessage(GetDlgItem(hDlg, IDASKLIST), LB_ADDSTRING, 0, (LPARAM)"Continue and return to debugger");
#endif
      SendMessage(GetDlgItem(hDlg, IDASKLIST), LB_SETCURSEL, 2, 0);
      SetFocus(GetDlgItem(hDlg, IDASKLIST));
      return FALSE;
    case WM_CLOSE:
      EndDialog(hDlg, BX_LOG_ASK_CHOICE_DIE);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK:
          EndDialog(hDlg, SendMessage(GetDlgItem(hDlg, IDASKLIST), LB_GETCURSEL, 0, 0));
          break;
        case IDCANCEL:
          EndDialog(hDlg, BX_LOG_ASK_CHOICE_DIE);
          break;
      }
  }
  return FALSE;
}

static BOOL CALLBACK StringParamProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static bx_param_string_c *param;
  char buffer[512];
  const char *title;

  switch (msg) {
    case WM_INITDIALOG:
      param = (bx_param_string_c *)lParam;
      title = param->get_label();
      if ((title == NULL) || (strlen(title) == 0)) {
        title = param->get_name();
      }
      SetWindowText(hDlg, title);
      SetWindowText(GetDlgItem(hDlg, IDSTRING), param->getptr());
      SendMessage(GetDlgItem(hDlg, IDSTRING), EM_SETLIMITTEXT, param->get_maxsize(), 0);
      return TRUE;
      break;
    case WM_CLOSE:
      EndDialog(hDlg, -1);
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDOK:
          GetDlgItemText(hDlg, IDSTRING, buffer, param->get_maxsize() + 1);
          param->set(buffer);
          EndDialog(hDlg, 1);
          break;
        case IDCANCEL:
          EndDialog(hDlg, -1);
          break;
      }
  }
  return FALSE;
}

static BOOL CALLBACK FloppyDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static bx_param_filename_c *param;
  static bx_param_bool_c *status;
  static bx_param_enum_c *devtype;
  static bx_param_enum_c *mediatype;
  char mesg[MAX_PATH];
  char path[MAX_PATH];
  char pname[80];
  const char *title;
  int i, cap;

  switch (msg) {
    case WM_INITDIALOG:
      param = (bx_param_filename_c *)lParam;
      param->get_param_path(pname, 80);
      if (!strcmp(pname, BXPN_FLOPPYA_PATH)) {
        status = SIM->get_param_bool(BXPN_FLOPPYA_STATUS);
        devtype = SIM->get_param_enum(BXPN_FLOPPYA_DEVTYPE);
        mediatype = SIM->get_param_enum(BXPN_FLOPPYA_TYPE);
      } else {
        status = SIM->get_param_bool(BXPN_FLOPPYB_STATUS);
        devtype = SIM->get_param_enum(BXPN_FLOPPYB_DEVTYPE);
        mediatype = SIM->get_param_enum(BXPN_FLOPPYB_TYPE);
      }
      cap = devtype->get() - (int)devtype->get_min();
      SetWindowText(GetDlgItem(hDlg, IDDEVTYPE), floppy_devtype_names[cap]);
      i = 0;
      while (floppy_type_names[i] != NULL) {
        SendMessage(GetDlgItem(hDlg, IDMEDIATYPE), CB_ADDSTRING, 0, (LPARAM)floppy_type_names[i]);
        SendMessage(GetDlgItem(hDlg, IDMEDIATYPE), CB_SETITEMDATA, i, (LPARAM)(mediatype->get_min() + i));
        i++;
      }
      cap = mediatype->get() - (int)mediatype->get_min();
      SendMessage(GetDlgItem(hDlg, IDMEDIATYPE), CB_SETCURSEL, cap, 0);
      if (status->get()) {
        SendMessage(GetDlgItem(hDlg, IDSTATUS), BM_SETCHECK, BST_CHECKED, 0);
      }
      lstrcpy(path, param->getptr());
      title = param->get_label();
      if (!title) title = param->get_name();
      SetWindowText(hDlg, title);
      if (lstrlen(path) && lstrcmp(path, "none")) {
        SetWindowText(GetDlgItem(hDlg, IDPATH), path);
      }
      return TRUE;
      break;
    case WM_CLOSE:
      EndDialog(hDlg, -1);
      return TRUE;
      break;
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
        case IDBROWSE:
          GetDlgItemText(hDlg, IDPATH, path, MAX_PATH);
          if (AskFilename(hDlg, param, path) > 0) {
            SetWindowText(GetDlgItem(hDlg, IDPATH), path);
            SendMessage(GetDlgItem(hDlg, IDSTATUS), BM_SETCHECK, BST_CHECKED, 0);
            SendMessage(GetDlgItem(hDlg, IDMEDIATYPE), CB_SELECTSTRING, (WPARAM)-1, (LPARAM)"auto");
            EnableWindow(GetDlgItem(hDlg, IDCREATE), FALSE);
          }
          return TRUE;
          break;
        case IDOK:
          status->set(0);
          if (SendMessage(GetDlgItem(hDlg, IDSTATUS), BM_GETCHECK, 0, 0) == BST_CHECKED) {
            GetDlgItemText(hDlg, IDPATH, path, MAX_PATH);
            if (lstrlen(path) == 0) {
              lstrcpy(path, "none");
            }
          } else {
            lstrcpy(path, "none");
          }
          param->set(path);
          i = SendMessage(GetDlgItem(hDlg, IDMEDIATYPE), CB_GETCURSEL, 0, 0);
          cap = SendMessage(GetDlgItem(hDlg, IDMEDIATYPE), CB_GETITEMDATA, i, 0);
          mediatype->set(cap);
          if (lstrcmp(path, "none")) {
            status->set(1);
          }
          EndDialog(hDlg, 1);
          return TRUE;
          break;
        case IDCANCEL:
          EndDialog(hDlg, -1);
          return TRUE;
          break;
        case IDMEDIATYPE:
          if (HIWORD(wParam) == CBN_SELCHANGE) {
            i = SendMessage(GetDlgItem(hDlg, IDMEDIATYPE), CB_GETCURSEL, 0, 0);
            EnableWindow(GetDlgItem(hDlg, IDCREATE), (floppy_type_n_sectors[i] > 0));
          }
          break;
        case IDCREATE:
          GetDlgItemText(hDlg, IDPATH, path, MAX_PATH);
          backslashes(path);
          i = SendMessage(GetDlgItem(hDlg, IDMEDIATYPE), CB_GETCURSEL, 0, 0);
          if (CreateImage(hDlg, floppy_type_n_sectors[i], path)) {
            wsprintf(mesg, "Created a %s disk image called %s", floppy_type_names[i], path);
            MessageBox(hDlg, mesg, "Image created", MB_OK);
          }
          return TRUE;
          break;
      }
  }
  return FALSE;
}

void SetStandardLogOptions(HWND hDlg)
{
  int level, idx;
  int defchoice[5];

  for (level=0; level<5; level++) {
    int mod = 0;
    int first = SIM->get_log_action (mod, level);
    BOOL consensus = true;
    // now compare all others to first.  If all match, then use "first" as
    // the initial value.
    for (mod=1; mod<SIM->get_n_log_modules(); mod++) {
      if (first != SIM->get_log_action (mod, level)) {
        consensus = false;
        break;
      }
    }
    if (consensus)
      defchoice[level] = first;
    else
      defchoice[level] = 4;
  }
  for (level=0; level<5; level++) {
    idx = 0;
    SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_RESETCONTENT, 0, 0);
    for (int action=0; action<5; action++) {
      if (((level > 1) && (action > 0)) || ((level < 2) && ((action < 2) || (action > 3)))) {
        SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_ADDSTRING, 0, (LPARAM)log_choices[action]);
        SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_SETITEMDATA, idx, action);
        if (action == defchoice[level]) {
          SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_SETCURSEL, idx, 0);
        }
        idx++;
      }
    }
  }
  EnableWindow(GetDlgItem(hDlg, IDDEVLIST), FALSE);
}

void SetAdvancedLogOptions(HWND hDlg)
{
  int idx, level, mod;

  idx = SendMessage(GetDlgItem(hDlg, IDDEVLIST), LB_GETCURSEL, 0, 0);
  mod = SendMessage(GetDlgItem(hDlg, IDDEVLIST), LB_GETITEMDATA, idx, 0);
  for (level=0; level<5; level++) {
    idx = 0;
    SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_RESETCONTENT, 0, 0);
    for (int action=0; action<4; action++) {
      if (((level > 1) && (action > 0)) || ((level < 2) && (action < 2))) {
        SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_ADDSTRING, 0, (LPARAM)log_choices[action]);
        SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_SETITEMDATA, idx, action);
        if (action == SIM->get_log_action (mod, level)) {
          SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_SETCURSEL, idx, 0);
        }
        idx++;
      }
    }
  }
}

void InitLogOptionsDialog(HWND hDlg)
{
  int idx, mod;
  char prefix[8];

  for (mod=0; mod<SIM->get_n_log_modules(); mod++) {
    if (strcmp(SIM->get_prefix(mod), "[     ]")) {
      lstrcpyn(prefix, SIM->get_prefix(mod), sizeof(prefix));
      lstrcpy(prefix, prefix+1);
      prefix[5] = 0;
      idx = SendMessage(GetDlgItem(hDlg, IDDEVLIST), LB_ADDSTRING, 0, (LPARAM)prefix);
      SendMessage(GetDlgItem(hDlg, IDDEVLIST), LB_SETITEMDATA, idx, mod);
    }
  }
  SetStandardLogOptions(hDlg);
}

void ApplyLogOptions(HWND hDlg, BOOL advanced)
{
  int idx, level, mod, value;

  if (advanced) {
    idx = SendMessage(GetDlgItem(hDlg, IDDEVLIST), LB_GETCURSEL, 0, 0);
    mod = SendMessage(GetDlgItem(hDlg, IDDEVLIST), LB_GETITEMDATA, idx, 0);
    for (level=0; level<5; level++) {
      idx = SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_GETCURSEL, 0, 0);
      value = SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_GETITEMDATA, idx, 0);
      SIM->set_log_action (mod, level, value);
    }
    EnableWindow(GetDlgItem(hDlg, IDDEVLIST), TRUE);
  } else {
    for (level=0; level<5; level++) {
      idx = SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_GETCURSEL, 0, 0);
      value = SendMessage(GetDlgItem(hDlg, IDLOGEVT1+level), CB_GETITEMDATA, idx, 0);
      if (value < 4) {
        // set new default
        SIM->set_default_log_action (level, value);
        // apply that action to all modules (devices)
        SIM->set_log_action (-1, level, value);
      }
    }
  }
  EnableWindow(GetDlgItem(hDlg, IDADVLOGOPT), TRUE);
}

static BOOL CALLBACK LogOptDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static BOOL advanced;
  static BOOL changed;
  long noticode;

  switch (msg) {
    case WM_INITDIALOG:
      InitLogOptionsDialog(hDlg);
      advanced = FALSE;
      changed = FALSE;
      EnableWindow(GetDlgItem(hDlg, IDAPPLY), FALSE);
      return TRUE;
    case WM_CLOSE:
      EndDialog(hDlg, 0);
      break;
    case WM_COMMAND:
      noticode = HIWORD(wParam);
      switch(noticode) {
        case CBN_SELCHANGE: /* LBN_SELCHANGE is the same value */
          switch (LOWORD(wParam)) {
            case IDDEVLIST:
              SetAdvancedLogOptions(hDlg);
              break;
            case IDLOGEVT1:
            case IDLOGEVT2:
            case IDLOGEVT3:
            case IDLOGEVT4:
            case IDLOGEVT5:
              if (!changed) {
                EnableWindow(GetDlgItem(hDlg, IDADVLOGOPT), FALSE);
                if (advanced) {
                  EnableWindow(GetDlgItem(hDlg, IDDEVLIST), FALSE);
                }
                changed = TRUE;
                EnableWindow(GetDlgItem(hDlg, IDAPPLY), TRUE);
              }
              break;
          }
          break;
        default:
          switch (LOWORD(wParam)) {
            case IDADVLOGOPT:
              if (SendMessage(GetDlgItem(hDlg, IDADVLOGOPT), BM_GETCHECK, 0, 0) == BST_CHECKED) {
                EnableWindow(GetDlgItem(hDlg, IDDEVLIST), TRUE);
                SendMessage(GetDlgItem(hDlg, IDDEVLIST), LB_SETCURSEL, 0, 0);
                SetAdvancedLogOptions(hDlg);
                advanced = TRUE;
              } else {
                SendMessage(GetDlgItem(hDlg, IDDEVLIST), LB_SETCURSEL, (WPARAM)-1, 0);
                SetStandardLogOptions(hDlg);
                advanced = FALSE;
              }
              break;
            case IDAPPLY:
              ApplyLogOptions(hDlg, advanced);
              EnableWindow(GetDlgItem(hDlg, IDAPPLY), FALSE);
              changed = FALSE;
              break;
            case IDOK:
              if (changed) {
                ApplyLogOptions(hDlg, advanced);
              }
              EndDialog(hDlg, 1);
              break;
            case IDCANCEL:
              EndDialog(hDlg, 0);
              break;
          }
      }
      break;
  }
  return FALSE;
}

void LogOptionsDialog(HWND hwnd)
{
  DialogBox(NULL, MAKEINTRESOURCE(LOGOPT_DLG), hwnd, (DLGPROC)LogOptDlgProc);
}

typedef struct {
  const char *label;
  const char *param;
} edit_opts_t;

edit_opts_t start_options[] = {
  {"Logfile", "log"},
  {"Log Options", "*"},
  {"CPU", "cpu"},
  {"Memory", "memory"},
  {"Clock & CMOS", "clock_cmos"},
  {"PCI", "pci"},
  {"Display & Interface", "display"},
  {"Keyboard & Mouse", "keyboard_mouse"},
  {"Disk & Boot", BXPN_MENU_DISK_WIN32},
  {"Serial / Parallel / USB", "ports"},
  {"Network card", "network"},
  {"Sound Blaster 16", BXPN_SB16},
  {"Other", "misc"},
#if BX_PLUGINS
  {"User-defined Options", "user"},
#endif
  {NULL, NULL}
};

edit_opts_t runtime_options[] = {
  {"CD-ROM", BXPN_MENU_RUNTIME_CDROM},
  {"USB", BXPN_MENU_RUNTIME_USB},
  {"Misc", BXPN_MENU_RUNTIME_MISC},
  {"Log Options", "*"},
  {NULL, NULL}
};
static BOOL CALLBACK MainMenuDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static bx_bool runtime;
  int choice, code, i;
  bx_param_filename_c *rcfile;
  char path[BX_PATHNAME_LEN];
  const char *pname;

  switch (msg) {
    case WM_INITDIALOG:
      runtime = (bx_bool)lParam;
      EnableWindow(GetDlgItem(hDlg, IDEDITCFG), FALSE);
      if (runtime) {
        SetWindowText(hDlg, "Bochs Runtime Menu");
        EnableWindow(GetDlgItem(hDlg, IDREADRC), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDRESETCFG), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDRESTORE), FALSE);
        SetWindowText(GetDlgItem(hDlg, IDOK), "&Continue");
        i = 0;
        while (runtime_options[i].label != NULL) {
          SendMessage(GetDlgItem(hDlg, IDEDITBOX), LB_ADDSTRING, 0, (LPARAM)runtime_options[i].label);
          i++;
        }
        choice = IDOK;
      } else {
        i = 0;
        while (start_options[i].label != NULL) {
          SendMessage(GetDlgItem(hDlg, IDEDITBOX), LB_ADDSTRING, 0, (LPARAM)start_options[i].label);
          i++;
        }
        if (SIM->get_param_enum(BXPN_BOCHS_START)->get() == BX_LOAD_START) {
          choice = IDREADRC;
        } else {
          choice = IDOK;
        }
      }
      SetFocus(GetDlgItem(hDlg, choice));
      return FALSE;
    case WM_CLOSE:
      EndDialog(hDlg, -1);
      break;
    case WM_COMMAND:
      code = HIWORD(wParam);
      switch (LOWORD(wParam)) {
        case IDREADRC:
          rcfile = new bx_param_filename_c(NULL, "rcfile", "Load Bochs Config File",
                                           "", "bochsrc.bxrc", BX_PATHNAME_LEN);
          rcfile->set_extension("bxrc");
          if (AskFilename(hDlg, rcfile, NULL) > 0) {
            SIM->reset_all_param();
            SIM->read_rc(rcfile->getptr());
          }
          delete rcfile;
          break;
        case IDWRITERC:
          rcfile = new bx_param_filename_c(NULL, "rcfile", "Save Bochs Config File",
                                           "", "bochsrc.bxrc", BX_PATHNAME_LEN);
          rcfile->set_extension("bxrc");
          rcfile->set_options(rcfile->SAVE_FILE_DIALOG);
          if (AskFilename(hDlg, rcfile, NULL) > 0) {
            SIM->write_rc(rcfile->getptr(), 1);
          }
          delete rcfile;
          break;
        case IDEDITBOX:
          if ((code == LBN_SELCHANGE) ||
              (code == LBN_DBLCLK)) {
            EnableWindow(GetDlgItem(hDlg, IDEDITCFG), TRUE);
          }
          if (code != LBN_DBLCLK) {
            break;
          }
        case IDEDITCFG:
          i = SendMessage(GetDlgItem(hDlg, IDEDITBOX), LB_GETCURSEL, 0, 0);
          if (runtime) {
            pname = runtime_options[i].param;
          } else {
            pname = start_options[i].param;
          }
          if (lstrcmp(pname, "*")) {
            if (((bx_list_c*)SIM->get_param(pname))->get_size() > 0) {
              win32ParamDialog(hDlg, pname);
            } else {
              MessageBox(hDlg, "Nothing to configure in this section", "Warning", MB_ICONEXCLAMATION);
            }
          } else {
            LogOptionsDialog(hDlg);
          }
          break;
        case IDRESETCFG:
          if (MessageBox(hDlg, "Reset all options back to their factory defaults ?",
                         "Reset Configuration", MB_ICONEXCLAMATION | MB_YESNO) == IDYES) {
            SIM->reset_all_param();
          }
          break;
        case IDRESTORE:
          path[0] = 0;
          if (BrowseDir("Restore Bochs state from...", path) >= 0) {
            SIM->get_param_bool(BXPN_RESTORE_FLAG)->set(1);
            SIM->get_param_string(BXPN_RESTORE_PATH)->set(path);
            EndDialog(hDlg, 1);
          }
          break;
        case IDOK:
          EndDialog(hDlg, 1);
          break;
        case IDCANCEL:
          bx_user_quit = 1;
          EndDialog(hDlg, -1);
          break;
      }
  }
  return FALSE;
}

void LogAskDialog(BxEvent *event)
{
  event->retcode = DialogBoxParam(NULL, MAKEINTRESOURCE(ASK_DLG), GetBochsWindow(),
                                  (DLGPROC)LogAskProc, (LPARAM)event);
}

int AskString(bx_param_string_c *param)
{
  return DialogBoxParam(NULL, MAKEINTRESOURCE(STRING_DLG), GetBochsWindow(),
                        (DLGPROC)StringParamProc, (LPARAM)param);
}

int FloppyDialog(bx_param_filename_c *param)
{
  return DialogBoxParam(NULL, MAKEINTRESOURCE(FLOPPY_DLG), GetBochsWindow(),
                        (DLGPROC)FloppyDlgProc, (LPARAM)param);
}

int MainMenuDialog(HWND hwnd, bx_bool runtime)
{
  return DialogBoxParam(NULL, MAKEINTRESOURCE(MAINMENU_DLG), hwnd,
                        (DLGPROC)MainMenuDlgProc, (LPARAM)runtime);
}

BxEvent* win32_notify_callback(void *unused, BxEvent *event)
{
  int opts;
  bx_param_c *param;
  bx_param_string_c *sparam;
  char pname[BX_PATHNAME_LEN];

  event->retcode = -1;
  switch (event->type)
  {
    case BX_SYNC_EVT_LOG_ASK:
      LogAskDialog(event);
      return event;
#if BX_DEBUGGER && BX_DEBUGGER_GUI
    case BX_SYNC_EVT_GET_DBG_COMMAND:
      {
        // sim is at a "break" -- internal debugger is ready for a command
        debug_cmd = new char[512];
        debug_cmd_ready = FALSE;
        HitBreak();
        while (debug_cmd_ready == FALSE && bx_user_quit == 0)
        {
          if (vgaw_refresh != FALSE)  // is the GUI frontend requesting a VGAW refresh?
            SIM->refresh_vga();
          vgaw_refresh = FALSE;
          Sleep(10);
        }
        if (bx_user_quit != 0)
          BX_EXIT(0);
        event->u.debugcmd.command = debug_cmd;
        event->retcode = 1;
        return event;
      }
    case BX_ASYNC_EVT_DBG_MSG:
      ParseIDText (event->u.logmsg.msg);
      return event;
#endif
    case BX_SYNC_EVT_ASK_PARAM:
      param = event->u.param.param;
      if (param->get_type() == BXT_PARAM_STRING) {
        sparam = (bx_param_string_c *)param;
        opts = sparam->get_options();
        if (opts & sparam->IS_FILENAME) {
          if (opts & sparam->SELECT_FOLDER_DLG) {
            event->retcode = BrowseDir(sparam->get_label(), sparam->getptr());
          } else if (param->get_parent() == NULL) {
            event->retcode = AskFilename(GetBochsWindow(), (bx_param_filename_c *)sparam, NULL);
          } else {
            event->retcode = FloppyDialog((bx_param_filename_c *)sparam);
          }
          return event;
        } else {
          event->retcode = AskString(sparam);
          return event;
        }
      } else if (param->get_type() == BXT_LIST) {
        SIM->get_first_cdrom()->get_param_path(pname, BX_PATHNAME_LEN);
        event->retcode = win32ParamDialog(GetBochsWindow(), pname);
        return event;
      } else if (param->get_type() == BXT_PARAM_BOOL) {
        UINT flag = MB_YESNO | MB_SETFOREGROUND;
        if (((bx_param_bool_c *)param)->get() == 0) {
          flag |= MB_DEFBUTTON2;
        }
        ((bx_param_bool_c *)param)->set(MessageBox(GetActiveWindow(), param->get_description(), param->get_label(), flag) == IDYES);
        event->retcode = 0;
        return event;
      }
    case BX_SYNC_EVT_TICK: // called periodically by siminterface.
      event->retcode = 0;
      // fall into default case
    default:
      return event;
  }
}

static int win32_ci_callback(void *userdata, ci_command_t command)
{
  switch (command)
  {
    case CI_START:
      SIM->set_notify_callback(win32_notify_callback, NULL);
      if (SIM->get_param_enum(BXPN_BOCHS_START)->get() == BX_QUICK_START) {
        SIM->begin_simulation(bx_startup_flags.argc, bx_startup_flags.argv);
        // we don't expect it to return, but if it does, quit
        SIM->quit_sim(1);
      } else {
        if (MainMenuDialog(GetActiveWindow(), 0) == 1) {
          SIM->begin_simulation(bx_startup_flags.argc, bx_startup_flags.argv);
        }
        SIM->quit_sim(1);
      }
      break;
    case CI_RUNTIME_CONFIG:
      if (MainMenuDialog(GetBochsWindow(), 1) < 0) {
        bx_user_quit = 1;
#if !BX_DEBUGGER
        bx_atexit();
        SIM->quit_sim(1);
#else
        bx_dbg_exit(1);
#endif
        return -1;
      }
      break;
    case CI_SHUTDOWN:
      break;
  }
  return 0;
}

int init_win32_config_interface()
{
  SIM->register_configuration_interface("win32config", win32_ci_callback, NULL);
  return 0;  // success
}

#endif // BX_USE_TEXTCONFIG && defined(WIN32)
