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

    BrowserWindow::BrowserWindow( const wxString& p_title, const wxSize p_size )
        : wxFrame( nullptr, wxID_ANY, p_title, wxDefaultPosition, p_size )
        , m_index( std::make_shared<DatIndex>( ) )
        , m_progress( nullptr )
        , m_currentTask( nullptr )
        , m_catTree( nullptr )
        , m_previewPanel( nullptr )
        , m_previewGLCanvas( nullptr ) {
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

        // Fix bug when open other dat file and find by file id not working.
        m_findFirstTime = true;
    }

    //============================================================================/

    void BrowserWindow::viewEntry( const DatIndexEntry& p_entry ) {
        switch ( p_entry.fileType( ) ) {
        //case ANFT_MapParam:
        case ANFT_Model:
            if ( m_previewGLCanvas->previewFile( m_datFile, p_entry ) ) {
                m_previewPanel->destroyViewer( );
                m_uiManager.GetPane( wxT( "panel_content" ) ).Hide( );
                m_uiManager.GetPane( wxT( "gl_content" ) ).Show( );
            }
            break;
        default:
            if ( m_previewPanel->previewFile( m_datFile, p_entry ) ) {
                // Clear the OpenGL canvas to reduce memory usage
                m_previewGLCanvas->clear( );
                m_uiManager.GetPane( wxT( "gl_content" ) ).Hide( );
                m_uiManager.GetPane( wxT( "panel_content" ) ).Show( );
            }
        }
        m_uiManager.Update( );
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
        }
    }

    //============================================================================/

    void BrowserWindow::onScanTaskComplete( ) {
        m_catTree->setDatIndex( m_index );

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

    void BrowserWindow::onTreeExtractFile( CategoryTree& p_tree, bool p_mode ) {
        auto entries = p_tree.getSelectedEntries( );
        Exporter *exporter;

        if ( entries.GetSize( ) ) {
            if ( p_mode ) {
                exporter = new Exporter( entries, m_datFile, Exporter::EM_Converted );
            } else {
                exporter = new Exporter( entries, m_datFile, Exporter::EM_Raw );
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
        wxArrayTreeItemIds Selections;
        wxTreeItemId item;
        wxTreeItemIdValue cookie;

        if ( m_findFirstTime ) {
            // Warning: This would slow down the browser, only use on first time
            m_catTree->ExpandAll( );
            m_catTree->CollapseAll( );

            m_findFirstTime = false;
        }

        wxString value = m_findTextBox->GetValue( );
        if ( value.IsEmpty( ) || !value.IsNumber( ) ) {
            wxMessageBox( wxT( "Please enter file id in number." ), wxT( " " ), wxOK | wxICON_EXCLAMATION, this );
            return;
        }

        //if ( m_catTree->GetSelections( Selections ) ) {
        //  item = Selections[0];
        //} else {
            item = m_catTree->GetFirstChild( m_catTree->GetRootItem( ), cookie );
        //}

        item = m_catTree->findEntry( item, value );
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
