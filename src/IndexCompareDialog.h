/** \file       IndexCompareDialog.h
 *  \brief      Modeless index comparison dialog with preview pane.
 */

#pragma once

#ifndef INDEXCOMPAREDIALOG_H_INCLUDED
#define INDEXCOMPAREDIALOG_H_INCLUDED

#include <functional>
#include <memory>
#include <vector>

#include <wx/dialog.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/string.h>

#include "AsyncFileLoader.h"
#include "DatFile.h"
#include "DatIndex.h"

namespace gw2b {
    class PreviewGLCanvas;
    class PreviewPanel;

    enum DiffStatus {
        DS_Added,
        DS_Changed,
        DS_Removed,
    };

    struct DiffRow {
        DiffStatus  status;
        uint        baseId;
        uint        fileId;
        uint        mftOld;
        uint        mftNew;
        wxString    name;
        wxString    category;
    };

    uint64 diffEntryKey( const DatIndexEntry* p_entry );
    wxString diffCategoryPath( const DatIndexCategory* p_category );

    class IndexCompareDialog : public wxDialog {
    public:
        using NavigateCallback = std::function<void( uint, uint )>;

        IndexCompareDialog( wxWindow* p_parent, DatFile& p_datFile, const std::shared_ptr<DatIndex>& p_index,
            std::shared_ptr<std::vector<DiffRow>> p_rows, const wxString& p_summary, size_t p_displayCap,
            NavigateCallback p_onNavigate );
        ~IndexCompareDialog( ) override;

    private:
        DatFile&                                m_datFile;
        std::shared_ptr<DatIndex>               m_index;
        std::shared_ptr<std::vector<DiffRow>>   m_rows;
        NavigateCallback                        m_onNavigate;
        size_t                                  m_displayCap;

        wxListCtrl*                             m_list;
        wxStaticText*                           m_previewStatus;
        PreviewPanel*                           m_previewPanel;
        PreviewGLCanvas*                        m_previewGLCanvas;
        AsyncFileLoader                         m_fileLoader;

        const DiffRow* getRowAt( long p_listIndex ) const;
        const DatIndexEntry* findEntry( const DiffRow& p_row ) const;
        void populateList( );
        void previewRow( const DiffRow* p_row );
        void showPreviewMessage( const wxString& p_message );
        void displayLoadedFile( ANetFileType p_fileType, const Array<byte>& p_data );
        void showPreviewViews( bool p_useGlCanvas );

        void onListSelect( wxListEvent& p_event );
        void onListActivate( wxListEvent& p_event );
        void onListRightClick( wxListEvent& p_event );
        void onFileLoaded( wxThreadEvent& p_event );
        void onSaveCsv( wxCommandEvent& p_event );
        void onClose( wxCommandEvent& p_event );
        void onCloseWindow( wxCloseEvent& p_event );
    };

}; // namespace gw2b

#endif // INDEXCOMPAREDIALOG_H_INCLUDED
