/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2016 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <3d_viewer/eda_3d_viewer.h>
#include <bitmaps.h>
#include <board_commit.h>
#include <class_board.h>
#include <class_module.h>
#include <confirm.h>
#include <dialog_helpers.h>
#include <eda_pattern_match.h>
#include <fctsys.h>
#include <footprint_info.h>
#include <footprint_viewer_frame.h>
#include <fp_lib_table.h>
#include <kiway.h>
#include <lib_id.h>
#include <memory>
#include <msgpanel.h>
#include <pcb_draw_panel_gal.h>
#include <pcb_painter.h>
#include <pcbnew.h>
#include <pcbnew_id.h>
#include <pcbnew_settings.h>
#include <footprint_editor_settings.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <tool/action_toolbar.h>
#include <tool/common_control.h>
#include <tool/common_tools.h>
#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>
#include <tool/zoom_tool.h>
#include <tools/pcb_viewer_tools.h>
#include <tools/pcb_actions.h>
#include <tools/pcbnew_control.h>
#include <tools/pcbnew_picker_tool.h>
#include <tools/selection_tool.h>
#include <wildcards_and_files_ext.h>
#include <wx/tokenzr.h>

using namespace std::placeholders;


#define NEXT_PART       1
#define NEW_PART        0
#define PREVIOUS_PART   -1


BEGIN_EVENT_TABLE( FOOTPRINT_VIEWER_FRAME, EDA_DRAW_FRAME )
    // Window events
    EVT_CLOSE( FOOTPRINT_VIEWER_FRAME::OnCloseWindow )
    EVT_SIZE( FOOTPRINT_VIEWER_FRAME::OnSize )
    EVT_ACTIVATE( FOOTPRINT_VIEWER_FRAME::OnActivate )

    EVT_MENU( wxID_EXIT, FOOTPRINT_VIEWER_FRAME::OnExitKiCad )
    EVT_MENU( wxID_CLOSE, FOOTPRINT_VIEWER_FRAME::CloseFootprintViewer )

    // Toolbar events
    EVT_TOOL( ID_MODVIEW_NEXT, FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList )
    EVT_TOOL( ID_MODVIEW_PREVIOUS, FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList )
    EVT_TOOL( ID_ADD_FOOTPRINT_TO_BOARD, FOOTPRINT_VIEWER_FRAME::AddFootprintToPCB )
    EVT_CHOICE( ID_ON_ZOOM_SELECT, FOOTPRINT_VIEWER_FRAME::OnSelectZoom )
    EVT_CHOICE( ID_ON_GRID_SELECT, FOOTPRINT_VIEWER_FRAME::OnSelectGrid )

    EVT_UPDATE_UI( ID_ON_GRID_SELECT, FOOTPRINT_VIEWER_FRAME::OnUpdateSelectGrid )
    EVT_UPDATE_UI( ID_ON_ZOOM_SELECT, FOOTPRINT_VIEWER_FRAME::OnUpdateSelectZoom )
    EVT_UPDATE_UI( ID_ADD_FOOTPRINT_TO_BOARD, FOOTPRINT_VIEWER_FRAME::OnUpdateFootprintButton )

    EVT_TEXT( ID_MODVIEW_LIB_FILTER, FOOTPRINT_VIEWER_FRAME::OnLibFilter )
    EVT_TEXT( ID_MODVIEW_FOOTPRINT_FILTER, FOOTPRINT_VIEWER_FRAME::OnFPFilter )

    // listbox events
    EVT_LISTBOX( ID_MODVIEW_LIB_LIST, FOOTPRINT_VIEWER_FRAME::ClickOnLibList )
    EVT_LISTBOX( ID_MODVIEW_FOOTPRINT_LIST, FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList )
    EVT_LISTBOX_DCLICK( ID_MODVIEW_FOOTPRINT_LIST, FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList )

END_EVENT_TABLE()


/* Note:
 * FOOTPRINT_VIEWER_FRAME can be created in "modal mode", or as a usual frame.
 * In modal mode:
 *  a tool to export the selected footprint is shown in the toolbar
 *  the style is wxFRAME_FLOAT_ON_PARENT
 */

#define PARENT_STYLE   ( KICAD_DEFAULT_DRAWFRAME_STYLE | wxFRAME_FLOAT_ON_PARENT )
#define MODAL_STYLE    ( KICAD_DEFAULT_DRAWFRAME_STYLE | wxSTAY_ON_TOP )
#define NONMODAL_STYLE ( KICAD_DEFAULT_DRAWFRAME_STYLE )


FOOTPRINT_VIEWER_FRAME::FOOTPRINT_VIEWER_FRAME( KIWAY* aKiway, wxWindow* aParent,
                                                FRAME_T aFrameType ) :
    PCB_BASE_FRAME( aKiway, aParent, aFrameType, _( "Footprint Library Browser" ),
            wxDefaultPosition, wxDefaultSize,
            aFrameType == FRAME_FOOTPRINT_VIEWER_MODAL ? ( aParent ? PARENT_STYLE : MODAL_STYLE )
                                                       : NONMODAL_STYLE,
            aFrameType == FRAME_FOOTPRINT_VIEWER_MODAL ? FOOTPRINT_VIEWER_FRAME_NAME_MODAL
                                                       : FOOTPRINT_VIEWER_FRAME_NAME )
{
    wxASSERT( aFrameType == FRAME_FOOTPRINT_VIEWER_MODAL || aFrameType == FRAME_FOOTPRINT_VIEWER );

    if( aFrameType == FRAME_FOOTPRINT_VIEWER_MODAL )
        SetModal( true );

    m_AboutTitle = "Footprint Library Viewer";

    // Force the items to always snap
    m_magneticItems.pads     = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    m_magneticItems.tracks   = MAGNETIC_OPTIONS::CAPTURE_ALWAYS;
    m_magneticItems.graphics = true;

    // Force the frame name used in config. the footprint viewer frame has a name
    // depending on aFrameType (needed to identify the frame by wxWidgets),
    // but only one configuration is preferable.
    m_configName = FOOTPRINT_VIEWER_FRAME_NAME;

    // Give an icon
    wxIcon  icon;
    icon.CopyFromBitmap( KiBitmap( modview_icon_xpm ) );
    SetIcon( icon );

    wxPanel* libPanel = new wxPanel( this );
    wxSizer* libSizer = new wxBoxSizer( wxVERTICAL );

    m_libFilter = new wxTextCtrl( libPanel, ID_MODVIEW_LIB_FILTER, wxEmptyString,
                                  wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    m_libFilter->SetHint( _( "Filter" ) );
    libSizer->Add( m_libFilter, 0, wxEXPAND, 5 );

    m_libList = new wxListBox( libPanel, ID_MODVIEW_LIB_LIST, wxDefaultPosition, wxDefaultSize,
                               0, NULL, wxLB_HSCROLL | wxNO_BORDER );
    libSizer->Add( m_libList, 1, wxEXPAND, 5 );

    libPanel->SetSizer( libSizer );
    libPanel->Fit();

    wxPanel* fpPanel = new wxPanel( this );
    wxSizer* fpSizer = new wxBoxSizer( wxVERTICAL );

    m_fpFilter = new wxTextCtrl( fpPanel, ID_MODVIEW_FOOTPRINT_FILTER, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
    m_fpFilter->SetHint( _( "Filter" ) );
    m_fpFilter->SetToolTip(
            _( "Filter on footprint name, keywords, description and pad count.\n"
               "Search terms are separated by spaces.  All search terms must match.\n"
               "A term which is a number will also match against the pad count." ) );
    fpSizer->Add( m_fpFilter, 0, wxEXPAND, 5 );

    m_fpList = new wxListBox( fpPanel, ID_MODVIEW_FOOTPRINT_LIST, wxDefaultPosition, wxDefaultSize,
                              0, NULL, wxLB_HSCROLL | wxNO_BORDER );
    fpSizer->Add( m_fpList, 1, wxEXPAND, 5 );

    fpPanel->SetSizer( fpSizer );
    fpPanel->Fit();

    SetBoard( new BOARD() );
    // In viewer, the default net clearance is not known (it depends on the actual board).
    // So we do not show the default clearance, by setting it to 0
    // The footprint or pad specific clearance will be shown
    GetBoard()->GetDesignSettings().GetDefault()->SetClearance( 0 );

    // Don't show the default board solder mask clearance in the footprint viewer.  Only the
    // footprint or pad clearance setting should be shown if it is not 0.
    GetBoard()->GetDesignSettings().m_SolderMaskMargin = 0;

    // Ensure all layers and items are visible:
    GetBoard()->SetVisibleAlls();
    SetScreen( new PCB_SCREEN( GetPageSizeIU() ) );

    GetScreen()->m_Center = true;      // Center coordinate origins on screen.
    LoadSettings( config() );
    GetGalDisplayOptions().m_axesEnabled = true;

    // Create GAL canvas
    m_canvasType = LoadCanvasTypeSetting();
    auto drawPanel = new PCB_DRAW_PANEL_GAL( this, -1, wxPoint( 0, 0 ), m_FrameSize,
                                             GetGalDisplayOptions(), m_canvasType );
    SetCanvas( drawPanel );

    // Create the manager and dispatcher & route draw panel events to the dispatcher
    m_toolManager = new TOOL_MANAGER;
    m_toolManager->SetEnvironment( GetBoard(), drawPanel->GetView(),
                                   drawPanel->GetViewControls(), config(), this );
    m_actions = new PCB_ACTIONS();
    m_toolDispatcher = new TOOL_DISPATCHER( m_toolManager, m_actions );
    drawPanel->SetEventDispatcher( m_toolDispatcher );

    m_toolManager->RegisterTool( new PCBNEW_CONTROL );
    m_toolManager->RegisterTool( new SELECTION_TOOL );
    m_toolManager->RegisterTool( new COMMON_TOOLS );    // for std context menus (zoom & grid)
    m_toolManager->RegisterTool( new COMMON_CONTROL );
    m_toolManager->RegisterTool( new PCBNEW_PICKER_TOOL ); // for setting grid origin
    m_toolManager->RegisterTool( new ZOOM_TOOL );
    m_toolManager->RegisterTool( new PCB_VIEWER_TOOLS );

    m_toolManager->GetTool<PCB_VIEWER_TOOLS>()->SetFootprintFrame( true );

    m_toolManager->InitTools();
    m_toolManager->InvokeTool( "pcbnew.InteractiveSelection" );

    ReCreateMenuBar();
    ReCreateHToolbar();
    ReCreateVToolbar();
    ReCreateOptToolbar();

    ReCreateLibraryList();
    UpdateTitle();

    // If a footprint was previously loaded, reload it
    if( getCurNickname().size() && getCurFootprintName().size() )
    {
        LIB_ID id;

        id.SetLibNickname( getCurNickname() );
        id.SetLibItemName( getCurFootprintName() );
        GetBoard()->Add( loadFootprint( id ) );
    }

    drawPanel->DisplayBoard( m_Pcb );

    m_auimgr.SetManagedWindow( this );

    // Horizontal items; layers 4 - 6
    m_auimgr.AddPane( m_mainToolBar, EDA_PANE().VToolbar().Name( "MainToolbar" ).Top().Layer(6) );
    m_auimgr.AddPane( m_optionsToolBar, EDA_PANE().VToolbar().Name( "OptToolbar" ).Left().Layer(3) );
    m_auimgr.AddPane( m_messagePanel, EDA_PANE().Messages().Name( "MsgPanel" ).Bottom().Layer(6) );

    // Vertical items; layers 1 - 3
    m_auimgr.AddPane( libPanel, EDA_PANE().Palette().Name( "Libraries" ).Left().Layer(2)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 200, -1 ) );
    m_auimgr.AddPane( fpPanel, EDA_PANE().Palette().Name( "Footprints" ).Left().Layer( 1)
                      .CaptionVisible( false ).MinSize( 100, -1 ).BestSize( 300, -1 ) );

    m_auimgr.AddPane( GetCanvas(), EDA_PANE().Canvas().Name( "DrawFrame" ).Center() );

    // after changing something to the aui manager call Update() to reflect the changes
    m_auimgr.Update();

    // The canvas should not steal the focus from the list boxes
    GetCanvas()->SetCanFocus( false );
    GetCanvas()->GetGAL()->SetAxesEnabled( true );
    ActivateGalCanvas();

    // Restore last zoom.  (If auto-zooming we'll adjust when we load the footprint.)
    PCBNEW_SETTINGS* cfg = GetPcbNewSettings();
    wxASSERT( cfg );
    GetCanvas()->GetView()->SetScale( cfg->m_FootprintViewerZoom );

    updateView();
    InitExitKey();

    if( !IsModal() )        // For modal mode, calling ShowModal() will show this frame
    {
        ReCreateFootprintList();
        Raise();            // On some window managers, this is needed
        Show( true );
    }
}


FOOTPRINT_VIEWER_FRAME::~FOOTPRINT_VIEWER_FRAME()
{
    // Shutdown all running tools
    if( m_toolManager )
        m_toolManager->ShutdownAllTools();

    GetCanvas()->StopDrawing();
    GetCanvas()->GetView()->Clear();
    // Be sure any event cannot be fired after frame deletion:
    GetCanvas()->SetEvtHandlerEnabled( false );
}


void FOOTPRINT_VIEWER_FRAME::OnCloseWindow( wxCloseEvent& Event )
{
    // A workaround to avoid flicker, in modal mode when modview frame is destroyed,
    // when the aui toolbar is not docked (i.e. shown in a miniframe)
    // (useful on windows only)
    m_mainToolBar->SetFocus();

    GetCanvas()->StopDrawing();

    if( IsModal() )
    {
        // Only dismiss a modal frame once, so that the return values set by
        // the prior DismissModal() are not bashed for ShowModal().
        if( !IsDismissed() )
            DismissModal( false );

        // window to be destroyed by the caller of KIWAY_PLAYER::ShowModal()
    }
    else
        Destroy();
}


void FOOTPRINT_VIEWER_FRAME::OnSize( wxSizeEvent& SizeEv )
{
    if( m_auimgr.GetManagedWindow() )
        m_auimgr.Update();

    SizeEv.Skip();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateLibraryList()
{
    m_libList->Clear();

    std::vector<wxString> nicknames = Prj().PcbFootprintLibs()->GetLogicalLibs();
    std::set<wxString>    excludes;

    if( !m_libFilter->GetValue().IsEmpty() )
    {
        wxStringTokenizer tokenizer( m_libFilter->GetValue() );

        while( tokenizer.HasMoreTokens() )
        {
            const wxString       term = tokenizer.GetNextToken().Lower();
            EDA_COMBINED_MATCHER matcher( term );
            int                  matches, position;

            for( const wxString& nickname : nicknames )
            {
                if( !matcher.Find( nickname.Lower(), matches, position ) )
                    excludes.insert( nickname );
            }
        }
    }

    for( const wxString& nickname : nicknames )
    {
        if( !excludes.count( nickname ) )
            m_libList->Append( nickname );
    }

    // Search for a previous selection:
    int index =  m_libList->FindString( getCurNickname(), true );

    if( index == wxNOT_FOUND )
    {
        if( m_libList->GetCount() > 0 )
        {
            m_libList->SetSelection( 0 );
            wxCommandEvent dummy;
            ClickOnLibList( dummy );
        }
        else
        {
            setCurNickname( wxEmptyString );
            setCurFootprintName( wxEmptyString );
        }
    }
    else
    {
        m_libList->SetSelection( index, true );
        wxCommandEvent dummy;
        ClickOnLibList( dummy );
    }

    GetCanvas()->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::ReCreateFootprintList()
{
    m_fpList->Clear();

    if( !getCurNickname() )
        setCurFootprintName( wxEmptyString );

    auto fp_info_list = FOOTPRINT_LIST::GetInstance( Kiway() );

    wxString nickname = getCurNickname();

    fp_info_list->ReadFootprintFiles( Prj().PcbFootprintLibs(), !nickname ? NULL : &nickname );

    if( fp_info_list->GetErrorCount() )
    {
        fp_info_list->DisplayErrors( this );

        // For footprint libraries that support one footprint per file, there may have been
        // valid footprints read so show the footprints that loaded properly.
        if( fp_info_list->GetList().empty() )
            return;
    }

    std::set<wxString> excludes;

    if( !m_fpFilter->GetValue().IsEmpty() )
    {
        wxStringTokenizer tokenizer( m_fpFilter->GetValue() );

        while( tokenizer.HasMoreTokens() )
        {
            const wxString       term = tokenizer.GetNextToken().Lower();
            EDA_COMBINED_MATCHER matcher( term );
            int                  matches, position;

            for( const std::unique_ptr<FOOTPRINT_INFO>& footprint : fp_info_list->GetList() )
            {
                wxString search = footprint->GetFootprintName() + " " + footprint->GetSearchText();
                bool     matched = matcher.Find( search.Lower(), matches, position );

                if( !matched && term.IsNumber() )
                    matched = ( wxAtoi( term ) == (int)footprint->GetPadCount() );

                if( !matched )
                    excludes.insert( footprint->GetFootprintName() );
            }
        }
    }

    for( const std::unique_ptr<FOOTPRINT_INFO>& footprint : fp_info_list->GetList() )
    {
        if( !excludes.count( footprint->GetFootprintName() ) )
            m_fpList->Append( footprint->GetFootprintName() );
    }

    int index = m_fpList->FindString( getCurFootprintName(), true );

    if( index == wxNOT_FOUND )
    {
        if( m_fpList->GetCount() > 0 )
        {
            m_fpList->SetSelection( 0 );
            m_fpList->EnsureVisible( 0 );

            wxCommandEvent dummy;
            ClickOnFootprintList( dummy );
        }
        else
            setCurFootprintName( wxEmptyString );
    }
    else
    {
        m_fpList->SetSelection( index, true );
        m_fpList->EnsureVisible( index );
    }
}


void FOOTPRINT_VIEWER_FRAME::OnLibFilter( wxCommandEvent& aEvent )
{
    ReCreateLibraryList();

    // Required to avoid interaction with SetHint()
    // See documentation for wxTextEntry::SetHint
    aEvent.Skip();
}


void FOOTPRINT_VIEWER_FRAME::OnFPFilter( wxCommandEvent& aEvent )
{
    ReCreateFootprintList();

    // Required to avoid interaction with SetHint()
    // See documentation for wxTextEntry::SetHint
    aEvent.Skip();
}


void FOOTPRINT_VIEWER_FRAME::OnCharHook( wxKeyEvent& aEvent )
{
    if( aEvent.GetKeyCode() == WXK_UP )
    {
        wxWindow* focused = FindFocus();

        if( m_libFilter->HasFocus() || m_libList->HasFocus() )
            selectPrev( m_libList );
        else
            selectPrev( m_fpList );

        // Need to reset the focus after selection due to GTK mouse-refresh
        // that captures the mouse into the canvas to update scrollbars
        if( focused )
            focused->SetFocus();
    }
    else if( aEvent.GetKeyCode() == WXK_DOWN )
    {
        wxWindow* focused = FindFocus();

        if( m_libFilter->HasFocus() || m_libList->HasFocus() )
            selectNext( m_libList );
        else
            selectNext( m_fpList );

        // Need to reset the focus after selection due to GTK mouse-refresh
        // that captures the mouse into the canvas to update scrollbars
        if( focused )
            focused->SetFocus();
    }
    else if( aEvent.GetKeyCode() == WXK_TAB && m_libFilter->HasFocus() )
    {
        if( !aEvent.ShiftDown() )
            m_fpFilter->SetFocus();
        else
            aEvent.Skip();
    }
    else if( aEvent.GetKeyCode() == WXK_TAB && m_fpFilter->HasFocus() )
    {
        if( aEvent.ShiftDown() )
            m_libFilter->SetFocus();
        else
            aEvent.Skip();
    }
    else if( aEvent.GetKeyCode() == WXK_RETURN && m_fpList->GetSelection() >= 0 )
    {
        wxCommandEvent dummy;
        AddFootprintToPCB( dummy );
    }
    else
        aEvent.Skip();
}


void FOOTPRINT_VIEWER_FRAME::selectPrev( wxListBox* aListBox )
{
    int prev = aListBox->GetSelection() - 1;

    if( prev >= 0 )
    {
        aListBox->SetSelection( prev );
        aListBox->EnsureVisible( prev );

        wxCommandEvent dummy;

        if( aListBox == m_libList )
            ClickOnLibList( dummy );
        else
            ClickOnFootprintList( dummy );
    }
}


void FOOTPRINT_VIEWER_FRAME::selectNext( wxListBox* aListBox )
{
    int next = aListBox->GetSelection() + 1;

    if( next < (int)aListBox->GetCount() )
    {
        aListBox->SetSelection( next );
        aListBox->EnsureVisible( next );

        wxCommandEvent dummy;

        if( aListBox == m_libList )
            ClickOnLibList( dummy );
        else
            ClickOnFootprintList( dummy );
    }
}


void FOOTPRINT_VIEWER_FRAME::ClickOnLibList( wxCommandEvent& aEvent )
{
    int ii = m_libList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_libList->GetString( ii );

    if( getCurNickname() == name )
        return;

    setCurNickname( name );

    ReCreateFootprintList();
    UpdateTitle();

    // The m_libList has now the focus, in order to be able to use arrow keys
    // to navigate inside the list.
    // the gal canvas must not steal the focus to allow navigation
    GetCanvas()->SetStealsFocus( false );
    m_libList->SetFocus();
}


void FOOTPRINT_VIEWER_FRAME::ClickOnFootprintList( wxCommandEvent& aEvent )
{
    if( m_fpList->GetCount() == 0 )
        return;

    int ii = m_fpList->GetSelection();

    if( ii < 0 )
        return;

    wxString name = m_fpList->GetString( ii );

    if( getCurFootprintName().CmpNoCase( name ) != 0 )
    {
        setCurFootprintName( name );

        // Delete the current footprint (MUST reset tools first)
        GetToolManager()->ResetTools( TOOL_BASE::MODEL_RELOAD );

        GetBoard()->DeleteAllModules();

        LIB_ID id;
        id.SetLibNickname( getCurNickname() );
        id.SetLibItemName( getCurFootprintName() );

        try
        {
            GetBoard()->Add( loadFootprint( id ) );
        }
        catch( const IO_ERROR& ioe )
        {
            wxString msg = wxString::Format( _( "Could not load footprint '%s' from library '%s'."
                                                "\n\n%s" ),
                                             getCurFootprintName(),
                                             getCurNickname(),
                                             ioe.Problem() );
            DisplayError( this, msg );
        }

        UpdateTitle();

        updateView();

        GetCanvas()->Refresh();
        Update3DView( true );
    }

    // The m_fpList has now the focus, in order to be able to use arrow keys
    // to navigate inside the list.
    // the gal canvas must not steal the focus to allow navigation
    GetCanvas()->SetStealsFocus( false );
    m_fpList->SetFocus();
}


void FOOTPRINT_VIEWER_FRAME::DClickOnFootprintList( wxCommandEvent& aEvent )
{
    AddFootprintToPCB( aEvent );
}


void FOOTPRINT_VIEWER_FRAME::AddFootprintToPCB( wxCommandEvent& aEvent )
{
    if( IsModal() )
    {
        if( m_fpList->GetSelection() >= 0 )
        {
            LIB_ID fpid( getCurNickname(), m_fpList->GetStringSelection() );
            DismissModal( true, fpid.Format() );
        }
        else
        {
            DismissModal( false );
        }

        Close( true );
    }
    else if( GetBoard()->GetFirstModule() )
    {
        PCB_EDIT_FRAME* pcbframe = (PCB_EDIT_FRAME*) Kiway().Player( FRAME_PCB_EDITOR, false );

        if( pcbframe == NULL )      // happens when the board editor is not active (or closed)
        {
            DisplayErrorMessage( this, _( "No board currently open." ) );
            return;
        }

        pcbframe->GetToolManager()->RunAction( PCB_ACTIONS::selectionClear, true );
        BOARD_COMMIT commit( pcbframe );

        // Create the "new" module
        MODULE* newmodule = (MODULE*) GetBoard()->GetFirstModule()->Duplicate();
        newmodule->SetParent( pcbframe->GetBoard() );
        newmodule->SetLink( 0 );

        KIGFX::VIEW_CONTROLS* viewControls = pcbframe->GetCanvas()->GetViewControls();
        VECTOR2D              cursorPos = viewControls->GetCursorPosition();

        commit.Add( newmodule );
        viewControls->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
        pcbframe->PlaceModule( newmodule );
        newmodule->SetPosition( wxPoint( 0, 0 ) );
        viewControls->SetCrossHairCursorPosition( cursorPos, false );
        commit.Push( wxT( "Insert module" ) );

        pcbframe->Raise();
        pcbframe->GetToolManager()->RunAction( PCB_ACTIONS::placeModule, true, newmodule );

        newmodule->ClearFlags();
    }
}


void FOOTPRINT_VIEWER_FRAME::LoadSettings( APP_SETTINGS_BASE* aCfg )
{
    auto cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxCHECK( cfg, /*void*/ );

    // We don't allow people to change this right now, so make sure it's on
    GetWindowSettings( cfg )->cursor.always_show_cursor = true;

    PCB_BASE_FRAME::LoadSettings( aCfg );

    // Fetch grid settings from Footprint Editor
    auto fpedit = Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();

    GetGalDisplayOptions().ReadWindowSettings( fpedit->m_Window );
}


void FOOTPRINT_VIEWER_FRAME::SaveSettings( APP_SETTINGS_BASE* aCfg )
{
    auto cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxCHECK( cfg, /*void*/ );

    // We don't want to store anything other than the window settings
    PCB_BASE_FRAME::SaveSettings( cfg );

    cfg->m_FootprintViewerZoom = GetCanvas()->GetView()->GetScale();
}


WINDOW_SETTINGS* FOOTPRINT_VIEWER_FRAME::GetWindowSettings( APP_SETTINGS_BASE* aCfg )
{
    auto cfg = dynamic_cast<PCBNEW_SETTINGS*>( aCfg );
    wxCHECK_MSG( cfg, nullptr, "config not existing" );

    return &cfg->m_FootprintViewer;
}


COLOR_SETTINGS* FOOTPRINT_VIEWER_FRAME::GetColorSettings()
{
    auto* settings = Pgm().GetSettingsManager().GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>();

    if( settings )
        return Pgm().GetSettingsManager().GetColorSettings( settings->m_ColorTheme );
    else
        return Pgm().GetSettingsManager().GetColorSettings();
}


bool FOOTPRINT_VIEWER_FRAME::GetAutoZoom()
{
    // It is stored in pcbnew's settings
    PCBNEW_SETTINGS* cfg = GetPcbNewSettings();
    wxCHECK( cfg, false );
    return cfg->m_FootprintViewerAutoZoom;
}


void FOOTPRINT_VIEWER_FRAME::SetAutoZoom( bool aAutoZoom )
{
    // It is stored in pcbnew's settings
    PCBNEW_SETTINGS* cfg = GetPcbNewSettings();
    wxASSERT( cfg );
    cfg->m_FootprintViewerAutoZoom = aAutoZoom;
}


void FOOTPRINT_VIEWER_FRAME::CommonSettingsChanged( bool aEnvVarsChanged )
{
    PCB_BASE_FRAME::CommonSettingsChanged( aEnvVarsChanged );

    if( aEnvVarsChanged )
        ReCreateLibraryList();
}


const wxString FOOTPRINT_VIEWER_FRAME::getCurNickname()
{
    return Prj().GetRString( PROJECT::PCB_FOOTPRINT_VIEWER_NICKNAME );
}


void FOOTPRINT_VIEWER_FRAME::setCurNickname( const wxString& aNickname )
{
    Prj().SetRString( PROJECT::PCB_FOOTPRINT_VIEWER_NICKNAME, aNickname );
}


const wxString FOOTPRINT_VIEWER_FRAME::getCurFootprintName()
{
    return Prj().GetRString( PROJECT::PCB_FOOTPRINT_VIEWER_FPNAME );
}


void FOOTPRINT_VIEWER_FRAME::setCurFootprintName( const wxString& aName )
{
    Prj().SetRString( PROJECT::PCB_FOOTPRINT_VIEWER_FPNAME, aName );
}


void FOOTPRINT_VIEWER_FRAME::OnActivate( wxActivateEvent& event )
{
    if( event.GetActive() )
    {
        // Ensure we have the right library list:
        std::vector< wxString > libNicknames = Prj().PcbFootprintLibs()->GetLogicalLibs();
        bool                    stale = false;

        if( libNicknames.size() != m_libList->GetCount() )
            stale = true;
        else
        {
            for( unsigned ii = 0;  ii < libNicknames.size();  ii++ )
            {
                if( libNicknames[ii] != m_libList->GetString( ii ) )
                {
                    stale = true;
                    break;
                }
            }
        }

        if( stale )
        {
            ReCreateLibraryList();
            UpdateTitle();
        }
    }

    event.Skip();    // required under wxMAC
}


void FOOTPRINT_VIEWER_FRAME::OnUpdateFootprintButton( wxUpdateUIEvent& aEvent )
{
    aEvent.Enable( GetBoard()->GetFirstModule() != nullptr );
}


bool FOOTPRINT_VIEWER_FRAME::ShowModal( wxString* aFootprint, wxWindow* aParent )
{
    if( aFootprint && !aFootprint->IsEmpty() )
    {
        wxString msg;
        LIB_TABLE* fpTable = Prj().PcbFootprintLibs();
        LIB_ID fpid;

        fpid.Parse( *aFootprint, LIB_ID::ID_PCB, true );

        if( fpid.IsValid() )
        {
            wxString nickname = fpid.GetLibNickname();

            if( !fpTable->HasLibrary( fpid.GetLibNickname(), false ) )
            {
                msg.sprintf( _( "The current configuration does not include a library with the\n"
                                "nickname \"%s\".  Use Manage Footprint Libraries\n"
                                "to edit the configuration." ), nickname );
                DisplayErrorMessage( aParent, _( "Footprint library not found." ), msg );
            }
            else if ( !fpTable->HasLibrary( fpid.GetLibNickname(), true ) )
            {
                msg.sprintf( _( "The library with the nickname \"%s\" is not enabled\n"
                                "in the current configuration.  Use Manage Footprint Libraries to\n"
                                "edit the configuration." ), nickname );
                DisplayErrorMessage( aParent, _( "Footprint library not enabled." ), msg );
            }
            else
            {
                // Update last selection:
                setCurNickname( nickname );
                setCurFootprintName( fpid.GetLibItemName() );
                m_libList->SetStringSelection( nickname );
            }
        }
    }

    // Rebuild the fp list from the last selected library,
    // and show the last selected footprint
    ReCreateFootprintList();
    SelectAndViewFootprint( NEW_PART );

    bool retval = KIWAY_PLAYER::ShowModal( aFootprint, aParent );

    m_libFilter->SetFocus();
    return retval;
}


void FOOTPRINT_VIEWER_FRAME::Update3DView( bool aForceReload, const wxString* aTitle )
{
    wxString title = wxString::Format( _( "3D Viewer" ) + wxT( " \u2014 %s" ),
                                       getCurFootprintName() );
    PCB_BASE_FRAME::Update3DView( aForceReload, &title );
}


COLOR4D FOOTPRINT_VIEWER_FRAME::GetGridColor()
{
    return GetColorSettings()->GetColor( LAYER_GRID );
}


void FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList( wxCommandEvent& event )
{
    switch( event.GetId() )
    {
    case ID_MODVIEW_NEXT:
        SelectAndViewFootprint( NEXT_PART );
        break;

    case ID_MODVIEW_PREVIOUS:
        SelectAndViewFootprint( PREVIOUS_PART );
        break;

    default:
        wxString id = wxString::Format( "%i", event.GetId() );
        wxFAIL_MSG( "FOOTPRINT_VIEWER_FRAME::OnIterateFootprintList error: id = " + id );
    }
}


void FOOTPRINT_VIEWER_FRAME::UpdateTitle()
{
    wxString title;
    wxString path;

    title.Printf( _( "Footprint Library Browser" ) + L" \u2014 %s",
                  getCurNickname().IsEmpty() ? _( "no library selected" ) : getCurNickname() );

    // Now, add the full path, for info
    if( !getCurNickname().IsEmpty() )
    {
        FP_LIB_TABLE* libtable = Prj().PcbFootprintLibs();
        const LIB_TABLE_ROW* row = libtable->FindRow( getCurNickname() );

        if( row )
            title << L" \u2014 " << row->GetFullURI( true );
    }

    SetTitle( title );
}


void FOOTPRINT_VIEWER_FRAME::SelectAndViewFootprint( int aMode )
{
    if( !getCurNickname() )
        return;

    int selection = m_fpList->FindString( getCurFootprintName(), true );

    if( aMode == NEXT_PART )
    {
        if( selection != wxNOT_FOUND && selection < (int)m_fpList->GetCount() - 1 )
            selection++;
    }

    if( aMode == PREVIOUS_PART )
    {
        if( selection != wxNOT_FOUND && selection > 0 )
            selection--;
    }

    if( selection != wxNOT_FOUND )
    {
        m_fpList->SetSelection( selection );
        m_fpList->EnsureVisible( selection );

        setCurFootprintName( m_fpList->GetString((unsigned) selection ) );

        // Delete the current footprint
        GetBoard()->DeleteAllModules();

        MODULE* footprint = Prj().PcbFootprintLibs()->FootprintLoad( getCurNickname(),
                                                                     getCurFootprintName() );

        if( footprint )
            GetBoard()->Add( footprint, ADD_MODE::APPEND );

        Update3DView( true );

        updateView();
    }

    UpdateTitle();

    GetCanvas()->Refresh();
}


void FOOTPRINT_VIEWER_FRAME::updateView()
{
    GetCanvas()->UpdateColors();
    GetCanvas()->DisplayBoard( GetBoard() );

    m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    if( GetAutoZoom() )
        m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    else
        m_toolManager->RunAction( ACTIONS::centerContents, true );

    UpdateMsgPanel();
}


void FOOTPRINT_VIEWER_FRAME::OnExitKiCad( wxCommandEvent& event )
{
    Kiway().OnKiCadExit();
}


void FOOTPRINT_VIEWER_FRAME::CloseFootprintViewer( wxCommandEvent& event )
{
    Close( false );
}


BOARD_ITEM_CONTAINER* FOOTPRINT_VIEWER_FRAME::GetModel() const
{
    return GetBoard()->GetFirstModule();
}

