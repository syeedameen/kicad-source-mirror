/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2010-2011 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file hotkeys_basic.cpp
 * @brief Some functions to handle hotkeys in KiCad
 */

#include <fctsys.h>
#include <kiface_i.h>
#include <hotkeys_basic.h>
#include <id.h>
#include <confirm.h>
#include <kicad_string.h>
#include <gestfich.h>
#include <eda_base_frame.h>
#include <macros.h>
#include <menus_helpers.h>
#include <eda_draw_frame.h>

#include <tool/tool_manager.h>
#include "dialogs/dialog_hotkey_list.h"
#include <wx/apptrait.h>
#include <wx/stdpaths.h>
#include <wx/tokenzr.h>
#include <tool/tool_action.h>


wxString g_CommonSectionTag( wxT( "[common]" ) );


/* Class to handle hotkey commands hotkeys have a default value
 * This class allows the real key code changed by user from a key code list
 * file.
 */

EDA_HOTKEY::EDA_HOTKEY( const wxChar* infomsg, int idcommand, int keycode, int idmenuevent ) :
                        m_KeyCode( keycode ), m_InfoMsg( infomsg ), m_Idcommand( idcommand ), 
                        m_IdMenuEvent( idmenuevent )
{
}


/* class to handle the printable name and the keycode
 */
struct hotkey_name_descr
{
    const wxChar* m_Name;
    int           m_KeyCode;
};

/* table giving the hotkey name from the hotkey code, for special keys
 * Note : when modifiers (ATL, SHIFT, CTRL) do not modify
 * the code of the key, do need to enter the modified key code
 * For instance wxT( "F1" ), WXK_F1 handle F1, AltF1, CtrlF1 ...
 * Key names are:
 *        "Space","Ctrl+Space","Alt+Space" or
 *      "Alt+A","Ctrl+F1", ...
 */
#define KEY_NON_FOUND -1
static struct hotkey_name_descr hotkeyNameList[] =
{
    { wxT( "F1" ),           WXK_F1                                                   },
    { wxT( "F2" ),           WXK_F2                                                   },
    { wxT( "F3" ),           WXK_F3                                                   },
    { wxT( "F4" ),           WXK_F4                                                   },
    { wxT( "F5" ),           WXK_F5                                                   },
    { wxT( "F6" ),           WXK_F6                                                   },
    { wxT( "F7" ),           WXK_F7                                                   },
    { wxT( "F8" ),           WXK_F8                                                   },
    { wxT( "F9" ),           WXK_F9                                                   },
    { wxT( "F10" ),          WXK_F10                                                  },
    { wxT( "F11" ),          WXK_F11                                                  },
    { wxT( "F12" ),          WXK_F12                                                  },

    { wxT( "Esc" ),          WXK_ESCAPE                                               },
    { wxT( "Del" ),          WXK_DELETE                                               },
    { wxT( "Tab" ),          WXK_TAB                                                  },
    { wxT( "Back" ),         WXK_BACK                                                 },
    { wxT( "Ins" ),          WXK_INSERT                                               },

    { wxT( "Home" ),         WXK_HOME                                                 },
    { wxT( "End" ),          WXK_END                                                  },
    { wxT( "PgUp" ),         WXK_PAGEUP                                               },
    { wxT( "PgDn" ),         WXK_PAGEDOWN                                             },

    { wxT( "Up" ),           WXK_UP                                                   },
    { wxT( "Down" ),         WXK_DOWN                                                 },
    { wxT( "Left" ),         WXK_LEFT                                                 },
    { wxT( "Right" ),        WXK_RIGHT                                                },

    { wxT( "Return" ),       WXK_RETURN                                               },

    { wxT( "Space" ),        WXK_SPACE                                                },

    { wxT( "<unassigned>" ), 0                                                        },

    // Do not change this line: end of list
    { wxT( "" ),             KEY_NON_FOUND                                            }
};

// name of modifier keys.
// Note: the Ctrl key is Cmd key on Mac OS X.
// However, in wxWidgets defs, the key WXK_CONTROL is the Cmd key,
// so the code using WXK_CONTROL should be ok on any system.
// (on Mac OS X the actual Ctrl key code is WXK_RAW_CONTROL)
#ifdef __WXMAC__
#define USING_MAC_CMD
#endif

#ifdef USING_MAC_CMD
#define MODIFIER_CTRL       wxT( "Cmd+" )
#else
#define MODIFIER_CTRL       wxT( "Ctrl+" )
#endif
#define MODIFIER_CMD_MAC    wxT( "Cmd+" )
#define MODIFIER_CTRL_BASE  wxT( "Ctrl+" )
#define MODIFIER_ALT        wxT( "Alt+" )
#define MODIFIER_SHIFT      wxT( "Shift+" )


/**
 * Function KeyNameFromKeyCode
 * return the key name from the key code
 * Only some wxWidgets key values are handled for function key ( see
 * hotkeyNameList[] )
 * @param aKeycode = key code (ascii value, or wxWidgets value for function keys)
 * @param aIsFound = a pointer to a bool to return true if found, or false. an be nullptr default)
 * @return the key name in a wxString
 */
wxString KeyNameFromKeyCode( int aKeycode, bool* aIsFound )
{
    wxString keyname, modifier, fullkeyname;
    int      ii;
    bool     found = false;

    // Assume keycode of 0 is "unassigned"
    if( (aKeycode & MD_CTRL) != 0 )
        modifier << MODIFIER_CTRL;

    if( (aKeycode & MD_ALT) != 0 )
        modifier << MODIFIER_ALT;

    if( (aKeycode & MD_SHIFT) != 0 )
        modifier << MODIFIER_SHIFT;

    aKeycode &= ~( MD_CTRL | MD_ALT | MD_SHIFT );

    if( (aKeycode > ' ') && (aKeycode < 0x7F ) )
    {
        found   = true;
        keyname.Append( (wxChar)aKeycode );
    }
    else
    {
        for( ii = 0; ; ii++ )
        {
            if( hotkeyNameList[ii].m_KeyCode == KEY_NON_FOUND ) // End of list
            {
                keyname = wxT( "<unknown>" );
                break;
            }

            if( hotkeyNameList[ii].m_KeyCode == aKeycode )
            {
                keyname = hotkeyNameList[ii].m_Name;
                found   = true;
                break;
            }
        }
    }

    if( aIsFound )
        *aIsFound = found;

    fullkeyname = modifier + keyname;
    return fullkeyname;
}


/**
 * AddHotkeyName
 * @param aText - the base text on which to append the hotkey
 * @param aHotKey - the hotkey keycode
 * @param aStyle - IS_HOTKEY to add <tab><keyname> (shortcuts in menus, same as hotkeys)
 *                 IS_COMMENT to add <spaces><(keyname)> mainly in tool tips
 */
wxString AddHotkeyName( const wxString& aText, int aHotKey, HOTKEY_ACTION_TYPE aStyle )
{
    wxString msg = aText;
    wxString keyname = KeyNameFromKeyCode( aHotKey );

    if( !keyname.IsEmpty() )
    {
        switch( aStyle )
        {
        case IS_HOTKEY:
            msg << wxT( "\t" ) << keyname;
            break;

        case IS_COMMENT:
            msg << wxT( " (" ) << keyname << wxT( ")" );
            break;
        }
    }

#ifdef USING_MAC_CMD
    // On OSX, the modifier equivalent to the Ctrl key of PCs
    // is the Cmd key, but in code we should use Ctrl as prefix in menus
    msg.Replace( MODIFIER_CMD_MAC, MODIFIER_CTRL_BASE );
#endif

    return msg;
}


/* AddHotkeyName
 * Add the key name from the Command id value ( m_Idcommand member value)
 *  aText = a wxString. returns aText + key name
 *  aList = pointer to a EDA_HOTKEY_CONFIG DescrList of commands
 *  aCommandId = Command Id value
 *  aShortCutType = IS_HOTKEY to add <tab><keyname> (active shortcuts in menus)
 *                  IS_ACCELERATOR to add <tab><Shift+keyname> (active accelerators in menus)
 *                  IS_COMMENT to add <spaces><(keyname)>
 * Return a wxString (aText + key name) if key found or aText without modification
 */
wxString AddHotkeyName( const wxString&           aText,
                        struct EDA_HOTKEY_CONFIG* aDescList,
                        int                       aCommandId,
                        HOTKEY_ACTION_TYPE        aShortCutType )
{
    // JEY TODO: obsolete once 3DViewer and ProjectManager are moved over...
    wxString     msg = aText;
    wxString     keyname;
    EDA_HOTKEY** list;

    if( aDescList )
    {
        for( ; aDescList->m_HK_InfoList != nullptr; aDescList++ )
        {
            list    = aDescList->m_HK_InfoList;
            keyname = KeyNameFromCommandId( list, aCommandId );

            if( !keyname.IsEmpty() )
            {
                switch( aShortCutType )
                {
                case IS_HOTKEY:
                    msg << wxT( "\t" ) << keyname;
                    break;

                case IS_COMMENT:
                    msg << wxT( " (" ) << keyname << wxT( ")" );
                    break;
                }

                break;
            }
        }
    }

#ifdef USING_MAC_CMD
    // On OSX, the modifier equivalent to the Ctrl key of PCs
    // is the Cmd key, but in code we should use Ctrl as prefix in menus
    msg.Replace( MODIFIER_CMD_MAC, MODIFIER_CTRL_BASE );
#endif

    return msg;
}


/**
 * Function KeyNameFromCommandId
 * return the key name from the Command id value ( m_Idcommand member value)
 * @param aList = pointer to a EDA_HOTKEY list of commands
 * @param aCommandId = Command Id value
 * @return the key name in a wxString
 */
wxString KeyNameFromCommandId( EDA_HOTKEY** aList, int aCommandId )
{
    // JEY TODO: obsolete once 3DViewer and ProjectManager are moved over...
    wxString keyname;

    for( ; *aList != nullptr; aList++ )
    {
        EDA_HOTKEY* hk_decr = *aList;

        if( hk_decr->m_Idcommand == aCommandId )
        {
            keyname = KeyNameFromKeyCode( hk_decr->m_KeyCode );
            break;
        }
    }

    return keyname;
}


/**
 * Function KeyCodeFromKeyName
 * return the key code from its key name
 * Only some wxWidgets key values are handled for function key
 * @param keyname = wxString key name to find in hotkeyNameList[],
 *   like F2 or space or an usual (ascii) char.
 * @return the key code
 */
int KeyCodeFromKeyName( const wxString& keyname )
{
    int ii, keycode = KEY_NON_FOUND;

    // Search for modifiers: Ctrl+ Alt+ and Shift+
    // Note: on Mac OSX, the Cmd key is equiv here to Ctrl
    wxString key = keyname;
    wxString prefix;
    int modifier = 0;

    while( true )
    {
        prefix.Empty();

        if( key.StartsWith( MODIFIER_CTRL_BASE ) )
        {
            modifier |= MD_CTRL;
            prefix = MODIFIER_CTRL_BASE;
        }
        else if( key.StartsWith( MODIFIER_CMD_MAC ) )
        {
            modifier |= MD_CTRL;
            prefix = MODIFIER_CMD_MAC;
        }
        else if( key.StartsWith( MODIFIER_ALT ) )
        {
            modifier |= MD_ALT;
            prefix = MODIFIER_ALT;
        }
        else if( key.StartsWith( MODIFIER_SHIFT ) )
        {
            modifier |= MD_SHIFT;
            prefix = MODIFIER_SHIFT;
        }
        else
        {
            break;
        }

        if( !prefix.IsEmpty() )
            key.Remove( 0, prefix.Len() );
    }

    if( (key.length() == 1) && (key[0] > ' ') && (key[0] < 0x7F) )
    {
        keycode = key[0];
        keycode += modifier;
        return keycode;
    }

    for( ii = 0; hotkeyNameList[ii].m_KeyCode != KEY_NON_FOUND; ii++ )
    {
        if( key.CmpNoCase( hotkeyNameList[ii].m_Name ) == 0 )
        {
            keycode = hotkeyNameList[ii].m_KeyCode + modifier;
            break;
        }
    }

    return keycode;
}


/* 
 * DisplayHotkeyList
 * Displays the hotkeys registered with the given tool manager.
 */
void DisplayHotkeyList( EDA_BASE_FRAME* aParent, TOOL_MANAGER* aToolManager )
{
    DIALOG_LIST_HOTKEYS dlg( aParent, aToolManager );
    dlg.ShowModal();
}


int WriteHotKeyConfig( std::map<std::string, TOOL_ACTION*> aActionMap )
{
    wxFileName fn( "user" );

    fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );
    fn.SetPath( GetKicadConfigPath() );

    if( !wxFile::Exists( fn.GetFullPath() ) )
        return 0;

    wxFile file( fn.GetFullPath(), wxFile::OpenMode::read );

    if( !file.IsOpened() )       // There is a problem to open file
        return 0;

    // Read entire hotkey set into map
    //
    wxString                input;
    std::map<wxString, int> hotkeys;

    file.ReadAll( &input );
    input.Replace( "\r\n", "\n" );  // Convert Windows files to Unix line-ends
    wxStringTokenizer fileTokenizer( input, "\n", wxTOKEN_STRTOK );

    while( fileTokenizer.HasMoreTokens() )
    {
        wxStringTokenizer lineTokenizer( fileTokenizer.GetNextToken(), "\t" );

        wxString cmdName = lineTokenizer.GetNextToken();
        wxString keyName = lineTokenizer.GetNextToken();

        if( !cmdName.IsEmpty() )
            hotkeys[ cmdName ] = KeyCodeFromKeyName( keyName );
    }

    file.Close();
   
    // Overlay this app's hotkey definitions onto the map
    //
    for( const auto& ii : aActionMap )
    {
        if( ii.second->GetHotKey() )
            hotkeys[ ii.first ] = ii.second->GetHotKey();
    }
    
    // Write entire hotkey set
    //
    file.Open( fn.GetFullPath(), wxFile::OpenMode::write );
    
    for( const auto& ii : hotkeys )
        file.Write( wxString::Format( "%s\t%s\n", ii.first, KeyNameFromKeyCode( ii.second ) ) );

    file.Close();

    return 1;
}


int ReadLegacyHotkeyConfig( const wxString& aAppname, std::map<std::string, int>& aMap )
{
    // For Eeschema and Pcbnew frames, we read the new combined file.
    // For other kifaces, we read the frame-based file
    if( aAppname == LIB_EDIT_FRAME_NAME || aAppname == SCH_EDIT_FRAME_NAME )
    {
        return ReadLegacyHotkeyConfigFile( EESCHEMA_HOTKEY_NAME, aMap );
    }
    else if( aAppname == PCB_EDIT_FRAME_NAME || aAppname == FOOTPRINT_EDIT_FRAME_NAME )
    {
        return ReadLegacyHotkeyConfigFile( PCBNEW_HOTKEY_NAME, aMap );
    }

    return ReadLegacyHotkeyConfigFile( aAppname, aMap );
}


int ReadLegacyHotkeyConfigFile( const wxString& aFilename, std::map<std::string, int>& aMap )
{
    wxFileName fn( aFilename );

    fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );
    fn.SetPath( GetKicadConfigPath() );

    if( !wxFile::Exists( fn.GetFullPath() ) )
        return 0;

    wxFile cfgfile( fn.GetFullPath() );
    
    if( !cfgfile.IsOpened() )       // There is a problem to open file
        return 0;

    // get length
    cfgfile.SeekEnd();
    wxFileOffset size = cfgfile.Tell();
    cfgfile.Seek( 0 );

    // read data
    std::vector<char> buffer( size );
    cfgfile.Read( buffer.data(), size );
    wxString data( buffer.data(), wxConvUTF8, size );

    // Is this the wxConfig format? If so, remove "Keys=" and parse the newlines.
    if( data.StartsWith( wxT("Keys="), &data ) )
        data.Replace( "\\n", "\n", true );

    // parse
    wxStringTokenizer tokenizer( data, L"\r\n", wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
    {
        wxString          line = tokenizer.GetNextToken();
        wxStringTokenizer lineTokenizer( line );

        wxString          line_type = lineTokenizer.GetNextToken();

        if( line_type[0]  == '#' ) // comment
            continue;

        if( line_type[0]  == '[' ) // tags ignored reading legacy hotkeys
            continue;

        if( line_type == wxT( "$Endlist" ) )
            break;

        if( line_type != wxT( "shortcut" ) )
            continue;

        // Get the key name
        lineTokenizer.SetString( lineTokenizer.GetString(), L"\"\r\n\t ", wxTOKEN_STRTOK );
        wxString keyname = lineTokenizer.GetNextToken();

        wxString remainder = lineTokenizer.GetString();

        // Get the command name
        wxString fctname = remainder.AfterFirst( '\"' ).BeforeFirst( '\"' );

        // Add the pair to the map
        aMap[ fctname.ToStdString() ] = KeyCodeFromKeyName( keyname );
    }

    // cleanup
    cfgfile.Close();
    return 1;
}


void EDA_BASE_FRAME::ImportHotkeyConfigFromFile( EDA_HOTKEY_CONFIG* aDescList,
                                                 const wxString& aDefaultShortname )
{
    wxString ext  = DEFAULT_HOTKEY_FILENAME_EXT;
    wxString mask = wxT( "*." ) + ext;


    wxString path = GetMruPath();
    wxFileName fn( aDefaultShortname );
    fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );

    wxString  filename = EDA_FILE_SELECTOR( _( "Read Hotkey Configuration File:" ),
                                           path,
                                           fn.GetFullPath(),
                                           ext,
                                           mask,
                                           this,
                                           wxFD_OPEN,
                                           true );

    if( filename.IsEmpty() )
        return;

    // JEY TODO: implement import of new hotkeys file....
    //::ReadHotkeyConfigFile( filename, aDescList, false );
    //WriteHotKeyConfig( aDescList );
    SetMruPath( wxFileName( filename ).GetPath() );
}


void EDA_BASE_FRAME::ExportHotkeyConfigToFile( EDA_HOTKEY_CONFIG* aDescList,
                                               const wxString& aDefaultShortname )
{
    wxString ext  = DEFAULT_HOTKEY_FILENAME_EXT;
    wxString mask = wxT( "*." ) + ext;

#if 0
    wxString path = wxPathOnly( Prj().GetProjectFullName() );
#else
    wxString path = GetMruPath();
#endif
    wxFileName fn( aDefaultShortname );
    fn.SetExt( DEFAULT_HOTKEY_FILENAME_EXT );

    wxString filename = EDA_FILE_SELECTOR( _( "Write Hotkey Configuration File:" ),
                                           path,
                                           fn.GetFullPath(),
                                           ext,
                                           mask,
                                           this,
                                           wxFD_SAVE,
                                           true );

    if( filename.IsEmpty() )
        return;

    // JEY TODO: make this whole routine oboslete?
    //WriteHotKeyConfig( aDescList, &filename );
    SetMruPath( wxFileName( filename ).GetPath() );
}

