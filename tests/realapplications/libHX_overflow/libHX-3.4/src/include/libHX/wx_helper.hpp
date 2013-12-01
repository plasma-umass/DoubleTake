#ifndef _LIBHX_WXHELPER_HPP
#define _LIBHX_WXHELPER_HPP 1

/* Convert from UTF-8 to wxString; only valid within the scope of the result */
#define wxfu8(s)	wxString((s), wxConvUTF8)

/* Convert from UTF-8 to wxString for varargs/wxPrintf */
#define wxfv8(s)	(wxfu8(s).c_str())

/* Convert from wxString to UTF-8; limited validity */
#define wxtu8(s)	static_cast<const char *>((s).ToUTF8())

/* Common frame flags */
#define wxCFF     (wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCLOSE_BOX | \
                  wxMAXIMIZE_BOX | wxMINIMIZE_BOX | wxFRAME_NO_TASKBAR)
#define wxACV     wxALIGN_CENTER_VERTICAL
#define wxDPOS    wxDefaultPosition
#define wxDSIZE   wxDefaultSize

#endif /* _LIBHX_WXHELPER_HPP */
