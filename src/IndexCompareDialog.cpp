/** \file       IndexCompareDialog.cpp
 *  \brief      Modeless index comparison dialog with preview pane.
 */

#include "stdafx.h"

#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/statline.h>
#include <wx/textfile.h>

#include "IndexCompareDialog.h"
#include "PreviewGLCanvas.h"
#include "PreviewPanel.h"

namespace gw2b {

    namespace {

        wxGLAttributes createPreviewGlAttributes( ) {
            wxGLAttributes vAttrs;
            vAttrs.PlatformDefaults( ).Defaults( ).EndList( );
            if ( !wxGLCanvas::IsDisplaySupported( vAttrs ) ) {
                vAttrs.Reset( );
                vAttrs.PlatformDefaults( ).RGBA( ).DoubleBuffer( ).Depth( 16 ).EndList( );
            }
            return vAttrs;
        }

        void copyTextToClipboard( const wxString& p_text ) {
            if ( !wxTheClipboard->Open( ) ) {
                return;
            }
            wxTheClipboard->SetData( new wxTextDataObject( p_text ) );
            wxTheClipboard->Close( );
        }

        wxString formatDiffRowFileId( const DiffRow& p_row ) {
            if ( p_row.fileId ) {
                return wxString::Format( wxT( "%u" ), p_row.fileId );
            }
            if ( p_row.baseId ) {
                return wxString::Format( wxT( "%u" ), p_row.baseId );
            }
            return p_row.name;
        }

        wxString formatDiffRowForClipboard( const DiffRow& p_row ) {
            wxString statusStr = ( p_row.status == DS_Added ) ? wxT( "Added" )
                : ( p_row.status == DS_Removed ) ? wxT( "Removed" ) : wxT( "Changed" );
            wxString mft = ( p_row.status == DS_Added ) ? wxString::Format( wxT( "%u" ), p_row.mftNew )
                : ( p_row.status == DS_Removed ) ? wxString::Format( wxT( "%u" ), p_row.mftOld )
                : wxString::Format( wxT( "%u > %u" ), p_row.mftOld, p_row.mftNew );
            return wxString::Format( wxT( "%s\t%s\t%s\t%s\t%s" ),
                statusStr, p_row.name, formatDiffRowFileId( p_row ), mft, p_row.category );
        }

    } // namespace

    uint64 diffEntryKey( const DatIndexEntry* p_entry ) {
        if ( p_entry->baseId( ) ) {
            return static_cast<uint64>( p_entry->baseId( ) );
        }
        if ( p_entry->fileId( ) ) {
            return ( static_cast<uint64>( 1 ) << 40 ) | p_entry->fileId( );
        }
        return ( static_cast<uint64>( 1 ) << 41 ) | p_entry->mftEntry( );
    }

    wxString diffCategoryPath( const DatIndexCategory* p_category ) {
        wxString path;
        for ( auto category = p_category; category != nullptr; category = category->parent( ) ) {
            if ( path.IsEmpty( ) ) {
                path = category->name( );
            } else {
                path = category->name( ) + wxT( "/" ) + path;
            }
        }
        return path;
    }

    IndexCompareDialog::IndexCompareDialog( wxWindow* p_parent, DatFile& p_datFile,
        const std::shared_ptr<DatIndex>& p_index, std::shared_ptr<std::vector<DiffRow>> p_rows,
        const wxString& p_summary, size_t p_displayCap, NavigateCallback p_onNavigate )
        : wxDialog( p_parent, wxID_ANY, wxT( "Index Comparison" ), wxDefaultPosition, wxSize( 1180, 640 ),
            wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER )
        , m_datFile( p_datFile )
        , m_index( p_index )
        , m_rows( std::move( p_rows ) )
        , m_onNavigate( std::move( p_onNavigate ) )
        , m_displayCap( p_displayCap )
        , m_list( nullptr )
        , m_previewStatus( nullptr )
        , m_previewPanel( nullptr )
        , m_previewGLCanvas( nullptr )
        , m_fileLoader( p_datFile, this ) {
        auto rootSizer = new wxBoxSizer( wxVERTICAL );

        rootSizer->Add( new wxStaticText( this, wxID_ANY, p_summary ), wxSizerFlags( ).Border( wxALL, 8 ) );
        rootSizer->Add( new wxStaticText( this, wxID_ANY,
            wxT( "Select a row to preview. Double-click Added or Changed rows to jump in the main window. Right-click to copy." ) ),
            wxSizerFlags( ).Border( wxLEFT | wxRIGHT | wxBOTTOM, 8 ) );

        auto splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_LIVE_UPDATE );
        auto listPanel = new wxPanel( splitter );
        auto previewHost = new wxPanel( splitter );

        m_list = new wxListCtrl( listPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
            wxLC_REPORT | wxLC_SINGLE_SEL | wxLC_HRULES );
        m_list->AppendColumn( wxT( "Status" ), wxLIST_FORMAT_LEFT, 75 );
        m_list->AppendColumn( wxT( "Name" ), wxLIST_FORMAT_LEFT, 130 );
        m_list->AppendColumn( wxT( "File ID" ), wxLIST_FORMAT_LEFT, 80 );
        m_list->AppendColumn( wxT( "MFT (old > new)" ), wxLIST_FORMAT_LEFT, 130 );
        m_list->AppendColumn( wxT( "Category" ), wxLIST_FORMAT_LEFT, 220 );

        auto listSizer = new wxBoxSizer( wxVERTICAL );
        listSizer->Add( m_list, wxSizerFlags( 1 ).Expand( ).Border( wxALL, 4 ) );
        listPanel->SetSizer( listSizer );

        m_previewStatus = new wxStaticText( previewHost, wxID_ANY, wxT( "Select a row to preview." ) );
        auto previewContent = new wxPanel( previewHost );
        auto previewContentSizer = new wxBoxSizer( wxVERTICAL );
        m_previewPanel = new PreviewPanel( previewContent );
        auto glAttrs = createPreviewGlAttributes( );
        if ( wxGLCanvas::IsDisplaySupported( glAttrs ) ) {
            m_previewGLCanvas = new PreviewGLCanvas( previewContent, glAttrs );
            m_previewGLCanvas->Hide( );
        }
        previewContentSizer->Add( m_previewPanel, wxSizerFlags( 1 ).Expand( ) );
        if ( m_previewGLCanvas ) {
            previewContentSizer->Add( m_previewGLCanvas, wxSizerFlags( 1 ).Expand( ) );
        }
        previewContent->SetSizer( previewContentSizer );

        auto previewSizer = new wxBoxSizer( wxVERTICAL );
        previewSizer->Add( new wxStaticText( previewHost, wxID_ANY, wxT( "Preview" ) ), wxSizerFlags( ).Border( wxLEFT | wxTOP | wxRIGHT, 8 ) );
        previewSizer->Add( m_previewStatus, wxSizerFlags( ).Border( wxLEFT | wxRIGHT, 8 ) );
        previewSizer->Add( previewContent, wxSizerFlags( 1 ).Expand( ).Border( wxALL, 8 ) );
        previewHost->SetSizer( previewSizer );

        splitter->SplitVertically( listPanel, previewHost, 620 );
        splitter->SetMinimumPaneSize( 220 );
        rootSizer->Add( splitter, wxSizerFlags( 1 ).Expand( ).Border( wxLEFT | wxRIGHT, 8 ) );

        if ( m_rows->size( ) > m_displayCap ) {
            auto note = wxString::Format(
                wxT( "Showing the first %u of %u differences. Use \"Save to CSV...\" to export them all." ),
                static_cast<uint>( m_displayCap ), static_cast<uint>( m_rows->size( ) ) );
            rootSizer->Add( new wxStaticText( this, wxID_ANY, note ), wxSizerFlags( ).Border( wxALL, 8 ) );
        }

        auto btnSizer = new wxBoxSizer( wxHORIZONTAL );
        btnSizer->Add( new wxButton( this, wxID_SAVE, wxT( "Save to CSV..." ) ), wxSizerFlags( ).Border( wxALL, 8 ) );
        btnSizer->AddStretchSpacer( );
        btnSizer->Add( new wxButton( this, wxID_CLOSE, wxT( "Close" ) ), wxSizerFlags( ).Border( wxALL, 8 ) );
        rootSizer->Add( btnSizer, wxSizerFlags( ).Expand( ) );

        this->SetSizer( rootSizer );
        this->populateList( );

        m_list->Bind( wxEVT_LIST_ITEM_SELECTED, &IndexCompareDialog::onListSelect, this );
        m_list->Bind( wxEVT_LIST_ITEM_ACTIVATED, &IndexCompareDialog::onListActivate, this );
        m_list->Bind( wxEVT_LIST_ITEM_RIGHT_CLICK, &IndexCompareDialog::onListRightClick, this );
        this->Bind( EVT_FILE_LOADED, &IndexCompareDialog::onFileLoaded, this );
        this->Bind( wxEVT_BUTTON, &IndexCompareDialog::onSaveCsv, this, wxID_SAVE );
        this->Bind( wxEVT_BUTTON, &IndexCompareDialog::onClose, this, wxID_CLOSE );
        this->Bind( wxEVT_CLOSE_WINDOW, &IndexCompareDialog::onCloseWindow, this );
        this->showPreviewMessage( wxT( "Select a row to preview." ) );
    }

    IndexCompareDialog::~IndexCompareDialog( ) {
        m_fileLoader.cancelAndWait( );
    }

    const DiffRow* IndexCompareDialog::getRowAt( long p_listIndex ) const {
        if ( p_listIndex < 0 || !m_list ) {
            return nullptr;
        }
        auto rowIndex = m_list->GetItemData( p_listIndex );
        if ( rowIndex < 0 || static_cast<size_t>( rowIndex ) >= m_rows->size( ) ) {
            return nullptr;
        }
        return &( *m_rows )[static_cast<size_t>( rowIndex )];
    }

    const DatIndexEntry* IndexCompareDialog::findEntry( const DiffRow& p_row ) const {
        if ( !m_index ) {
            return nullptr;
        }
        if ( p_row.baseId != 0 ) {
            return m_index->findEntryByBaseId( p_row.baseId );
        }
        if ( p_row.fileId != 0 ) {
            for ( uint i = 0; i < m_index->numEntries( ); i++ ) {
                auto entry = m_index->entry( i );
                if ( entry && entry->fileId( ) == p_row.fileId ) {
                    return entry;
                }
            }
        }
        for ( uint i = 0; i < m_index->numEntries( ); i++ ) {
            auto entry = m_index->entry( i );
            if ( entry && entry->mftEntry( ) == p_row.mftNew ) {
                return entry;
            }
        }
        return nullptr;
    }

    void IndexCompareDialog::populateList( ) {
        size_t displayCount = std::min( m_rows->size( ), m_displayCap );

        m_list->Freeze( );
        for ( size_t i = 0; i < displayCount; i++ ) {
            auto const& row = ( *m_rows )[i];
            wxString statusStr = ( row.status == DS_Added ) ? wxT( "Added" )
                : ( row.status == DS_Removed ) ? wxT( "Removed" ) : wxT( "Changed" );
            long idx = m_list->InsertItem( static_cast<long>( i ), statusStr );
            m_list->SetItem( idx, 1, row.name );
            m_list->SetItem( idx, 2, formatDiffRowFileId( row ) );
            wxString mft = ( row.status == DS_Added ) ? wxString::Format( wxT( "%u" ), row.mftNew )
                : ( row.status == DS_Removed ) ? wxString::Format( wxT( "%u" ), row.mftOld )
                : wxString::Format( wxT( "%u > %u" ), row.mftOld, row.mftNew );
            m_list->SetItem( idx, 3, mft );
            m_list->SetItem( idx, 4, row.category );
            m_list->SetItemData( idx, static_cast<long>( i ) );
            wxColour colour = ( row.status == DS_Added ) ? wxColour( 0, 128, 0 )
                : ( row.status == DS_Removed ) ? wxColour( 176, 0, 0 ) : wxColour( 180, 120, 0 );
            m_list->SetItemTextColour( idx, colour );
        }
        m_list->Thaw( );
    }

    void IndexCompareDialog::showPreviewMessage( const wxString& p_message ) {
        m_previewPanel->destroyViewer( );
        m_previewPanel->Hide( );
        if ( m_previewGLCanvas ) {
            m_previewGLCanvas->clear( );
            m_previewGLCanvas->Hide( );
        }
        m_previewStatus->SetLabel( p_message );
        m_previewStatus->GetParent( )->Layout( );
    }

    void IndexCompareDialog::showPreviewViews( bool p_useGlCanvas ) {
        m_previewStatus->SetLabel( wxEmptyString );
        if ( p_useGlCanvas && m_previewGLCanvas ) {
            m_previewPanel->destroyViewer( );
            m_previewPanel->Hide( );
            m_previewGLCanvas->Show( );
        } else {
            if ( m_previewGLCanvas ) {
                m_previewGLCanvas->clear( );
                m_previewGLCanvas->Hide( );
            }
            m_previewPanel->Show( );
        }
        m_previewPanel->GetParent( )->Layout( );
    }

    void IndexCompareDialog::previewRow( const DiffRow* p_row ) {
        if ( !p_row ) {
            this->showPreviewMessage( wxT( "Select a row to preview." ) );
            return;
        }
        if ( p_row->status == DS_Removed ) {
            this->showPreviewMessage( wxT( "Removed from the current index. Preview is only available for Added and Changed rows." ) );
            return;
        }

        auto entry = this->findEntry( *p_row );
        if ( !entry ) {
            this->showPreviewMessage( wxT( "Could not find this entry in the current index." ) );
            return;
        }

        m_previewStatus->SetLabel( wxString::Format( wxT( "Loading %s..." ), entry->name( ) ) );
        m_fileLoader.request( entry->mftEntry( ), entry->fileType( ), entry->name( ) );
    }

    void IndexCompareDialog::displayLoadedFile( ANetFileType p_fileType, const Array<byte>& p_data ) {
        switch ( p_fileType ) {
        case ANFT_Model:
            this->showPreviewMessage(
                wxT( "Model preview is not available in index comparison. Images, textures, audio, and text are supported." ) );
            break;
        default:
            if ( m_previewPanel->previewData( m_datFile, p_fileType, p_data ) ) {
                this->showPreviewViews( false );
            } else {
                this->showPreviewMessage( wxT( "Failed to preview this file." ) );
            }
            break;
        }
    }

    void IndexCompareDialog::onListSelect( wxListEvent& p_event ) {
        this->previewRow( this->getRowAt( p_event.GetIndex( ) ) );
        p_event.Skip( );
    }

    void IndexCompareDialog::onListActivate( wxListEvent& p_event ) {
        auto const* row = this->getRowAt( p_event.GetIndex( ) );
        if ( !row || ( row->status != DS_Added && row->status != DS_Changed ) ) {
            return;
        }
        if ( m_onNavigate ) {
            m_onNavigate( row->baseId, row->fileId );
        }
    }

    void IndexCompareDialog::onListRightClick( wxListEvent& p_event ) {
        auto const* row = this->getRowAt( p_event.GetIndex( ) );
        if ( !row ) {
            return;
        }

        wxMenu menu;
        auto copyFileId = menu.Append( wxID_ANY, wxT( "Copy File ID" ) );
        auto copyRow = menu.Append( wxID_ANY, wxT( "Copy Row" ) );

        menu.Bind( wxEVT_MENU, [row] ( wxCommandEvent& ) {
            copyTextToClipboard( formatDiffRowFileId( *row ) );
        }, copyFileId->GetId( ) );
        menu.Bind( wxEVT_MENU, [row] ( wxCommandEvent& ) {
            copyTextToClipboard( formatDiffRowForClipboard( *row ) );
        }, copyRow->GetId( ) );

        m_list->PopupMenu( &menu, p_event.GetPoint( ) );
    }

    void IndexCompareDialog::onFileLoaded( wxThreadEvent& p_event ) {
        auto result = p_event.GetPayload<std::shared_ptr<FileLoadResult>>( );
        if ( !result ) {
            return;
        }
        if ( result->generation != m_fileLoader.currentGeneration( ) ) {
            return;
        }
        if ( !result->success || result->data.empty( ) ) {
            this->showPreviewMessage( wxT( "Failed to load file data for preview." ) );
            return;
        }

        Array<byte> data( result->data.size( ) );
        ::memcpy( data.GetPointer( ), result->data.data( ), result->data.size( ) );
        this->displayLoadedFile( result->fileType, data );
    }

    void IndexCompareDialog::onSaveCsv( wxCommandEvent& WXUNUSED( p_event ) ) {
        wxFileDialog save( this, wxT( "Save comparison" ), wxEmptyString, wxT( "index-diff.csv" ),
            wxT( "CSV file (*.csv)|*.csv|All files (*.*)|*.*" ), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );
        if ( save.ShowModal( ) != wxID_OK ) {
            return;
        }
        wxTextFile file( save.GetPath( ) );
        if ( file.Exists( ) ) {
            file.Open( );
        } else {
            file.Create( );
        }
        file.Clear( );
        file.AddLine( wxT( "Status,Name,FileId,BaseId,MftOld,MftNew,Category" ) );
        for ( auto const& row : *m_rows ) {
            wxString statusStr = ( row.status == DS_Added ) ? wxT( "Added" )
                : ( row.status == DS_Removed ) ? wxT( "Removed" ) : wxT( "Changed" );
            file.AddLine( wxString::Format( wxT( "%s,%s,%u,%u,%u,%u,\"%s\"" ),
                statusStr, row.name, row.fileId, row.baseId, row.mftOld, row.mftNew, row.category ) );
        }
        file.Write( );
        file.Close( );
    }

    void IndexCompareDialog::onClose( wxCommandEvent& WXUNUSED( p_event ) ) {
        this->Destroy( );
    }

    void IndexCompareDialog::onCloseWindow( wxCloseEvent& WXUNUSED( p_event ) ) {
        this->Destroy( );
    }

}; // namespace gw2b
