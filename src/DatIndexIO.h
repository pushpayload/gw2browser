/** \file       DatIndexIO.h
 *  \brief      Contains the declaration for the index reader and writer classes.
 *  \author     Rhoot
 */

/**
 * Copyright (C) 2015 Khralkatorrix <https://github.com/kytulendu>
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

#ifndef DATINDEXREADER_H_INCLUDED
#define DATINDEXREADER_H_INCLUDED

#include <vector>

#include <wx/file.h>

#include "DatIndex.h"

namespace gw2b {

    enum DatIndexMagicNumber {
        DatIndex_Magic = 0x4944,
        DatIndex_Version = 0x4,             /**< Current index format version (written by this build). */
        DatIndex_MinVersion = 0x2,          /**< Oldest index format version we can still read. */
        DatIndex_VersionFingerprint = 0x3,  /**< First version to store the .dat fingerprint after the header. */
        DatIndex_VersionExtended = 0x4,     /**< First version to store path CRC, file size, and indexed-at time. */
        DatIndex_RootCategory = -0x1,
    };

    /** Metadata read from an index file header without loading the full index. */
    struct DatIndexMetadata {
        uint16  version = 0;
        uint64  datTimestamp = 0;
        uint64  datFingerprint = 0;
        uint32  datPathCrc = 0;
        uint64  datFileSize = 0;
        uint64  indexedAt = 0;
        uint32  numEntries = 0;

        /** Returns true if this index describes the given .dat state. */
        bool matchesDat( uint64 p_fingerprint, uint64 p_timestamp, uint64 p_fileSize, uint32 p_pathCrc ) const;
        /** Returns true if the index file belongs to the given .dat path CRC. */
        bool isAssociatedWithPath( uint32 p_pathCrc, const wxString& p_filename ) const;
    };

    /** Reads only the header metadata from an index file.
    *  \param[in]  p_filename   Index file to inspect.
    *  \param[out] p_metadata   Populated metadata on success.
    *  \return bool    true if the file is a readable index header. */
    bool peekIndexMetadata( const wxString& p_filename, DatIndexMetadata& p_metadata );

#pragma pack(push, 1)

    /** Structure of the .dat index header in the file. */
    struct DatIndexHead {
        union {
            char magic[2];          /**< Contains 'DI'. */
            uint16 magicInteger;    /**< Contains 0x4944, in little endian. */
        };
        uint16 version;             /**< Index format version. */
        uint64 datTimestamp;        /**< Indexed .dat file's timestamp. */
        uint32 numEntries;          /**< Amount of entries in the index. */
        uint32 numCategories;       /**< Amount of categories in the index. */
    };

    /** Structure of the fixed-width category fields in the .dat index file. */
    struct DatIndexCategoryFields {
        int32 parent;               /**< Index of the category's parent. -1 for none. */
        uint16 nameLength;          /**< Length of the category's name, in bytes. */
    };

    /** Structure of the fixed-width entry fields in the .dat index file. */
    struct DatIndexEntryFields {
        int32 category;             /**< Index of the category it belongs to. */
        uint32 baseId;              /**< Base ID of the indexed file. */
        uint32 fileId;              /**< File ID of the indexed file. */
        uint32 mftEntry;            /**< MFT entry number of the indexed file. */
        uint32 fileType;            /**< Type of the indexed file. */
        uint16 nameLength;          /**< Length of the entry's name, in bytes. */
    };

#pragma pack(pop)

    /** Responsible for reading a .dat index from file. */
    class DatIndexReader {
        DatIndex&       m_index;
        DatIndexHead    m_header;
        wxFile          m_file;
    public:
        /** Result of the Read() operation. */
        enum ReadResult {
            RR_Failure = 0,            /**< Read failed. Bit 2+ should contain reason. */
            RR_Success = 1,            /**< Read succeeded. */
            // Bit 1 should only be set for success flags, to allow for (result & RR_Success)
            RR_UnsupportedFile = ( ( 1 << 1 ) | 0 ),   /**< The file is of an unsupported type. */
            RR_Outdated = ( ( 1 << 2 ) | 0 ),   /**< The index file version is outdated. */
            RR_CorruptFile = ( ( 1 << 3 ) | 0 ),   /**< The index file appears to be corrupt. */
        };
    public:
        /** Constructor.
        *  \param[in]  p_index  Index to read into. */
        DatIndexReader( DatIndex& p_index );
        /** Destructor. */
        ~DatIndexReader( );

        /** Opens the given file for reading.
        *  \param[in]  p_filename   File to open.
        *  \return bool    true if open was successful, false if not. */
        bool open( const wxString& p_filename );
        /** Closes the opened file. */
        void close( );
        /** Determines whether this task is done.
        *  \return bool    true if the task is done, false if not. */
        bool isDone( ) const;
        /** Determines whether there is an open index file.
        *  \return bool    true if there is an open index file, false if not. */
        bool isOpen( ) const {
            return m_file.IsOpened( );
        }

        /** Gets the current amount of read categories.
        *  \return uint    amount of categories. */
        uint currentCategory( ) const {
            return m_index.numCategories( );
        }
        /** Gets the total amount of categories in the file.
        *  \return uint    amount of categories. */
        uint numCategories( ) const {
            return m_header.numCategories;
        }

        /** Gets the current amount of read entries.
        *  \return uint    amount of entries. */
        uint currentEntry( ) const {
            return m_index.numEntries( );
        }
        /** Gets the total amount of entries in the file.
        *  \return uint    amount of entries. */
        uint numEntries( ) const {
            return m_header.numEntries;
        }

        /** Performs a read cycle, reading some categories/entries from the file
        *  and adding them to the index.
        *  \param[in]  p_amount     Amount of read cycles to perform.
        *  \return ReadResult  The result of the read operation(s). */
        ReadResult read( uint p_amount = 1 );
    }; // class DatIndexReader

    /** Responsible for writing a .dat index to file. */
    class DatIndexWriter {
        DatIndex&               m_index;
        wxFile                  m_file;
        uint                    m_categoriesWritten;
        uint                    m_entriesWritten;
        std::vector<char>       m_buffer;
    public:
        /** Constructor.
        *  \param[in]  p_index  Index to write onto disk. */
        DatIndexWriter( DatIndex& p_index );
        /** Destructor. */
        ~DatIndexWriter( );

        /** Opens the given file for writing.
        *  \param[in]  p_filename   File to open.
        *  \return bool    true if open was successful, false if not. */
        bool open( const wxString& p_filename );
        /** Closes the opened file. */
        void close( );
        /** Determines whether this task is done.
        *  \return bool    true if the task is done, false if not. */
        bool isDone( ) const;
        /** Determines whether there is an open index file.
        *  \return bool    true if there is an open index file, false if not. */
        bool isOpen( ) const {
            return m_file.IsOpened( );
        }

        /** Gets the current amount of written categories.
        *  \return uint    amount of categories. */
        uint currentCategory( ) const {
            return m_categoriesWritten;
        }
        /** Gets the total amount of categories to write.
        *  \return uint    amount of categories. */
        uint numCategories( ) const {
            return m_index.numCategories( );
        }

        /** Gets the current amount of written entries.
        *  \return uint    amount of entries. */
        uint currentEntry( ) const {
            return m_entriesWritten;
        }
        /** Gets the total amount of entries to write.
        *  \return uint    amount of entries. */
        uint numEntries( ) const {
            return m_index.numEntries( );
        }

        /** Performs a write cycle, writing some categories/entries to the file.
        *  \param[in]  p_amount     Amount of write cycles to perform.
        *  \return bool    true if successful, false if not. */
        bool write( uint p_amount = 1 );
    private:
        bool appendToBuffer( const void* p_data, size_t p_size );
        bool flushBuffer( );

    }; // class DatIndexWriter

}; // namespace gw2b

#endif // DATINDEXREADER_H_INCLUDED
