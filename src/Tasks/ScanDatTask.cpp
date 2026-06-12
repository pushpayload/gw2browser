/** \file       ScanDatTask.cpp
 *  \brief      Contains declaration of the ScanDatTask class.
 *  \author     Rhoot
 */

/**
 * Copyright (C) 2014-2019 Khralkatorrix <https://github.com/kytulendu>
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

#include <unordered_set>

#include "ScanDatTask.h"

#include "DatFile.h"
#include "DatIndex.h"
#include "FileReader.h"

namespace gw2b {

    namespace {

        const std::unordered_set<uint>& bitmapFontChunkIds( ) {
            static const std::unordered_set<uint> ids = {
                154824, 154825, 154826, 154827, 154828, 154829, 154842, 154843,
                154844, 154845, 154846, 154876, 154877, 154878, 154879, 154880,
                154881, 154882, 154883, 154884, 154885, 154886, 154887, 154889,
                154890, 154891, 154892, 154893, 154894, 154895, 154896, 154898,
                154899, 154900, 154901, 154902, 154903, 154904, 154905, 154906,
                154907, 154908, 154909, 154910, 154911, 154912, 154913, 154914,
                154915, 154916, 154917, 154918, 154919, 154920, 154921, 154922,
                154923, 154924, 154925, 154926, 154927, 154928, 154929, 154930,
                154931, 154932, 154933, 154934, 154935, 154936, 154937, 154938,
                154939, 154940, 154941, 154942, 154944, 439864, 439865, 439866,
                439867, 439868, 439869, 439870, 439871, 439872, 439873, 439874,
                439875, 439876, 439877, 439878, 439879, 439880, 439881, 439882,
                439883, 439884, 439885, 439886, 439887, 439896, 439897, 439898,
                439899, 439900, 439901, 439902, 439903, 439904, 439905, 439906,
                439907, 439908, 439909, 439910, 439911, 439912, 439913, 439914,
                439915, 439916, 439917, 439918, 439919, 439920, 439921, 439922,
                439923, 439924, 439925, 439926, 439927, 439928, 439929, 439930,
                439931, 439932, 439933, 439934, 439935, 439936, 439937, 439938,
                439939, 439940, 439941, 439942, 439943, 439944, 439945, 439946,
                439947, 439948, 439949, 439950, 439951, 439952, 439953, 439954,
                439955, 439956, 439957, 439958, 439959, 439960, 439961, 439962,
                439963, 439964, 439965, 439966, 439967, 439968, 439969, 439970,
                439971, 439972, 439973, 439974, 439975, 439976, 439977, 439978,
                439979, 439980, 439981, 439982, 439983, 439984, 439985, 439986,
                439987, 439988, 439989, 439990, 439991, 439992, 439993, 439994,
                439995, 439996, 439997, 439998, 439999, 459802, 459803, 459804,
                459805, 459806, 459807, 459808, 459809, 459926, 459935, 459981,
                858064, 858065, 858066, 858067, 858068, 858069, 858070, 858071,
                858072, 858073, 858074, 858075, 858076, 858077, 858078, 858079,
                858080, 858081, 858082, 858083, 858084, 858085, 858086, 858087,
                858088, 858089, 858090, 858091, 858092, 858093, 858094, 858095,
                858096, 858097, 858098, 858099, 858100, 858101, 858102, 858103,
                858104, 858105, 858106, 858107, 858108, 858109, 858110, 858111,
                858112, 858113, 858114, 858115, 858116, 858117, 858118, 858119,
                858120, 858121, 858122, 858123, 858124, 858125, 858126, 858127,
                858128, 858129, 858130, 858131, 858132, 858133, 858134, 858135
            };
            return ids;
        }

    } // namespace

    ScanDatTask::ScanDatTask( const std::shared_ptr<DatIndex>& p_index, DatFile& p_datFile )
        : m_index( p_index )
        , m_datFile( p_datFile )
        , m_batchSize( 0 )
        , m_batchUpdateActive( false ) {
        Ensure::notNull( p_index.get( ) );
        Ensure::notNull( &p_datFile );
    }

    ScanDatTask::~ScanDatTask( ) {
        this->endBatchUpdateIfNeeded( );
    }

    bool ScanDatTask::init( ) {
        this->setMaxProgress( m_datFile.numFiles( ) );
        this->setCurrentProgress( m_index->highestMftEntry( ) + 1 );

        uint filesLeft = m_datFile.numFiles( ) - ( m_index->highestMftEntry( ) + 1 );
        m_index->reserveEntries( filesLeft );

        int threads = omp_get_max_threads( );
        if ( threads < 1 ) {
            threads = 1;
        }
        m_batchSize = static_cast<uint>( threads ) * 64;
        if ( m_batchSize < 256 ) {
            m_batchSize = 256;
        }

        m_index->beginBatchUpdate( );
        m_batchUpdateActive = true;

        return true;
    }

    void ScanDatTask::abort( ) {
        this->endBatchUpdateIfNeeded( );
    }

    void ScanDatTask::endBatchUpdateIfNeeded( ) {
        if ( m_batchUpdateActive ) {
            m_index->endBatchUpdate( );
            m_batchUpdateActive = false;
        }
    }

    void ScanDatTask::perform( ) {
        const uint batchStart = this->currentProgress( );
        const uint batchEnd = wxMin( batchStart + m_batchSize, this->maxProgress( ) );
        const uint batchCount = batchEnd - batchStart;

        if ( batchCount == 0 ) {
            this->endBatchUpdateIfNeeded( );
            return;
        }

        std::vector<ScanResult> results( batchCount );

#pragma omp parallel
        {
            DatFile::ThreadContext context;
            const bool contextReady = context.open( m_datFile );
            Array<byte> buffer( 32 );

#pragma omp for schedule( dynamic )
            for ( int i = 0; i < static_cast<int>( batchCount ); i++ ) {
                if ( contextReady ) {
                    results[i] = this->scanEntry( batchStart + i, context, buffer );
                } else {
                    results[i].valid = false;
                }
            }
        }

        for ( uint i = 0; i < batchCount; i++ ) {
            this->commitResult( results[i] );
        }

        this->setText( wxString::Format( wxT( "Scanning .dat: %d/%d" ), batchEnd, this->maxProgress( ) ) );
        this->setCurrentProgress( batchEnd );

        if ( this->isDone( ) ) {
            this->endBatchUpdateIfNeeded( );
        }
    }

    ScanDatTask::ScanResult ScanDatTask::scanEntry( uint p_entryNumber, DatFile::ThreadContext& p_context, Array<byte>& p_buffer ) {
        ScanResult result;
        result.entryNumber = p_entryNumber;
        result.valid = false;

        uint bytetoread = 32;
        if ( p_buffer.GetSize( ) < bytetoread ) {
            p_buffer.SetSize( bytetoread );
        }

        uint size = m_datFile.peekFile( p_entryNumber, bytetoread, p_buffer.GetPointer( ), p_context );
        if ( !size ) {
            return result;
        }

        ANetFileType fileType;
        auto results = m_datFile.identifyFileType( p_buffer.GetPointer( ), size, fileType );

        uint lastRequestedSize = bytetoread;
        while ( results == DatFile::IR_NotEnoughData ) {
            uint sizeRequired = this->requiredIdentificationSize( p_buffer.GetPointer( ), size, fileType );

            if ( sizeRequired == lastRequestedSize ) {
                break;
            }
            lastRequestedSize = sizeRequired;

            if ( p_buffer.GetSize( ) < sizeRequired ) {
                p_buffer.SetSize( sizeRequired );
            }
            size = m_datFile.peekFile( p_entryNumber, sizeRequired, p_buffer.GetPointer( ), p_context );
            results = m_datFile.identifyFileType( p_buffer.GetPointer( ), size, fileType );
        }

        if ( !size ) {
            return result;
        }

        result.baseId = m_datFile.baseIdFromFileNum( p_entryNumber );
        result.fileId = m_datFile.fileIdFromFileNum( p_entryNumber );
        result.fileType = fileType;
        result.displayName = ( result.baseId == 0 )
            ? wxString::Format( wxT( "ID-less_%d" ), p_entryNumber )
            : wxString::Format( wxT( "%d" ), result.baseId );

        this->buildCategoryPath( fileType, p_buffer.GetPointer( ), size, p_entryNumber, p_context, result.categoryPath );
        result.valid = true;
        return result;
    }

    void ScanDatTask::commitResult( const ScanResult& p_result ) {
        if ( !p_result.valid ) {
            return;
        }

        DatIndexCategory* category = nullptr;
        for ( size_t i = 0; i < p_result.categoryPath.size( ); i++ ) {
            if ( i == 0 ) {
                category = m_index->findOrAddCategory( p_result.categoryPath[i], false );
            } else {
                category = category->findOrAddSubCategory( p_result.categoryPath[i] );
            }
        }

        auto& newEntry = m_index->addIndexEntry( false )
            ->setBaseId( p_result.baseId )
            .setFileId( p_result.fileId )
            .setFileType( p_result.fileType )
            .setMftEntry( p_result.entryNumber )
            .setName( p_result.displayName );

        if ( category ) {
            category->addEntry( &newEntry );
        }
        newEntry.finalizeAdd( );
    }

    uint ScanDatTask::requiredIdentificationSize( const byte* p_data, size_t p_size, ANetFileType p_fileType ) {
        switch ( p_fileType ) {
        case ANFT_Binary:
            if ( p_size >= 0x40 ) {
                return *reinterpret_cast<const uint32*>( p_data + 0x3c ) + 0x18;
            } else {
                return 0x140;
            }
        case ANFT_Sound:
            return 0x80;
        default:
            return 0x20;
        }
    }

    bool ScanDatTask::isBitmapFontChunk( uint p_baseId ) {
        return bitmapFontChunkIds( ).count( p_baseId ) != 0;
    }

#define PushCategory( x )     { p_path.push_back( x ); }
#define PushSubCategory( x )  { p_path.push_back( x ); }

    void ScanDatTask::buildCategoryPath( ANetFileType p_fileType, const byte* p_data, size_t p_size, uint p_entryNumber,
        DatFile::ThreadContext& p_context, std::vector<wxString>& p_path ) {
        p_path.clear( );

        switch ( p_fileType ) {
        case ANFT_ATEX:
        case ANFT_ATTX:
        case ANFT_ATEC:
        case ANFT_ATEP:
        case ANFT_ATEU:
        case ANFT_ATET:
        case ANFT_DDS:
        case ANFT_JPEG:
        case ANFT_WEBP:
        case ANFT_PNG:
        case ANFT_CTEX:
            PushCategory( wxT( "Textures" ) );

            switch ( p_fileType ) {
            case ANFT_ATEX:
                PushSubCategory( wxT( "Generic Textures" ) );
                break;
            case ANFT_ATTX:
                PushSubCategory( wxT( "Terrain Textures" ) );
                break;
            case ANFT_ATEC:
                PushSubCategory( wxT( "ATEC" ) );
                break;
            case ANFT_ATEP:
                PushSubCategory( wxT( "Map Textures" ) );
                break;
            case ANFT_ATEU:
                PushSubCategory( wxT( "UI Textures" ) );
                break;
            case ANFT_ATET:
                PushSubCategory( wxT( "ATET" ) );
                break;
            case ANFT_CTEX:
                PushSubCategory( wxT( "CTEX" ) );
                break;
            case ANFT_DDS:
                PushSubCategory( wxT( "DDS" ) );
                break;
            case ANFT_JPEG:
                PushSubCategory( wxT( "JPEG" ) );
                break;
            case ANFT_WEBP:
                PushSubCategory( wxT( "WebP" ) );
                break;
            case ANFT_PNG:
                PushSubCategory( wxT( "PNG" ) );
                break;
            }

            if ( ( p_fileType == ANFT_ATEX || p_fileType == ANFT_ATTX || p_fileType == ANFT_ATEC ||
                p_fileType == ANFT_ATEP || p_fileType == ANFT_ATEU || p_fileType == ANFT_ATET ) ) {
                uint16 width = *reinterpret_cast<const uint16*>( p_data + 8 );
                uint16 height = *reinterpret_cast<const uint16*>( p_data + 10 );
                PushSubCategory( wxString::Format( wxT( "%ux%u" ), width, height ) );
            } else if ( p_fileType == ANFT_DDS && p_size >= 20 ) {
                uint32 width = *reinterpret_cast<const uint32*>( p_data + 16 );
                uint32 height = *reinterpret_cast<const uint32*>( p_data + 12 );
                PushSubCategory( wxString::Format( wxT( "%ux%u" ), width, height ) );
            }

            break;
        case ANFT_Sound:
        case ANFT_MP3:
        case ANFT_Ogg:
        case ANFT_PackedMP3:
        case ANFT_PackedOgg:
        case ANFT_asndMP3:
        case ANFT_asndOgg:
            PushCategory( wxT( "Sounds" ) );

            switch ( p_fileType ) {
            case ANFT_MP3:
                PushSubCategory( wxT( "MP3" ) );
                break;
            case ANFT_Ogg:
                PushSubCategory( wxT( "Ogg" ) );
                break;
            case ANFT_asndMP3:
                PushSubCategory( wxT( "asndMP3" ) );
                break;
            case ANFT_asndOgg:
                PushSubCategory( wxT( "asndOgg" ) );
                break;
            case ANFT_PackedMP3:
                PushSubCategory( wxT( "PackedMP3" ) );
                break;
            case ANFT_PackedOgg:
                PushSubCategory( wxT( "PackedOgg" ) );
                break;
            }
            break;

        case ANFT_Binary:
        case ANFT_EXE:
        case ANFT_DLL:
            PushCategory( wxT( "Binaries" ) );
            break;

        case ANFT_StringFile:
        {
            PushCategory( wxT( "Strings" ) );

            auto fileSize = m_datFile.fileSize( p_entryNumber, p_context );
            auto buffer = allocate<byte>( fileSize );
            auto readSize = m_datFile.readFile( p_entryNumber, buffer, p_context );

            auto end = buffer + readSize - 2;
            auto language = static_cast<uint32>( *end );

            switch ( language ) {
            case language::English:
                PushSubCategory( wxT( "English" ) );
                break;
            case language::Korean:
                PushSubCategory( wxT( "Korean" ) );
                break;
            case language::French:
                PushSubCategory( wxT( "French" ) );
                break;
            case language::German:
                PushSubCategory( wxT( "German" ) );
                break;
            case language::Spanish:
                PushSubCategory( wxT( "Spanish" ) );
                break;
            case language::Chinese:
                PushSubCategory( wxT( "Chinese" ) );
                break;
            default:
                PushSubCategory( wxT( "Unknown" ) );
                PushSubCategory( wxString::Format( wxT( "%u" ), language ) );
            }
            freePointer( buffer );
            break;
        }
        case ANFT_Manifest:
            PushCategory( wxT( "Manifests" ) );
            break;
        case ANFT_TEXT:
        case ANFT_UTF8:
            PushCategory( wxT( "Text" ) );
            break;
        case ANFT_TextPackManifest:
            PushCategory( wxT( "TextPack Manifests" ) );
            break;
        case ANFT_TextPackVariant:
            PushCategory( wxT( "TextPack Variant" ) );
            break;
        case ANFT_TextPackVoices:
            PushCategory( wxT( "TextPack Voices" ) );
            break;
        case ANFT_Bank:
            PushCategory( wxT( "Soundbank" ) );
            break;
        case ANFT_BankIndex:
            PushCategory( wxT( "Soundbank Index" ) );
            break;
        case ANFT_AudioScript:
            PushCategory( wxT( "Audio Scripts" ) );
            break;
        case ANFT_Model:
        {
            PushCategory( wxT( "Models" ) );
            uint baseId = m_datFile.baseIdFromFileNum( p_entryNumber );
            PushSubCategory( wxString::Format( wxT( "%i" ), ( ( uint32 ) baseId / 10000 ) ) + wxT( "xxxx" ) );
            break;
        }
        case ANFT_ModelCollisionManifest:
            PushCategory( wxT( "Model Collision Manifest" ) );
            break;
        case ANFT_DependencyTable:
            PushCategory( wxT( "Dependency Tables" ) );
            break;
        case ANFT_EULA:
            PushCategory( wxT( "EULA" ) );
            break;
        case ANFT_Cinematic:
            PushCategory( wxT( "Cinematics" ) );
            break;
        case ANFT_MapCollision:
            PushCategory( wxT( "Map Collision" ) );
            break;
        case ANFT_GameContent:
            PushCategory( wxT( "Game Content" ) );
            break;
        case ANFT_GameContentPortalManifest:
            PushCategory( wxT( "Game Content Portal Manifest" ) );
            break;
        case ANFT_MapParam:
            PushCategory( wxT( "Map" ) );
            break;
        case ANFT_MapShadow:
            PushCategory( wxT( "Map Shadow" ) );
            break;
        case ANFT_MapMetadata:
            PushCategory( wxT( "Map Metadata" ) );
            break;
        case ANFT_PagedImageTable:
            PushCategory( wxT( "Paged Image Table" ) );
            break;
        case ANFT_Material:
            PushCategory( wxT( "Materials" ) );
            break;
        case ANFT_Composite:
            PushCategory( wxT( "Composite Data" ) );
            break;
        case ANFT_AnimSequences:
            PushCategory( wxT( "Animation Sequences" ) );
            break;
        case ANFT_EmoteAnimation:
            PushCategory( wxT( "Emote Animations" ) );
            break;
        case ANFT_FontFile:
            PushCategory( wxT( "Font" ) );
            break;
        case ANFT_BitmapFontFile:
            PushCategory( wxT( "Bitmap Font" ) );
            break;
        case ANFT_Bink2Video:
            PushCategory( wxT( "Bink Videos" ) );
            break;
        case ANFT_ShaderCache:
            PushCategory( wxT( "Shader Cache" ) );
            break;
        case ANFT_Config:
            PushCategory( wxT( "Configuration" ) );
            break;
        case ANFT_PF:
        case ANFT_ARAP:
            PushCategory( wxT( "Misc" ) );

            if ( p_fileType == ANFT_PF && p_size >= 12 ) {
                PushSubCategory( wxString( reinterpret_cast<const char*>( p_data + 8 ), 4 ) );
            }
            break;
        default:
        {
            uint baseId = m_datFile.baseIdFromFileNum( p_entryNumber );
            if ( this->isBitmapFontChunk( baseId ) ) {
                PushCategory( wxT( "Bitmap Font" ) );
                PushSubCategory( wxT( "Chunk" ) );
            } else {
                PushCategory( wxT( "Unknown" ) );
            }
        }
        }
    }

#undef PushCategory
#undef PushSubCategory

}; // namespace gw2b
