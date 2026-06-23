/** \file       AsyncFileLoader.h
 *  \brief      Asynchronous .dat file reader for preview panes.
 */

#pragma once

#ifndef ASYNCFILELOADER_H_INCLUDED
#define ASYNCFILELOADER_H_INCLUDED

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include <wx/event.h>
#include <wx/string.h>

#include "DatFile.h"

namespace gw2b {

    wxDECLARE_EVENT( EVT_FILE_LOADED, wxThreadEvent );

    /** Result of an asynchronous file load, carried by EVT_FILE_LOADED. */
    struct FileLoadResult {
        uint                generation = 0;
        uint                fileNum = 0;
        ANetFileType        fileType = ANFT_Unknown;
        wxString            name;
        std::vector<byte>   data;
        bool                success = false;
    };

    /** Reads files from a DatFile on a dedicated worker thread. */
    class AsyncFileLoader {
    public:
        AsyncFileLoader( DatFile& p_datFile, wxEvtHandler* p_sink );
        ~AsyncFileLoader( );

        uint request( uint p_fileNum, ANetFileType p_fileType, const wxString& p_name );
        uint currentGeneration( ) const {
            return m_generation.load( );
        }
        void cancelAndWait( );

    private:
        void workerMain( );

        DatFile&                    m_datFile;
        wxEvtHandler*               m_sink;

        std::thread                 m_thread;
        std::mutex                  m_mutex;
        std::condition_variable     m_cv;
        std::condition_variable     m_idleCv;

        bool                        m_stop = false;
        bool                        m_hasRequest = false;
        bool                        m_busy = false;

        uint                        m_reqFileNum = 0;
        ANetFileType                m_reqFileType = ANFT_Unknown;
        wxString                    m_reqName;
        uint                        m_reqGeneration = 0;

        std::atomic<uint>           m_generation{ 0 };

        DatFile::ThreadContext      m_context;
        wxString                    m_contextPath;
        bool                        m_contextReady = false;
    };

}; // namespace gw2b

#endif // ASYNCFILELOADER_H_INCLUDED
