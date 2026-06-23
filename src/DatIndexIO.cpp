/** \file       DatIndexIO.cpp
 *  \brief      Contains the definition for the index reader and writer classes.
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

#include "DatIndexIO.h"

namespace gw2b {

    namespace {

        const size_t MIN_HEADER_SIZE = sizeof( DatIndexHead );
        const size_t MIN_V3_SIZE = MIN_HEADER_SIZE + sizeof( uint64 );
        const size_t MIN_V4_SIZE = MIN_V3_SIZE + sizeof( uint32 ) + sizeof( uint64 ) + sizeof( uint64 );

        bool readMetadataFields( wxFile& p_file, const DatIndexHead& p_header, DatIndexMetadata& p_metadata ) {
            p_metadata.version = p_header.version;
            p_metadata.datTimestamp = p_header.datTimestamp;
            p_metadata.numEntries = p_header.numEntries;

            if ( p_header.version >= DatIndex_VersionFingerprint ) {
                if ( p_file.Read( &p_metadata.datFingerprint, sizeof( p_metadata.datFingerprint ) )
                    < static_cast<ssize_t>( sizeof( p_metadata.datFingerprint ) ) ) {
                    return false;
                }
            }

            if ( p_header.version >= DatIndex_VersionExtended ) {
                if ( p_file.Read( &p_metadata.datPathCrc, sizeof( p_metadata.datPathCrc ) )
                    < static_cast<ssize_t>( sizeof( p_metadata.datPathCrc ) ) ) {
                    return false;
                }
                if ( p_file.Read( &p_metadata.datFileSize, sizeof( p_metadata.datFileSize ) )
                    < static_cast<ssize_t>( sizeof( p_metadata.datFileSize ) ) ) {
                    return false;
                }
                if ( p_file.Read( &p_metadata.indexedAt, sizeof( p_metadata.indexedAt ) )
                    < static_cast<ssize_t>( sizeof( p_metadata.indexedAt ) ) ) {
                    return false;
                }
            }

            return true;
        }

        size_t minimumMetadataSize( uint16 p_version ) {
            if ( p_version >= DatIndex_VersionExtended ) {
                return MIN_V4_SIZE;
            }
            if ( p_version >= DatIndex_VersionFingerprint ) {
                return MIN_V3_SIZE;
            }
            return MIN_HEADER_SIZE;
        }

        bool writeExtendedFields( wxFile& p_file, const DatIndex& p_index ) {
            uint64 datFingerprint = p_index.datFingerprint( );
            auto bytesWritten = p_file.Write( &datFingerprint, sizeof( datFingerprint ) );
            if ( bytesWritten < static_cast<ssize_t>( sizeof( datFingerprint ) ) ) {
                return false;
            }

            uint32 datPathCrc = p_index.datPathCrc( );
            bytesWritten = p_file.Write( &datPathCrc, sizeof( datPathCrc ) );
            if ( bytesWritten < static_cast<ssize_t>( sizeof( datPathCrc ) ) ) {
                return false;
            }

            uint64 datFileSize = p_index.datFileSize( );
            bytesWritten = p_file.Write( &datFileSize, sizeof( datFileSize ) );
            if ( bytesWritten < static_cast<ssize_t>( sizeof( datFileSize ) ) ) {
                return false;
            }

            uint64 indexedAt = p_index.indexedAt( );
            bytesWritten = p_file.Write( &indexedAt, sizeof( indexedAt ) );
            if ( bytesWritten < static_cast<ssize_t>( sizeof( indexedAt ) ) ) {
                return false;
            }

            return true;
        }

        void applyMetadataToIndex( const DatIndexMetadata& p_metadata, DatIndex& p_index ) {
            p_index.setDatTimestamp( p_metadata.datTimestamp );
            p_index.setDatFingerprint( p_metadata.datFingerprint );
            p_index.setDatPathCrc( p_metadata.datPathCrc );
            p_index.setDatFileSize( p_metadata.datFileSize );
            p_index.setIndexedAt( p_metadata.indexedAt );
        }

    } // namespace

    bool DatIndexMetadata::matchesDat( uint64 p_fingerprint, uint64 p_timestamp, uint64 p_fileSize, uint32 p_pathCrc ) const {
        if ( datPathCrc != 0 && datPathCrc != p_pathCrc ) {
            return false;
        }
        if ( datFingerprint != 0 && p_fingerprint != 0 ) {
            return datFingerprint == p_fingerprint;
        }
        if ( datTimestamp != p_timestamp ) {
            return false;
        }
        if ( datFileSize != 0 && p_fileSize != 0 ) {
            return datFileSize == p_fileSize;
        }
        return true;
    }

    bool DatIndexMetadata::isAssociatedWithPath( uint32 p_pathCrc, const wxString& p_filename ) const {
        if ( datPathCrc != 0 ) {
            return datPathCrc == p_pathCrc;
        }

        wxFileName fileName( p_filename );
        auto name = fileName.GetName( );
        auto prefix = wxString::Format( wxT( "%x" ), p_pathCrc );
        if ( name == prefix ) {
            return true;
        }
        return name.StartsWith( prefix + wxT( "_" ) );
    }

    bool peekIndexMetadata( const wxString& p_filename, DatIndexMetadata& p_metadata ) {
        p_metadata = DatIndexMetadata( );

        if ( !wxFile::Exists( p_filename ) ) {
            return false;
        }

        wxFile file;
        if ( !file.Open( p_filename ) ) {
            return false;
        }

        DatIndexHead header;
        if ( file.Read( &header, sizeof( header ) ) < static_cast<ssize_t>( sizeof( header ) ) ) {
            return false;
        }
        if ( header.magicInteger != DatIndex_Magic ) {
            return false;
        }
        if ( header.version < DatIndex_MinVersion || header.version > DatIndex_Version ) {
            return false;
        }
        if ( static_cast<size_t>( file.Length( ) ) < minimumMetadataSize( header.version ) ) {
            return false;
        }

        return readMetadataFields( file, header, p_metadata );
    }

    //----------------------------------------------------------------------------
    //      DatIndexReader
    //----------------------------------------------------------------------------

    DatIndexReader::DatIndexReader( DatIndex& p_index )
        : m_index( p_index ) {
        Ensure::notNull( &p_index );
        ::memset( &m_header, 0, sizeof( m_header ) );
    }

    DatIndexReader::~DatIndexReader( ) {
        this->close( );
    }

    bool DatIndexReader::open( const wxString& p_filename ) {
        this->close( );
        if ( !wxFile::Exists( p_filename ) ) {
            return false;
        }

        m_file.Open( p_filename );
        if ( m_file.IsOpened( ) && static_cast<size_t>( m_file.Length( ) ) > sizeof( m_header ) ) {
            m_file.Read( &m_header, sizeof( m_header ) );
            if ( m_header.magicInteger != DatIndex_Magic ) {
                this->close( ); return false;
            }
            if ( m_header.version < DatIndex_MinVersion || m_header.version > DatIndex_Version ) {
                this->close( ); return false;
            }
            if ( static_cast<size_t>( m_file.Length( ) ) < minimumMetadataSize( m_header.version ) ) {
                this->close( ); return false;
            }

            DatIndexMetadata metadata;
            metadata.version = m_header.version;
            metadata.datTimestamp = m_header.datTimestamp;
            metadata.numEntries = m_header.numEntries;
            if ( !readMetadataFields( m_file, m_header, metadata ) ) {
                this->close( ); return false;
            }

            m_index.clear( ); // always start with a fresh index
            applyMetadataToIndex( metadata, m_index );
            m_index.reserveEntries( m_header.numEntries );
            m_index.reserveCategories( m_header.numCategories );
            return true;
        }

        return false;
    }

    void DatIndexReader::close( ) {
        m_file.Close( );
        ::memset( &m_header, 0, sizeof( m_header ) );
    }

    bool DatIndexReader::isDone( ) const {
        return ( m_index.numCategories( ) == m_header.numCategories )
            && ( m_index.numEntries( ) == m_header.numEntries );
    }

    DatIndexReader::ReadResult DatIndexReader::read( uint p_amount ) {
        ReadResult result = RR_Failure;

        for ( uint i = 0; i < p_amount; i++ ) {
            ssize_t bytesRead;

            // First read all categories, one at a time
            if ( m_index.numCategories( ) < m_header.numCategories ) {
                // Read fixed-width fields
                DatIndexCategoryFields fields;
                bytesRead = m_file.Read( &fields, sizeof( fields ) );
                if ( bytesRead < static_cast<ssize_t>( sizeof( fields ) ) ) {
                    result = RR_CorruptFile; goto READ_FAILED;
                }
                // Read name
                Array<char> nameData( fields.nameLength );
                bytesRead = m_file.Read( nameData.GetPointer( ), nameData.GetSize( ) );
                if ( bytesRead < ( ssize_t ) nameData.GetSize( ) ) {
                    result = RR_CorruptFile; goto READ_FAILED;
                }
                // Add category
                auto name = wxString::FromUTF8Unchecked( nameData.GetPointer( ), nameData.GetSize( ) );
                auto category = m_index.addIndexCategory( name, false );
                // Set parent
                if ( fields.parent != DatIndex_RootCategory ) {
                    auto parent = m_index.category( fields.parent );
                    if ( parent ) {
                        parent->addSubCategory( category );
                    }
                }
            }

            // If all categories are read, start reading the files instead (note the 'else')
            else if ( m_index.numEntries( ) < m_header.numEntries ) {
                // Read fixed-width fields
                DatIndexEntryFields fields;
                bytesRead = m_file.Read( &fields, sizeof( fields ) );
                if ( bytesRead < static_cast<ssize_t>( sizeof( fields ) ) ) {
                    result = RR_CorruptFile; goto READ_FAILED;
                }
                // Read name
                Array<char> nameData( fields.nameLength );
                bytesRead = m_file.Read( nameData.GetPointer( ), nameData.GetSize( ) );
                if ( bytesRead < ( ssize_t ) nameData.GetSize( ) ) {
                    result = RR_CorruptFile; goto READ_FAILED;
                }
                // Add entry
                auto name = wxString::FromUTF8Unchecked( nameData.GetPointer( ), nameData.GetSize( ) );
                auto& newEntry = m_index.addIndexEntry( false )
                    ->setBaseId( fields.baseId )
                    .setFileId( fields.fileId )
                    .setMftEntry( fields.mftEntry )
                    .setFileType( ( ANetFileType ) fields.fileType )
                    .setName( name );
                auto category = m_index.category( fields.category );
                if ( !category ) {
                    result = RR_CorruptFile; goto READ_FAILED;
                }
                category->addEntry( &newEntry );
                newEntry.finalizeAdd( );
            }

            // If both are done we can skip this loop
            else {
                break;
            }
        }

        return RR_Success;

    READ_FAILED:
        return result;
    }

    //----------------------------------------------------------------------------
    //      DatIndexWriter
    //----------------------------------------------------------------------------

    DatIndexWriter::DatIndexWriter( DatIndex& p_index )
        : m_index( p_index )
        , m_categoriesWritten( 0 )
        , m_entriesWritten( 0 ) {
        Ensure::notNull( &p_index );
    }

    DatIndexWriter::~DatIndexWriter( ) {
        this->close( );
    }

    bool DatIndexWriter::open( const wxString& p_filename ) {
        this->close( );

        m_file.Open( p_filename, wxFile::write );
        if ( m_file.IsOpened( ) ) {
            DatIndexHead header;
            header.magicInteger = DatIndex_Magic;
            header.version = DatIndex_Version;
            header.datTimestamp = m_index.datTimestamp( );
            header.numEntries = m_index.numEntries( );
            header.numCategories = m_index.numCategories( );

            auto bytesWritten = m_file.Write( &header, sizeof( header ) );
            if ( bytesWritten < sizeof( header ) ) {
                this->close( ); return false;
            }

            if ( !writeExtendedFields( m_file, m_index ) ) {
                this->close( ); return false;
            }

            return true;
        }

        return false;
    }

    void DatIndexWriter::close( ) {
        this->flushBuffer( );
        m_file.Close( );
        m_categoriesWritten = 0;
        m_entriesWritten = 0;
        m_buffer.clear( );
    }

    bool DatIndexWriter::appendToBuffer( const void* p_data, size_t p_size ) {
        const char* bytes = static_cast<const char*>( p_data );
        m_buffer.insert( m_buffer.end( ), bytes, bytes + p_size );

        if ( m_buffer.size( ) >= 1024 * 1024 ) {
            return this->flushBuffer( );
        }

        return true;
    }

    bool DatIndexWriter::flushBuffer( ) {
        if ( m_buffer.empty( ) ) {
            return true;
        }

        ssize_t bytesWritten = m_file.Write( m_buffer.data( ), m_buffer.size( ) );
        m_buffer.clear( );

        return bytesWritten >= 0;
    }

    bool DatIndexWriter::isDone( ) const {
        return ( m_index.numEntries( ) == m_entriesWritten )
            && ( m_index.numCategories( ) == m_categoriesWritten );
    }

    bool DatIndexWriter::write( uint p_amount ) {
        for ( uint i = 0; i < p_amount; i++ ) {
            // First write categories, one at a time
            if ( m_categoriesWritten < m_index.numCategories( ) ) {
                auto category = m_index.category( m_categoriesWritten );
                auto parent = category->parent( );
                wxScopedCharBuffer nameBuffer = category->name( ).ToUTF8( );
                // Fixed-width fields
                DatIndexCategoryFields fields;
                fields.parent = ( parent ? parent->index( ) : -1 );
                fields.nameLength = nameBuffer.length( );
                if ( !this->appendToBuffer( &fields, sizeof( fields ) ) ) {
                    return false;
                }
                // Name
                if ( !this->appendToBuffer( nameBuffer, fields.nameLength ) ) {
                    return false;
                }
                // Increase the counter
                m_categoriesWritten++;
            }

            // Then, write entries one at a time (note the 'else')
            else if ( m_entriesWritten < m_index.numEntries( ) ) {
                auto entry = m_index.entry( m_entriesWritten );
                auto category = entry->category( );
                auto nameBuffer = entry->name( ).ToUTF8( );
                // Fixed-width fields
                DatIndexEntryFields fields;
                fields.category = category->index( );
                fields.baseId = entry->baseId( );
                fields.fileId = entry->fileId( );
                fields.mftEntry = entry->mftEntry( );
                fields.fileType = entry->fileType( );
                fields.nameLength = nameBuffer.length( );
                if ( !this->appendToBuffer( &fields, sizeof( fields ) ) ) {
                    return false;
                }
                // Name
                if ( !this->appendToBuffer( nameBuffer, fields.nameLength ) ) {
                    return false;
                }
                // Increase the counter
                m_entriesWritten++;
            }

            // Both done = ditch this loop
            else {
                break;
            }
        }

        if ( this->isDone( ) && !this->flushBuffer( ) ) {
            return false;
        }

        // Remove dirty flag if everything is saved
        if ( this->isDone( ) ) {
            m_index.setDirty( false );
        }

        return true;
    }

}; // namespace gw2b
