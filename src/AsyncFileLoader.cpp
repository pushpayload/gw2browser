/** \file       AsyncFileLoader.cpp
 *  \brief      Asynchronous .dat file reader for preview panes.
 */

#include "stdafx.h"

#include "AsyncFileLoader.h"

namespace gw2b {

    wxDEFINE_EVENT( EVT_FILE_LOADED, wxThreadEvent );

    AsyncFileLoader::AsyncFileLoader( DatFile& p_datFile, wxEvtHandler* p_sink )
        : m_datFile( p_datFile )
        , m_sink( p_sink ) {
        m_thread = std::thread( &AsyncFileLoader::workerMain, this );
    }

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

    uint AsyncFileLoader::request( uint p_fileNum, ANetFileType p_fileType, const wxString& p_name ) {
        uint generation = ++m_generation;
        {
            std::lock_guard<std::mutex> lock( m_mutex );
            m_reqFileNum = p_fileNum;
            m_reqFileType = p_fileType;
            m_reqName = wxString( p_name.c_str( ) );
            m_reqGeneration = generation;
            m_hasRequest = true;
        }
        m_cv.notify_one( );
        return generation;
    }

    void AsyncFileLoader::cancelAndWait( ) {
        std::unique_lock<std::mutex> lock( m_mutex );
        m_hasRequest = false;
        ++m_generation;
        m_idleCv.wait( lock, [this] { return !m_busy; } );
    }

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

}; // namespace gw2b
