/** \file       BrowserWindow.cpp
 *  \brief      Contains definition of the browser window.
 *  \author     Rhoot
 */

/**
 * Copyright (C) 2014-2018 Khralkatorrix <https://github.com/kytulendu>
 * Copyright (C) 2012 Rhoot <https://github.com/rhoot>
 *
 * This file is part of Gw2Browser.
 *
 * Gw2Browser is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"

#include <wx/aui/aui.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>

#include "Imported/crc.h"

#include "EventId.h"
#include "CategoryTree.h"
#include "Exporter.h"
#include "FileReader.h"
#include "ProgressStatusBar.h"
#include "PreviewPanel.h"
#include "PreviewGLCanvas.h"

#include "Tasks/ReadIndexTask.h"
#include "Tasks/ScanDatTask.h"
#include "Tasks/WriteIndexTask.h"

#include "BrowserWindow.h"

namespace gw2b {

    wxDEFINE_EVENT( EVT_FILE_LOADED, wxThreadEvent );

    BrowserWindow::BrowserWindow( const wxString& p_title, const wxSize p_size )
        : wxFrame( nullptr, wxID_ANY, p_title, wxDefaultPosition, p_size )
        , m_index( std::make_shared<DatIndex>( ) )
        , m_progress( nullptr )
        , m_currentTask( nullptr )
        , m_catTree( nullptr )
        , m_previewPanel( nullptr )
        , m_previewGLCanvas( nullptr )
        , m_fileLoader( m_datFile, this ) {
        // Initializes all available image handlers
        wxInitAllImageHandlers( );
        // Notify wxAUI which frame to use
        m_uiManager.SetManagedWindow( this );

        auto menuBar = new wxMenuBar;

        // File menu
        auto fileMenu = new wxMenu;
        wxAcceleratorEntry openAccel( wxACCEL_CTRL, 'O' );
        fileMenu->Append( wxID_OPEN, wxT( "&Open" ), wxT( "Open a file for browsing" ) )->SetAccel( &openAccel );
        fileMenu->AppendSeparator( );
        fileMenu->Append( wxID_EXIT, wxT( "E&xit\tAlt+F4" ) );
        // View menu
        auto viewMenu = new wxMenu;
        viewMenu->AppendCheckItem( ID_ShowFindFile, wxT( "&Show Find File Window" ), wxT( "Toggle show find file window" ) );
        viewMenu->AppendCheckItem( ID_ShowFileList, wxT( "&Show File List Window" ), wxT( "Toggle show file list window" ) );
        viewMenu->AppendCheckItem( ID_ShowLog, wxT( "&Show Log Window" ), wxT( "Toggle show log window" ) );
        //viewMenu->Append( ID_ResetLayout, wxT( "&Reset Layout" ) );
        viewMenu->AppendSeparator( );
        viewMenu->Append( ID_ClearLog, wxT( "&Clear Log" ), wxT( "Clear log window content" ) );
        //viewMenu->AppendSeparator( );
        //viewMenu->Append( ID_SetBackgroundColor, wxT( "&Set Background color" ) );
        //viewMenu->AppendCheckItem( ID_ShowGrid, wxT( "&Show Grid" ) );
        //viewMenu->AppendSeparator( );
        //viewMenu->Append( ID_SetCanvasSize, wxT( "&Set GLCanvas Size" ) );
        // Tools menu
        //auto toolsMenu = new wxMenu;
        //toolsMenu->Append( ID_Settings, wxT( "&Settings" ) );
        // Help menu
        auto helpMenu = new wxMenu( );
        helpMenu->Append( wxID_ABOUT, wxT( "&About Gw2Browser" ) );

        // Attach menu
        menuBar->Append( fileMenu, wxT( "&File" ) );
        menuBar->Append( viewMenu, wxT( "&View" ) );
        //menuBar->Append( toolsMenu, wxT( "&Tools" ) );
        menuBar->Append( helpMenu, wxT( "&Help" ) );
        this->SetMenuBar( menuBar );

        // Setup statusbar
        m_progress = new ProgressStatusBar( this );
        this->SetStatusBar( m_progress );

        // Text control use for loging
        m_log = new wxTextCtrl( this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize( 600, 70 ), wxTE_MULTILINE );
        // Make log window read only
        m_log->SetEditable( false );
        m_logTarget = wxLog::SetActiveTarget( new wxLogTextCtrl( m_log ) );

        // Category tree
        m_catTree = new CategoryTree( this );
        m_catTree->setDatIndex( m_index );
        m_catTree->addListener( this );

        // Find panel
        auto findPanel = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxSize( 170, 50 ), wxBORDER_SIMPLE | wxTAB_TRAVERSAL );
        auto flex = new wxFlexGridSizer( 1, 2, 0, 0 );
        m_findTextBox = new wxTextCtrl( findPanel, wxID_ANY, wxT( "" ), wxDefaultPosition, wxSize( 120, -1 ), wxTE_PROCESS_ENTER );
        auto btnFindFile = new wxButton( findPanel, ID_BtnFindFile, wxT( "Go" ), wxDefaultPosition, wxSize( 25, 25 ) );

        flex->Add( m_findTextBox, 1, wxALL | wxALIGN_CENTRE, 5 );
        flex->Add( btnFindFile, 1, wxALL | wxALIGN_CENTRE, 5 );
        findPanel->SetSizer( flex );

        // Add the panes to the manager

        // Find file panel
        m_uiManager.AddPane( findPanel, wxAuiPaneInfo( ).Name( wxT( "FindFilePanel" ) ).Caption( wxT( "Find By File Id" ) ).BestSize( wxSize( 170, 40 ) ).Top( ).Left( ).Resizable(false) );

        // CategoryTree
        m_uiManager.AddPane( m_catTree, wxAuiPaneInfo( ).Name( wxT( "CategoryTree" ) ).Caption( wxT( "File List" ) ).BestSize( wxSize( 170, 500 ) ).Left( ) );

        // Log window
        m_uiManager.AddPane( m_log, wxAuiPaneInfo( ).Name( wxT( "LogWindow" ) ).Caption( wxT( "Log" ) ).Bottom( ).Layer( 1 ).Position( 1 ).Hide( ) );

        // Preview panel
        m_previewPanel = new PreviewPanel( this );

        // OpenGL canvas
        m_previewGLCanvas = nullptr;
        wxGLAttributes vAttrs;
        // Defaults should be accepted
        vAttrs.PlatformDefaults().Defaults().EndList();
        bool accepted = wxGLCanvas::IsDisplaySupported(vAttrs) ;

        if (accepted)
        {
            wxLogMessage("The display supports required visual attributes.");
        }
        else
        {
            wxLogMessage("First try with OpenGL default visual attributes failed.");
            // Try again without sample buffers
            vAttrs.Reset();
            vAttrs.PlatformDefaults().RGBA().DoubleBuffer().Depth(16).EndList();
            accepted = wxGLCanvas::IsDisplaySupported(vAttrs) ;

            if (!accepted)
            {
                wxMessageBox("Visual attributes for OpenGL are not accepted.\nGw2Browser will exit now.",
                             "Error with OpenGL", wxOK | wxICON_ERROR);
            }
            else
            {
                wxLogMessage("Second try with other visual attributes worked.");
            }
        }

        if (accepted)
        {
            m_previewGLCanvas = new PreviewGLCanvas( this, vAttrs );
        }

        // Main content window
        m_uiManager.AddPane( m_previewGLCanvas, wxAuiPaneInfo( ).Name( wxT( "gl_content" ) ).CenterPane( ) );
        m_uiManager.AddPane( m_previewPanel, wxAuiPaneInfo( ).Name( wxT( "panel_content" ) ).CenterPane( ).Hide( )  );

        // Set default settings
        this->SetDefaults( );

        wxAuiDockArt* art = m_uiManager.GetArtProvider();
        art->SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_COLOUR,          *wxLIGHT_GREY );
        art->SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_GRADIENT_COLOUR, *wxLIGHT_GREY );
        art->SetColour( wxAUI_DOCKART_INACTIVE_CAPTION_TEXT_COLOUR,     *wxBLACK );

        // Tell the manager to "commit" all the changes just made
        m_uiManager.Update( );

        // Have to set the window size here after initialize OpenGL canvas,
        // or else the OpenGL canvas doesn't display if not resize the window
        this->SetClientSize(820, 512);
        this->SetMinSize(wxSize(820, 512));

        // Hook up events
        this->Bind( wxEVT_MENU, &BrowserWindow::onOpenEvt, this, wxID_OPEN );
        this->Bind( wxEVT_MENU, &BrowserWindow::onExitEvt, this, wxID_EXIT );
        this->Bind( wxEVT_MENU, &BrowserWindow::onAboutEvt, this, wxID_ABOUT );
        this->Bind( wxEVT_MENU, &BrowserWindow::onTogglePaneEvt, this, ID_ShowFindFile );
        this->Bind( wxEVT_MENU, &BrowserWindow::onTogglePaneEvt, this, ID_ShowFileList );
        this->Bind( wxEVT_MENU, &BrowserWindow::onTogglePaneEvt, this, ID_ShowLog );
        this->Bind( wxEVT_MENU, &BrowserWindow::onClearLogEvt, this, ID_ClearLog );
        this->Bind( wxEVT_BUTTON, &BrowserWindow::onButtonEvt, this );
        this->Bind( wxEVT_TEXT_ENTER, &BrowserWindow::onEnterPressedInSrchBoxEvt, this );
        this->Bind( wxEVT_AUI_PANE_CLOSE, &BrowserWindow::onPaneCloseEvt, this );
        this->Bind( wxEVT_CLOSE_WINDOW, &BrowserWindow::onCloseEvt, this );
        this->Bind( EVT_FILE_LOADED, &BrowserWindow::onFileLoaded, this );
    }

    //============================================================================/

    BrowserWindow::~BrowserWindow( ) {
        deletePointer( m_currentTask );
        deletePointer( m_logTarget );
        // Deinitialize the frame manager
        m_uiManager.UnInit( );
    }

    //============================================================================/

    bool BrowserWindow::performTask( Task* p_task ) {
        Ensure::notNull( p_task );

        // Already have a task running?
        if ( m_currentTask ) {
            if ( m_currentTask->canAbort( ) ) {
                m_currentTask->abort( );
                deletePointer( m_currentTask );
                this->Unbind( wxEVT_IDLE, &BrowserWindow::onPerformTaskEvt, this );
                m_progress->hideProgressBar( );
            } else {
                deletePointer( p_task );
                return false;
            }
        }

        // Initialize succeeded?
        m_currentTask = p_task;
        if ( !m_currentTask->init( ) ) {
            deletePointer( m_currentTask );
            return false;
        }

        this->Bind( wxEVT_IDLE, &BrowserWindow::onPerformTaskEvt, this );
        m_progress->setMaxValue( m_currentTask->maxProgress( ) );
        m_progress->showProgressBar( );
        return true;
    }

    //============================================================================/

    void BrowserWindow::openFile( const wxString& p_path ) {
        // Make sure no background read is touching the .dat while we re-open it.
        m_fileLoader.cancelAndWait( );

        // Try to open the file
        if ( !m_datFile.open( p_path ) ) {
            wxMessageBox( wxString::Format( wxT( "Failed to open file: %s" ), p_path ),
                wxMessageBoxCaptionStr, wxOK | wxCENTER | wxICON_ERROR );
            return;
        }
        wxLogMessage( wxT( "Open dat file: %s" ), p_path );
        m_datPath = p_path;

        // Open the index file
        uint64 datTimeStamp = wxFileModificationTime( p_path );
        auto indexFile = this->findDatIndex( );
        auto readIndexTask = new ReadIndexTask( m_index, indexFile.GetFullPath( ), datTimeStamp );

        // Start reading the index
        readIndexTask->addOnCompleteHandler( [this] ( ) { this->onReadIndexComplete( ); } );
        if ( !this->performTask( readIndexTask ) ) {
            this->reIndexDat( );
        }

        // Hide GLCanvas while loading index file or scanning dat file
        // As this will improve performance
        m_previewGLCanvas->clear();
        m_uiManager.GetPane(wxT("gl_content")).Hide();
        m_uiManager.GetPane(wxT("panel_content")).Show();
        m_uiManager.Update();
    }

    //============================================================================/

    void BrowserWindow::viewEntry( const DatIndexEntry& p_entry ) {
        // Decouple selection from loading: read (and decompress) the file on a
        // worker thread. The viewer is built when the data arrives, in
        // onFileLoaded( ). This keeps the UI responsive and lets rapid selection
        // changes supersede slower, no-longer-wanted loads.
        m_fileLoader.request( p_entry.mftEntry( ), p_entry.fileType( ), p_entry.name( ) );
        m_progress->SetStatusText( wxString::Format( wxT( "Loading %s..." ), p_entry.name( ) ) );
    }

    //============================================================================/

    void BrowserWindow::onFileLoaded( wxThreadEvent& p_event ) {
        auto result = p_event.GetPayload<std::shared_ptr<FileLoadResult>>( );
        if ( !result ) {
            return;
        }

        // Discard results superseded by a more recent selection.
        if ( result->generation != m_fileLoader.currentGeneration( ) ) {
            return;
        }

        m_progress->SetStatusText( wxEmptyString );

        if ( !result->success || result->data.empty( ) ) {
            return;
        }

        // Wrap the bytes into an Array on the UI thread, avoiding any cross-thread
        // reference counting of the shared Array type.
        Array<byte> data( result->data.size( ) );
        ::memcpy( data.GetPointer( ), result->data.data( ), result->data.size( ) );

        this->displayLoadedFile( result->fileType, data );
    }

    //============================================================================/

    void BrowserWindow::displayLoadedFile( ANetFileType p_fileType, const Array<byte>& p_data ) {
        switch ( p_fileType ) {
        //case ANFT_MapParam:
        case ANFT_Model:
            if ( m_previewGLCanvas && m_previewGLCanvas->previewData( m_datFile, p_fileType, p_data ) ) {
                m_previewPanel->destroyViewer( );
                m_uiManager.GetPane( wxT( "panel_content" ) ).Hide( );
                m_uiManager.GetPane( wxT( "gl_content" ) ).Show( );
            }
            break;
        default:
            if ( m_previewPanel->previewData( m_datFile, p_fileType, p_data ) ) {
                // Clear the OpenGL canvas to reduce memory usage
                if ( m_previewGLCanvas ) {
                    m_previewGLCanvas->clear( );
                }
                m_uiManager.GetPane( wxT( "gl_content" ) ).Hide( );
                m_uiManager.GetPane( wxT( "panel_content" ) ).Show( );
            }
        }
        m_uiManager.Update( );
    }

    //============================================================================/

    AsyncFileLoader::AsyncFileLoader( DatFile& p_datFile, wxEvtHandler* p_sink )
        : m_datFile( p_datFile )
        , m_sink( p_sink ) {
        m_thread = std::thread( &AsyncFileLoader::workerMain, this );
    }

    //============================================================================/

    AsyncFileLoader::~AsyncFileLoader( ) {
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_stop = true;
            m_hasRequest = false;
        }
        m_cv.notify_all( );
        if ( m_thread.joinable( ) ) {
            m_thread.join( );
        }
    }

    //============================================================================/

    uint AsyncFileLoader::request( uint p_fileNum, ANetFileType p_fileType, const wxString& p_name ) {
        uint generation = ++m_generation;
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_reqFileNum = p_fileNum;
            m_reqFileType = p_fileType;
            // Force a deep copy so the worker never shares a string buffer with
            // the UI thread.
            m_reqName = wxString( p_name.c_str( ) );
            m_reqGeneration = generation;
            m_hasRequest = true;
        }
        m_cv.notify_one( );
        return generation;
    }

    //============================================================================/

    void AsyncFileLoader::cancelAndWait( ) {
        std::unique_lock<std::mutex> lock( m_mutex );
        m_hasRequest = false;
        // Bump the generation so any in-flight result is discarded by the sink.
        ++m_generation;
        m_idleCv.wait( lock, [this] { return !m_busy; } );
    }

    //============================================================================/

    void AsyncFileLoader::workerMain( ) {
        for ( ;; ) {
            uint fileNum;
            ANetFileType fileType;
            wxString name;
            uint generation;

            {
                std::unique_lock<std::mutex> lock( m_mutex );
                m_cv.wait( lock, [this] { return m_hasRequest || m_stop; } );
                if ( m_stop ) {
                    return;
                }
                fileNum = m_reqFileNum;
                fileType = m_reqFileType;
                name = wxString( m_reqName.c_str( ) );
                generation = m_reqGeneration;
                m_hasRequest = false;
                m_busy = true;
            }

            // (Re)open the per-thread read context if the .dat changed.
            if ( !m_contextReady || m_contextPath != m_datFile.path( ) ) {
                m_contextReady = m_context.open( m_datFile );
                m_contextPath = wxString( m_datFile.path( ).c_str( ) );
            }

            auto result = std::make_shared<FileLoadResult>( );
            result->generation = generation;
            result->fileNum = fileNum;
            result->fileType = fileType;
            result->name = name;
            result->success = false;

            if ( m_contextReady ) {
                uint size = m_datFile.fileSize( fileNum, m_context );
                if ( size > 0 && size != UINT_MAX ) {
                    result->data.resize( size );
                    uint read = m_datFile.readFile( fileNum, result->data.data( ), m_context );
                    if ( read > 0 ) {
                        result->data.resize( read );
                        result->success = true;
                    } else {
                        result->data.clear( );
                    }
                }
            }

            // Hand the result back to the UI thread.
            auto evt = new wxThreadEvent( EVT_FILE_LOADED );
            evt->SetPayload( result );
            m_sink->QueueEvent( evt );

            {
                std::lock_guard<std::mutex> lock( m_mutex );
                m_busy = false;
            }
            m_idleCv.notify_all( );
        }
    }

    //============================================================================/

    bool BrowserWindow::OGLAvailable( ) {
        //Test if visual attributes were accepted.
        if ( !m_previewGLCanvas ) {
            return false;
        }

        //Test if OGL context could be created.
        return m_previewGLCanvas->glCtxAvailable( );
    }

    //============================================================================/

    wxFileName BrowserWindow::findDatIndex( ) {
        wxStandardPathsBase& stdp = wxStandardPaths::Get( );
        auto configPath = stdp.GetUserDataDir( );

        auto datPathCrc = ::compute_crc( INITIAL_CRC, m_datPath.char_str( ), m_datPath.Length( ) );
        auto indexFileName = wxString::Format( wxT( "%x.idx" ), datPathCrc );

        return wxFileName( configPath, indexFileName );
    }

    //============================================================================/

    void BrowserWindow::indexDat( ) {
        auto scanTask = new ScanDatTask( m_index, m_datFile );
        scanTask->addOnCompleteHandler( [this] ( ) { this->onScanTaskComplete( ); } );
        this->performTask( scanTask );
    }

    //============================================================================/

    void BrowserWindow::reIndexDat( ) {
        m_index->clear( );
        m_index->setDatTimestamp( wxFileModificationTime( m_datPath ) );
        this->indexDat( );
    }

    //============================================================================/

    void BrowserWindow::onOpenEvt( wxCommandEvent& WXUNUSED( p_event ) ) {
        wxFileDialog dialog( this, wxFileSelectorPromptStr, wxEmptyString, wxT( "Gw2.dat" ),
            wxT( "Guild Wars 2 DAT|*.dat" ), wxFD_OPEN | wxFD_FILE_MUST_EXIST );

        if ( dialog.ShowModal( ) == wxID_OK ) {
            this->openFile( dialog.GetPath( ) );
        }
    }

    //============================================================================/

    void BrowserWindow::onExitEvt( wxCommandEvent& WXUNUSED( p_event ) ) {
        this->Close( true );
    }

    //============================================================================/

    void BrowserWindow::onAboutEvt( wxCommandEvent& WXUNUSED( p_event ) ) {
        wxAboutDialogInfo info;
        InitAboutInfo( info );

        wxAboutBox( info, this );
    }

    //============================================================================/

    void BrowserWindow::onButtonEvt( wxCommandEvent& p_event ) {
        auto id = p_event.GetId( );
        switch ( id ) {
        case ID_BtnFindFile:
            this->onFindFile( );
            break;
        }
    }

    //============================================================================/

    void BrowserWindow::onCloseEvt( wxCloseEvent& p_event ) {
        // Drop out if we can't cancel the window closing
        if ( !p_event.CanVeto( ) ) {
            p_event.Skip( );
            return;
        }

        // Cancel current task if possible.
        if ( m_currentTask ) {
            if ( m_currentTask->canAbort( ) ) {
                m_currentTask->abort( );
                deletePointer( m_currentTask );
                this->Unbind( wxEVT_IDLE, &BrowserWindow::onPerformTaskEvt, this );
            } else {
                this->Disable( );
                m_currentTask->addOnCompleteHandler( [this] ( ) { this->tryClose( ); } );
                p_event.Veto( );
                return;
            }
        }

        // Add a write task if the index is dirty
        if ( !m_currentTask && m_index->isDirty( ) ) {
            auto indexPath = this->findDatIndex( );
            if ( !indexPath.DirExists( ) ) {
                indexPath.Mkdir( 511, wxPATH_MKDIR_FULL );
            }

            auto writeTask = new WriteIndexTask( m_index, indexPath.GetFullPath( ) );
            writeTask->addOnCompleteHandler( [this] ( ) { this->onWriteTaskCloseCompleted( ); } );
            if ( this->performTask( writeTask ) ) {
                this->Disable( );
                p_event.Veto( );
                return;
            }
        }

        p_event.Skip( );
    }

    //============================================================================/

    void BrowserWindow::onPerformTaskEvt( wxIdleEvent& p_event ) {
        Ensure::notNull( m_currentTask );
        m_currentTask->perform( );

        if ( !m_currentTask->isDone( ) ) {
            m_progress->update( m_currentTask->currentProgress( ), m_currentTask->text( ) );
            p_event.RequestMore( );
        } else {
            this->Unbind( wxEVT_IDLE, &BrowserWindow::onPerformTaskEvt, this );
            m_progress->SetStatusText( wxEmptyString );
            m_progress->hideProgressBar( );

            auto oldTask = m_currentTask;
            m_currentTask = nullptr;
            oldTask->invokeOnCompleteHandler( );
            deletePointer( oldTask );
        }
    }

    //============================================================================/

    void BrowserWindow::onTogglePaneEvt( wxCommandEvent &p_event ) {
        // wxAUI Stuff
        if ( GetMenuBar( )->IsChecked( ID_ShowFindFile ) ) {
            m_uiManager.GetPane( wxT( "FindFilePanel" ) ).Show( );
        } else {
            m_uiManager.GetPane( wxT( "FindFilePanel" ) ).Hide( );
        }

        if ( GetMenuBar( )->IsChecked( ID_ShowFileList ) ) {
            m_uiManager.GetPane( wxT( "CategoryTree" ) ).Show( );
        } else {
            m_uiManager.GetPane( wxT( "CategoryTree" ) ).Hide( );
        }

        if ( GetMenuBar( )->IsChecked( ID_ShowLog ) ) {
            m_uiManager.GetPane( wxT( "LogWindow" ) ).Show( );
        } else {
            m_uiManager.GetPane( wxT( "LogWindow" ) ).Hide( );
        }

        m_uiManager.Update( );
    }

    //============================================================================/

    void BrowserWindow::onClearLogEvt( wxCommandEvent &p_event ) {
        m_log->Clear( );
    }

    //============================================================================/

    void BrowserWindow::onPaneCloseEvt( wxAuiManagerEvent &p_event ) {
        auto evt = p_event.GetPane( )->window;
        if ( evt == m_uiManager.GetPane( wxT( "FindFilePanel" ) ).window ) {
            this->GetMenuBar( )->Check( ID_ShowFindFile, false );
        }
        if ( evt == m_uiManager.GetPane( wxT( "CategoryTree" ) ).window ) {
            this->GetMenuBar( )->Check( ID_ShowFileList, false );
        }
        if ( evt == m_uiManager.GetPane( wxT( "LogWindow" ) ).window ) {
            this->GetMenuBar( )->Check( ID_ShowLog, false );
        }

        m_uiManager.Update( );
    }

    //============================================================================/

    void BrowserWindow::onEnterPressedInSrchBoxEvt( wxCommandEvent &p_event ) {
        this->onFindFile( );
    }

    //============================================================================/

    void BrowserWindow::onReadIndexComplete( ) {
        // If it failed, it was cleared.
        if ( m_index->datTimestamp( ) == 0 || m_index->numEntries( ) == 0 ) {
            this->reIndexDat( );
            return;
        }

        // Was it complete?
        auto isComplete = ( m_index->highestMftEntry( ) == m_datFile.numFiles( ) );
        if ( !isComplete ) {
            this->indexDat( );
        } else {
            m_catTree->refreshAfterBulkUpdate( );
        }
    }

    //============================================================================/

    void BrowserWindow::onScanTaskComplete( ) {
        m_catTree->refreshAfterBulkUpdate( );

        auto writeTask = new WriteIndexTask( m_index, this->findDatIndex( ).GetFullPath( ) );
        this->performTask( writeTask );
    }

    //============================================================================/

    void BrowserWindow::onWriteTaskCloseCompleted( ) {
        // Forcing this here causes the OnCloseEvt to not try to write the index
        // again. In case it failed the first time, it's likely to fail again and
        // we don't want to get stuck in an infinite loop.
        this->Close( true );
    }

    //============================================================================/

    void BrowserWindow::tryClose( ) {
        this->Close( false );
    }

    //============================================================================/

    void BrowserWindow::onTreeEntryClicked( CategoryTree& p_tree, const DatIndexEntry& p_entry ) {
        wxLogMessage( wxT( "Open Entry: %s" ), p_entry.name( ) );
        this->viewEntry( p_entry );
    }

    //============================================================================/

    void BrowserWindow::onTreeCategoryClicked( CategoryTree& p_tree, const DatIndexCategory& p_category ) {
        // TODO
    }

    //============================================================================/

    void BrowserWindow::onTreeCleared( CategoryTree& p_tree ) {
        // TODO
    }

    //============================================================================/

    void BrowserWindow::onTreeBackgroundLoadBegin( CategoryTree& p_tree, uint p_total ) {
        // Don't fight an active task for the progress bar.
        if ( m_currentTask ) {
            return;
        }
        m_progress->setMaxValue( p_total );
        m_progress->showProgressBar( );
        m_progress->update( 0, wxString::Format( wxT( "Loading entries: 0/%u" ), p_total ) );
    }

    //============================================================================/

    void BrowserWindow::onTreeBackgroundLoadUpdate( CategoryTree& p_tree, uint p_current, uint p_total ) {
        if ( m_currentTask ) {
            return;
        }
        m_progress->update( p_current, wxString::Format( wxT( "Loading entries: %u/%u" ), p_current, p_total ) );
    }

    //============================================================================/

    void BrowserWindow::onTreeBackgroundLoadEnd( CategoryTree& p_tree ) {
        if ( m_currentTask ) {
            return;
        }
        m_progress->SetStatusText( wxEmptyString );
        m_progress->hideProgressBar( );
    }

    //============================================================================/

    void BrowserWindow::onTreeExtractFile( CategoryTree& p_tree, bool p_mode ) {
        auto entries = p_tree.getSelectedEntries( );
        Exporter *exporter;

        if ( entries.GetSize( ) ) {
            if ( p_mode ) {
                exporter = new Exporter( this, entries, m_datFile, Exporter::EM_Converted );
            } else {
                exporter = new Exporter( this, entries, m_datFile, Exporter::EM_Raw );
            }
            delete exporter;
        }
    }

    //============================================================================/

    void BrowserWindow::InitAboutInfo( wxAboutDialogInfo& info ) {
        info.SetName( APP_TITLE );
        info.SetVersion( wxString::Format(
            " %d.%d.%d.%d\n%s",
            APP_MAJOR_VERSION,
            APP_MINOR_VERSION,
            APP_RELEASE_NUMBER,
            APP_SUBRELEASE_NUMBER,
            APP_SUBRELEASE_NUMBER ? wxT( "Release" ) : wxT( "Development" )
            ) );

        info.SetCopyright( wxString::FromAscii(
            "Copyright (C) 2014-2023 Khralkatorrix - https://github.com/kytulendu\n"
            "Copyright (C) 2020 Rengyr - https://github.com/Rengyr\n"
            "Copyright (C) 2019 BoyC - https://twitter.com/BoyCcns\n"
            "Copyright (C) 2013 Till034 - https://github.com/Till034\n"
            "Copyright (C) 2012 Rhoot - https://github.com/rhoot\n"
            "\n"
            "Guild Wars 2 (C)2010-2019 ArenaNet, LLC. All rights reserved.\n"
            "Guild Wars, Guild Wars 2, Guild Wars 2: Heart of Thorns,\n"
            "Guild Wars 2: Path of Fire, Guild Wars 2: End of Dragons,\n"
            "Guild Wars 2: Secrets of the Obscure,\n"
            "ArenaNet, NCSOFT, the Interlocking NC Logo, and all associated logos\n"
            "and designs are trademarks or registered trademarks of NCSOFT Corporation.\n"
            "All other trademarks are the property of their respective owners.\n"
            ) );

        info.SetDescription( wxString::FromAscii(
            "Opens a Guild Wars 2 .dat file and allows the user to browse and extract\n"
            "its content.\n"
            ) );

        info.SetWebSite( wxT( "https://github.com/kytulendu/gw2browser" ) );

        info.SetLicence( wxString::FromAscii(
            "Gw2Browser is free software: you can redistribute it and/or modify\n"
            "it under the terms of the GNU General Public License as published by\n"
            "the Free Software Foundation, either version 3 of the License,\n"
            "or ( at your option ) any later version.\n"
            "\n"
            "This program is distributed in the hope that it will be useful,\n"
            "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
            "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the\n"
            "GNU General Public License for more details.\n"
            "\n"
            "You should have received a copy of the GNU General Public License\n"
            "along with this program.If not, see <http://www.gnu.org/licenses/>.\n"
            "\n"
            "See README.md for code license exceptions."
            ) );

        info.AddDeveloper( wxT( "Khralkatorrix" ) );
        info.AddDeveloper( wxT( "Rhoot" ) );
    }

    //============================================================================/

    void BrowserWindow::SetDefaults( ) {
        this->GetMenuBar( )->Check( ID_ShowFindFile, true );
        this->GetMenuBar( )->Check( ID_ShowFileList, true );
        this->GetMenuBar( )->Check( ID_ShowLog, false );
    }

    void BrowserWindow::onFindFile( ) {
        wxString value = m_findTextBox->GetValue( );
        if ( value.IsEmpty( ) || !value.IsNumber( ) ) {
            wxMessageBox( wxT( "Please enter file id in number." ), wxT( " " ), wxOK | wxICON_EXCLAMATION, this );
            return;
        }

        // Freeze the tree while we expand the path to the entry, to avoid flicker.
        m_catTree->Freeze( );
        auto item = m_catTree->findEntry( value );
        m_catTree->Thaw( );

        if ( !item.IsOk( ) ) {
            wxMessageBox( wxString::Format( "Cannot Find file id \"%s\".", value ), wxT( " " ), wxOK | wxICON_EXCLAMATION, this );
            return;
        }

        // Deselect all
        m_catTree->UnselectAll( );
        // Select item
        m_catTree->SelectItem( item );
        // Scroll the specified item into view.
        m_catTree->ScrollTo( item );
    }

}; // namespace gw2b
