/** \file       WriteIndexTask.cpp
 *  \brief      Contains definition of the WriteIndexTask class.
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
#include "WriteIndexTask.h"

namespace gw2b {

    namespace {

        const uint IO_BATCH_SIZE = 8192;

    } // namespace

    WriteIndexTask::WriteIndexTask( const std::shared_ptr<DatIndex>& p_index, const wxFileName& p_filename )
        : m_index( p_index )
        , m_writer( *p_index )
        , m_filename( p_filename )
        , m_errorOccured( false )
        , m_workerStarted( false )
        , m_ioProgress( 0 )
        , m_workerDone( false ) {
        Ensure::notNull( p_index.get( ) );
    }

    WriteIndexTask::~WriteIndexTask( ) {
        this->joinWorker( );
    }

    bool WriteIndexTask::init( ) {
        if ( !m_filename.DirExists( ) ) {
            m_filename.Mkdir( 511, wxPATH_MKDIR_FULL );
        }

        if ( m_index->isDirty( ) ) {
            bool result = m_writer.open( m_filename.GetFullPath( ) );
            if ( result ) {
                this->setMaxProgress( m_writer.numEntries( ) + m_writer.numCategories( ) );
            }
            return result;
        }
        return false;
    }

    void WriteIndexTask::joinWorker( ) {
        if ( m_worker.joinable( ) ) {
            m_worker.join( );
        }
    }

    void WriteIndexTask::runWrite( ) {
        while ( !m_writer.isDone( ) ) {
            if ( !m_writer.write( IO_BATCH_SIZE ) ) {
                m_errorOccured = true;
                auto path = m_filename.GetFullPath( );
                if ( wxFile::Exists( path ) ) {
                    wxRemoveFile( path );
                }
                break;
            }

            m_ioProgress.store( m_writer.currentEntry( ) + m_writer.currentCategory( ) );
        }

        m_workerDone = true;
    }

    void WriteIndexTask::perform( ) {
        if ( m_errorOccured.load( ) ) {
            return;
        }

        if ( !m_workerStarted ) {
            m_workerStarted = true;
            m_worker = std::thread( &WriteIndexTask::runWrite, this );
            return;
        }

        this->setCurrentProgress( m_ioProgress.load( ) );
        this->setText( wxT( "Saving .dat index..." ) );

        if ( !m_workerDone.load( ) ) {
            return;
        }

        this->joinWorker( );
        this->setCurrentProgress( this->maxProgress( ) );

        if ( !m_errorOccured.load( ) ) {
            m_index->setDirty( false );
        }
    }

    void WriteIndexTask::abort( ) {
        this->joinWorker( );
        m_writer.close( );

        auto path = m_filename.GetFullPath( );
        if ( wxFile::Exists( path ) ) {
            wxRemoveFile( path );
        }
    }

    void WriteIndexTask::clean( ) {
        m_writer.close( );
    }

    bool WriteIndexTask::isDone( ) const {
        return m_errorOccured.load( ) || m_workerDone.load( );
    }

}; // namespace gw2b
