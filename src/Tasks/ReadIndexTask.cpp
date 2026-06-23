/** \file       ReadIndexTask.cpp
 *  \brief      Contains declaration of the ReadIndexTask class.
 *  \author     Rhoot
 */

/**
 * Copyright (C) 2014-2016 Khralkatorrix <https://github.com/kytulendu>
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
#include "ReadIndexTask.h"

namespace gw2b {

    namespace {

        const uint IO_BATCH_SIZE = 8192;

    } // namespace

    ReadIndexTask::ReadIndexTask( const std::shared_ptr<DatIndex>& p_index, const wxString& p_filename,
        uint64 p_datTimestamp, uint64 p_datFingerprint, uint64 p_datFileSize, uint32 p_datPathCrc )
        : m_index( p_index )
        , m_reader( *p_index )
        , m_filename( p_filename )
        , m_errorOccured( false )
        , m_datTimestamp( p_datTimestamp )
        , m_datFingerprint( p_datFingerprint )
        , m_datFileSize( p_datFileSize )
        , m_datPathCrc( p_datPathCrc )
        , m_batchUpdateActive( false )
        , m_workerStarted( false )
        , m_ioProgress( 0 )
        , m_workerDone( false )
        , m_abortRequested( false ) {
        Ensure::notNull( p_index.get( ) );
    }

    ReadIndexTask::~ReadIndexTask( ) {
        m_abortRequested = true;
        this->joinWorker( );
        this->endBatchUpdateIfNeeded( );
    }

    bool ReadIndexTask::init( ) {
        m_index->clear( );
        m_index->setDirty( false );

        bool result = m_reader.open( m_filename );
        if ( result ) {
            DatIndexMetadata metadata;
            metadata.datTimestamp = m_index->datTimestamp( );
            metadata.datFingerprint = m_index->datFingerprint( );
            metadata.datFileSize = m_index->datFileSize( );
            metadata.datPathCrc = m_index->datPathCrc( );
            result = metadata.matchesDat( m_datFingerprint, m_datTimestamp, m_datFileSize, m_datPathCrc )
                && metadata.isAssociatedWithPath( m_datPathCrc, m_filename );
        }
        if ( result ) {
            m_index->beginBatchUpdate( );
            m_batchUpdateActive = true;
            this->setMaxProgress( m_reader.numEntries( ) + m_reader.numCategories( ) );
        }

        return result;
    }

    void ReadIndexTask::joinWorker( ) {
        if ( m_worker.joinable( ) ) {
            m_worker.join( );
        }
    }

    void ReadIndexTask::endBatchUpdateIfNeeded( ) {
        if ( m_batchUpdateActive ) {
            m_index->endBatchUpdate( );
            m_batchUpdateActive = false;
        }
    }

    void ReadIndexTask::runRead( ) {
        while ( !m_reader.isDone( ) && !m_abortRequested.load( ) ) {
            auto result = m_reader.read( IO_BATCH_SIZE );
            m_ioProgress.store( m_reader.currentEntry( ) + m_reader.currentCategory( ) );

            if ( !( result & DatIndexReader::RR_Success ) ) {
                m_errorOccured = true;
                m_index->clear( );
                break;
            }
        }

        m_workerDone = true;
    }

    void ReadIndexTask::perform( ) {
        if ( m_errorOccured.load( ) ) {
            return;
        }

        if ( !m_workerStarted ) {
            m_workerStarted = true;
            m_worker = std::thread( &ReadIndexTask::runRead, this );
            return;
        }

        this->setCurrentProgress( m_ioProgress.load( ) );
        this->setText( wxT( "Reading .dat index..." ) );

        if ( !m_workerDone.load( ) ) {
            return;
        }

        this->joinWorker( );
        this->setCurrentProgress( this->maxProgress( ) );
        this->endBatchUpdateIfNeeded( );
    }

    void ReadIndexTask::abort( ) {
        m_abortRequested = true;
        this->joinWorker( );

        if ( m_errorOccured.load( ) ) {
            m_index->clear( );
        }

        this->endBatchUpdateIfNeeded( );
        this->clean( );
    }

    void ReadIndexTask::clean( ) {
        m_reader.close( );
    }

    bool ReadIndexTask::isDone( ) const {
        return m_errorOccured.load( ) || m_workerDone.load( );
    }

}; // namespace gw2b
