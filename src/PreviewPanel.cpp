/** \file       PreviewPanel.cpp
 *  \brief      Contains definition of the preview panel control.
 *  \author     Rhoot
 */

/**
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

#include "DatFile.h"
#include "DatIndex.h"
#include "Exception.h"

#include "Viewers/BinaryViewer/BinaryViewer.h"
#include "Viewers/ImageViewer/ImageViewer.h"
#include "Viewers/StringViewer/StringViewer.h"
#include "Viewers/TextViewer/TextViewer.h"
#include "Viewers/SoundPlayer/SoundPlayer.h"

#include "PreviewPanel.h"

namespace gw2b {

    PreviewPanel::PreviewPanel( wxWindow* p_parent, const wxPoint& p_location, const wxSize& p_size )
        : wxPanel( p_parent, wxID_ANY, p_location, p_size )
        , m_currentView( nullptr )
        , m_currentDataType( FileReader::DT_None ) {
        // FINE I'LL USE A GOD DAMN SIZER STUPID WXWIDGETS
        auto sizer = new wxBoxSizer( wxHORIZONTAL );
        this->SetSizer( sizer );
    }

    PreviewPanel::~PreviewPanel( ) {
    }

    bool PreviewPanel::previewFile( DatFile& p_datFile, const DatIndexEntry& p_entry ) {
        auto entryData = p_datFile.readFile( p_entry.mftEntry( ) );
        return this->previewData( p_datFile, p_entry.fileType( ), entryData );
    }

    bool PreviewPanel::previewData( DatFile& p_datFile, ANetFileType p_fileType, const Array<byte>& p_data ) {
        if ( !p_data.GetSize( ) ) {
            return false;
        }

        // Create file reader
        auto reader = FileReader::readerForData( p_data, p_datFile, p_fileType );

        if ( reader ) {
            if ( m_currentView ) {
                // Check if we can re-use the current viewer
                if ( m_currentDataType == reader->dataType( ) ) {
                    m_currentView->setReader( reader );
                    return true;
                }

                // Destroy the old viewer
                if ( m_currentView ) {
                    this->GetSizer( )->Remove( 0 );
                    m_currentView->Destroy( );
                }
            }

            m_currentView = this->createViewerForDataType( reader->dataType( ), p_datFile );
            if ( m_currentView ) {
                // Workaround for wxWidgets fuckups
                this->GetSizer( )->Add( m_currentView, wxSizerFlags( ).Expand( ).Proportion( 1 ) );
                this->GetSizer( )->Layout( );
                this->GetSizer( )->Fit( this );
                // Set the reader
                m_currentView->setReader( reader );
                m_currentDataType = reader->dataType( );
                return true;
            }
        }

        return false;
    }

    void PreviewPanel::destroyViewer( ) {
        // Destroy the viewer
        if ( m_currentView ) {
            this->GetSizer( )->Remove( 0 );
            m_currentView->Destroy( );
            m_currentView = nullptr;
        }
    }

    Viewer* PreviewPanel::createViewerForDataType( FileReader::DataType p_dataType, DatFile& p_datFile ) {
        Viewer* newViewer = nullptr;
        try {
            switch ( p_dataType ) {
            case FileReader::DT_Sound:
                newViewer = new SoundPlayer( this );
                break;
            case FileReader::DT_Image:
                newViewer = new ImageViewer( this );
                break;
            case FileReader::DT_String:
                newViewer = new StringViewer( this );
                break;
            case FileReader::DT_EULA:
            case FileReader::DT_Text:
                newViewer = new TextViewer( this );
                break;
            case FileReader::DT_Binary:
            default:
                newViewer = new BinaryViewer( this );
                break;
            }
        } catch ( const exception::Exception& err ) {
            wxLogMessage( wxString( err.what( ) ) );
        }

        return newViewer;
    }

}; // namespace gw2b
