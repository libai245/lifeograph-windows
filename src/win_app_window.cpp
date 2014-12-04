/***********************************************************************************

    Copyright (C) 2014-2015 Ahmet �zt�rk (aoz_2@yahoo.com)

    This file is part of Lifeograph.

    Lifeograph is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Lifeograph is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lifeograph.  If not, see <http://www.gnu.org/licenses/>.

***********************************************************************************/


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <richedit.h>
#include <string>
#include <cstdlib>
#include <cassert>

#ifndef RICHEDIT_CLASS
    #ifdef UNICODE
        #define RICHEDIT_CLASS "RichEdit20W"
    #else
        #define RICHEDIT_CLASS "RichEdit20A"
    #endif
#endif

#include "../rc/resource.h"
#include "strings.hpp"
#include "lifeograph.hpp"
#include "win_app_window.hpp"

// TEMPORARY
#define IDRT_MAIN       12338
#define IDLV_MAIN       12339
#define IDCAL_MAIN      12340


using namespace LIFEO;

// NON-MEMBER PROCEDURES
inline static LRESULT CALLBACK app_window_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
    return WinAppWindow::p->proc( hwnd, msg, wParam, lParam );
}

LRESULT CALLBACK calendar_proc( HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam,
                                UINT_PTR uIdSubclass, DWORD_PTR dwRefData )
{
    switch( msg )
    {
        case WM_LBUTTONDBLCLK:
            WinAppWindow::p->handle_calendar_doubleclick();
            return TRUE;
        // TODO: add a menu for chapters..
        //case WM_RBUTTONUP:
            //return TRUE;
    }
    
    return DefSubclassProc( hwnd, msg, wparam, lparam );
}

WinAppWindow* WinAppWindow::p = NULL;

// CONSTRUCTOR
WinAppWindow::WinAppWindow()
:    m_entry_view( NULL ), m_seconds_remaining( LOGOUT_COUNTDOWN + 1 ), m_auto_logout_status( 1 )
{
    p = this;

    Cipher::init();

    Diary::d = new Diary;
    m_login = new WinLogin;
    m_entry_view = new EntryView;
    m_diary_view = new DiaryView;
    m_richedit = new RichEdit;
}

WinAppWindow::~WinAppWindow()
{
    if( Diary::d )
        delete Diary::d;
}

int
WinAppWindow::run( HINSTANCE hInstance )
{
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    wc.cbSize        = sizeof( WNDCLASSEX );
    wc.style         = 0;
    wc.lpfnWndProc   = app_window_proc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hInstance;
    wc.hIcon         = LoadIcon( NULL, IDI_APPLICATION );
    wc.hCursor       = LoadCursor( NULL, IDC_ARROW );
    wc.hbrBackground = ( HBRUSH ) ( COLOR_WINDOW + 1 );
    wc.lpszMenuName  = MAKEINTRESOURCE( IDM_MAIN );
    wc.lpszClassName = L"WindowClass";
    wc.hIconSm       = LoadIcon( hInstance, L"A" ); /* A is name used by project icons */

    if( !RegisterClassEx( &wc ) )
    {
        MessageBoxA( 0, "Window Registration Failed!", "Error!",
                     MB_ICONEXCLAMATION|MB_OK|MB_SYSTEMMODAL );
        return 0;
    }

    hwnd = CreateWindowEx( WS_EX_CLIENTEDGE,
                           L"WindowClass",
                           L"Lifeograph",
                           WS_OVERLAPPEDWINDOW,
                           CW_USEDEFAULT, CW_USEDEFAULT,
                           640, 480,
                           NULL, NULL, hInstance, NULL );

    if( hwnd == NULL )
    {
        MessageBoxA( 0, "Window Creation Failed!", "Error!",
                    MB_ICONEXCLAMATION|MB_OK|MB_SYSTEMMODAL );
        return 0;
    }

    ShowWindow( hwnd, 1 );
    UpdateWindow( hwnd );

    while( GetMessage( &Msg, NULL, 0, 0 ) > 0 )
    {
        TranslateMessage( &Msg );
        DispatchMessage( &Msg );
    }
    return Msg.wParam;
}

LRESULT
WinAppWindow::proc( HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam )
{
    switch( Message )
    {
        case WM_CREATE:
            m_hwnd = hwnd;
            handle_create();
            break;
        case WM_SIZE:
            if( wParam != SIZE_MINIMIZED )
                handle_resize( LOWORD( lParam ), HIWORD( lParam ) );
            break;
        case WM_SETFOCUS:
            SetFocus( GetDlgItem( hwnd, IDRT_MAIN ) );
            break;
        case WM_ENTERMENULOOP:
            if( wParam == false )
                update_menu();
            break;
        case WM_COMMAND:
            switch( LOWORD( wParam ) )
            {
                case IDMI_DIARY_CREATE:
                    // TODO
                    break;
                case IDMI_DIARY_OPEN:
                    m_login->add_existing_diary();
                    break;
                case IDMI_EXPORT:
                    if( Lifeograph::loginstatus == Lifeograph::LOGGED_IN )
                        m_diary_view->export_diary();
                    break;
                case IDMI_QUIT_WO_SAVE:
                    PostMessage( hwnd, WM_CLOSE, 0, true );
                    break;
                case IDMI_QUIT:
                    PostMessage( hwnd, WM_CLOSE, 0, 0 );
                    break;
                case IDMI_ENTRY_DISMISS:
                    m_entry_view->dismiss_entry();
                    break;
                case IDMI_ABOUT:
                    MessageBoxA( NULL, "Lifeograph 0.0.3", "About...", MB_OK );
                    break;
                case IDRT_MAIN:
                    if( HIWORD( wParam ) == EN_CHANGE )
                        m_richedit->handle_change();
            }
            break;
        case WM_NOTIFY:
            handle_notify( ( int ) wParam, lParam );
            break;
        case WM_CLOSE:
            if( Lifeograph::p->loginstatus == Lifeograph::LOGGED_IN )
                if( ! finish_editing( ! lParam ) )
                    break;
            DestroyWindow( hwnd );
            break;
        case WM_DESTROY:
            PostQuitMessage( 0 );
            break;
        default:
            return DefWindowProc( hwnd, Message, wParam, lParam );
    }
    return 0;
}

void
WinAppWindow::handle_create()
{
    INITCOMMONCONTROLSEX iccx;
    iccx.dwSize = sizeof( INITCOMMONCONTROLSEX );
    iccx.dwICC  = ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES;
    InitCommonControlsEx( &iccx );
    
    // try to load latest version of rich edit control
    static HINSTANCE hlib = LoadLibrary( L"RICHED20.DLL" );
    if( !hlib )
    {
        MessageBoxA( NULL, "Failed to load Rich Edit", "Error", MB_OK | MB_ICONERROR );
        return;
    }

    // RICH EDIT
    m_richedit->m_hwnd =
            CreateWindowEx( 0, //WS_EX_CLIENTEDGE,
                            RICHEDIT_CLASS, L"",
                    	    WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_WANTRETURN,
                            0, 0, CW_USEDEFAULT, CW_USEDEFAULT,
                            m_hwnd, ( HMENU ) IDRT_MAIN, GetModuleHandle( NULL ), NULL );

    SendMessage( m_richedit->m_hwnd, WM_SETFONT,
                 ( WPARAM ) GetStockObject( DEFAULT_GUI_FONT ), MAKELPARAM( TRUE, 0 ) );
    SendMessage( m_richedit->m_hwnd, EM_SETEVENTMASK, 0, ( LPARAM ) ENM_CHANGE );

    // LIST VIEW
    m_list =
            CreateWindowExW( 0, WC_LISTVIEWW, L"",
                            WS_CHILD|WS_VISIBLE|WS_VSCROLL|LVS_REPORT,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            m_hwnd, ( HMENU ) IDLV_MAIN, GetModuleHandle( NULL ), NULL );
    init_list();

    // CALENDAR
    m_calendar =
            CreateWindowEx( 0, L"SysMonthCal32", L"",
                            WS_CHILD|WS_VISIBLE|WS_BORDER|MCS_DAYSTATE|MCS_NOTODAY,
                            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                            m_hwnd, ( HMENU ) IDCAL_MAIN, Lifeograph::p->hInst, NULL );
    SendMessage( m_calendar, WM_SETFONT,
                 ( WPARAM ) GetStockObject( DEFAULT_GUI_FONT ), MAKELPARAM( TRUE, 0 ) );
                 
    SetWindowSubclass( m_calendar, calendar_proc, 0, 0 );
    SetClassLongPtr( m_calendar, GCL_STYLE, CS_DBLCLKS ); // make it accept double clicks

    SYSTEMTIME lt;
    GetLocalTime( &lt );
    MonthCal_SetToday( m_calendar, &lt );
    
    m_login->handle_start();
}

const static float EDITOR_RATIO = 0.6;

void
WinAppWindow::handle_resize( short width, short height )
{
    RECT rc;
    MonthCal_GetMinReqRect( m_calendar, &rc );
    
    const int EDITOR_WIDTH = width * EDITOR_RATIO;
    
    MoveWindow( GetDlgItem( m_hwnd, IDRT_MAIN ), 0, 0, EDITOR_WIDTH, height, TRUE );
    MoveWindow( m_list,
                EDITOR_WIDTH, 0, width - EDITOR_WIDTH, height - rc.bottom,
                TRUE );
    MoveWindow( m_calendar,
                EDITOR_WIDTH, height - rc.bottom, width - EDITOR_WIDTH, rc.bottom,
                TRUE );
}

void
WinAppWindow::handle_notify( int id, LPARAM lparam )
{
    switch( id )
    {
        case IDLV_MAIN:
            if( ( ( LPNMHDR ) lparam )->code == NM_CLICK )
            {
                int iSelect = SendMessage( m_list, LVM_GETNEXTITEM, -1, LVNI_FOCUSED ); // return item selected

                if( iSelect != -1 )
                {
                    LVITEM lvi;
                    lvi.mask        = LVIF_PARAM;
                    lvi.iItem       = iSelect;
                    if( ListView_GetItem( m_list, &lvi ) )
                    {
                        DiaryElement* elem = Diary::d->get_element( lvi.lParam );
                        if( elem )
                            elem->show();
                    }
                }
            }
            PRINT_DEBUG( "....." );
            break;
        case IDCAL_MAIN:
            if( ( ( LPNMHDR ) lparam )->code == MCN_GETDAYSTATE )
            {
                NMDAYSTATE* ds( ( NMDAYSTATE* ) lparam );
                MONTHDAYSTATE mds[ ds->cDayState ];
                
                fill_monthdaystate( ds->stStart.wYear,
                                    ds->stStart.wMonth,
                                    mds,
                                    ds->cDayState );
                
                ds->prgDayState = mds;
            }
            break;
    }
}

// LOG OUT
bool
WinAppWindow::finish_editing( bool opt_save )
{
    // files added to recent list here if not already there
    if( ! Diary::d->get_path().empty() )
        if( Lifeograph::stock_diaries.find( Diary::d->get_path() ) ==
                Lifeograph::stock_diaries.end() )
            Lifeograph::settings.recentfiles.insert( Diary::d->get_path() );

    if( ! Diary::d->is_read_only() )
    {
        m_entry_view->sync();
        // TODO Diary::d->set_last_elem( panel_main->get_cur_elem() );

        if( opt_save )
        {
            if( Diary::d->write() != SUCCESS )
            {
                MessageBoxA( m_hwnd,
                            STRING::CANNOT_WRITE_SUB,
                            STRING::CANNOT_WRITE,
                            MB_OK|MB_ICONERROR );
                return false;
            }
        }
        else
        {
            if( MessageBoxA( m_hwnd,
                            "Your changes will be backed up in .~unsaved~."
                            "If you exit normally, your diary is saved automatically.",
                            "Are you sure you want to log out without saving?",
                            MB_YESNO | MB_ICONQUESTION ) != IDYES )
                return false;
            // back up changes
            Diary::d->write( Diary::d->get_path() + ".~unsaved~" );
        }
    }

    // CLEARING
    // TODO: m_loginstatus = LOGGING_OUT_IN_PROGRESS;

    Lifeograph::m_internaloperation++;
//    panel_main->handle_logout();
//    panel_diary->handle_logout();
//    m_entry_view->get_buffer()->handle_logout();
//    panel_extra->handle_logout();

    if( Lifeograph::loginstatus == Lifeograph::LOGGED_IN )
        Lifeograph::loginstatus = Lifeograph::LOGGED_OUT;
    Diary::d->clear();

    Lifeograph::m_internaloperation--;
    
    return true;
}

void
WinAppWindow::logout( bool opt_save )
{
    Lifeograph::p->m_flag_open_directly = false;   // should be reset to prevent logging in again
    if( finish_editing( opt_save ) )
        m_login->handle_logout();

    m_auto_logout_status = 1;
}

void
WinAppWindow::login()
{
    // LOGIN
    // the following must come before handle_login()s
    m_auto_logout_status = ( Lifeograph::settings.autologout && Diary::d->is_encrypted() ) ? 0 : 1;

    Lifeograph::m_internaloperation++;

//    panel_main->handle_login(); // must come before m_diary_view->handle_login() for header bar order
//    m_view_login->handle_login();
//    m_tag_view->handle_login();
//    m_filter_view->handle_login();
//    m_entry_view->handle_login();
//    m_diary_view->handle_login();
//    m_chapter_view->handle_login();
//    panel_diary->handle_login();
//    panel_extra->handle_login();

    update_entry_list();
    update_calendar();

    Lifeograph::m_internaloperation--;

    Lifeograph::loginstatus = Lifeograph::LOGGED_IN;

    update_title();
}

BOOL
WinAppWindow::init_list()
{
    SendMessage( m_list, WM_SETFONT,
                 ( WPARAM ) GetStockObject( DEFAULT_GUI_FONT ), MAKELPARAM( TRUE, 0 ) );
    ListView_SetExtendedListViewStyle( m_list, LVS_EX_FULLROWSELECT );
    ListView_SetUnicodeFormat( m_list, true );
    
    // COLUMNS
    LV_COLUMN lvc;

    lvc.mask     = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    lvc.fmt      = LVCFMT_LEFT;
    lvc.iSubItem = 0;
    lvc.pszText  = ( wchar_t* ) L"Entries";
    lvc.cx       = 100;          // width of column in pixels

    ListView_InsertColumn( m_list, 0, &lvc );
    
    // IMAGE LISTS
    HICON hIconItem;     // Icon for list-view items.
    HIMAGELIST hLarge;   // Image list for icon view.
    HIMAGELIST hSmall;   // Image list for other views.

    hSmall = ImageList_Create( GetSystemMetrics( SM_CXSMICON ),
                               GetSystemMetrics( SM_CYSMICON ),
                               ILC_MASK, 1, 1 );

    for( int index = 0; index < 2; index++ )
    {
        hIconItem = LoadIcon( Lifeograph::p->hInst, MAKEINTRESOURCE( IDI_ENTRY16 + index ) );
        ImageList_AddIcon( hSmall, hIconItem );
        DestroyIcon( hIconItem );
    }

    // Assign the image lists to the list-view control.
    ListView_SetImageList( m_list, hSmall, LVSIL_SMALL );

    return TRUE;
}

void
WinAppWindow::update_title()
{
    Ustring title( PROGRAM_NAME );

    if( Lifeograph::loginstatus == Lifeograph::LOGGED_IN )
    {
        title += " - ";
        title += Diary::d->get_name();

        if( Diary::d->is_read_only() )
            title += " <Read Only>";
    }

    SetWindowTextA( m_hwnd, title.c_str() );
}

void
WinAppWindow::update_menu()
{
    HMENU hmenu = GetMenu( m_hwnd );
    bool logged_in = Lifeograph::loginstatus == Lifeograph::LOGGED_IN;
    
    EnableMenuItem( hmenu, IDMI_EXPORT,
                    MF_BYCOMMAND | ( logged_in ? MF_ENABLED : MF_GRAYED ) );
    EnableMenuItem( hmenu, IDMI_QUIT_WO_SAVE,
                    MF_BYCOMMAND | ( logged_in ? MF_ENABLED : MF_GRAYED ) );
                            
    EnableMenuItem( hmenu, IDMI_ENTRY_DISMISS,
                    MF_BYCOMMAND | ( m_entry_view->get_element() ? MF_ENABLED : MF_GRAYED ) );
                            
    //CheckMenuItem( hmenu, IDMI_, MF_BYCOMMAND | ( ? MF_CHECKED : MF_UNCHECKED ) );
}

void
WinAppWindow::update_entry_list()
{
    int i = 0;
    LVITEM lvi;
    lvi.mask      = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_PARAM;
    lvi.stateMask = 0;
    lvi.iSubItem  = 0;
    lvi.state     = 0;
    lvi.iImage    = 0;

    for( auto& kv : Diary::d->get_entries() )
    {
        lvi.iItem   = i++;
        lvi.pszText = HELPERS::convert_utf8_to_16( kv.second->get_list_str() );
        lvi.lParam  = ( LPARAM ) kv.second->get_id();

        SendMessageW( m_list, LVM_INSERTITEM, 0, ( LPARAM ) &lvi );
    }
}

#define BOLDDAY(ds, iDay)  if (iDay > 0 && iDay < 32)(ds) |= (0x00000001 << (iDay - 1))

void
WinAppWindow::fill_monthdaystate( int year, int month, MONTHDAYSTATE mds[], int size )
{
    for( int i = 0; i < size; i++ )
    {
        mds[ i ] = 0;

        for( auto& kv_entry : Diary::d->get_entries() )
        {
            Entry* entry = kv_entry.second;

            if( /*entry->get_filtered_out() ||*/ entry->get_date().is_ordinal() )
                continue;

            if( entry->get_date().get_year() == year )
            {
                if( entry->get_date().get_month() == month )
                    BOLDDAY( mds[ i ], entry->get_date().get_day() );
                else if( entry->get_date().get_month() < month )
                    break;
            }
            else
            if( entry->get_date().get_year() < year )
                break;
        }

        // increase month and, if needed, year
        if( ++month > 12 )
        {
            month = 1;
            year++;
        }
    }
}

void
WinAppWindow::update_calendar()
{
    // does not seem to work!
    SYSTEMTIME st[ 2 ];
    int size = SendMessage( m_calendar, MCM_GETMONTHRANGE, GMR_DAYSTATE, ( LPARAM ) &st );
    MONTHDAYSTATE mds[ size ];
    
    fill_monthdaystate( st[ 0 ].wYear, st[ 0 ].wMonth, mds, size );
    
    SendMessage( m_calendar, MCM_SETDAYSTATE, size, ( LPARAM ) &mds );
}

void
WinAppWindow::update_startup_elem()
{
    // TODO
}

void
WinAppWindow::handle_calendar_doubleclick()
{
    if( Lifeograph::loginstatus != Lifeograph::LOGGED_IN || Diary::d->is_read_only() )
        return;

    SYSTEMTIME st;
    if( ! SendMessage( m_calendar, MCM_GETCURSEL, 0, ( LPARAM ) &st ) )
        return;
        
    Date date( st.wYear, st.wMonth, st.wDay );
    Entry* entry( Diary::d->create_entry( date.m_date ) );

    update_entry_list();
    update_calendar();

    entry->show();
}

bool
WinAppWindow::confirm_dismiss_element( const DiaryElement* elem )
{
    return(
        MessageBox( m_hwnd,
                    convert_utf8_to_16(
                            STR::compose( "Are you sure, you want to dismiss ",
                                          elem->get_name() ) ),
                    L"Confirm Dismiss",
                    MB_YESNO | MB_ICONWARNING ) == IDYES );
}
