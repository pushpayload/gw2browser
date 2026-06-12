/** \file       Exporter.h
 *  \brief      Contains the declaration of the base class of exporters for the
 *              various file types.
 *  \author     Khralkatorrix
 */

/**
 * Copyright (C) 2016-2019 Khralkatorrix <https://github.com/kytulendu>
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

#ifndef EXPORTER_H_INCLUDED
#define EXPORTER_H_INCLUDED

#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/progdlg.h>

#include "Util/Array.h"
#include "ANetStructs.h"
#include "FileReader.h"

namespace gw2b {
    class DatFile;
    class DatIndexCategory;
    class DatIndexEntry;

    class Exporter : public wxFrame {
    public:
        enum ExtractionMode {
            EM_Raw,
            EM_Converted,
        };

    private:
        DatFile&                    m_datFile;
        wxWindow*                   m_owner;
        Array<const DatIndexEntry*> m_entries;
        wxProgressDialog*           m_progress;
        uint                        m_currentProgress;
        wxString                    m_path;
        wxFileName                  m_filename;
        ExtractionMode              m_mode;
        ANetFileType                m_fileType;

    public:
        /** Constructor.
        *  \param[in]  p_owner         Window to parent the export dialogs to.
        *  \param[in]  p_entries       Entry to extract.
        *  \param[in]  p_datFile       .dat file containing the file.
        *  \param[in]  p_mode          File extract mode.*/
        Exporter( wxWindow* p_owner, const Array<const DatIndexEntry*>& p_entries, DatFile& p_datFile, ExtractionMode p_mode );

    private:
        /** Gets an appropriate file extension for the contents.
        *  \return wxString             File extension. */
        const wxChar* GetExtension( ) const;
        const wxString GetWildcard( ) const;
        void extractFile( const DatIndexEntry& p_entry );
        void exportImage( FileReader* p_reader, const wxString& p_entryname );
        void exportString( FileReader* p_reader, const wxString& p_entryname );
        void exportEula( FileReader* p_reader, const wxString& p_entryname );
        void exportSound( FileReader* p_reader, const wxString& p_entryname );
        void exportSoundBank( FileReader* p_reader, const wxString& p_entryname );
        void exportModel( FileReader* p_reader, const wxString& p_entryname );
        void exportModelTexture( uint32 p_fileid );
        void exportGameContent( FileReader* p_reader, const wxString& p_entryname );
        void exportBitmapFont( FileReader* p_reader, const wxString& p_entryname );
        void writeImage( wxImage p_image );
        void writeXML( std::unique_ptr<tinyxml2::XMLDocument> p_xml );
        bool writeFile( const Array<byte>& p_data );
        void appendPaths( wxFileName& p_path, const DatIndexCategory& p_category );

    };

}; // namespace gw2b

#endif // EXPORTER_H_INCLUDED
