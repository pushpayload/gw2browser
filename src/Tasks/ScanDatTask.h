/** \file       ScanDatTask.h
 *  \brief      Contains declaration of the ScanDatTask class.
 *  \author     Rhoot
 */

/**
 * Copyright (C) 2014 Khralkatorrix <https://github.com/kytulendu>
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

#ifndef TASKS_SCANDATTASK_H_INCLUDED
#define TASKS_SCANDATTASK_H_INCLUDED

#include <atomic>
#include <thread>
#include <vector>

#include "ANetStructs.h"
#include "DatFile.h"
#include "Task.h"

namespace gw2b {
    class DatIndex;
    class DatIndexCategory;

    class ScanDatTask : public Task {
        struct ScanResult {
            uint32                  entryNumber;
            uint32                  baseId;
            uint32                  fileId;
            ANetFileType            fileType;
            wxString                displayName;
            std::vector<wxString>   categoryPath;
            bool                    valid;
        };

        std::shared_ptr<DatIndex>   m_index;
        DatFile&                    m_datFile;
        uint                        m_scanStart;
        uint                        m_scanEnd;
        bool                        m_batchUpdateActive;
        bool                        m_workerStarted;
        std::thread                 m_worker;
        std::atomic<uint>           m_scanProgress;
        std::atomic<bool>           m_workerDone;
        std::atomic<bool>           m_abortRequested;
    public:
        ScanDatTask( const std::shared_ptr<DatIndex>& p_index, DatFile& p_datFile );
        virtual ~ScanDatTask( );

        virtual bool init( ) override;
        virtual void perform( ) override;
        virtual void abort( ) override;
    private:
        uint requiredIdentificationSize( const byte* p_data, size_t p_size, ANetFileType p_fileType );
        bool isBitmapFontChunk( uint p_baseId );
        void buildCategoryPath( ANetFileType p_fileType, const byte* p_data, size_t p_size, uint p_entryNumber,
            DatFile::ThreadContext& p_context, std::vector<wxString>& p_path );
        ScanResult scanEntry( uint p_entryNumber, DatFile::ThreadContext& p_context, Array<byte>& p_buffer );
        void commitResult( const ScanResult& p_result );
        void endBatchUpdateIfNeeded( );
        void runScan( );
        void joinWorker( );
    }; // class ScanDatTask

}; // namespace gw2b

#endif // TASKS_SCANDATTASK_H_INCLUDED
