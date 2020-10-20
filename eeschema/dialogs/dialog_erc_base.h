///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_HTML_REPORT_BOX;
class WX_INFOBAR;

#include "dialog_shim.h"
#include <wx/infobar.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/html/htmlwin.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dataview.h>
#include <wx/notebook.h>
#include <wx/stattext.h>
#include <wx/checkbox.h>
#include <widgets/number_badge.h>
#include <wx/button.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_ERASE_DRC_MARKERS 1000

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ERC_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ERC_BASE : public DIALOG_SHIM
{
	private:

	protected:
		WX_INFOBAR* m_infoBar;
		wxNotebook* m_notebook;
		wxPanel* messagesPanel;
		WX_HTML_REPORT_BOX* m_messages;
		wxPanel* violationsPanel;
		wxDataViewCtrl* m_markerDataView;
		wxStaticText* m_showLabel;
		wxCheckBox* m_showAll;
		wxCheckBox* m_showErrors;
		NUMBER_BADGE* m_errorsBadge;
		wxCheckBox* m_showWarnings;
		NUMBER_BADGE* m_warningsBadge;
		wxCheckBox* m_showExclusions;
		NUMBER_BADGE* m_exclusionsBadge;
		wxButton* m_saveReport;
		wxStaticLine* m_staticline1;
		wxBoxSizer* m_buttonsSizer;
		wxButton* m_buttondelmarkers;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseErcDialog( wxCloseEvent& event ) { event.Skip(); }
		virtual void OnLinkClicked( wxHtmlLinkEvent& event ) { event.Skip(); }
		virtual void OnERCItemDClick( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnERCItemRClick( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnERCItemSelected( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnSeverity( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSaveReport( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEraseDrcMarkersClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRunERCClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_ERC_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Electrical Rules Checker"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_ERC_BASE();

};

