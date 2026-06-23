/** \file       BrowserWindow.h
 *  \brief      Contains declaration of the browser window.
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

#pragma once

#ifndef BROWSERWINDOW_H_INCLUDED
#define BROWSERWINDOW_H_INCLUDED

#include <wx/aui/aui.h>
#include <wx/filename.h>
#include <wx/splitter.h>
#include <wx/aboutdlg.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "CategoryTree.h"
#include "DatFile.h"
#include "PreviewPanel.h"
#include "PreviewGLCanvas.h"

namespace gw2b {
    class DatIndex;
    class PreviewPanel;
    class PreviewGLCanvas;
    class ProgressStatusBar;
    class Task;

    /** Event raised (on the UI thread) when an async file load completes. */
    wxDECLARE_EVENT( EVT_FILE_LOADED, wxThreadEvent );

    /** Result of an asynchronous file load, carried by EVT_FILE_LOADED. */
    struct FileLoadResult {
        uint                generation = 0;     /**< Request id, for discarding stale loads. */
        uint                fileNum = 0;        /**< MFT file entry number. */
        ANetFileType        fileType = ANFT_Unknown;
        wxString            name;               /**< Display name of the entry. */
        std::vector<byte>   data;               /**< Decompressed file contents. */
        bool                success = false;
    };

    /** Reads files from a DatFile on a dedicated worker thread so the UI does not
    *   block while the data is fetched and decompressed. Only the most recent
    *   request matters; older in-flight loads are superseded and discarded by the
    *   sink via the generation counter. The completed bytes are delivered to the
    *   given sink on the UI thread through an EVT_FILE_LOADED event. */
    class AsyncFileLoader {
    public:
        /** Constructor. Starts the worker thread (idle until a request arrives).
        *  \param[in]  p_datFile    .dat file to read from (must outlive this object).
        *  \param[in]  p_sink       Handler that receives EVT_FILE_LOADED events. */
        AsyncFileLoader( DatFile& p_datFile, wxEvtHandler* p_sink );
        /** Destructor. Stops and joins the worker thread. */
        ~AsyncFileLoader( );

        /** Queues an asynchronous load, superseding any pending request.
        *  \return uint    Generation assigned to this request. */
        uint request( uint p_fileNum, ANetFileType p_fileType, const wxString& p_name );
        /** Gets the most recent generation handed out. */
        uint currentGeneration( ) const {
            return m_generation.load( );
        }
        /** Cancels any pending request and blocks until an in-progress read (if
        *  any) finishes. Used before mutating the DatFile (e.g. opening a new one). */
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

        // Worker-thread owned read state.
        DatFile::ThreadContext      m_context;
        wxString                    m_contextPath;
        bool                        m_contextReady = false;
    }; // class AsyncFileLoader

    /** Represents the browser's main window. */
    class BrowserWindow : public wxFrame, public ICategoryTreeListener {
        wxString                    m_datPath;
        uint32                      m_datPathCrc;
        wxString                    m_activeIndexPath;
        DatFile                     m_datFile;
        std::shared_ptr<DatIndex>   m_index;
        ProgressStatusBar*          m_progress;
        Task*                       m_currentTask;
        wxAuiManager                m_uiManager;
        CategoryTree*               m_catTree;
        PreviewPanel*               m_previewPanel;
        PreviewGLCanvas*            m_previewGLCanvas;
        wxTextCtrl*                 m_log;
        wxLog*                      m_logTarget;
        wxTextCtrl*                 m_findTextBox;
        wxDialog*                   m_compareDialog;
        AsyncFileLoader             m_fileLoader;

    public:
        /** Constructs the frame with the given title and size.
        *  \param[in]  p_title  Title of window.
        *  \param[in]  p_size   Size of window. */
        BrowserWindow( const wxString& p_title, const wxSize p_size = wxDefaultSize );
        /** Destructor. */
        ~BrowserWindow( );
        /** Opens the given .dat file for browsing.
        *  \param[in]  p_path   Path to the .dat file to open. */
        void openFile( const wxString& p_path );
        /** Tries to close this window, but does not force it (same as calling
        *  Close(false)). */
        void tryClose( );
        /** Opens the preview pane with the given entry's contents in it.
        *  \param[in]  p_entry  entry to view. */
        void viewEntry( const DatIndexEntry& p_entry );
        /** Check if OpenGL context can be create. */
        bool OGLAvailable( );

    private:
        /** Performs the given task periodically, until it is done.
        *  \param[in]  p_task   Task to perform. Ownership is taken.
        *  \return bool    true if the task's init succeeded, false if not. */
        bool performTask( Task* p_task );

        /** Returns the directory where .dat index files are stored. */
        wxFileName indexStorageDir( ) const;
        /** Collects index file paths associated with the current .dat path. */
        wxArrayString collectIndexCandidates( ) const;
        /** Finds an on-disk index whose header matches the currently open .dat. */
        wxString findMatchingIndex( uint64 p_datTimestamp, uint64 p_datFingerprint, uint64 p_datFileSize ) const;
        /** Allocates a new index path using {pathCrc}_{indexedAt}.idx. */
        wxString allocateNewIndexPath( ) const;
        /** Stamps the current .dat metadata into the in-memory index. */
        void stampIndexMetadata( );
        /** Resumes indexing the loaded .dat file. */
        void indexDat( );
        /** Re-indexes the loaded .dat file. */
        void reIndexDat( );

        /** Executed when the user clicks <em>File -> Open</em> in the menu.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onOpenEvt( wxCommandEvent& p_event );
        /** Executed when the user clicks <em>File -> Exit</em> in the menu.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onExitEvt( wxCommandEvent& p_event );
        /** Executed when the user clicks <em>Help -> About</em> in the menu.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onAboutEvt( wxCommandEvent& p_event );
        /** Executed when the window is closing.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onCloseEvt( wxCloseEvent& p_event );
        /** Executed when the a button is pressed.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onButtonEvt( wxCommandEvent& p_event );
        /** Performs the currently active task repeatedly until it is complete.
        *  \param[in]  p_event  Idle event object used to request more idle events. */
        void onPerformTaskEvt( wxIdleEvent& p_event );
        /** Executed when the user clicks <em>View -> Menu</em> in the menu.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onTogglePaneEvt( wxCommandEvent &p_event );
        /** Executed when the user clicks <em>View -> Clear Log</em> in the menu.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onClearLogEvt( wxCommandEvent &p_event );
        /** Executed when the user clicks <em>File -> Compare With Index</em>.
        *  Loads another index file and shows what changed relative to the current one.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onCompareIndexEvt( wxCommandEvent &p_event );
        /** Executed when the user close aui pane.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onPaneCloseEvt( wxAuiManagerEvent &p_event );
        /** Executed when the user press enter key in search box.
        *  \param[in]  p_event  Unused event object handed to us by wxWidgets. */
        void onEnterPressedInSrchBoxEvt( wxCommandEvent &p_event );

        /** Raised when the index has been read. */
        void onReadIndexComplete( );
        /** Raised on the UI thread when an async file load finishes.
        *  \param[in]  p_event  Thread event carrying a FileLoadResult payload. */
        void onFileLoaded( wxThreadEvent& p_event );
        /** Builds and shows the appropriate viewer for already-read file data.
        *  \param[in]  p_fileType   File type of the data.
        *  \param[in]  p_data       Decompressed file contents. */
        void displayLoadedFile( ANetFileType p_fileType, const Array<byte>& p_data );

        /** Raised when the .dat has finished indexing. */
        void onScanTaskComplete( );
        /** Raised when the write task has finished, if invoked from onCloseEvt. */
        void onWriteTaskCloseCompleted( );

        /** Raised when the user clicks an item in the category tree.
        *  \param[in]  p_tree   tree that raised the event.
        *  \param[in]  p_entry  entry that was clicked. */
        virtual void onTreeEntryClicked( CategoryTree& p_tree, const DatIndexEntry& p_entry ) override;
        /** Raised when the user clicks a category in the category tree.
        *  \param[in]  p_tree       tree that raised the event.
        *  \param[in]  p_category   category that was clicked. */
        virtual void onTreeCategoryClicked( CategoryTree& p_tree, const DatIndexCategory& p_category ) override;
        /** Raised when the category tree was cleared.
        *  \param[in]  p_tree   tree that was cleared. */
        virtual void onTreeCleared( CategoryTree& p_tree ) override;
        /** Raised when the tree begins loading a category in the background.
        *  \param[in]  p_tree   tree that raised the event.
        *  \param[in]  p_total  total number of entries to load. */
        virtual void onTreeBackgroundLoadBegin( CategoryTree& p_tree, uint p_total ) override;
        /** Raised periodically while the tree loads a category in the background.
        *  \param[in]  p_tree     tree that raised the event.
        *  \param[in]  p_current  number of entries processed so far.
        *  \param[in]  p_total    total number of entries to load. */
        virtual void onTreeBackgroundLoadUpdate( CategoryTree& p_tree, uint p_current, uint p_total ) override;
        /** Raised when the tree's background load finishes or is cancelled.
        *  \param[in]  p_tree   tree that raised the event. */
        virtual void onTreeBackgroundLoadEnd( CategoryTree& p_tree ) override;
        /** Raised when the user wants to extract raw files.
        *  \param[in]  p_tree   category tree invoking the callback.
        *  \param[in]  p_mode   if false extract raw file, if true extract converted file. */
        virtual void onTreeExtractFile( CategoryTree& p_tree, bool p_mode ) override;

        /** Initialize about dialog data.*/
        void InitAboutInfo( wxAboutDialogInfo& info );
        /** Set menu default settings.*/
        void SetDefaults( );
        /** Call when "Go" button on find file panel is pressed. */
        void onFindFile( );
        /** Reveals an entry in the category tree by base id or file id. */
        void navigateToDiffEntry( uint p_baseId, uint p_fileId );

    }; // class BrowserWindow

}; // namespace gw2b

#endif // BROWSERWINDOW_H_INCLUDED
